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

#include "ostd/types.hh"
#include "ostd/platform.hh"

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

    void call() {
        if (p_finished) {
            throw coroutine_error{"dead coroutine"};
        }
        p_coro = detail::ostd_jump_fcontext(p_coro, this).ctx;
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

    void yield_jump() {
        p_coro = detail::ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    bool is_done() const {
        return p_finished;
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

template<typename T>
struct coroutine;

namespace detail {
    template<typename ...A>
    struct coro_types {
        using yield_type = std::tuple<A &&...>;
    };

    template<typename A>
    struct coro_types<A> {
        using yield_type = A &&;
    };

    template<typename A, typename B>
    struct coro_types<A, B> {
        using yield_type = std::pair<A &&, B &&>;
    };

    template<typename ...A>
    using coro_args = typename coro_types<A...>::yield_type;

    template<typename ...A, size_t ...I>
    inline coro_args<A...> yield_ret(
        std::tuple<A...> &args, std::index_sequence<I...>
    ) {
        if constexpr(sizeof...(A) == 1) {
            return std::move(std::get<0>(args));
        } else if constexpr(sizeof...(A) == 2) {
            return std::make_pair(std::forward<A>(std::get<I>(args))...);
        } else {
            return std::move(args);
        }
    }

    /* we need this because yield is specialized based on result */
    template<typename R, typename ...A>
    struct coro_base {
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        coro_args<A...> yield(R &&ret) {
            p_result = std::forward<R>(ret);
            p_ctx.yield_jump();
            return yield_ret(p_args, std::make_index_sequence<sizeof...(A)>{});
        }

    protected:
        std::tuple<A...> p_args;
        R p_result;
        coroutine_context p_ctx;
    };

    template<typename ...A>
    struct coro_base<void, A...> {
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        coro_args<A...> yield() {
            p_ctx.yield_jump();
            return yield_ret(p_args, std::make_index_sequence<sizeof...(A)>{});
        }

    protected:
        std::tuple<A...> p_args;
        coroutine_context p_ctx;
    };
} /* namespace detail */

template<typename R, typename ...A>
struct coroutine<R(A...)>: detail::coro_base<R, A...> {
    template<typename F>
    coroutine(F func, size_t ss = COROUTINE_DEFAULT_STACK_SIZE):
        detail::coro_base<R, A...>(&context_call, ss), p_func(std::move(func))
    {}

    ~coroutine() {
        this->p_ctx.unwind();
    }

    operator bool() const {
        return this->p_ctx.is_done();
    }

    R operator()(A ...args) {
        return this->call(std::forward<A>(args)...);
    }
private:
    R call(A ...args) {
        this->p_args = std::forward_as_tuple(std::forward<A>(args)...);
        this->p_ctx.call();
        if constexpr(!std::is_same_v<R, void>) {
            return std::forward<R>(this->p_result);
        }
    }

    template<size_t ...I>
    R call_helper(std::index_sequence<I...>) {
        return p_func(*this, std::forward<A>(std::get<I>(this->p_args))...);
    }

    static void context_call(void *data) {
        using indices = std::index_sequence_for<A...>;
        coroutine &self = *(static_cast<coroutine *>(data));
        if constexpr(std::is_same_v<R, void>) {
            self.call_helper(indices{});
        } else {
            self.p_result = self.call_helper(indices{});
        }
    }

    std::function<R(coroutine<R(A...)> &, A...)> p_func;
};

} /* namespace ostd */

#endif
