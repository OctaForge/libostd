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
 * @include glob.cc
 * @include listdir.cc
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_FILESYSTEM_HH
#define OSTD_FILESYSTEM_HH

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

#include "ostd/platform.hh"
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
    OSTD_EXPORT bool glob_match_filename_impl(
        typename filesystem::path::value_type const *fname,
        typename filesystem::path::value_type const *wname
    ) noexcept;

    OSTD_EXPORT void glob_match_impl(
        void (*out)(filesystem::path const &, void *),
        typename filesystem::path::iterator beg,
        typename filesystem::path::iterator &end,
        filesystem::path pre, void *data
    );
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
    detail::glob_match_impl([](filesystem::path const &p, void *outp) {
        static_cast<std::remove_reference_t<OutputRange> *>(outp)->put(p);
    }, path.begin(), pend, filesystem::path{}, &out);
    return std::forward<OutputRange>(out);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
