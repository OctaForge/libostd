/** @addtogroup Utilities
 * @{
 */

/** @file path.hh
 *
 * @brief A module for manipulation of filesystem paths.
 *
 * The path module implements facilities for manipulation of both pure
 * paths and actual filesystem paths. POSIX and Windows path encodings
 * are supported and common filesystem related operations are implemented
 * in a portable manner.
 *
 * This module replaces the C++17 filesystem module, aiming to be simpler
 * and higher level. For instance, it uses purely 8-bit encoding on all
 * platforms, taking care of conversions internally.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_PATH_HH
#define OSTD_PATH_HH

#include <utility>
#include <initializer_list>
#include <type_traits>

#include <ostd/string.hh>

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

struct path {
    static constexpr char separator = '/';

    enum class format {
        native = 0,
        posix,
        windows
    };

    template<typename R>
    path(R range, format fmt = format::native): p_fmt(fmt) {
        if constexpr(std::is_constructible_v<std::string, R const &>) {
            p_path = range;
        } else if (range.empty()) {
            p_path = ".";
        } else {
            p_path = range.front();
            range.pop_front();
            for (auto const &elem: range) {
                append(elem);
            }
        }
    }

    path(format fmt = format::native): path(".", fmt) {}

    template<typename T>
    path(std::initializer_list<T> init, format fmt = format::native):
        path(iter(init), fmt)
    {}

    path join(path const &p) const {
        path ret{*this};
        ret.append(p);
        return ret;
    }

    path &append(path const &p) {
        if (p_path != "/") {
            p_path.push_back('/');
        }
        p_path += p.string();
        return *this;
    }

    path &operator/=(path const &p) {
        return append(p);
    }

    string_range string() const {
        return p_path;
    }

    format path_format() const {
        return p_fmt;
    }

private:
    std::string p_path;
    format p_fmt;
};

inline path operator/(path const &p1, path const &p2) {
    return p1.join(p2);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
