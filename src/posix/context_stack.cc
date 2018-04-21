/* Stack allocation implementation for coroutine contexts.
 * For POSIX systems only, other implementations are stored elsewhere.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#ifndef OSTD_PLATFORM_POSIX
#  error "Incorrect platform"
#endif

#include <cstddef>
#include <cstdlib>
#include <new>
#include <mutex>

#include "ostd/context_stack.hh"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>

namespace ostd {

namespace detail {
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

    OSTD_EXPORT void *stack_alloc(std::size_t sz) {
        if constexpr(CONTEXT_USE_MMAP) {
            void *p = mmap(
                nullptr, sz, PROT_READ | PROT_WRITE,
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
    }

    OSTD_EXPORT void stack_free(void *p, std::size_t sz) noexcept {
        if constexpr(CONTEXT_USE_MMAP) {
            munmap(p, sz);
        } else {
            std::free(p);
        }
    }

    OSTD_EXPORT std::size_t stack_main_size() noexcept {
        struct rlimit l;
        getrlimit(RLIMIT_STACK, &l);
        return std::size_t(l.rlim_cur);
    }

    OSTD_EXPORT void stack_protect(void *p, std::size_t sz) noexcept {
        mprotect(p, sz, PROT_NONE);
    }

    /* used by stack traits */
    inline void ctx_pagesize(std::size_t *s) noexcept {
        *s = std::size_t(sysconf(_SC_PAGESIZE));
    }

    inline void ctx_rlimit_get(rlimit *l) noexcept {
        getrlimit(RLIMIT_STACK, l);
    }

    inline rlimit ctx_rlimit() noexcept {
        static rlimit l;
        static std::once_flag fl;
        std::call_once(fl, ctx_rlimit_get, &l);
        return l;
    }
} /* namespace detail */

OSTD_EXPORT bool stack_traits::is_unbounded() noexcept {
    return detail::ctx_rlimit().rlim_max == RLIM_INFINITY;
}

OSTD_EXPORT std::size_t stack_traits::page_size() noexcept {
    static std::size_t size = 0;
    static std::once_flag fl;
    std::call_once(fl, detail::ctx_pagesize, &size);
    return size;
}

OSTD_EXPORT std::size_t stack_traits::minimum_size() noexcept {
    /* typically 8 KiB but can be much larger on some platforms */
    return SIGSTKSZ;
}

OSTD_EXPORT std::size_t stack_traits::maximum_size() noexcept {
    /* can be RLIM_INFINITY, but that's ok, see above */
    return std::size_t(detail::ctx_rlimit().rlim_max);
}

OSTD_EXPORT std::size_t stack_traits::default_size() noexcept {
    /* default to at least 64 KiB (see minimum_size comment) */
    constexpr std::size_t r = std::max(8 * 8 * 1024, SIGSTKSZ);
    if (is_unbounded()) {
        return r;
    }
    std::size_t m = maximum_size();
    if (r > m) {
        return m;
    }
    return r;
}

} /* namespace ostd */
