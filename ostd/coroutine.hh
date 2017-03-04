/* Coroutines for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for further information.
 */

#ifndef OSTD_COROUTINE_HH
#define OSTD_COROUTINE_HH

/* currently there is only POSIX support using obsolete ucontext stuff...
 * we will want to implement Windows support using its fibers and also
 * lightweight custom context switching with handwritten asm where we
 * want this to run, but ucontext will stay as a fallback
 */

#include <signal.h>
#include <ucontext.h>

#include <memory>
#include <exception>
#include <stdexcept>
#include <utility>
#include <tuple>

#include "ostd/types.hh"

namespace ostd {

constexpr size_t COROUTINE_DEFAULT_STACK_SIZE = SIGSTKSZ;

struct coroutine_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct coroutine_context {
    coroutine_context(size_t ss, void (*callp)(void *), void *data):
        p_stack(new byte[ss])
    {
        getcontext(&p_coro);
        p_coro.uc_link = &p_orig;
        p_coro.uc_stack.ss_sp = p_stack.get();
        p_coro.uc_stack.ss_size = ss;
        using mcfp = void (*)();
        using cpfp = void (*)(void *);
        if constexpr(sizeof(void *) > sizeof(int)) {
            union intu {
                struct { int p1, p2; };
                void *p;
                cpfp fp;
            };
            intu ud, uf;
            ud.p = data;
            uf.fp = callp;
            using amcfp = void (*)(int, int, int, int);
            amcfp mcf = [](int f1, int f2, int d1, int d2) -> void {
                intu ud2, uf2;
                uf2.p1 = f1, uf2.p2 = f2;
                ud2.p1 = d1, ud2.p2 = d2;
                (uf2.fp)(ud2.p);
            };
            makecontext(
                &p_coro, reinterpret_cast<mcfp>(mcf), 4,
                uf.p1, uf.p2, ud.p1, ud.p2
            );
        } else {
            using amcfp = void (*)(int, int);
            amcfp mcf = [](int f1, int d1) {
                reinterpret_cast<cpfp>(f1)(reinterpret_cast<void *>(d1));
            };
            makecontext(&p_coro, reinterpret_cast<mcfp>(mcf), 2, callp, data);
        }
    }

    void call() {
        if (p_finished) {
            throw coroutine_error{"dead coroutine"};
        }
        swapcontext(&p_orig, &p_coro);
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    void yield_swap() {
        swapcontext(&p_coro, &p_orig);
    }

    void set_eh() {
        p_except = std::current_exception();
    }

    void set_done() {
        p_finished = true;
    }

    bool is_done() const {
        return p_finished;
    }

private:
    /* TODO: new'ing the stack is sub-optimal */
    std::unique_ptr<byte[]> p_stack;
    ucontext_t p_coro;
    ucontext_t p_orig;
    std::exception_ptr p_except;
    bool p_finished = false;
};

template<typename T>
struct coroutine;

namespace detail {
    /* we need this because yield is specialized based on result */
    template<typename R, typename ...A>
    struct coro_base {
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        std::tuple<A &&...> yield(R &&ret) {
            p_result = std::forward<R>(ret);
            p_ctx.yield_swap();
            return std::move(p_args);
        }

    protected:
        R ctx_call(A ...args) {
            p_args = std::forward_as_tuple(std::forward<A>(args)...);
            p_ctx.call();
            return std::forward<R>(p_result);
        }

        std::tuple<A...> p_args;
        R p_result;
        coroutine_context p_ctx;
    };

    template<typename ...A>
    struct coro_base<void, A...> {
        coro_base(void (*callp)(void *), size_t ss):
            p_ctx(ss, callp, this)
        {}

        std::tuple<A &&...> yield() {
            p_ctx.yield_swap();
            return std::move(p_args);
        }

    protected:
        void ctx_call(A ...args) {
            p_args = std::forward_as_tuple(std::forward<A>(args)...);
            p_ctx.call();
        }

        std::tuple<A...> p_args;
        coroutine_context p_ctx;
    };
} /* namespace detail */

template<typename R, typename ...A>
struct coroutine<R(A...)>: detail::coro_base<R, A...> {
    coroutine(
        std::function<R(coroutine<R(A...)> &, A...)> func,
        size_t ss = COROUTINE_DEFAULT_STACK_SIZE
    ):
        detail::coro_base<R, A...>(&ctx_func, ss), p_func(std::move(func))
    {}

    operator bool() const {
        return this->p_ctx.is_done();
    }

    R operator()(A ...args) {
        return this->ctx_call(std::forward<A>(args)...);
    }
private:
    template<size_t ...I>
    R call(std::index_sequence<I...>) {
        return p_func(*this, std::forward<A>(std::get<I>(this->p_args))...);
    }

    static void ctx_func(void *data) {
        coroutine &self = *(static_cast<coroutine *>(data));
        try {
            using indices = std::index_sequence_for<A...>;
            if constexpr(std::is_same_v<R, void>) {
                self.call(indices{});
            } else {
                self.p_result = self.call(indices{});
            }
        } catch (...) {
            self.p_ctx.set_eh();
        }
        self.p_ctx.set_done();
    }

    std::function<R(coroutine<R(A...)> &, A...)> p_func;
};

} /* namespace ostd */

#endif
