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

    bool has_drive() const {
        if (is_win()) {
            return (has_letter(p_path) || (p_path.substr(0, 2) == "\\\\"));
        }
        return false;
    }

    std::string root() const {
        if (has_root()) {
            char sep = separator();
            return std::string{&sep, 1};
        }
        return "";
    }

    bool has_root() const {
        if (is_win()) {
            return (
                (p_path.data()[0] == '\\') ||
                (
                    (p_path.length() >= 3) &&
                    (p_path[2] == '\\') && has_letter(p_path)
                )
            );
        }
        return (p_path.data()[0] == '/');
    }

    std::string anchor() const {
        auto ret = drive();
        ret.append(root());
        return ret;
    }

    bool has_anchor() const {
        return has_root() || has_drive();
    }

    path parent() const {
        path rel = relative_to(anchor());
        if (rel.p_path == ".") {
            return *this;
        }
        return ostd::string_range{
            p_path.data(), strrchr(p_path.data(), separator())
        };
    }

    bool has_parent() const {
        return (parent().p_path != p_path);
    }

    path relative() const {
        return relative_to(anchor());
    }

    bool has_relative() const {
        return (relative().p_path != ".");
    }

    std::string name() const {
        path rel = relative_to(anchor());
        auto pos = rel.p_path.rfind(separator());
        if (pos == std::string::npos) {
            return std::move(rel.p_path);
        }
        return rel.p_path.substr(pos + 1);
    }

    bool has_name() const {
        return !name().empty();
    }

    std::string suffix() const {
        path rel = relative_to(anchor());
        auto pos = rel.p_path.rfind('.');
        if (pos == std::string::npos) {
            return "";
        }
        return rel.p_path.substr(pos);
    }

    bool has_suffix() const {
        return !suffix().empty();
    }

    std::string suffixes() const {
        auto nm = name();
        auto pos = nm.find('.');
        if (pos == std::string::npos) {
            return "";
        }
        return nm.substr(pos);
    }

    bool has_suffixes() const {
        return !suffixes().empty();
    }

    std::string stem() const {
        auto nm = name();
        auto pos = nm.find('.');
        if (pos == std::string::npos) {
            return nm;
        }
        return nm.substr(0, pos);
    }

    bool has_stem() const {
        return !stem().empty();
    }

    path relative_to(path other) const {
        if (other.p_path == ".") {
            return *this;
        }
        if (other.p_fmt != p_fmt) {
            other = path{other, p_fmt};
        }
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

    path &remove_name() {
        auto nm = name();
        if (nm.empty()) {
            /* TODO: throw */
            return *this;
        }
        p_path.erase(p_path.length() - nm.length() - 1, nm.length() + 1);
        return *this;
    }

    path without_name() const {
        path ret{*this};
        ret.remove_name();
        return ret;
    }

    path &replace_name(string_range name) {
        remove_name();
        append_str(std::string{name});
        return *this;
    }

    path with_name(string_range name) {
        path ret{*this};
        ret.replace_name(name);
        return ret;
    }

    path &replace_suffix(string_range sfx) {
        auto osfx = suffix();
        if (!osfx.empty()) {
            p_path.erase(p_path.length() - osfx.length(), osfx.length());
        }
        p_path.append(sfx);
        return *this;
    }

    path &replace_suffixes(string_range sfx) {
        auto sfxs = suffixes();
        if (!sfxs.empty()) {
            p_path.erase(p_path.length() - sfxs.length(), sfxs.length());
        }
        p_path.append(sfx);
        return *this;
    }

    path with_suffix(string_range sfx) {
        path ret{*this};
        ret.replace_suffix(sfx);
        return ret;
    }

    path with_suffixes(string_range sfx) {
        path ret{*this};
        ret.replace_suffixes(sfx);
        return ret;
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

    path &append_concat(path const &p) {
        append_concat_str(p.p_path);
        return *this;
    }

    path concat(path const &p) const {
        path ret{*this};
        ret.append_concat(p);
        return ret;
    }

    path &operator/=(path const &p) {
        return append(p);
    }

    path &operator+=(path const &p) {
        return append_concat(p);
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

    static bool has_letter(std::string const &s) {
        if (s.size() < 2) {
            return false;
        }
        char ltr = s[0] | 32;
        return (s[1] == ':') && (ltr >= 'a') &&  (ltr <= 'z');
    }

    void cleanup_str(std::string &s, char sep, bool allow_twoslash) {
        std::size_t start = 0;
        /* replace multiple separator sequences and . parts */
        char const *p = &s[start];
        if (allow_twoslash && is_sep(p[0]) && is_sep(p[1])) {
            /* it's okay for windows paths to start with double backslash,
             * but if it's triple or anything like that replace anyway
             */
            start += 1;
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
                    if (is_sep(p[cnt + 1])) {
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
    }

    void strip_trailing(char sep) {
        std::size_t plen = p_path.length();
        if (sep == '\\') {
            char const *p = p_path.data();
            if ((plen <= 2) && (p[0] == '\\') && (p[1] == '\\')) {
                return;
            }
            if ((plen <= 3) && has_letter(p_path)) {
                return;
            }
        } else if (plen <= 1) {
            return;
        }
        if (p_path.back() == sep) {
            p_path.pop_back();
        }
    }

    void append_str(std::string s) {
        char sep = separator();
        bool win = is_win();
        /* replace multiple separator sequences and . parts */
        cleanup_str(s, sep, win);
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
                /* empty path: replace */
                p_path = std::move(s);
            } else {
                if (p_path.back() != sep) {
                    p_path.push_back(sep);
                }
                p_path.append(s);
            }
        }
        strip_trailing(sep);
    }

    void append_concat_str(std::string s) {
        char sep = separator();
        /* replace multiple separator sequences and . parts */
        cleanup_str(s, sep, false);
        if (p_path == ".") {
            /* empty path: replace */
            p_path = std::move(s);
        } else {
            if ((p_path.back() == sep) && (s.front() == sep)) {
                p_path.pop_back();
            }
            p_path.append(s);
        }
        strip_trailing(sep);
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

inline path operator+(path const &p1, path const &p2) {
    return p1.concat(p2);
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
