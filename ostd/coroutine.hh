/* Coroutines for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for further information.
 */

#ifndef OSTD_COROUTINE_HH
#define OSTD_COROUTINE_HH

#include <stdexcept>
#include <utility>
#include <tuple>
#include <type_traits>
#include <optional>

#include "ostd/types.hh"
#include "ostd/range.hh"

#include "ostd/internal/context.hh"

namespace ostd {

struct coroutine_error: std::runtime_error {
    using std::runtime_error::runtime_error;
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

        void call_helper(Y &&yielder) {
            std::apply([this, yielder = std::move(yielder)](auto ...args) {
                coro_rtype<R>::store(p_result, std::forward<R>(p_func(
                    std::move(yielder), std::forward<A>(*args)...
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
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        void get_args() {}
        void set_args() {}

        R get_result() {
            return std::forward<R>(coro_rtype<R>::get(p_result));
        }

        void call_helper(Y &&yielder) {
            coro_rtype<R>::store(
                p_result, std::forward<R>(p_func(std::move(yielder)))
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
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        coro_args<A...> get_args() {
            return coro_types<A...>::get(p_args);
        }

        void set_args(coro_arg<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        void get_result() {}

        void call_helper(Y &&yielder) {
            std::apply([this, yielder = std::move(yielder)](auto ...args) {
                p_func(std::move(yielder), std::forward<A>(*args)...);
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
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        void get_args() {}
        void set_args() {}

        void get_result() {}

        void call_helper(Y &&yielder) {
            p_func(std::move(yielder));
        }

        void swap(coro_stor &other) {
            std::swap(p_func, other.p_func);
        }

        std::function<void(Y)> p_func;
    };
} /* namespace detail */

template<typename R, typename ...A>
struct coroutine<R(A...)>: detail::coroutine_context {
private:
    using base_t = detail::coroutine_context;
    /* necessary so that context callback can access privates */
    friend struct detail::coroutine_context;

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
        this->template make_context<coroutine<R(A...)>>(sa);
    }

    template<typename SA = default_stack>
    coroutine(std::nullptr_t, SA = SA{0}): base_t(), p_stor() {
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

private:
    void resume_call() {
        p_stor.call_helper(yield_type{*this});
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
struct generator: detail::coroutine_context {
private:
    using base_t = detail::coroutine_context;
    friend struct detail::coroutine_context;

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
        this->template make_context<generator<T>>(sa);
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
        detail::coroutine_context::call();
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
        detail::coroutine_context::swap(other);
    }

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
