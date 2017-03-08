/* Context switching for coroutines.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_CONTEXT_HH
#define OSTD_INTERNAL_CONTEXT_HH

#include <exception>
#include <utility>

#include "ostd/types.hh"
#include "ostd/platform.hh"
#include "ostd/context_stack.hh"

namespace ostd {
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

struct coroutine_context {
protected:
    struct forced_unwind {
        fcontext_t ctx;
        forced_unwind(fcontext_t c): ctx(c) {}
    };

    enum class state {
        INIT = 0, HOLD, EXEC, TERM
    };

    coroutine_context() {}

    coroutine_context(coroutine_context const &) = delete;
    coroutine_context(coroutine_context &&c):
        p_stack(std::move(c.p_stack)), p_coro(c.p_coro), p_orig(c.p_orig),
        p_except(std::move(c.p_except)), p_state(std::move(c.p_state)),
        p_sa(c.p_sa)
    {
        c.p_coro = c.p_orig = nullptr;
        c.p_stack = { nullptr, 0 };
        c.p_state = state::TERM;
        c.p_sa = nullptr;
    }

    coroutine_context &operator=(coroutine_context const &) = delete;
    coroutine_context &operator=(coroutine_context &&c) {
        swap(c);
        return *this;
    }

    void call() {
        p_state = state::EXEC;
        coro_jump();
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    void unwind() {
        if (p_state == state::INIT) {
            /* this coroutine never got to live :(
             * let it call the entry point at least this once...
             * this will kill the stack so we don't leak memory
             */
            coro_jump();
            return;
        }
        ostd_ontop_fcontext(
            std::exchange(p_coro, nullptr), nullptr,
            [](transfer_t t) -> transfer_t {
                throw forced_unwind{t.ctx};
            }
        );
    }

    template<typename SA>
    void finish() {
        ostd_ontop_fcontext(p_orig, this, [](transfer_t t) -> transfer_t {
            auto &self = *(static_cast<coroutine_context *>(t.data));
            auto &sa = *(static_cast<SA *>(self.p_sa));
            sa.deallocate(self.p_stack);
            return { nullptr, nullptr };
        });
    }

    void coro_jump() {
        p_coro = ostd_jump_fcontext(p_coro, this).ctx;
    }

    void yield_jump() {
        p_state = state::HOLD;
        p_orig = ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    void swap(coroutine_context &other) noexcept {
        using std::swap;
        swap(p_stack, other.p_stack);
        swap(p_coro, other.p_coro);
        swap(p_orig, other.p_orig);
        swap(p_except, other.p_except);
    }

    template<typename SA>
    void make_context(SA &sa, void (*callp)(transfer_t)) {
        p_stack = sa.allocate();

        constexpr size_t salign = alignof(SA);
        constexpr size_t sasize = sizeof(SA);
        void *sp = static_cast<byte *>(p_stack.ptr) - sasize - salign;
        size_t space = sasize + salign;
        sp = std::align(salign, sasize, sp, space);
        size_t asize = p_stack.size -
            (static_cast<byte *>(p_stack.ptr) - static_cast<byte *>(sp));

        p_coro = ostd_make_fcontext(sp, asize, callp);
        p_sa = new (sp) SA(std::move(sa));
    }

    stack_context p_stack;
    fcontext_t p_coro;
    fcontext_t p_orig;
    std::exception_ptr p_except;
    state p_state = state::INIT;
    void *p_sa;
};

} /* namespace detail */
} /* namespace ostd */

#endif
