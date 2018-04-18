/* Decides between POSIX and Windows for path.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <utility>

#include "ostd/platform.hh"

#if defined(OSTD_PLATFORM_WIN32)
#  include "src/win32/path.cc"
#elif defined(OSTD_PLATFORM_POSIX)
#  include "src/posix/path.cc"
#else
#  error "Unsupported platform"
#endif

namespace ostd {

/* place the vtables in here */

path_error::~path_error() {}

namespace fs {
    fs_error::~fs_error() {}
} /* namespace fs */

} /* namespace ostd */

namespace ostd {
namespace detail {

inline char const *glob_match_brackets(char match, char const *wp) noexcept {
    bool neg = (*wp == '!');
    if (neg) {
        ++wp;
    }

    /* grab the first character as it can be ] */
    auto c = *wp++;
    if (!c) {
        /* unterminated */
        return nullptr;
    }

    /* make sure it's terminated somewhere */
    auto *eb = wp;
    for (; *eb != ']'; ++eb) {
        if (!*eb) {
            return nullptr;
        }
    }
    ++eb;

    /* no need to worry about \0 from now on */
    do {
        /* character range */
        if ((*wp == '-') && (*(wp + 1) != ']')) {
            auto lc = *(wp + 1);
            wp += 2;
            if ((match >= c) && (match <= lc)) {
                return neg ? nullptr : eb;
            }
            c = *wp++;
            continue;
        }
        /* single-char match */
        if (match == c) {
            return neg ? nullptr : eb;
        }
        c = *wp++;
    } while (c != ']');

    /* loop ended, so no match */
    return neg ? eb : nullptr;
}

OSTD_EXPORT bool glob_match_path_impl(
    char const *fname, char const *wname
) noexcept {
    /* skip any matching prefix if present */
    while (*wname && (*wname != '*')) {
        if (!*wname || (*wname == '*')) {
            break;
        }
        if (*fname) {
            /* ? wildcard matches any character */
            if (*wname == '?') {
                ++wname;
                ++fname;
                continue;
            }
            /* [...] wildcard */
            if (*wname == '[') {
                wname = glob_match_brackets(*fname, wname + 1);
                if (!wname) {
                    return false;
                }
                ++fname;
                continue;
            }
        }
        if ((*wname == '?') && *fname) {
            ++wname;
            ++fname;
            continue;
        }
        if (*fname++ != *wname++) {
            return false;
        }
    }
    /* skip * wildcards; a wildcard matches 0 or more */
    if (*wname == '*') {
        while (*wname == '*') {
            ++wname;
        }
        /* was trailing so everything matches */
        if (!*wname) {
            return true;
        }
    }
    /* prefix skipping matched entire filename */
    if (!*fname) {
        return true;
    }
    /* empty pattern and non-empty filename */
    if (!*wname) {
        return false;
    }
    /* keep incrementing filename until it matches */
    while (*fname) {
        if (glob_match_path_impl(fname, wname)) {
            return true;
        }
        ++fname;
    }
    return false;
}

} /* namespace detail */
} /* namespace ostd */

namespace ostd {
namespace fs {
namespace detail {

OSTD_EXPORT void glob_match_impl(
    void (*out)(path const &, void *), typename path::range r,
    path pre, void *data
) {
    while (!r.empty()) {
        auto cur = std::string{r.front()};
        auto *cs = cur.c_str();
        /* this part of the path might contain wildcards */
        for (auto c = *cs; c; c = *++cs) {
            /* ** as a name does recursive expansion */
            if ((c == '*') && (*(cs + 1) == '*') && !*(cs + 2)) {
                r.pop_front();
                auto ip = pre.empty() ? "." : pre;
                if (!is_directory(ip)) {
                    return;
                }
                recursive_directory_range dr{ip};
                /* include "current" dir in the match */
                if (!r.empty()) {
                    glob_match_impl(out, r, pre, data);
                }
                for (auto &de: dr) {
                    /* followed by more path, only consider dirs */
                    auto dp = de.path();
                    if (!r.empty() && !is_directory(dp)) {
                        continue;
                    }
                    /* otherwise also match files */
                    glob_match_impl(out, r, dp, data);
                }
                return;
            }
            /* wildcards *, ?, [...] */
            if ((c == '*') || (c == '?') || (c == '[')) {
                r.pop_front();
                auto ip = pre.empty() ? "." : pre;
                if (!is_directory(ip)) {
                    return;
                }
                directory_range dr{ip};
                for (auto &de: dr) {
                    auto p = path{de.path().name()};
                    if (!p.match(cur)) {
                        continue;
                    }
                    glob_match_impl(out, r, pre / p, data);
                }
                return;
            }
        }
        pre /= cur;
        r.pop_front();
    }
    out(pre, data);
}

} /* namespace detail */
} /* namesapce fs */
} /* namespace ostd */