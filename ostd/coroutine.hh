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
    /* like reference_wrapper but for any value... doesn't have
     * to be default constructible, so we can't store it directly
     */
    template<typename T>
    struct arg_wrapper {
        arg_wrapper() = default;
        arg_wrapper(T arg): p_init(true) {
            new (&p_arg) T(std::move(arg));
        }
        ~arg_wrapper() {
            if (p_init) {
                reinterpret_cast<T *>(&p_arg)->~T();
            }
        }

        void operator=(T arg) {
            if (p_init) {
                reinterpret_cast<T *>(&p_arg)->~T();
            } else {
                p_init = true;
            }
            new (&p_arg) T(std::move(arg));
        }
        operator T &&() {
            return std::move(*reinterpret_cast<T *>(&p_arg));
        }

        void swap(arg_wrapper &other) {
            using std::swap;
            swap(
                *reinterpret_cast<T *>(&p_arg),
                *reinterpret_cast<T *>(&other.p_arg)
            );
        }

    private:
        std::aligned_storage_t <sizeof(T), alignof(T)> p_arg;
        bool p_init = false;
    };

    template<typename T>
    struct arg_wrapper<T &&> {
        arg_wrapper() = default;
        arg_wrapper(T &&arg): p_arg(&arg) {}

        void operator=(T &&arg) {
            p_arg = &arg;
        }
        operator T &&() {
            return *p_arg;
        }

        void swap(arg_wrapper &other) {
            using std::swap;
            swap(p_arg, other.p_arg);
        }

    private:
        T *p_arg = nullptr;
    };

    template<typename T>
    struct arg_wrapper<T &> {
        arg_wrapper() = default;
        arg_wrapper(T &arg): p_arg(&arg) {}

        void operator=(T &arg) {
            p_arg = &arg;
        }
        operator T &() {
            return *p_arg;
        }

        void swap(arg_wrapper &other) {
            using std::swap;
            swap(p_arg, other.p_arg);
        }

    private:
        T *p_arg = nullptr;
    };

    template<typename T>
    inline void swap(arg_wrapper<T> &a, arg_wrapper<T> &b) {
        a.swap(b);
    }

    template<typename ...A>
    struct coro_types {
        using yield_type = std::tuple<A...>;

        template<size_t ...I>
        static yield_type get_impl(
            std::tuple<std::add_pointer_t<A>...> &args,
            std::index_sequence<I...>
        ) {
            return std::make_tuple(std::forward<A>(*std::get<I>(args))...);
        }

        static yield_type get(std::tuple<std::add_pointer_t<A>...> &args) {
            return get_impl(args, std::index_sequence_for<A...>{});
        }
    };

    template<typename A>
    struct coro_types<A> {
        using yield_type = A;

        static yield_type get(std::tuple<std::add_pointer_t<A>> &args) {
            return std::forward<A>(*std::get<0>(args));
        }
    };

    template<typename A, typename B>
    struct coro_types<A, B> {
        using yield_type = std::pair<A, B>;

        static yield_type get(
            std::tuple<std::add_pointer_t<A>, std::add_pointer_t<B>> &args
        ) {
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

    template<typename R, typename ...A>
    struct coro_yielder;

    /* default case, yield returns args and takes a value */
    template<typename R, typename ...A>
    struct coro_base: coroutine_context {
    protected:
        friend struct coro_yielder<R, A...>;

        template<size_t ...I>
        coro_args<A...> get_args(std::index_sequence<I...>) {
            return coro_types<A...>::get(p_args);
        }

        void set_args(std::add_pointer_t<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        R get_result() {
            return std::forward<R>(p_result);
        }

        template<typename Y, typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            p_result = std::forward<R>(func(
                Y{*this}, std::forward<A>(*std::get<I>(p_args))...
            ));
        }

        void swap(coro_base &other) {
            using std::swap;
            swap(p_args, other.p_args);
            swap(p_result, other.p_result);
            coroutine_context::swap(other);
        }

        std::tuple<std::add_pointer_t<A>...> p_args;
        arg_wrapper<R> p_result;
    };

    /* yield takes a value but doesn't return any args */
    template<typename R>
    struct coro_base<R>: coroutine_context {
    protected:
        friend struct coro_yielder<R>;

        template<size_t ...I>
        void get_args(std::index_sequence<I...>) {}

        void set_args() {}

        R get_result() {
            return std::forward<R>(p_result);
        }

        template<typename Y, typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            p_result = std::forward<R>(func(Y{*this}));
        }

        void swap(coro_base &other) {
            using std::swap;
            swap(p_result, other.p_result);
            coroutine_context::swap(other);
        }

        arg_wrapper<R> p_result;
    };

    /* yield doesn't take a value and returns args */
    template<typename ...A>
    struct coro_base<void, A...>: coroutine_context {
    protected:
        friend struct coro_yielder<void, A...>;

        template<size_t ...I>
        coro_args<A...> get_args(std::index_sequence<I...>) {
            return coro_types<A...>::get(p_args);
        }

        void set_args(std::add_pointer_t<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        void get_result() {}

        template<typename Y, typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            func(Y{*this}, std::forward<A>(*std::get<I>(p_args))...);
        }

        void swap(coro_base &other) {
            using std::swap;
            swap(p_args, other.p_args);
            coroutine_context::swap(other);
        }

        std::tuple<std::add_pointer_t<A>...> p_args;
    };

    /* yield doesn't take a value or return any args */
    template<>
    struct coro_base<void>: coroutine_context {
    protected:
        friend struct coro_yielder<void>;

        template<size_t ...I>
        void get_args(std::index_sequence<I...>) {}

        void set_args() {}

        void get_result() {}

        template<typename Y, typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            func(Y{*this});
        }

        void swap(coro_base &other) {
            coroutine_context::swap(other);
        }
    };

    template<typename R, typename ...A>
    struct coro_yielder {
        coro_yielder(coro_base<R, A...> &coro): p_coro(coro) {}

        coro_args<A...> operator()(R &&ret) {
            p_coro.p_result = std::forward<R>(ret);
            p_coro.yield_jump();
            return p_coro.get_args(std::make_index_sequence<sizeof...(A)>{});
        }

        coro_args<A...> operator()(R const &ret) {
            p_coro.p_result = ret;
            p_coro.yield_jump();
            return p_coro.get_args(std::make_index_sequence<sizeof...(A)>{});
        }

    private:
        coro_base<R, A...> &p_coro;
    };

    template<typename R, typename ...A>
    struct coro_yielder<R &, A...> {
        coro_yielder(coro_base<R &, A...> &coro): p_coro(coro) {}

        coro_args<A...> operator()(R &ret) {
            p_coro.p_result = ret;
            p_coro.yield_jump();
            return p_coro.get_args(std::make_index_sequence<sizeof...(A)>{});
        }

    private:
        coro_base<R &, A...> &p_coro;
    };

    template<typename R, typename ...A>
    struct coro_yielder<R &&, A...> {
        coro_yielder(coro_base<R &&, A...> &coro): p_coro(coro) {}

        coro_args<A...> operator()(R &&ret) {
            p_coro.p_result = std::forward<R>(ret);
            p_coro.yield_jump();
            return p_coro.get_args(std::make_index_sequence<sizeof...(A)>{});
        }

    private:
        coro_base<R &&, A...> &p_coro;
    };

    template<typename ...A>
    struct coro_yielder<void, A...> {
        coro_yielder(coro_base<void, A...> &coro): p_coro(coro) {}

        coro_args<A...> operator()() {
            p_coro.yield_jump();
            return p_coro.get_args(std::make_index_sequence<sizeof...(A)>{});
        }

    private:
        coro_base<void, A...> &p_coro;
    };
} /* namespace detail */

template<typename R, typename ...A>
struct coroutine<R(A...)>: detail::coro_base<R, A...> {
private:
    using base_t = detail::coro_base<R, A...>;
    /* necessary so that context callback can access privates */
    friend struct detail::coroutine_context;

public:
    using yield_type = detail::coro_yielder<R, A...>;

    /* we have no way to assign a function anyway... */
    coroutine() = delete;

    /* 0 means default size decided by the stack allocator */
    template<typename F, typename SA = default_stack>
    coroutine(F func, SA sa = SA{}):
        base_t(), p_func(std::move(func))
    {
        /* that way there is no context creation/stack allocation */
        if (!p_func) {
            this->set_dead();
            return;
        }
        this->template make_context<coroutine<R(A...)>>(sa);
    }

    template<typename SA = default_stack>
    coroutine(std::nullptr_t, SA = SA{0}):
        base_t(), p_func(nullptr)
    {
        this->set_dead();
    }

    coroutine(coroutine const &) = delete;
    coroutine(coroutine &&c) noexcept:
        base_t(std::move(c)), p_func(std::move(c.p_func))
    {
        c.p_func = nullptr;
    }

    coroutine &operator=(coroutine const &) = delete;
    coroutine &operator=(coroutine &&c) noexcept {
        base_t::operator=(std::move(c));
        p_func = std::move(c.p_func);
        c.p_func = nullptr;
    }

    explicit operator bool() const noexcept {
        return !this->is_dead();
    }

    R resume(A ...args) {
        if (this->is_dead()) {
            throw coroutine_error{"dead coroutine"};
        }
        this->set_args(&args...);
        detail::coroutine_context::call();
        return this->get_result();
    }

    R operator()(A ...args) {
        /* duplicate the logic so we don't copy/move the args */
        if (this->is_dead()) {
            throw coroutine_error{"dead coroutine"};
        }
        this->set_args(&args...);
        detail::coroutine_context::call();
        return this->get_result();
    }

    void swap(coroutine &other) noexcept {
        std::swap(p_func, other.p_func);
        base_t::swap(other);
    }

private:
    void resume_call() {
        base_t::template call_helper<yield_type>(
            p_func, std::index_sequence_for<A...>{}
        );
    }

    std::function<R(yield_type, A...)> p_func;
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

    struct yielder {
        yielder(generator<T> &g): p_gen(g) {}

        void operator()(T &&ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }

        void operator()(T &ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }
    private:
        generator<T> &p_gen;
    };

public:
    using range = generator_range<T>;

    using yield_type = yielder;

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
    T *p_result = nullptr;
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
