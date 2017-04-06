/* Stack allocation implementation for coroutine contexts.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdlib>
#include <new>
#include <mutex>

#include "ostd/internal/win32.hh"
#include "ostd/platform.hh"
#include "ostd/context_stack.hh"

#ifdef OSTD_PLATFORM_POSIX
#  include <unistd.h>
#  include <sys/mman.h>
#  include <sys/resource.h>
#  include <sys/time.h>
#  include <signal.h>
#endif

namespace ostd {

namespace detail {
#ifdef OSTD_PLATFORM_POSIX
#  if defined(MAP_ANON) || defined(MAP_ANONYMOUS)
    constexpr bool CONTEXT_USE_MMAP = true;
#    ifdef MAP_ANON
    constexpr auto CONTEXT_MAP_ANON = MAP_ANON;
#    else
    constexpr auto CONTEXT_MAP_ANON = MAP_ANONYMOUS;
#    endif
#  else
    constexpr bool CONTEXT_USE_MMAP = false;
#  endif
#endif

    OSTD_EXPORT void *stack_alloc(size_t sz) {
#if defined(OSTD_PLATFORM_WIN32)
        void *p = VirtualAlloc(0, sz, MEM_COMMIT, PAGE_READWRITE);
        if (!p) {
            throw std::bad_alloc{};
        }
        return p;
#elif defined(OSTD_PLATFORM_POSIX)
        if constexpr(CONTEXT_USE_MMAP) {
            void *p = mmap(
                0, sz, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | CONTEXT_MAP_ANON, -1, 0
            );
            if (p == MAP_FAILED) {
                throw std::bad_alloc{};
            }
            return p;
        }
        void *p = std::malloc(sz);
        if (!p) {
            throw std::bad_alloc{};
        }
        return p;
#endif
    }

    OSTD_EXPORT void stack_free(void *p, size_t sz) noexcept {
#if defined(OSTD_PLATFORM_WIN32)
        (void)sz;
        VirtualFree(p, 0, MEM_RELEASE);
#elif defined(OSTD_PLATFORM_POSIX)
        if constexpr(CONTEXT_USE_MMAP) {
            munmap(p, sz);
        } else {
            std::free(p);
        }
#endif
    }

    OSTD_EXPORT size_t stack_main_size() noexcept {
#if defined(OSTD_PLATFORM_WIN32)
        /* 4 MiB for windows... */
        return 4 * 1024 * 1024;
#else
        struct rlimit l;
        getrlimit(RLIMIT_STACK, &l);
        return size_t(l.rlim_cur);
#endif
    }

    OSTD_EXPORT void stack_protect(void *p, size_t sz) noexcept {
#if defined(OSTD_PLATFORM_WIN32)
        DWORD oo;
        VirtualProtect(p, sz, PAGE_READWRITE | PAGE_GUARD, &oo);
#elif defined(OSTD_PLATFORM_POSIX)
        mprotect(p, sz, PROT_NONE);
#endif
    }

    /* used by stack traits */
    inline void ctx_pagesize(size_t *s) noexcept {
#if defined(OSTD_PLATFORM_WIN32)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        *s = size_t(si.dwPageSize);
#elif defined(OSTD_PLATFORM_POSIX)
        *s = size_t(sysconf(_SC_PAGESIZE));
#endif
    }

#ifdef OSTD_PLATFORM_POSIX
    inline void ctx_rlimit_get(rlimit *l) noexcept {
        getrlimit(RLIMIT_STACK, l);
    }

    inline rlimit ctx_rlimit() noexcept {
        static rlimit l;
        static std::once_flag fl;
        std::call_once(fl, ctx_rlimit_get, &l);
        return l;
    }
#endif
} /* namespace detail */

bool stack_traits::is_unbounded() noexcept {
#if defined(OSTD_PLATFORM_WIN32)
    return true;
#elif defined(OSTD_PLATFORM_POSIX)
    return detail::ctx_rlimit().rlim_max == RLIM_INFINITY;
#endif
}

size_t stack_traits::page_size() noexcept {
    static size_t size = 0;
    static std::once_flag fl;
    std::call_once(fl, detail::ctx_pagesize, &size);
    return size;
}

size_t stack_traits::minimum_size() noexcept {
#if defined(OSTD_PLATFORM_WIN32)
    /* no func on windows, sane default of 8 KiB */
    return 8 * 1024;
#elif defined(OSTD_PLATFORM_POSIX)
    /* typically 8 KiB but can be much larger on some platforms */
    return SIGSTKSZ;
#endif
}

size_t stack_traits::maximum_size() noexcept {
#if defined(OSTD_PLATFORM_WIN32)
    /* value is technically undefined when is_unbounded() is
     * true, just default to 1 GiB so we actually return something
     */
    return 1024 * 1024 * 1024;
#elif defined(OSTD_PLATFORM_POSIX)
    /* can be RLIM_INFINITY, but that's ok, see above */
    return size_t(detail::ctx_rlimit().rlim_max);
#endif
}

size_t stack_traits::default_size() noexcept {
#if defined(OSTD_PLATFORM_WIN32)
    /* no func on windows either, default to 64 KiB */
    return 8 * 8 * 1024;
#elif defined(OSTD_PLATFORM_POSIX)
    /* default to at least 64 KiB (see minimum_size comment) */
    constexpr size_t r = std::max(8 * 8 * 1024, SIGSTKSZ);
    if (is_unbounded()) {
        return r;
    }
    size_t m = maximum_size();
    if (r > m) {
        return m;
    }
    return r;
#endif
}

struct coroutine_context;

namespace detail {
    OSTD_EXPORT thread_local coroutine_context *coro_current = nullptr;
}

} /* namespace ostd */
