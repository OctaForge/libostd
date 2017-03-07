/* Context switching and stack allocation.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_CONTEXT_HH
#define OSTD_INTERNAL_CONTEXT_HH

#include <cstdlib>
#include <memory>
#include <exception>

#include "ostd/types.hh"
#include "ostd/platform.hh"
#include "ostd/internal/win32.hh"

namespace ostd {
namespace detail {

struct context_stack_t {
    void *ptr;
    size_t size;
};

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

context_stack_t context_stack_alloc(size_t ss);
void context_stack_free(context_stack_t &st);

struct coroutine_context {
protected:
    struct forced_unwind {
        fcontext_t ctx;
        forced_unwind(fcontext_t c): ctx(c) {}
    };

    coroutine_context() {}

    ~coroutine_context() {
        context_stack_free(p_stack);
    }

    coroutine_context(coroutine_context const &) = delete;
    coroutine_context(coroutine_context &&c):
        p_stack(std::move(c.p_stack)), p_coro(c.p_coro), p_orig(c.p_orig),
        p_except(std::move(c.p_except))
    {
        c.p_coro = c.p_orig = nullptr;
        c.p_stack = { nullptr, 0 };
    }

    coroutine_context &operator=(coroutine_context const &) = delete;
    coroutine_context &operator=(coroutine_context &&c) {
        swap(c);
        return *this;
    }

    void call() {
        coro_jump();
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    void unwind() {
        ostd_ontop_fcontext(
            std::exchange(p_coro, nullptr), nullptr,
            [](transfer_t t) -> transfer_t {
                throw forced_unwind{t.ctx};
            }
        );
    }

    void coro_jump() {
        p_coro = ostd_jump_fcontext(p_coro, this).ctx;
    }

    void yield_jump() {
        p_orig = ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    void swap(coroutine_context &other) noexcept {
        using std::swap;
        swap(p_stack, other.p_stack);
        swap(p_coro, other.p_coro);
        swap(p_orig, other.p_orig);
        swap(p_except, other.p_except);
    }

    void make_context(size_t ss, void (*callp)(transfer_t)) {
        p_stack = context_stack_alloc(ss);
        p_coro = ostd_make_fcontext(p_stack.ptr, p_stack.size, callp);
    }

    /* TODO: new'ing the stack is sub-optimal */
    context_stack_t p_stack = { nullptr, 0 };
    fcontext_t p_coro;
    fcontext_t p_orig;
    std::exception_ptr p_except;
};

/* stack allocator */

inline context_stack_t context_stack_alloc(size_t ss) {
    auto p = static_cast<byte *>(std::malloc(ss));
    if (!p) {
        throw std::bad_alloc{};
    }
    return { p + ss, ss };
}

inline void context_stack_free(context_stack_t &st) {
    if (!st.ptr) {
        return;
    }
    auto p = static_cast<byte *>(st.ptr) - st.size;
    std::free(p);
    st.ptr = nullptr;
}

}
}

#endif
