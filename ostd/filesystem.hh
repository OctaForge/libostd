/** @addtogroup Utilities
 * @{
 */

/** @file filesystem.hh
 *
 * @brief Filesystem abstraction module.
 *
 * This module defines the namespace ostd::filesystem, which is either
 * std::experimental::filesystem or std::filesystem depending on which
 * is available. For portable applications, only use the subset of the
 * module covered by both versions.
 *
 * It also provides range integration for directory iterators and
 * ostd::format_traits specialization for std::filesystem::path.
 *
 * Additionally, it implements glob matching following POSIX with its
 * own extensions (mainly recursive glob matching via `**`).
 *
 * @include listdir.cc
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_FILESYSTEM_HH
#define OSTD_FILESYSTEM_HH

#include <utility>

#if __has_include(<filesystem>)
#  include <filesystem>
namespace ostd {
    namespace filesystem = std::filesystem;
}
#elif __has_include(<experimental/filesystem>)
#  include <experimental/filesystem>
namespace ostd {
    namespace filesystem = std::experimental::filesystem;
}
#else
#  error "Unsupported platform"
#endif

#include "ostd/range.hh"
#include "ostd/format.hh"

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

/** @brief Range integration for std::filesystem::directory_iterator.
 *
 * Allows directory iterators to be made into ranges via ostd::iter().
 *
 * @see ostd::ranged_traits<filesystem::recursive_directory_iterator>
 */
template<>
struct ranged_traits<filesystem::directory_iterator> {
    /** @brief The range type for the iterator. */
    using range = iterator_range<filesystem::directory_iterator>;

    /** @brief Creates a range for the iterator. */
    static range iter(filesystem::directory_iterator const &r) {
        return range{filesystem::begin(r), filesystem::end(r)};
    }
};

/** @brief Range integration for std::filesystem::recursive_directory_iterator.
 *
 * Allows recursive directory iterators to be made into ranges via ostd::iter().
 *
 * @see ostd::ranged_traits<filesystem::directory_iterator>
 */
template<>
struct ranged_traits<filesystem::recursive_directory_iterator> {
    /** @brief The range type for the iterator. */
    using range = iterator_range<filesystem::recursive_directory_iterator>;

    /** @brief Creates a range for the iterator. */
    static range iter(filesystem::recursive_directory_iterator const &r) {
        return range{filesystem::begin(r), filesystem::end(r)};
    }
};

/** @brief ostd::format_traits specialization for std::filesystem::path.
 *
 * This allows paths to be formatted as strings. The value is formatted
 * as if `path.string()` was formatted, using the exact ostd::format_spec.
 */
template<>
struct format_traits<filesystem::path> {
    /** @brief Formats the path's string value.
     *
     * This calls exactly
     *
     * ~~~{.cc}
     * fs.format_value(writer, p.string());
     * ~~~
     */
    template<typename R>
    static void to_format(
        filesystem::path const &p, R &writer, format_spec const &fs
    ) {
        fs.format_value(writer, p.string());
    }
};

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
                bool within = ((match >= c) && (match <= lc));
                if (within) {
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

    inline bool glob_match_filename_impl(
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
} /* namespace detail */

/** @brief Checks if the given path matches the given glob pattern.
 *
 * This matches the given filename against POSIX-style glob patterns.
 * The following patterns are supported:
 *
 * | Pattern | Description                                        |
 * |---------|----------------------------------------------------|
 * | *       | 0 or more characters                               |
 * | ?       | any single character                               |
 * | [abc]   | one character in the brackets                      |
 * | [a-z]   | one character within the range in the brackets     |
 * | [!abc]  | one character not in the brackets                  |
 * | [!a-z]  | one character not within the range in the brackets |
 *
 * The behavior is the same as in POSIX. You can combine ranges and
 * individual characters in the `[]` pattern together as well as define
 * multiple ranges in one (e.g. `[a-zA-Z_?]` matching alphabetics,
 * an underscore and a question mark). The behavior of the range varies
 * by locale. If the second character in the range is lower in value
 * than the first one, a match will never happen. To match the `]`
 * character in the brackets, make it the first one. To match the
 * dash character, make it the first or the last.
 *
 * You can also use the brackets to escape metacharacters. So to
 * match a literal `*`, use `[*]`.
 *
 * Keep in mind that an invalid bracket syntax (unterminated) will
 * always cause this to return `false`.
 *
 * This function is used in ostd::glob_match().
 */
inline bool glob_match_filename(
    filesystem::path const &filename, filesystem::path const &pattern
) noexcept {
    return detail::glob_match_filename_impl(filename.c_str(), pattern.c_str());
}

namespace detail {
    template<typename OR>
    inline void glob_match_impl(
        OR &out,
        typename filesystem::path::iterator beg,
        typename filesystem::path::iterator &end,
        filesystem::path pre
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
                        glob_match_impl(out, beg, end, pre);
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
                            glob_match_impl(out, beg, end, ap);
                        } else {
                            glob_match_impl(out, beg, end, dp);
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
                        glob_match_impl(out, beg, end, pre / p);
                    }
                    return;
                }
            }
            pre /= cur;
            ++beg;
        }
        out.put(pre);
    }
}

/** @brief Expands a path with glob patterns.
 *
 * Individual expanded paths are put in `out` and are of the standard
 * std::filesystem::path type. It supports standard patterns as defined
 * in ostd::glob_match_filename().
 *
 * So for example, `*.cc` will expand to `one.cc`, `two.cc` and so on.
 * A pattern like `foo/[cb]at.txt` will match `foo/cat.txt` and `foo/bat.txt`
 * but not `foo/Cat.txt`. The `foo/?at.txt` will match `foo/cat.txt`,
 * `foo/Cat.txt`, `foo/pat.txt`, `foo/vat.txt` or any other character
 * in the place.
 *
 * Additionally, a special `**` pattern is also supported which is not
 * matched by ostd::glob_match_filename(). It's only allowed if the entire
 * filename or directory name is `**`. When used as a directory name, it
 * will expand to all directories in the location and all subdirectories
 * of those directories. If used as a filename (at the end of the path),
 * then it expands to directories and subdirectories aswell as all files
 * in the location and in the directories or subdirectories. Keep in mind
 * that it is not a regular pattern and a `**` when found in a regular
 * context (i.e. not as entire filename/directory name) will be treated
 * as two regular `*` patterns.
 *
 * @throws std::filesystem_error if a filesystem error occurs.
 * @returns The forwarded `out`.
 */
template<typename OutputRange>
inline OutputRange &&glob_match(
    OutputRange &&out, filesystem::path const &path
) {
    auto pend = path.end();
    detail::glob_match_impl(out, path.begin(), pend, filesystem::path{});
    return std::forward<OutputRange>(out);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
