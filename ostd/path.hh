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

#include <cstdint>

#include <stack>
#include <list>
#include <utility>
#include <memory>
#include <initializer_list>
#include <type_traits>
#include <system_error>

#include <ostd/platform.hh>
#include <ostd/range.hh>
#include <ostd/string.hh>
#include <ostd/format.hh>
#include <ostd/algorithm.hh>

/* path representation is within ostd namespace, there aren't any APIs to
 * do actual filesystem manipulation, that's all in the fs namespace below
 */

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

namespace detail {
    struct path_range;
    struct path_parent_range;
}

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

    using range = detail::path_range;

    template<typename R>
    path(R range, format fmt = format::native):
        p_path("."), p_fmt(path_fmt(fmt))
    {
        if constexpr(std::is_constructible_v<std::string, R const &>) {
            append_str(std::string{range});
        } else if (!range.empty()) {
            for (auto const &elem: range) {
                append_str(std::string{elem});
            }
        }
    }

    path(format fmt = format::native): path(".", path_fmt(fmt)) {}

    template<typename T>
    path(std::initializer_list<T> init, format fmt = format::native):
        path(ostd::iter(init), path_fmt(fmt))
    {}

    path(path const &p):
        p_path(p.p_path), p_fmt(p.p_fmt)
    {}

    path(path const &p, format fmt):
        p_path(p.p_path), p_fmt(path_fmt(fmt))
    {
        convert_path(p);
    }

    path(path &&p) noexcept:
        p_path(std::move(p.p_path)), p_fmt(p.p_fmt)
    {
        p.p_path = ".";
    }

    path(path &&p, format fmt):
        p_path(std::move(p.p_path)), p_fmt(path_fmt(fmt))
    {
        p.p_path = ".";
        convert_path(p);
    }

    path &operator=(path const &p) {
        p_path = p.p_path;
        p_fmt = p.p_fmt;
        return *this;
    }

    path &operator=(path &&p) noexcept {
        swap(p);
        p.clear();
        return *this;
    }

    char separator() const noexcept {
        static const char seps[] = { native_separator, '/', '\\' };
        return seps[std::size_t(p_fmt)];
    }

    string_range drive() const noexcept {
        if (is_win()) {
            string_range path = p_path;
            if (has_dslash(path)) {
                char const *endp = strchr(p_path.data() + 2, '\\');
                if (!endp) {
                    return path;
                }
                char const *pendp = strchr(endp + 1, '\\');
                if (!pendp) {
                    return path;
                }
                return string_range{path.data(), pendp};
            } else if (has_letter(path)) {
                return path.slice(0, 2);
            }
        }
        return nullptr;
    }

    bool has_drive() const noexcept {
        if (is_win()) {
            return (has_letter(p_path) || has_dslash(p_path));
        }
        return false;
    }

    string_range root() const noexcept {
        char const *rootp = get_rootp();
        if (rootp) {
            return string_range{rootp, rootp + 1};
        }
        return nullptr;
    }

    bool has_root() const noexcept {
        return !!get_rootp();
    }

    string_range anchor() const noexcept {
        string_range dr = drive();
        if (dr.empty()) {
            return root();
        }
        char const *datap = dr.data();
        std::size_t datas = dr.size();
        if (datap[datas] == separator()) {
            return string_range{datap, datap + datas + 1};
        }
        return dr;
    }

    bool has_anchor() const noexcept {
        return has_root() || has_drive();
    }

    path parent() const {
        string_range sep;
        if (is_absolute()) {
            sep = ostd::find_last(relative_to_str(anchor()), separator());
            if (sep.empty()) {
                return path{anchor(), p_fmt};
            }
        } else {
            sep = ostd::find_last(string(), separator());
            if (sep.empty()) {
                return *this;
            }
        }
        return path{ostd::string_range{p_path.data(), sep.data()}, p_fmt};
    }

    bool has_parent() const noexcept {
        if (is_absolute()) {
            return (string() != anchor());
        }
        return !ostd::find(string(), separator()).empty();
    }

    detail::path_parent_range parents() const;

    path relative() const {
        return relative_to(anchor());
    }

    string_range name() const noexcept {
        string_range rel = relative_to_str(anchor());
        string_range sep = ostd::find_last(rel, separator());
        if (sep.empty()) {
            return rel;
        }
        sep.pop_front();
        return sep;
    }

    bool has_name() const noexcept {
        return !name().empty();
    }

    string_range suffix() const noexcept {
        return ostd::find_last(relative_to_str(anchor()), '.');
    }

    string_range suffixes() const noexcept {
        return ostd::find(name(), '.');
    }

    bool has_suffix() const noexcept {
        return !suffixes().empty();
    }

    string_range stem() const noexcept {
        auto nm = name();
        return nm.slice(0, nm.size() - ostd::find(nm, '.').size());
    }

    bool has_stem() const noexcept {
        return !stem().empty();
    }

    bool is_absolute() const noexcept {
        if (is_win()) {
            if (has_dslash(p_path)) {
                return true;
            }
            return (has_letter(p_path) && (p_path.data()[2] == '\\'));
        }
        return (p_path.data()[0] == '/');
    }

    bool is_relative() const noexcept {
        return !is_absolute();
    }

    path relative_to(path const &other) const {
        if (other.p_fmt != p_fmt) {
            return path{relative_to_str(path{other, p_fmt}.p_path), p_fmt};
        } else {
            return path{relative_to_str(other.p_path), p_fmt};
        }
    }

    path &remove_name() {
        auto nm = name();
        if (nm.empty()) {
            /* TODO: throw */
            return *this;
        }
        p_path.erase(p_path.size() - nm.size() - 1, nm.size() + 1);
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
            p_path.erase(p_path.size() - osfx.size(), osfx.size());
        }
        p_path.append(sfx);
        return *this;
    }

    path &replace_suffixes(string_range sfx) {
        auto sfxs = suffixes();
        if (!sfxs.empty()) {
            p_path.erase(p_path.size() - sfxs.size(), sfxs.size());
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
        append_str(p.p_path, p.p_fmt == p_fmt);
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

    string_range string() const noexcept {
        return p_path;
    }

    format path_format() const noexcept {
        return p_fmt;
    }

    void clear() {
        p_path = ".";
    }

    bool empty() const {
        return (p_path == ".");
    }

    void swap(path &other) noexcept {
        p_path.swap(other.p_path);
        std::swap(p_fmt, other.p_fmt);
    }

    range iter() const noexcept;

private:
    static format path_fmt(format f) noexcept {
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

    static bool is_sep(char c) noexcept {
        return ((c == '/') || (c == '\\'));
    }

    bool is_win() const noexcept {
        return p_fmt == format::windows;
    }

    static bool has_letter(string_range s) noexcept {
        if (s.size() < 2) {
            return false;
        }
        char ltr = s[0] | 32;
        return (s[1] == ':') && (ltr >= 'a') &&  (ltr <= 'z');
    }

    static bool has_dslash(string_range s) noexcept {
        if (s.size() < 2) {
            return false;
        }
        return (s.slice(0, 2) == "\\\\");
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
        for (; start < s.size(); ++start) {
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
        std::size_t plen = p_path.size();
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

    void append_str(std::string s, bool norm = false) {
        char sep = separator();
        bool win = is_win();
        /* replace multiple separator sequences and . parts */
        if (!norm) {
            cleanup_str(s, sep, win);
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

    void convert_path(path const &p) {
        if (p.p_fmt == p_fmt) {
            return;
        }
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

    string_range relative_to_str(string_range other) const noexcept {
        if (other == ".") {
            return p_path;
        }
        std::size_t oplen = other.size();
        if (string_range{p_path}.slice(0, oplen) == other) {
            if ((p_path.size() > oplen) && (p_path[oplen] == separator())) {
                ++oplen;
            }
            auto sl = string_range{p_path};
            return sl.slice(oplen, sl.size());
        }
        return nullptr;
    }

    char const *get_rootp() const noexcept {
        char const *datap = p_path.data();
        if (is_win()) {
            if (*datap == '\\')  {
                return datap;
            }
            if (has_letter(p_path) && (datap[2] == '\\')) {
                return datap + 2;
            }
            return nullptr;
        }
        if (*p_path.data() == '/') {
            return datap;
        }
        return nullptr;
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

namespace detail {
    struct path_range: input_range<path_range> {
        using range_category = forward_range_tag;
        using value_type = string_range;
        using reference = string_range;
        using size_type = std::size_t;

        path_range() = delete;
        path_range(path const &p) noexcept: p_rest(p.string()) {
            string_range drive = p.drive();
            if (!drive.empty()) {
                p_current = p.anchor();
                /* windows drive without root, cut rest a character earlier so
                 * that the next segment can be retrieved consistently
                 */
                if (p_current.size() == drive.size()) {
                    p_rest = p_rest.slice(drive.size() - 1, p_rest.size());
                } else {
                    p_rest = p_rest.slice(drive.size(), p_rest.size());
                }
                return;
            }
            string_range root = p.root();
            if (!root.empty()) {
                p_current = root;
                /* leave p_rest alone so that it begins with a separator */
                return;
            }
            auto sep = ostd::find(p_rest, p.separator());
            if (!sep.empty()) {
                p_current = string_range{p_rest.data(), sep.data()};
            } else {
                p_current = p_rest;
            }
            p_rest = p_rest.slice(p_current.size(), p_rest.size());
        }

        bool empty() const noexcept { return p_current.empty(); }

        void pop_front() noexcept {
            string_range ncur = p_rest;
            if (!ncur.empty()) {
                char sep = ncur.front();
                if (sep != '/') {
                    sep = '\\';
                }
                ncur.pop_front();
                string_range nsep = ostd::find(ncur, sep);
                p_current = ncur.slice(0, ncur.size() - nsep.size());
                p_rest = nsep;
            } else {
                p_current = nullptr;
            }
        }

        string_range front() const noexcept {
            return p_current;
        }

    private:
        string_range p_current, p_rest;
    };

    struct path_parent_range: input_range<path_parent_range> {
        using range_category = forward_range_tag;
        using value_type = path;
        using reference = path;
        using size_type = std::size_t;

        path_parent_range() = delete;
        path_parent_range(path const &p): p_path(p) {}

        bool empty() const noexcept { return !p_path.has_parent(); }

        void pop_front() {
            p_path = p_path.parent();
        }

        path front() const {
            return p_path.parent();
        }

    private:
        path p_path;
    };
}

inline typename path::range path::iter() const noexcept {
    return typename path::range{*this};
}

inline detail::path_parent_range path::parents() const {
    return detail::path_parent_range{*this};
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

enum class file_type {
    none = 0,
    not_found,
    regular,
    directory,
    symlink,
    block,
    character,
    fifo,
    socket,
    unknown
};

enum class perms {
    /* occupies 12 bits */
    none         = 0,
    owner_read   = 0400,
    owner_write  = 0200,
    owner_exec   = 0100,
    owner_all    = 0700,
    group_read   = 040,
    group_write  = 020,
    group_exec   = 010,
    group_all    = 070,
    others_read  = 04,
    others_write = 02,
    others_exec  = 01,
    others_all   = 07,
    all          = 0777,
    set_uid      = 04000,
    set_gid      = 02000,
    sticky_bit   = 01000,
    mask         = 07777,

    unknown      = 0xFFFF
};

inline perms operator|(perms a, perms b) {
    return perms(int(a) | int(b));
}

inline perms operator&(perms a, perms b) {
    return perms(int(a) & int(b));
}

inline perms operator^(perms a, perms b) {
    return perms(int(a) ^ int(b));
}

inline perms operator~(perms v) {
    return perms(~int(v));
}

inline perms &operator|=(perms &a, perms b) {
    a = (a | b);
    return a;
}

inline perms &operator&=(perms &a, perms b) {
    a = (a & b);
    return a;
}

inline perms &operator^=(perms &a, perms b) {
    a = (a ^ b);
    return a;
}

struct file_mode {
private:
    using UT = std::uint_least32_t;
    UT p_val;

public:
    file_mode() noexcept: file_mode(file_type::none) {}
    file_mode(file_type type, perms permissions = perms::unknown) noexcept:
        p_val(UT(permissions) | (UT(type) << 16))
    {}

    file_type type() const noexcept {
        return file_type(p_val >> 16);
    }

    void type(file_type type) noexcept {
        p_val = ((p_val & 0xFFFF) | (UT(type) << 16));
    }

    perms permissions() const noexcept {
        return perms(p_val & 0xFFFF);
    }

    void permissions(perms perm) noexcept {
        p_val = ((p_val & ~UT(0xFFFF)) | UT(perm));
    }
};

OSTD_EXPORT file_mode mode(path const &p);
OSTD_EXPORT file_mode symlink_mode(path const &p);

inline bool is_block_file(file_mode st) {
    return st.type() == file_type::block;
}

inline bool is_block_file(path const &p) {
    return is_block_file(mode(p));
}

inline bool is_character_file(file_mode st) {
    return st.type() == file_type::character;
}

inline bool is_character_file(path const &p) {
    return is_character_file(mode(p));
}

inline bool is_directory(file_mode st) {
    return st.type() == file_type::directory;
}

inline bool is_directory(path const &p) {
    return is_directory(mode(p));
}

inline bool is_regular_file(file_mode st) {
    return st.type() == file_type::regular;
}

inline bool is_regular_file(path const &p) {
    return is_regular_file(mode(p));
}

inline bool is_fifo(file_mode st) {
    return st.type() == file_type::fifo;
}

inline bool is_fifo(path const &p) {
    return is_fifo(mode(p));
}

inline bool is_symlink(file_mode st) {
    return st.type() == file_type::symlink;
}

inline bool is_symlink(path const &p) {
    return is_symlink(mode(p));
}

inline bool is_socket(file_mode st) {
    return st.type() == file_type::socket;
}

inline bool is_socket(path const &p) {
    return is_socket(mode(p));
}

inline bool is_other(file_mode st) {
    return st.type() == file_type::unknown;
}

inline bool is_other(path const &p) {
    return is_other(mode(p));
}

inline bool mode_known(file_mode st) {
    return st.type() != file_type::none;
}

inline bool mode_known(path const &p) {
    return mode_known(mode(p));
}

struct file_info {
    file_info() {}
    file_info(ostd::path const &p): p_path(p) {}

    ostd::path const &path() const noexcept {
        return p_path;
    }

    operator ostd::path const &() const noexcept {
        return p_path;
    }

    void clear() {
        p_path.clear();
    }

    void assign(ostd::path const &p) {
        p_path = p;
    }

    void assign(ostd::path &&p) {
        p_path = std::move(p);
    }

private:
    ostd::path p_path{};
};

namespace detail {
    struct OSTD_EXPORT dir_range_impl {
        void open(path const &p);
        void close() noexcept;
        void read_next();

        bool empty() const noexcept {
            return p_current.path().empty();
        }

        file_info const &front() const noexcept {
            return p_current;
        }

        ~dir_range_impl() {
            close();
        }

        file_info p_current{};
        path p_dir{};
        void *p_handle = nullptr;
    };

    struct OSTD_EXPORT rdir_range_impl {
        using hstack = std::stack<void *, std::list<void *>>;

        void open(path const &p);
        void close() noexcept;
        void read_next();

        bool empty() const noexcept {
            return p_current.path().empty();
        }

        file_info const &front() const noexcept {
            return p_current;
        }

        ~rdir_range_impl() {
            close();
        }

        file_info p_current{};
        path p_dir{};
        hstack p_handles{};
    };
}

struct directory_range: input_range<directory_range> {
    using range_category = input_range_tag;
    using value_type = file_info;
    using reference = file_info const &;
    using size_type = std::size_t;

    directory_range() = delete;
    directory_range(path const &p):
        p_impl{std::make_shared<detail::dir_range_impl>()}
    {
        p_impl->open(p);
    }

    bool empty() const noexcept {
        return p_impl->empty();
    }

    void pop_front() {
        p_impl->read_next();
    }

    reference front() const noexcept {
        return p_impl->front();
    }

private:
    std::shared_ptr<detail::dir_range_impl> p_impl;
};

struct recursive_directory_range: input_range<recursive_directory_range> {
    using range_category = input_range_tag;
    using value_type = file_info;
    using reference = file_info const &;
    using size_type = std::size_t;

    recursive_directory_range() = delete;
    recursive_directory_range(path const &p):
        p_impl{std::make_shared<detail::rdir_range_impl>()}
    {
        p_impl->open(p);
    }

    bool empty() const noexcept {
        return p_impl->empty();
    }

    void pop_front() {
        p_impl->read_next();
    }

    reference front() const noexcept {
        return p_impl->front();
    }

private:
    std::shared_ptr<detail::rdir_range_impl> p_impl;
};

OSTD_EXPORT path current_path();
OSTD_EXPORT path home_path();
OSTD_EXPORT path temp_path();

OSTD_EXPORT path absolute(path const &p);

OSTD_EXPORT path canonical(path const &p);
OSTD_EXPORT path weakly_canonical(path const &p);

OSTD_EXPORT path relative(path const &p, path const &base = current_path());

inline bool exists(file_mode s) noexcept {
    return (mode_known(s) && (s.type() != file_type::not_found));
}

OSTD_EXPORT bool exists(path const &p);

OSTD_EXPORT bool equivalent(path const &p1, path const &p2);

/** @} */

} /* namesapce fs */
} /* namesapce ostd */

#endif

/** @} */
