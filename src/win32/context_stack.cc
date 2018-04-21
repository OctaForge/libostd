/* Stack allocation implementation for coroutine contexts.
 * For Windows systems only, other implementations are stored elsewhere.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#ifndef OSTD_PLATFORM_WIN32
#  error "Incorrect platform"
#endif

#include <cstddef>
#include <cstdlib>
#include <new>
#include <mutex>

#include "ostd/context_stack.hh"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ostd {

namespace detail {
    OSTD_EXPORT void *stack_alloc(std::size_t sz) {
        void *p = VirtualAlloc(0, sz, MEM_COMMIT, PAGE_READWRITE);
        if (!p) {
            throw std::bad_alloc{};
        }
        return p;
    }

    OSTD_EXPORT void stack_free(void *p, std::size_t) noexcept {
        VirtualFree(p, 0, MEM_RELEASE);
    }

    OSTD_EXPORT std::size_t stack_main_size() noexcept {
        /* 4 MiB for Windows... */
    }

    OSTD_EXPORT void stack_protect(void *p, std::size_t sz) noexcept {
        DWORD oo;
        VirtualProtect(p, sz, PAGE_READWRITE | PAGE_GUARD, &oo);
    }

    /* used by stack traits */
    inline void ctx_pagesize(std::size_t *s) noexcept {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        *s = std::size_t(si.dwPageSize);
    }
} /* namespace detail */

OSTD_EXPORT bool stack_traits::is_unbounded() noexcept {
    return true;
}

OSTD_EXPORT std::size_t stack_traits::page_size() noexcept {
    static std::size_t size = 0;
    static std::once_flag fl;
    std::call_once(fl, detail::ctx_pagesize, &size);
    return size;
}

OSTD_EXPORT std::size_t stack_traits::minimum_size() noexcept {
    /* no func on Windows, sane default of 8 KiB */
    return 8 * 1024;
}

OSTD_EXPORT std::size_t stack_traits::maximum_size() noexcept {
    /* value is technically undefined when is_unbounded() is
     * true, just default to 1 GiB so we actually return something
     */
    return 1024 * 1024 * 1024;
}

OSTD_EXPORT std::size_t stack_traits::default_size() noexcept {
    /* no func on Windows either, default to 64 KiB */
    return 8 * 8 * 1024;
}

} /* namespace ostd */
