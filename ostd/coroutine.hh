/* Coroutines for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for further information.
 */

#ifndef OSTD_COROUTINE_HH
#define OSTD_COROUTINE_HH

#include <signal.h>

#include <memory>
#include <exception>
#include <stdexcept>
#include <utility>
#include <tuple>
#include <type_traits>
#include <optional>

#include "ostd/types.hh"
#include "ostd/platform.hh"
#include "ostd/range.hh"

/* from boost.context */
#ifdef OSTD_PLATFORM_WIN32
#  if (defined(i386) || defined(__i386__) || defined(__i386) || \
       defined(__i486__) || defined(__i586__) || defined(__i686__) || \
       defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) || \
       defined(__I86__) || defined(__INTEL__) || defined(__IA32__) || \
       defined(_M_IX86) || defined(_I86_))
#    define OSTD_CONTEXT_CDECL __cdecl
#  endif
#endif

#ifndef OSTD_CONTEXT_CDECL
#define OSTD_CONTEXT_CDECL
#endif

namespace ostd {

constexpr size_t COROUTINE_DEFAULT_STACK_SIZE = SIGSTKSZ;

struct coroutine_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {
    /* from boost.fcontext */
    using fcontext_t = void *;

    struct transfer_t {
        fcontext_t ctx;
        void *data;
    };

    extern "C" OSTD_EXPORT
    transfer_t OSTD_CONTEXT_CDECL ostd_jump_fcontext(
        fcontext_t const to, void *vp
    );

    extern "C" OSTD_EXPORT
    fcontext_t OSTD_CONTEXT_CDECL ostd_make_fcontext(
        void *sp, size_t size, void (*fn)(transfer_t)
    );

    extern "C" OSTD_EXPORT
    transfer_t OSTD_CONTEXT_CDECL ostd_ontop_fcontext(
        fcontext_t const to, void *vp, transfer_t (*fn)(transfer_t)
    );

    struct forced_unwind {
        fcontext_t ctx;
        forced_unwind(fcontext_t c): ctx(c) {}
    };
}

struct coroutine_context {
    coroutine_context(size_t ss, void (*callp)(void *), void *data):
        p_stack(new byte[ss]), p_callp(callp), p_data(data)
    {
        p_coro = detail::ostd_make_fcontext(p_stack.get() + ss, ss, context_call);
    }

    coroutine_context(coroutine_context const &) = delete;
    coroutine_context(coroutine_context &&c):
        p_stack(std::move(c.p_stack)), p_coro(c.p_coro), p_orig(c.p_orig),
        p_except(std::move(c.p_except)), p_callp(c.p_callp), p_data(c.p_data),
        p_finished(c.p_finished)
    {
        c.p_coro = c.p_orig = nullptr;
        c.p_data = nullptr;
        c.p_callp = nullptr;
        /* make sure it's not unwound */
        c.p_finished = true;
    }

    coroutine_context &operator=(coroutine_context const &) = delete;
    coroutine_context &operator=(coroutine_context &&c) {
        swap(c);
        /* make sure it's not unwound */
        c.p_finished = true;
        return *this;
    }

    void call() {
        if (p_finished) {
            throw coroutine_error{"dead coroutine"};
        }
        coro_jump();
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    void unwind() {
        if (p_finished) {
            return;
        }
        detail::ostd_ontop_fcontext(
            std::exchange(p_coro, nullptr), nullptr,
            [](detail::transfer_t t) -> detail::transfer_t {
                throw detail::forced_unwind{t.ctx};
            }
        );
    }

    void coro_jump() {
        p_coro = detail::ostd_jump_fcontext(p_coro, this).ctx;
    }

    void yield_jump() {
        p_orig = detail::ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    bool is_done() const {
        return p_finished;
    }

    void set_data(void *data) {
        p_data = data;
    }

    void swap(coroutine_context &other) noexcept {
        std::swap(p_stack, other.p_stack);
        std::swap(p_coro, other.p_coro);
        std::swap(p_orig, other.p_orig);
        std::swap(p_except, other.p_except);
        std::swap(p_callp, other.p_callp);
        std::swap(p_data, other.p_data);
        std::swap(p_finished, other.p_finished);
    }

private:
    static void context_call(detail::transfer_t t) {
        auto &self = *(static_cast<coroutine_context *>(t.data));
        self.p_orig = t.ctx;
        try {
            self.p_callp(self.p_data);
        } catch (detail::forced_unwind v) {
            self.p_orig = v.ctx;
        } catch (...) {
            self.p_except = std::current_exception();
        }
        self.p_finished = true;
        self.yield_jump();
    }

    /* TODO: new'ing the stack is sub-optimal */
    std::unique_ptr<byte[]> p_stack;
    detail::fcontext_t p_coro;
    detail::fcontext_t p_orig;
    std::exception_ptr p_except;
    void (*p_callp)(void *);
    void *p_data;
    bool p_finished = false;
};

inline void swap(coroutine_context &a, coroutine_context &b) {
    a.swap(b);
}

template<typename T>
struct coroutine;

template<typename T>
struct coroutine_range;

namespace detail {
    /* like reference_wrapper but for any value */
    template<typename T>
    struct arg_wrapper {
        arg_wrapper() = default;
        arg_wrapper(T arg): p_arg(std::move(arg)) {}

        void operator=(T arg) {
            p_arg = std::move(arg);
        }
        operator T &&() {
            return std::move(p_arg);
        }

        void swap(arg_wrapper &other) {
            std::swap(p_arg, other.p_arg);
        }

    private:
        T p_arg = T{};
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
            std::swap(p_arg, other.p_arg);
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
            std::swap(p_arg, other.p_arg);
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
    };

    template<typename A>
    struct coro_types<A> {
        using yield_type = A;
    };

    template<typename A, typename B>
    struct coro_types<A, B> {
        using yield_type = std::pair<A, B>;
    };

    template<typename ...A>
    using coro_args = typename coro_types<A...>::yield_type;

    template<typename ...A, size_t ...I>
    inline coro_args<A...> yield_ret(
        std::tuple<arg_wrapper<A>...> &args, std::index_sequence<I...>
    ) {
        if constexpr(sizeof...(A) == 1) {
            return std::forward<A...>(std::get<0>(args));
        } else if constexpr(sizeof...(A) == 2) {
            return std::make_pair(std::forward<A>(std::get<I>(args))...);
        } else {
            return std::move(args);
        }
    }

    /* default case, yield returns args and takes a value */
    template<typename R, typename ...A>
    struct coro_base {
    protected:
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        coro_base(coro_base const &) = delete;
        coro_base(coro_base &&c):
            p_args(std::move(c.p_args)), p_result(std::move(c.p_result)),
            p_ctx(std::move(c.p_ctx))
        {
            p_ctx.set_data(this);
        }

        coro_base &operator=(coro_base const &) = delete;
        coro_base &operator=(coro_base &&c) {
            std::swap(p_args, c.p_args);
            std::swap(p_result, c.p_result);
            std::swap(p_ctx, c.p_ctx);
            p_ctx.set_data(this);
            return *this;
        }

        struct yielder {
            yielder(coro_base<R, A...> &coro): p_coro(coro) {}

            coro_args<A...> operator()(R &&ret) {
                p_coro.p_result = std::forward<R>(ret);
                p_coro.p_ctx.yield_jump();
                return yield_ret(
                    p_coro.p_args, std::make_index_sequence<sizeof...(A)>{}
                );
            }
        private:
            coro_base<R, A...> &p_coro;
        };

        template<typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            p_result = std::forward<R>(
                func(yielder{*this}, std::forward<A>(std::get<I>(p_args))...)
            );
        }

        R call(A ...args) {
            p_args = std::make_tuple(arg_wrapper<A>(std::forward<A>(args))...);
            p_ctx.call();
            return std::forward<R>(p_result);
        }

        void swap(coro_base &other) {
            std::swap(p_args, other.p_args);
            std::swap(p_result, other.p_result);
            std::swap(p_ctx, other.p_ctx);
        }

        std::tuple<arg_wrapper<A>...> p_args;
        arg_wrapper<R> p_result;
        coroutine_context p_ctx;
    };

    /* yield takes a value but doesn't return any args */
    template<typename R>
    struct coro_base<R> {
        coroutine_range<R> iter();

    protected:
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        coro_base(coro_base const &) = delete;
        coro_base(coro_base &&c):
            p_result(std::move(c.p_result)),
            p_ctx(std::move(c.p_ctx))
        {
            p_ctx.set_data(this);
        }

        coro_base &operator=(coro_base const &) = delete;
        coro_base &operator=(coro_base &&c) {
            std::swap(p_result, c.p_result);
            std::swap(p_ctx, c.p_ctx);
            p_ctx.set_data(this);
            return *this;
        }

        struct yielder {
            yielder(coro_base<R> &coro): p_coro(coro) {}

            void operator()(R &&ret) {
                p_coro.p_result = std::forward<R>(ret);
                p_coro.p_ctx.yield_jump();
            }
        private:
            coro_base<R> &p_coro;
        };

        template<typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            p_result = std::forward<R>(func(yielder{*this}));
        }

        R call() {
            p_ctx.call();
            return std::forward<R>(this->p_result);
        }

        void swap(coro_base &other) {
            std::swap(p_result, other.p_result);
            std::swap(p_ctx, other.p_ctx);
        }

        arg_wrapper<R> p_result;
        coroutine_context p_ctx;
    };

    /* yield doesn't take a value and returns args */
    template<typename ...A>
    struct coro_base<void, A...> {
    protected:
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        coro_base(coro_base const &) = delete;
        coro_base(coro_base &&c):
            p_args(std::move(c.p_args)),
            p_ctx(std::move(c.p_ctx))
        {
            p_ctx.set_data(this);
        }

        coro_base &operator=(coro_base const &) = delete;
        coro_base &operator=(coro_base &&c) {
            std::swap(p_args, c.p_args);
            std::swap(p_ctx, c.p_ctx);
            p_ctx.set_data(this);
            return *this;
        }

        struct yielder {
            yielder(coro_base<void, A...> &coro): p_coro(coro) {}

            coro_args<A...> operator()() {
                p_coro.p_ctx.yield_jump();
                return yield_ret(
                    p_coro.p_args, std::make_index_sequence<sizeof...(A)>{}
                );
            }
        private:
            coro_base<void, A...> &p_coro;
        };

        template<typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            func(yielder{*this}, std::forward<A>(std::get<I>(p_args))...);
        }

        void call(A ...args) {
            p_args = std::make_tuple(arg_wrapper<A>(std::forward<A>(args))...);
            p_ctx.call();
        }

        void swap(coro_base &other) {
            std::swap(p_args, other.p_args);
            std::swap(p_ctx, other.p_ctx);
        }

        std::tuple<arg_wrapper<A>...> p_args;
        coroutine_context p_ctx;
    };

    /* yield doesn't take a value or return any args */
    template<>
    struct coro_base<void> {
    protected:
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        coro_base(coro_base const &) = delete;
        coro_base(coro_base &&c): p_ctx(std::move(c.p_ctx)) {
            p_ctx.set_data(this);
        }

        coro_base &operator=(coro_base const &) = delete;
        coro_base &operator=(coro_base &&c) {
            std::swap(p_ctx, c.p_ctx);
            p_ctx.set_data(this);
            return *this;
        }

        struct yielder {
            yielder(coro_base<void> &coro): p_coro(coro) {}

            void operator()() {
                p_coro.p_ctx.yield_jump();
            }
        private:
            coro_base<void> &p_coro;
        };

        template<typename F, size_t ...I>
        void call_helper(F &func, std::index_sequence<I...>) {
            func(yielder{*this});
        }

        void call() {
            p_ctx.call();
        }

        void swap(coro_base &other) {
            std::swap(p_ctx, other.p_ctx);
        }

        coroutine_context p_ctx;
    };
} /* namespace detail */

template<typename R, typename ...A>
struct coroutine<R(A...)>: detail::coro_base<R, A...> {
private:
    using base_t = detail::coro_base<R, A...>;

public:
    using yield_type = typename detail::coro_base<R, A...>::yielder;

    template<typename F>
    coroutine(F func, size_t ss = COROUTINE_DEFAULT_STACK_SIZE):
        detail::coro_base<R, A...>(&context_call, ss), p_func(std::move(func))
    {}

    coroutine(coroutine const &) = delete;
    coroutine(coroutine &&) = default;

    coroutine &operator=(coroutine const &) = delete;
    coroutine &operator=(coroutine &&) = default;

    ~coroutine() {
        this->p_ctx.unwind();
    }

    operator bool() const {
        return !this->p_ctx.is_done();
    }

    R resume(A ...args) {
        return this->call(std::forward<A>(args)...);
    }

    R operator()(A ...args) {
        return this->call(std::forward<A>(args)...);
    }

    void swap(coroutine &other) {
        std::swap(p_func, other.p_func);
        base_t::swap(other);
    }

private:
    static void context_call(void *data) {
        coroutine &self = *(static_cast<coroutine *>(data));
        self.call_helper(self.p_func, std::index_sequence_for<A...>{});
    }

    std::function<R(yield_type, A...)> p_func;
};

template<typename R, typename ...A>
inline void swap(coroutine<R(A...)> &a, coroutine<R(A...)> &b) {
    a.swap(b);
}

template<typename T>
struct coroutine_range: input_range<coroutine_range<T>> {
    using range_category  = input_range_tag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = stream_off_t;

    coroutine_range() = delete;
    coroutine_range(coroutine<T()> &c): p_coro(&c) {
        pop_front();
    }
    coroutine_range(coroutine_range const &r):
        p_coro(r.p_coro), p_item(r.p_item) {}

    bool empty() const {
        return !p_item;
    }

    void pop_front() {
        if (*p_coro) {
            p_item = (*p_coro)();
        } else {
            p_item = std::nullopt;
        }
    }

    reference front() const {
        return p_item.value();
    }

    bool equals_front(coroutine_range const &g) {
        return p_coro == g.p_coro;
    }
private:
    coroutine<T()> *p_coro;
    mutable std::optional<T> p_item;
};

namespace detail {
    template<typename R>
    coroutine_range<R> coro_base<R>::iter() {
        return coroutine_range<R>{static_cast<coroutine<R()> &>(*this)};
    }
}

} /* namespace ostd */

#endif
