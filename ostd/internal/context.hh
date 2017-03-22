/* Context switching for coroutines.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_CONTEXT_HH
#define OSTD_INTERNAL_CONTEXT_HH

#include <exception>
#include <utility>
#include <typeinfo>

#include "ostd/types.hh"
#include "ostd/platform.hh"
#include "ostd/context_stack.hh"

namespace ostd {

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
    }

    coroutine_context(coroutine_context const &) = delete;
    coroutine_context(coroutine_context &&c):
        p_stack(std::move(c.p_stack)), p_coro(c.p_coro), p_orig(c.p_orig),
        p_except(std::move(c.p_except)), p_state(std::move(c.p_state))
    {
        c.p_coro = c.p_orig = nullptr;
        c.p_stack = { nullptr, 0 };
        c.set_dead();
    }

    coroutine_context &operator=(coroutine_context const &) = delete;
    coroutine_context &operator=(coroutine_context &&c) {
        swap(c);
        return *this;
    }

    void call() {
        p_state = state::EXEC;
        /* switch to new coroutine */
        coroutine_context *curr = std::exchange(detail::coro_current, this);
        coro_jump();
        /* switch back to previous */
        detail::coro_current = curr;
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    void coro_jump() {
        p_coro = detail::ostd_jump_fcontext(p_coro, this).ctx;
    }

    void yield_jump() {
        p_state = state::HOLD;
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

    void swap(coroutine_context &other) noexcept {
        using std::swap;
        swap(p_stack, other.p_stack);
        swap(p_coro, other.p_coro);
        swap(p_orig, other.p_orig);
        swap(p_except, other.p_except);
        swap(p_state, other.p_state);
    }

    template<typename C, typename SA>
    void make_context(SA &sa) {
        p_stack = sa.allocate();

        void *sp = get_stack_ptr<SA>();
        size_t asize = p_stack.size -
            (static_cast<byte *>(p_stack.ptr) - static_cast<byte *>(sp));

        p_coro = detail::ostd_make_fcontext(sp, asize, &context_call<C, SA>);
        new (sp) SA(std::move(sa));
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
            /* this coroutine never got to live :(
             * let it call the entry point at least this once...
             * this will kill the stack so we don't leak memory
             */
            coro_jump();
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
    void finish() {
        set_dead();
        detail::ostd_ontop_fcontext(
            p_orig, this, [](detail::transfer_t t) -> detail::transfer_t {
                auto &self = *(static_cast<coroutine_context *>(t.data));
                auto &sa = *(static_cast<SA *>(self.get_stack_ptr<SA>()));
                SA dsa{std::move(sa)};
                /* in case it holds any state that needs destroying */
                sa.~SA();
                dsa.deallocate(self.p_stack);
                return { nullptr, nullptr };
            }
        );
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
        self.template finish<SA>();
    }

    stack_context p_stack;
    detail::fcontext_t p_coro = nullptr;
    detail::fcontext_t p_orig = nullptr;
    std::exception_ptr p_except;
    state p_state = state::HOLD;
};

} /* namespace ostd */

#endif
