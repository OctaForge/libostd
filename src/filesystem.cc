/* Filesystem implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <utility>

#include "ostd/filesystem.hh"

namespace ostd {
namespace detail {

inline typename filesystem::path::value_type const *glob_match_brackets(
    typename filesystem::path::value_type match,
    typename filesystem::path::value_type const *wp
) noexcept {
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

OSTD_EXPORT bool glob_match_filename_impl(
    typename filesystem::path::value_type const *fname,
    typename filesystem::path::value_type const *wname
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
        if (glob_match_filename_impl(fname, wname)) {
            return true;
        }
        ++fname;
    }
    return false;
}

OSTD_EXPORT void glob_match_impl(
    void (*out)(filesystem::path const &, void *),
    typename filesystem::path::iterator beg,
    typename filesystem::path::iterator &end,
    filesystem::path pre, void *data
) {
    while (beg != end) {
        auto cur = *beg;
        auto *cs = cur.c_str();
        /* this part of the path might contain wildcards */
        for (auto c = *cs; c; c = *++cs) {
            /* ** as a name does recursive expansion */
            if ((c == '*') && (*(cs + 1) == '*') && !*(cs + 2)) {
                ++beg;
                auto ip = pre.empty() ? "." : pre;
                if (!filesystem::is_directory(ip)) {
                    return;
                }
                filesystem::recursive_directory_iterator it{ip};
                /* include "current" dir in the match */
                if (beg != end) {
                    glob_match_impl(out, beg, end, pre, data);
                }
                for (auto &de: it) {
                    /* followed by more path, only consider dirs */
                    auto dp = de.path();
                    if ((beg != end) && !filesystem::is_directory(dp)) {
                        continue;
                    }
                    /* otherwise also match files */
                    if (pre.empty()) {
                        /* avoid ugly formatting, sadly we have to do
                         * with experimental fs api so no better way...
                         */
                        auto dpb = dp.begin(), dpe = dp.end();
                        if (*dpb == ".") {
                            ++dpb;
                        }
                        filesystem::path ap;
                        while (dpb != dpe) {
                            ap /= *dpb;
                            ++dpb;
                        }
                        glob_match_impl(out, beg, end, ap, data);
                    } else {
                        glob_match_impl(out, beg, end, dp, data);
                    }
                }
                return;
            }
            /* wildcards *, ?, [...] */
            if ((c == '*') || (c == '?') || (c == '[')) {
                ++beg;
                auto ip = pre.empty() ? "." : pre;
                if (!filesystem::is_directory(ip)) {
                    return;
                }
                filesystem::directory_iterator it{ip};
                for (auto &de: it) {
                    auto p = de.path().filename();
                    if (!glob_match_filename(p, cur)) {
                        continue;
                    }
                    glob_match_impl(out, beg, end, pre / p, data);
                }
                return;
            }
        }
        pre /= cur;
        ++beg;
    }
    out(pre, data);
}

} /* namespace detail */
} /* namespace ostd */
