/* Stack allocation for coroutine contexts.
 * API more or less compatible with the Boost.Context library.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_CONTEXT_STACK_HH
#define OSTD_CONTEXT_STACK_HH

#include <new>
#include <algorithm>

#include "ostd/types.hh"
#include "ostd/platform.hh"

#if !defined(OSTD_PLATFORM_POSIX) && !defined(OSTD_PLATFORM_WIN32)
#  error "Unsupported platform"
#endif

/* we can do this as we only support clang 4+ and gcc 7+
 * which always have support for segmented stacks
 */
#ifdef OSTD_USE_SEGMENTED_STACKS
#  if !defined(OSTD_PLATFORM_POSIX) || \
    (!defined(OSTD_TOOLCHAIN_GNU) && !defined(OSTD__TOOLCHAIN_CLANG))
#    error "compiler/toolchain does not support segmented_stack stacks"
#  endif
#  define OSTD_CONTEXT_SEGMENTS 10
#endif

#ifdef OSTD_USE_VALGRIND
#  include <valgrind/valgrind.h>
#endif

namespace ostd {

struct stack_context {
#ifdef OSTD_USE_SEGMENTED_STACKS
    using segments_context = void *[OSTD_CONTEXT_SEGMENTS];
#endif
    void *ptr = nullptr;
    size_t size = 0;
#ifdef OSTD_USE_SEGMENTED_STACKS
    segments_context segments_ctx = {};
#endif
#ifdef OSTD_USE_VALGRIND
    int valgrind_id = 0;
#endif
};

struct OSTD_EXPORT stack_traits {
    static bool is_unbounded() noexcept;
    static size_t page_size() noexcept;
    static size_t minimum_size() noexcept;
    static size_t maximum_size() noexcept;
    static size_t default_size() noexcept;
};

namespace detail {
    OSTD_EXPORT void *stack_alloc(size_t sz);
    OSTD_EXPORT void stack_free(void *p, size_t sz) noexcept;
    OSTD_EXPORT void stack_protect(void *p, size_t sz) noexcept;
}

template<typename TR, bool Protected>
struct basic_fixedsize_stack {
    using traits_type = TR;

    basic_fixedsize_stack(size_t ss = 0) noexcept:
        p_size(ss ? ss : TR::default_size())
    {}

    stack_context allocate() {
        size_t ss = p_size;

        ss = std::clamp(ss, TR::minimum_size(), TR::maximum_size());
        size_t pgs = TR::page_size();
        size_t npg = std::max(ss / pgs, size_t(size_t(Protected) + 1));
        size_t asize = npg * pgs;

        void *p = detail::stack_alloc(asize);
        if constexpr(Protected) {
            /* a single guard page */
            detail::stack_protect(p, pgs);
        }

        stack_context ret{static_cast<byte *>(p) + ss, ss};
#ifdef OSTD_USE_VALGRIND
        ret.valgrind_id = VALGRIND_STACK_REGISTER(ret.ptr, p);
#endif
        return ret;
    }

    void deallocate(stack_context &st) noexcept {
        if (!st.ptr) {
            return;
        }
#ifdef OSTD_USE_VALGRIND
        VALGRIND_STACK_DEREGISTER(st.valgrind_id);
#endif
        detail::stack_free(static_cast<byte *>(st.ptr) - st.size, st.size);
        st.ptr = nullptr;
    }

private:
    size_t p_size;
};

using fixedsize_stack = basic_fixedsize_stack<stack_traits, false>;
using protected_fixedsize_stack = basic_fixedsize_stack<stack_traits, true>;

#ifdef OSTD_USE_SEGMENTED_STACKS
namespace detail {
    extern "C" {
        /* from libgcc */
        void *__splitstack_makecontext(
            size_t st_size, void *ctx[OSTD_CONTEXT_SEGMENTS], size_t *size
        );
        void __splitstack_releasecontext(void *ctx[OSTD_CONTEXT_SEGMENTS]);
        void __splitstack_resetcontext(void *ctx[OSTD_CONTEXT_SEGMENTS]);
        void __splitstack_block_signals_context(
            void *ctx[OSTD_CONTEXT_SEGMENTS], int *new_val, int *old_val
        );
    }
}

template<typename TR>
struct basic_segmented_stack {
    using traits_type = TR;

    basic_segmented_stack(size_t ss = 0) noexcept:
        p_size(ss ? ss : TR::default_size())
    {}

    stack_context allocate() {
        size_t ss = p_size;

        stack_context ret;
        void *p = detail::__splitstack_makecontext(
            ss, ret.segments_ctx, &ret.size
        );
        if (!p) {
            throw std::bad_alloc{};
        }

        ret.ptr = static_cast<byte *>(p) + ret.size;

        int off = 0;
        detail::__splitstack_block_signals_context(ret.segments_ctx, &off, 0);

        return ret;
    }

    void deallocate(stack_context &st) noexcept {
        detail::__splitstack_releasecontext(st.segments_ctx);
    }

private:
    size_t p_size;
};

using segmented_stack = basic_segmented_stack<stack_traits>;
#endif /* OSTD_USE_SEGMENTED_STACKS */

#ifdef OSTD_USE_SEGMENTED_STACKS
using default_stack = segmented_stack;
#else
using default_stack = fixedsize_stack;
#endif

} /* namespace ostd */

#endif
