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

/** @} */

} /* namespace ostd */

#endif

/** @} */
