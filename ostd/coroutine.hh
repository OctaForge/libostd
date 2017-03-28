/* Coroutines for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for further information.
 */

#ifndef OSTD_COROUTINE_HH
#define OSTD_COROUTINE_HH

#include <exception>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <tuple>
#include <type_traits>
#include <optional>

#include "ostd/platform.hh"
#include "ostd/types.hh"
#include "ostd/range.hh"

#include "ostd/context_stack.hh"

namespace ostd {

struct coroutine_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct coroutine_context;

namespace detail {
    /* from boost.fcontext */
    using fcontext_t = void *;

    struct transfer_t {
        fcontext_t ctx;
        void *data;
    };

    extern "C" OSTD_EXPORT
    transfer_t OSTD_CDECL ostd_jump_fcontext(
        fcontext_t const to, void *vp
    );

    extern "C" OSTD_EXPORT
    fcontext_t OSTD_CDECL ostd_make_fcontext(
        void *sp, size_t size, void (*fn)(transfer_t)
    );

    extern "C" OSTD_EXPORT
    transfer_t OSTD_CDECL ostd_ontop_fcontext(
        fcontext_t const to, void *vp, transfer_t (*fn)(transfer_t)
    );

    OSTD_EXPORT extern thread_local coroutine_context *coro_current;
} /* namespace detail */

struct coroutine_context {
    static coroutine_context *current() {
        return detail::coro_current;
    }

protected:
    coroutine_context() {}

    /* coroutine context must be polymorphic */
    virtual ~coroutine_context() {
        unwind();
        free_stack();
    }

    coroutine_context(coroutine_context const &) = delete;
    coroutine_context(coroutine_context &&c):
        p_stack(std::move(c.p_stack)), p_coro(c.p_coro), p_orig(c.p_orig),
        p_except(std::move(c.p_except)), p_state(c.p_state), p_free(c.p_free)
    {
        c.p_coro = c.p_orig = nullptr;
        c.p_stack = { nullptr, 0 };
        c.p_free = nullptr;
        c.set_dead();
    }

    coroutine_context &operator=(coroutine_context const &) = delete;
    coroutine_context &operator=(coroutine_context &&c) {
        swap(c);
        return *this;
    }

    void call() {
        set_exec();
        /* switch to new coroutine */
        coroutine_context *curr = std::exchange(detail::coro_current, this);
        coro_jump();
        /* switch back to previous */
        detail::coro_current = curr;
        rethrow();
    }

    void coro_jump() {
        p_coro = detail::ostd_jump_fcontext(p_coro, this).ctx;
    }

    void yield_jump() {
        p_state = state::HOLD;
        p_orig = detail::ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    void yield_done() {
        set_dead();
        p_orig = detail::ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    bool is_hold() const {
        return (p_state == state::HOLD);
    }

    bool is_dead() const {
        return (p_state == state::TERM);
    }

    void set_dead() {
        p_state = state::TERM;
    }

    void set_exec() {
        p_state = state::EXEC;
    }

    void rethrow() {
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    void swap(coroutine_context &other) noexcept {
        using std::swap;
        swap(p_stack, other.p_stack);
        swap(p_coro, other.p_coro);
        swap(p_orig, other.p_orig);
        swap(p_except, other.p_except);
        swap(p_state, other.p_state);
        swap(p_free, other.p_free);
    }

    template<typename C, typename SA>
    void make_context(SA &sa) {
        p_stack = sa.allocate();

        void *sp = get_stack_ptr<SA>();
        size_t asize = p_stack.size -
            (static_cast<byte *>(p_stack.ptr) - static_cast<byte *>(sp));

        p_coro = detail::ostd_make_fcontext(sp, asize, &context_call<C, SA>);
        new (sp) SA(std::move(sa));
        p_free = &free_stack_call<SA>;
    }

private:
    /* we also store the stack allocator at the end of the stack */
    template<typename SA>
    void *get_stack_ptr() {
        /* 16 byte stack pointer alignment */
        constexpr size_t salign = 16;
        /* makes enough space so that we can store the allocator at
         * stack pointer location (we'll need it for dealloc later)
         */
        constexpr size_t sasize = sizeof(SA);

        void *sp = static_cast<byte *>(p_stack.ptr) - sasize - salign;
        size_t space = sasize + salign;
        sp = std::align(salign, sasize, sp, space);

        return sp;
    }

    struct forced_unwind {
        detail::fcontext_t ctx;
        forced_unwind(detail::fcontext_t c): ctx(c) {}
    };

    enum class state {
        HOLD = 0, EXEC, TERM
    };

    void unwind() {
        if (is_dead()) {
            /* this coroutine was either initialized with a null function or
             * it's already terminated and thus its stack has already unwound
             */
            return;
        }
        if (!p_orig) {
            /* this coroutine never got to live :( */
            return;
        }
        detail::ostd_ontop_fcontext(
            std::exchange(p_coro, nullptr), nullptr,
            [](detail::transfer_t t) -> detail::transfer_t {
                throw forced_unwind{t.ctx};
            }
        );
    }

    template<typename SA>
    static void free_stack_call(void *data) {
        auto &self = *(static_cast<coroutine_context *>(data));
        auto &sa = *(static_cast<SA *>(self.get_stack_ptr<SA>()));
        SA dsa{std::move(sa)};
        sa.~SA();
        dsa.deallocate(self.p_stack);
    }

    void free_stack() {
        if (p_free) {
            p_free(this);
        }
    }

    template<typename C, typename SA>
    static void context_call(detail::transfer_t t) {
        auto &self = *(static_cast<C *>(t.data));
        self.p_orig = t.ctx;
        if (self.is_hold()) {
            /* we never got to execute properly, we're HOLD because we
             * jumped here without setting the state to EXEC before that
             */
            goto release;
        }
        try {
            self.resume_call();
        } catch (coroutine_context::forced_unwind v) {
            /* forced_unwind is unique */
            self.p_orig = v.ctx;
        } catch (...) {
            /* some other exception, will be rethrown later */
            self.p_except = std::current_exception();
        }
        /* switch back, release stack */
release:
        self.yield_done();
    }

    stack_context p_stack;
    detail::fcontext_t p_coro = nullptr;
    detail::fcontext_t p_orig = nullptr;
    std::exception_ptr p_except;
    state p_state = state::HOLD;
    void (*p_free)(void *) = nullptr;
};

template<typename T>
struct coroutine;

namespace detail {
    template<typename T>
    using coro_arg = std::add_pointer_t<std::remove_reference_t<T>>;

    /* dealing with args */
    template<typename ...A>
    struct coro_types {
        using yield_type = std::tuple<A...>;

        static yield_type get(std::tuple<coro_arg<A>...> &args) {
            return std::apply([](auto ...args) {
                return std::make_tuple(std::forward<A>(*args)...);
            }, args);
        }
    };

    template<typename A>
    struct coro_types<A> {
        using yield_type = A;

        static yield_type get(std::tuple<coro_arg<A>> &args) {
            return std::forward<A>(*std::get<0>(args));
        }
    };

    template<typename A, typename B>
    struct coro_types<A, B> {
        using yield_type = std::pair<A, B>;

        static yield_type get(std::tuple<coro_arg<A>, coro_arg<B>> &args) {
            return std::make_pair(
                std::forward<A>(*std::get<0>(args)),
                std::forward<B>(*std::get<1>(args))
            );
        }
    };

    template<>
    struct coro_types<> {
        using yield_type = void;
    };

    template<typename ...A>
    using coro_args = typename coro_types<A...>::yield_type;

    /* storing and handling results */
    template<typename R>
    struct coro_rtype {
        using type = std::aligned_storage_t<sizeof(R), alignof(R)>;

        static void store(type &stor, R &&v) {
            new (&stor) R(std::move(v));
        }

        static void store(type &stor, R const &v) {
            new (&stor) R(v);
        }

        static R get(type &stor) {
            R &tstor = *reinterpret_cast<R *>(&stor);
            R ret{std::forward<R>(tstor)};
            /* this way we can make sure result is always uninitialized
             * except when resuming, so no need to keep a bool flag etc.
             */
            tstor.~R();
            return ret;
        }
    };

    template<typename R>
    struct coro_rtype<R &> {
        using type = R *;

        static void store(type &stor, R &v) {
            stor = &v;
        }

        static R &get(type stor) {
            return *stor;
        }
    };

    template<typename R>
    struct coro_rtype<R &&> {
        using type = std::aligned_storage_t<sizeof(R), alignof(R)>;

        static void store(type &stor, R &&v) {
            new (&stor) R(std::move(v));
        }

        static R &&get(type &stor) {
            R &tstor = *reinterpret_cast<R *>(&stor);
            R ret{std::forward<R>(tstor)};
            tstor.~R();
            return std::move(ret);
        }
    };

    template<typename R>
    using coro_result = typename coro_rtype<R>::type;

    /* default case, yield returns args and takes a value */
    template<typename Y, typename R, typename ...A>
    struct coro_stor {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        coro_args<A...> get_args() {
            return coro_types<A...>::get(p_args);
        }

        void set_args(coro_arg<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        R get_result() {
            return std::forward<R>(coro_rtype<R>::get(p_result));
        }

        template<typename C>
        void call_helper(C &coro) {
            std::apply([this, &coro](auto ...args) {
                coro_rtype<R>::store(p_result, std::forward<R>(p_func(
                    Y{coro}, std::forward<A>(*args)...
                )));
            }, p_args);
        }

        void swap(coro_stor &other) {
            using std::swap;
            swap(p_func, other.p_func);
            swap(p_args, other.p_args);
            /* no need to swap result as result is always only alive
             * for the time of a single resume, in which no swap happens
             */
        }

        std::function<R(Y, A...)> p_func;
        std::tuple<coro_arg<A>...> p_args;
        coro_result<R> p_result;
    };

    /* yield takes a value but doesn't return any args */
    template<typename Y, typename R>
    struct coro_stor<Y, R> {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        void get_args() {}
        void set_args() {}

        R get_result() {
            return std::forward<R>(coro_rtype<R>::get(p_result));
        }

        template<typename C>
        void call_helper(C &coro) {
            coro_rtype<R>::store(
                p_result, std::forward<R>(p_func(Y{coro}))
            );
        }

        void swap(coro_stor &other) {
            std::swap(p_func, other.p_func);
        }

        std::function<R(Y)> p_func;
        coro_result<R> p_result;
    };

    /* yield doesn't take a value and returns args */
    template<typename Y, typename ...A>
    struct coro_stor<Y, void, A...> {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        coro_args<A...> get_args() {
            return coro_types<A...>::get(p_args);
        }

        void set_args(coro_arg<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        void get_result() {}

        template<typename C>
        void call_helper(C &coro) {
            std::apply([this, &coro](auto ...args) {
                p_func(Y{coro}, std::forward<A>(*args)...);
            }, p_args);
        }

        void swap(coro_stor &other) {
            using std::swap;
            swap(p_func, other.p_func);
            swap(p_args, other.p_args);
        }

        std::function<void(Y, A...)> p_func;
        std::tuple<coro_arg<A>...> p_args;
    };

    /* yield doesn't take a value or return any args */
    template<typename Y>
    struct coro_stor<Y, void> {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        void get_args() {}
        void set_args() {}

        void get_result() {}

        template<typename C>
        void call_helper(C &coro) {
            p_func(Y{coro});
        }

        void swap(coro_stor &other) {
            std::swap(p_func, other.p_func);
        }

        std::function<void(Y)> p_func;
    };
} /* namespace detail */

template<typename R, typename ...A>
struct coroutine<R(A...)>: coroutine_context {
private:
    using base_t = coroutine_context;
    /* necessary so that context callback can access privates */
    friend struct coroutine_context;

    template<typename RR, typename ...AA>
    struct yielder {
        yielder(coroutine<RR(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()(RR &&ret) {
            detail::coro_rtype<RR>::store(
                p_coro.p_stor.p_result, std::move(ret)
            );
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

        detail::coro_args<AA...> operator()(RR const &ret) {
            detail::coro_rtype<RR>::store(p_coro.p_stor.p_result, ret);
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<RR(AA...)> &p_coro;
    };

    template<typename RR, typename ...AA>
    struct yielder<RR &, AA...> {
        yielder(coroutine<RR &(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()(RR &ret) {
            detail::coro_rtype<RR &>::store(p_coro.p_stor.p_result, ret);
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<RR &(AA...)> &p_coro;
    };

    template<typename RR, typename ...AA>
    struct yielder<RR &&, AA...> {
        yielder(coroutine<RR &&(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()(RR &&ret) {
            detail::coro_rtype<RR &&>::store(
                p_coro.p_stor.p_result, std::move(ret)
            );
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<RR &&(AA...)> &p_coro;
    };

    template<typename ...AA>
    struct yielder<void, AA...> {
        yielder(coroutine<void(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()() {
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<void(AA...)> &p_coro;
    };

public:
    using yield_type = yielder<R, A...>;

    /* we have no way to assign a function anyway... */
    coroutine() = delete;

    /* 0 means default size decided by the stack allocator */
    template<typename F, typename SA = default_stack>
    coroutine(F func, SA sa = SA{}): base_t(), p_stor(std::move(func)) {
        /* that way there is no context creation/stack allocation */
        if (!p_stor.p_func) {
            this->set_dead();
            return;
        }
        this->make_context<coroutine<R(A...)>>(sa);
    }

    template<typename SA = default_stack>
    coroutine(std::nullptr_t, SA = SA{}): base_t(), p_stor() {
        this->set_dead();
    }

    coroutine(coroutine const &) = delete;
    coroutine(coroutine &&c) noexcept:
        base_t(std::move(c)), p_stor(std::move(c.p_stor))
    {
        c.p_stor.p_func = nullptr;
    }

    coroutine &operator=(coroutine const &) = delete;
    coroutine &operator=(coroutine &&c) noexcept {
        base_t::operator=(std::move(c));
        p_stor = std::move(c.p_stor);
        c.p_stor.p_func = nullptr;
    }

    explicit operator bool() const noexcept {
        return !this->is_dead();
    }

    R resume(A ...args) {
        if (this->is_dead()) {
            throw coroutine_error{"dead coroutine"};
        }
        this->set_args(&args...);
        base_t::call();
        return this->get_result();
    }

    R operator()(A ...args) {
        /* duplicate the logic so we don't copy/move the args */
        if (this->is_dead()) {
            throw coroutine_error{"dead coroutine"};
        }
        p_stor.set_args(&args...);
        base_t::call();
        return p_stor.get_result();
    }

    void swap(coroutine &other) noexcept {
        p_stor.swap(other.p_stor);
        base_t::swap(other);
    }

    std::type_info const &target_type() const {
        return p_stor.p_func.target_type();
    }

    template<typename F>
    F *target() { return p_stor.p_func.target(); }

    template<typename F>
    F const *target() const { return p_stor.p_func.target(); }

private:
    void resume_call() {
        p_stor.call_helper(*this);
    }

    detail::coro_stor<yield_type, R, A...> p_stor;
};

template<typename R, typename ...A>
inline void swap(coroutine<R(A...)> &a, coroutine<R(A...)> &b) noexcept {
    a.swap(b);
}

template<typename T> struct generator_range;

namespace detail {
    template<typename T> struct generator_iterator;
}

template<typename T>
struct generator: coroutine_context {
private:
    using base_t = coroutine_context;
    friend struct coroutine_context;

    template<typename U>
    struct yielder {
        yielder(generator<U> &g): p_gen(g) {}

        void operator()(U &&ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }

        void operator()(U const &ret) {
            if constexpr(std::is_const_v<U>) {
                p_gen.p_result = &ret;
                p_gen.yield_jump();
            } else {
                T val{ret};
                p_gen.p_result = &val;
                p_gen.yield_jump();
            }
        }
    private:
        generator<U> &p_gen;
    };

    template<typename U>
    struct yielder<U &> {
        yielder(generator<U> &g): p_gen(g) {}

        void operator()(U &ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }
    private:
        generator<U> &p_gen;
    };

    template<typename U>
    struct yielder<U &&> {
        yielder(generator<U> &g): p_gen(g) {}

        void operator()(U &&ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }
    private:
        generator<U> &p_gen;
    };

public:
    using range = generator_range<T>;

    using yield_type = yielder<T>;

    generator() = delete;

    template<typename F, typename SA = default_stack>
    generator(F func, SA sa = SA{}):
        base_t(), p_func(std::move(func))
    {
        /* that way there is no context creation/stack allocation */
        if (!p_func) {
            this->set_dead();
            return;
        }
        this->make_context<generator<T>>(sa);
        /* generate an initial value */
        resume();
    }

    template<typename SA = default_stack>
    generator(std::nullptr_t, SA = SA{0}):
        base_t(), p_func(nullptr)
    {
        this->set_dead();
    }

    generator(generator const &) = delete;
    generator(generator &&c) noexcept:
        base_t(std::move(c)), p_func(std::move(c.p_func)), p_result(c.p_result)
    {
        c.p_func = nullptr;
        c.p_result = nullptr;
    }

    generator &operator=(generator const &) = delete;
    generator &operator=(generator &&c) noexcept {
        base_t::operator=(std::move(c));
        p_func = std::move(c.p_func);
        p_result = c.p_result;
        c.p_func = nullptr;
        c.p_result = nullptr;
    }

    explicit operator bool() const noexcept {
        return !this->is_dead();
    }

    void resume() {
        if (this->is_dead()) {
            throw coroutine_error{"dead generator"};
        }
        coroutine_context::call();
    }

    T &value() {
        if (!p_result) {
            throw coroutine_error{"no value"};
        }
        return *p_result;
    }

    T const &value() const {
        if (!p_result) {
            throw coroutine_error{"no value"};
        }
        return *p_result;
    }

    bool empty() const noexcept {
        return !p_result;
    }

    generator_range<T> iter() noexcept;

    /* for range for loop; they're the same, operator!= bypasses comparing */
    detail::generator_iterator<T> begin() noexcept;
    detail::generator_iterator<T> end() noexcept;

    void swap(generator &other) noexcept {
        using std::swap;
        swap(p_func, other.p_func);
        swap(p_result, other.p_result);
        coroutine_context::swap(other);
    }

    std::type_info const &target_type() const {
        return p_func.target_type();
    }

    template<typename F>
    F *target() { return p_func.target(); }

    template<typename F>
    F const *target() const { return p_func.target(); }

private:
    void resume_call() {
        p_func(yield_type{*this});
        /* done, gotta null the item so that empty() returns true */
        p_result = nullptr;
    }

    std::function<void(yield_type)> p_func;
    /* we can use a pointer because even stack values are alive
     * as long as the coroutine is alive (and it is on every yield)
     */
    std::remove_reference_t<T> *p_result = nullptr;
};

template<typename T>
inline void swap(generator<T> &a, generator<T> &b) noexcept {
    a.swap(b);
}

namespace detail {
    template<typename T>
    struct yield_type_base {
        using type = typename generator<T>::yield_type;
    };
    template<typename R, typename ...A>
    struct yield_type_base<R(A...)> {
        using type = typename coroutine<R(A...)>::yield_type;
    };
}

template<typename T>
using yield_type = typename detail::yield_type_base<T>::type;

template<typename T>
struct generator_range: input_range<generator_range<T>> {
    using range_category  = input_range_tag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    generator_range() = delete;
    generator_range(generator<T> &g): p_gen(&g) {}

    bool empty() const noexcept {
        return p_gen->empty();
    }

    void pop_front() {
        p_gen->resume();
    }

    reference front() const {
        return p_gen->value();
    }

    bool equals_front(generator_range const &g) const noexcept {
        return p_gen == g.p_gen;
    }

    /* same behavior as on generator itself, for range for loop */
    detail::generator_iterator<T> begin() noexcept;
    detail::generator_iterator<T> end() noexcept;

private:
    generator<T> *p_gen;
};

template<typename T>
generator_range<T> generator<T>::iter() noexcept {
    return generator_range<T>{*this};
}

namespace detail {
    /* deliberately incomplete, only for range for loop */
    template<typename T>
    struct generator_iterator {
        generator_iterator() = delete;
        generator_iterator(generator<T> &g): p_gen(&g) {}

        bool operator!=(generator_iterator const &) noexcept {
            return !p_gen->empty();
        }

        T &operator*() const {
            return p_gen->value();
        }

        generator_iterator &operator++() {
            p_gen->resume();
            return *this;
        }

    private:
        generator<T> *p_gen;
    };
} /* namespace detail */

template<typename T>
detail::generator_iterator<T> generator<T>::begin() noexcept {
    return detail::generator_iterator<T>{*this};
}

template<typename T>
detail::generator_iterator<T> generator<T>::end() noexcept {
    return detail::generator_iterator<T>{*this};
}

template<typename T>
detail::generator_iterator<T> generator_range<T>::begin() noexcept {
    return detail::generator_iterator<T>{*p_gen};
}

template<typename T>
detail::generator_iterator<T> generator_range<T>::end() noexcept {
    return detail::generator_iterator<T>{*p_gen};
}

} /* namespace ostd */

#endif
