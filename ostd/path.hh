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
    path(R range, format fmt = format::native): p_path("."), p_fmt(fmt) {
        if constexpr(std::is_constructible_v<std::string, R const &>) {
            append_str(std::string{range});
        } else if (!range.empty()) {
            for (auto const &elem: range) {
                append_str(std::string{elem});
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
    {
        if (path_fmt(fmt) != path_fmt(p.p_fmt)) {
            convert_path();
        }
    }

    path(path &&p):
        p_path(std::move(p.p_path)), p_fmt(p.p_fmt)
    {
        p.p_path = ".";
    }

    path(path &&p, format fmt):
        p_path(std::move(p.p_path)), p_fmt(fmt)
    {
        p.p_path = ".";
        if (path_fmt(fmt) != path_fmt(p.p_fmt)) {
            convert_path();
        }
    }

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
            } else if (has_letter(p_path)) {
                return p_path.substr(0, 2);
            }
        }
        return "";
    }

    std::string root() const {
        if (is_win()) {
            if (
                (p_path.data()[0] == '\\') ||
                (
                    (p_path.length() >= 3) &&
                    (p_path[2] == '\\') && has_letter(p_path)
                )
            ) {
                return "\\";
            }
            return "";
        }
        if (p_path.data()[0] == '/') {
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
        append_str(p.p_path);
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
        p_path = ".";
    }

    void swap(path &other) {
        p_path.swap(other.p_path);
        std::swap(p_fmt, other.p_fmt);
    }

private:
    static format path_fmt(format f) {
        static const format fmts[] = {
#ifdef OSTD_PLATFORM_WIN32
            format::windows,
#else
            format::posix,
#endif
            format::posix, format::windows
        };
        return fmts[std::size_t(f)];
    }

    static bool is_sep(char c) {
        return ((c == '/') || (c == '\\'));
    }

    bool is_win() const {
        return path_fmt(p_fmt) == format::windows;
    }

    void strip_sep() {
        if (!p_path.empty() && (p_path.back() == separator())) {
            p_path.pop_back();
        }
    }

    static bool has_letter(std::string const &s) {
        if (s.size() < 2) {
            return false;
        }
        char ltr = s[0] | 32;
        return (s[1] == ':') && (ltr >= 'a') &&  (ltr <= 'z');
    }

    void append_str(std::string s) {
        char sep = separator();
        bool win = is_win();
        /* replace multiple separator sequences and . parts */
        std::size_t start = 0;
        char const *p = s.data();
        if (win && is_sep(p[0]) && is_sep(p[1])) {
            /* it's okay for windows paths to start with double backslash,
             * but if it's triple or anything like that replace anyway
             */
            start = 1;
            ++p;
        }
        /* special case: path starts with ./ or is simply ., erase */
        if ((*p == '.') && (is_sep(p[1]) || (p[1] == '\0'))) {
            s.erase(start, 2 - int(p[1] == '\0'));
        }
        /* replace // and /./ sequences as well as separators */
        for (; start < s.length(); ++start) {
            p = &s[start];
            if (is_sep(*p)) {
                std::size_t cnt = 0;
                for (;;) {
                    if (p[cnt + 1] == sep) {
                        ++cnt;
                        continue;
                    }
                    if (
                        (p[cnt + 1] == '.') &&
                        (is_sep(p[cnt + 2]) || (p[cnt + 2] == '\0'))
                    ) {
                        cnt += 2;
                        continue;
                    }
                    break;
                }
                s.replace(start, cnt + 1, 1, sep);
            }
        }
        /* if the path has a root, replace the previous path, otherwise
         * append a separator followed by the path and be done with it
         *
         * if this is windows and we have a drive, it's like having a root
         */
        if ((s.data()[0] == sep) || (win && has_letter(s))) {
            p_path = std::move(s);
        } else if (!s.empty()) {
            /* empty paths are ., don't forget to clear that */
            if (p_path == ".") {
                p_path.clear();
            } else if (p_path != "/") {
                p_path.push_back(sep);
            }
            p_path.append(s);
        }
        /* strip trailing separator */
        if ((p_path.length() > 1) && (p_path.back() == sep)) {
            p_path.pop_back();
        }
    }

    void convert_path() {
        char froms = '\\', tos = '/';
        if (separator() == '\\') {
            froms = '/';
            tos = '\\';
        } else if (p_path.substr(0, 2) == "\\\\") {
            p_path.replace(0, 2, 1, '/');
        }
        for (auto &c: p_path) {
            if (c == froms) {
                c = tos;
            }
        }
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
