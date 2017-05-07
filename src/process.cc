/* Process handling implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstddef>
#include <string>
#include <memory>
#include <new>

#include "ostd/process.hh"

#ifdef OSTD_PLATFORM_WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <unistd.h>
#  include <wordexp.h>
#endif

namespace ostd {
namespace detail {

OSTD_EXPORT void split_args_impl(
    string_range const &str, void (*func)(string_range, void *), void *data
) {
#ifndef OSTD_PLATFORM_WIN32
    std::string strs{str};

    wordexp_t p;
    if (int err; (err = wordexp(strs.data(), &p, 0))) {
        switch (err) {
        case WRDE_BADCHAR:
            throw word_error{"illegal character"};
        case WRDE_BADVAL:
            /* no WRDE_UNDEF flag, won't happen */
            throw word_error{"undefined shell variable"};
        case WRDE_CMDSUB:
            /* no WRDE_NOCMD flag, won't happen */
            throw word_error{"invalid command substitution"};
        case WRDE_NOSPACE:
            throw std::bad_alloc{};
        case WRDE_SYNTAX:
            throw word_error{"syntax error"};
        default:
            throw word_error{"unknown error"};
        }
    }

    /* if the output range throws, make sure stuff gets freed */
    struct wordexp_guard {
        void operator()(wordexp_t *fp) const {
            wordfree(fp);
        }
    };
    std::unique_ptr<wordexp_t, wordexp_guard> guard{&p};

    for (std::size_t i = 0; i < p.we_wordc; ++i) {
        func(p.we_wordv[i], data);
    }
#else
    std::unique_ptr<wchar_t[]> wstr{new wchar_t[str.size() + 1]};
    memset(wstr.get(), 0, (str.size() + 1) * sizeof(wchar_t));

    if (!MultiByteToWideChar(
        CP_UTF8, 0, str.data(), str.size(), wstr.get(), str.size() + 1
    )) {
        switch (GetLastError()) {
        case ERROR_NO_UNICODE_TRANSLATION:
            throw word_error{"unicode conversion failed"};
        default:
            throw word_error{"unknown error"};
        }
    }

    int argc = 0;
    wchar_t **pwargs = CommandLineToArgvW(wstr.get(), &argc);

    if (!pwargs) {
        throw word_error{"command line parsing failed"};
    }

    /* if anything throws, make sure stuff gets freed */
    struct wchar_guard {
        void operator()(wchar_t **p) const {
            LocalFree(p);
        }
    };
    std::unique_ptr<wchar_t *, wchar_guard> wguard{pwargs};

    for (int i = 0; i < argc; ++i) {
        wchar_t *arg = pwargs[i];
        std::size_t arglen = wcslen(arg);

        std::size_t req = 0;
        if (!(req = WideCharToMultiByte(
            CP_UTF8, 0, arg, arglen, nullptr, 0, nullptr, nullptr
        ))) {
            switch (GetLastError()) {
            case ERROR_NO_UNICODE_TRANSLATION:
                throw word_error{"unicode conversion failed"};
            default:
                throw word_error{"unknown error"};
            }
        }

        std::unique_ptr<char[]> buf{new char[req]};
        if (!WideCharToMultiByte(
            CP_UTF8, 0, arg, arglen, buf.get(), req, nullptr, nullptr
        )) {
            switch (GetLastError()) {
            case ERROR_NO_UNICODE_TRANSLATION:
                throw word_error{"unicode conversion failed"};
            default:
                throw word_error{"unknown error"};
            }
        }

        func(string_range{buf.get(), buf.get() + req - 1}, data);
    }
#endif
}

} /* namespace detail */
} /* namespace ostd */
