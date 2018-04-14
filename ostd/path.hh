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

#include <string.h>

#include <utility>
#include <initializer_list>
#include <type_traits>
#include <system_error>

#include <ostd/platform.hh>
#include <ostd/string.hh>
#include <ostd/format.hh>

/* path representation is within ostd namespace, there aren't any APIs to
 * do actual filesystem manipulation, that's all in the fs namespace below
 */

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

struct path {
#ifdef OSTD_PLATFORM_WIN32
    static constexpr char native_separator = '\\';
#else
    static constexpr char native_separator = '/';
#endif

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

    path(path const &p):
        p_path(p.p_path), p_fmt(p.p_fmt)
    {}

    path(path const &p, format fmt):
        p_path(p.p_path), p_fmt(fmt)
    {}

    path(path &&p):
        p_path(std::move(p.p_path)), p_fmt(p.p_fmt)
    {}

    path(path &&p, format fmt):
        p_path(std::move(p.p_path)), p_fmt(fmt)
    {}

    path &operator=(path const &p) {
        p_path = p.p_path;
        p_fmt = p.p_fmt;
        return *this;
    }

    path &operator=(path &&p) {
        swap(p);
        p.clear();
        return *this;
    }

    char separator() const {
        static const char seps[] = { native_separator, '/', '\\' };
        return seps[std::size_t(p_fmt)];
    }

    std::string drive() const {
        if (is_win()) {
            if (p_path.substr(0, 2) == "\\\\") {
                char const *endp = strchr(p_path.data() + 2, '\\');
                if (!endp) {
                    return p_path;
                }
                char const *pendp = strchr(endp, '\\');
                if (!pendp) {
                    return p_path;
                }
                return std::string{p_path.data(), pendp};
            } else if (has_letter()) {
                return p_path.substr(0, 2);
            }
        }
        return "";
    }

    std::string root() const {
        if (is_win()) {
            if (
                (!p_path.empty() && (p_path[0] == '\\')) ||
                ((p_path.length() >= 3) && (p_path[2] == '\\') && has_letter())
            ) {
                return "\\";
            }
            return "";
        }
        if (!p_path.empty() && (p_path[0] == '/')) {
            return "/";
        }
        return "";
    }

    std::string anchor() const {
        auto ret = drive();
        ret.append(root());
        return ret;
    }

    path parent() const {
        path rel = relative_to(anchor());
        if (rel.p_path.empty()) {
            return *this;
        }
        return ostd::string_range{
            p_path.data(), strrchr(p_path.data(), separator())
        };
    }

    std::string name() const {
        path rel = relative_to(anchor());
        auto pos = rel.p_path.rfind(separator());
        if (pos == std::string::npos) {
            return std::move(rel.p_path);
        }
        return rel.p_path.substr(pos + 1);
    }

    std::string suffix() const {
        path rel = relative_to(anchor());
        auto pos = rel.p_path.rfind('.');
        if (pos == std::string::npos) {
            return "";
        }
        return rel.p_path.substr(pos);
    }

    std::string suffixes() const {
        auto nm = name();
        auto pos = nm.find('.');
        if (pos == std::string::npos) {
            return "";
        }
        return nm.substr(pos);
    }

    std::string stem() const {
        auto nm = name();
        auto pos = nm.find('.');
        if (pos == std::string::npos) {
            return nm;
        }
        return nm.substr(0, pos);
    }

    path relative_to(path other) const {
        if (other.p_fmt != p_fmt) {
            other = path{other, p_fmt};
        }
        other.strip_sep();
        if (p_path.substr(0, other.p_path.length()) == other.p_path) {
            std::size_t oplen = other.p_path.length();
            if ((p_path.length() > oplen) && (p_path[oplen] == separator())) {
                ++oplen;
            }
            return path{p_path.substr(oplen), p_fmt};
        }
        /* TODO: throw */
        return path{""};
    }

    path join(path const &p) const {
        path ret{*this};
        ret.append(p);
        return ret;
    }

    path &append(path const &p) {
        if (p_path != "/") {
            p_path.push_back(separator());
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

    void clear() {
        p_path.clear();
    }

    void swap(path &other) {
        p_path.swap(other.p_path);
        std::swap(p_fmt, other.p_fmt);
    }

private:
    format path_fmt() const {
        static const format fmts[] = {
#ifdef OSTD_PLATFORM_WIN32
            format::windows,
#else
            format::posix,
#endif
            format::posix, format::windows
        };
        return fmts[std::size_t(p_fmt)];
    }

    bool is_win() const {
        return p_fmt == format::windows;
    }

    void strip_sep() {
        if (!p_path.empty() && (p_path.back() == separator())) {
            p_path.pop_back();
        }
    }

    bool has_letter() const {
        if (p_path.size() < 2) {
            return false;
        }
        char ltr = p_path[0] | 32;
        return (p_path[1] == ':') && (ltr >= 'a') &&  (ltr <= 'z');
    }

    std::string p_path;
    format p_fmt;
};

inline path operator/(path const &p1, path const &p2) {
    return p1.join(p2);
}

template<>
struct format_traits<path> {
    template<typename R>
    static void to_format(path const &p, R &writer, format_spec const &fs) {
        fs.format_value(writer, p.string());
    }
};

/** @} */

} /* namespace ostd */

/* filesystem manipulation that relies on path representation above */

namespace ostd {
namespace fs {

/** @addtogroup Utilities
 * @{
 */

OSTD_EXPORT path cwd();
OSTD_EXPORT path home();

/** @} */

} /* namesapce fs */
} /* namesapce ostd */

#endif

/** @} */
