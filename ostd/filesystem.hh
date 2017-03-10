/* Filesystem API for OctaSTD. Currently POSIX only.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
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

template<>
struct ranged_traits<filesystem::directory_iterator> {
    using range = iterator_range<filesystem::directory_iterator>;

    static range iter(filesystem::directory_iterator const &r) {
        return range{filesystem::begin(r), filesystem::end(r)};
    }
};

template<>
struct ranged_traits<filesystem::recursive_directory_iterator> {
    using range = iterator_range<filesystem::recursive_directory_iterator>;

    static range iter(filesystem::recursive_directory_iterator const &r) {
        return range{filesystem::begin(r), filesystem::end(r)};
    }
};

template<>
struct format_traits<filesystem::path> {
    template<typename R>
    static void to_format(
        filesystem::path const &p, R &writer, format_spec const &fs
    ) {
        fs.format_value(writer, p.string());
    }
};

} /* namespace ostd */

#endif
