/* Context switching and stack allocation.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_CONTEXT_HH
#define OSTD_INTERNAL_CONTEXT_HH

#include <cstdlib>
#include <memory>
#include <exception>
#include <algorithm>

#include "ostd/types.hh"
#include "ostd/platform.hh"
#include "ostd/internal/win32.hh"

#ifdef OSTD_PLATFORM_POSIX
#  include <unistd.h>
#  include <sys/mman.h>
#  include <sys/resource.h>
#  include <sys/time.h>
#  include <signal.h>
#endif

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

#if defined(OSTD_PLATFORM_WIN32)

inline size_t context_get_page_size() {
    SYSTEM_INFO i;
    GetSystemInfo(&i);
    return size_t(si.dwPageSize);
}

inline size_t context_stack_get_min_size() {
    /* no func on windows, sane default */
    return sizeof(void *) * 1024;
}

inline size_t context_stack_get_max_size() {
    return 1024 * 1024 * 1024;
}

inline size_t context_stack_get_def_size() {
    return sizeof(void *) * context_stack_get_min_size();
}

#elif defined(OSTD_PLATFORM_POSIX)

inline size_t context_get_page_size() {
    return size_t(sysconf(_SC_PAGESIZE));
}

inline size_t context_stack_get_min_size() {
    return SIGSTKSZ;
}

inline size_t context_stack_get_max_size() {
    rlimit l;
    getrlimit(RLIMIT_STACK, &l);
    return size_t(l.rlim_max);
}

inline size_t context_stack_get_def_size() {
    rlimit l;
    size_t r = sizeof(void *) * SIGSTKSZ;

    getrlimit(RLIMIT_STACK, &l);
    if ((l.rlim_max != RLIM_INFINITY) && (l.rlim_max < r)) {
        return size_t(l.rlim_max);
    }

    return r;
}

#if defined(MAP_ANON) || defined(MAP_ANONYMOUS)
constexpr bool CONTEXT_USE_MMAP = true;
#  ifdef MAP_ANON
constexpr auto CONTEXT_MAP_ANON = MAP_ANON;
#  else
constexpr auto CONTEXT_MAP_ANON = MAP_ANONYMOUS;
#  endif
#else
constexpr bool CONTEXT_USE_MMAP = false;
#endif

#else /* OSTD_PLATFORM_POSIX */
#  error "Unsupported platform"
#endif

inline context_stack_t context_stack_alloc(size_t ss) {
    if (!ss) {
        ss = context_stack_get_def_size();
    } else {
        ss = std::clamp(
            ss, context_stack_get_min_size(), context_stack_get_max_size()
        );
    }
    size_t pgs = context_get_page_size();
    size_t npg = std::max(ss / pgs, size_t(2));
    size_t asize = npg * pgs;

#if defined(OSTD_PLATFORM_WIN32)
    void *p = VirtualAlloc(0, asize, MEM_COMMIT, PAGE_READWRITE);
    if (!p) {
        throw std::bad_alloc{}
    }
    DWORD oo;
    VirtualProtect(p, pgs, PAGE_READWRITE | PAGE_GUARD, &oo);
#elif defined(OSTD_PLATFORM_POSIX)
    void *p = nullptr;
    if constexpr(CONTEXT_USE_MMAP) {
        void *mp = mmap(
            0, asize, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | CONTEXT_MAP_ANON, -1, 0
        );
        if (mp != MAP_FAILED) {
            p = mp;
        }
    } else {
        p = std::malloc(asize);
    }
    if (!p) {
        throw std::bad_alloc{};
    }
    mprotect(p, pgs, PROT_NONE);
#endif

    return { static_cast<byte *>(p) + ss, ss };
}

inline void context_stack_free(context_stack_t &st) {
    if (!st.ptr) {
        return;
    }

    auto p = static_cast<byte *>(st.ptr) - st.size;
#if defined(OSTD_PLATFORM_WIN32)
    VirtualFree(p, 0, MEM_RELEASE);
#elif defined(OSTD_PLATFORM_POSIX)
    if constexpr(CONTEXT_USE_MMAP) {
        munmap(p, st.size);
    } else {
        std::free(p);
    }
#endif

    st.ptr = nullptr;
}

}
}

#endif
