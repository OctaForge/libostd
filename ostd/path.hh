/** @defgroup Filesystem Paths and filesystem
 *
 * @brief APIs related to path handling and filesystem manipulation.
 *
 * libostd provides APIs for handling filesystem paths with support for
 * POSIX and Windows path encoding on all systems. These operations are
 * purely lexical and will not involve system calls.
 *
 * In addition to that, it also provides a namespace `fs` containing APIs
 * that manipulate the file system; you can use those to query information
 * about files as well as change some of it.
 *
 * These APIs replace the C++ std::filesystem module. It is meant to be
 * level and more convenient to use, as well as supported on toolchains
 * that have a sufficient C++17 support level but do not ship the standard
 * filesystem module. Unlike the standard filesystem module, libostd paths
 * always use byte-oriented name encoding (typically UTF-8) and perform
 * transparent normalization. Glob matching, both lexical and filesystem
 * based, is also provided.
 *
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
 * platforms, taking care of conversions internally. It also makes sure
 * the path remains in a normalized (though not canonical, as that needs
 * actual FS operations) form at all times.
 *
 * It also implements an amount of extra functionality such as glob matching.
 *
 * @include glob.cc
 * @include listdir.cc
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_PATH_HH
#define OSTD_PATH_HH

#include <cstdint>

#include <stack>
#include <list>
#include <chrono>
#include <utility>
#include <memory>
#include <initializer_list>
#include <type_traits>
#include <stdexcept>
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

/** @addtogroup Filesystem
 * @{
 */

namespace detail {
    struct path_range;
    struct path_parent_range;

    OSTD_EXPORT bool glob_match_path_impl(
        char const *fname, char const *wname
    ) noexcept;
}

/** @brief An exception type thrown on lexical path errors.
 *
 * Path errors are thrown whne something in a path itself fails, without
 * involving system APIs in it. For example, trying to remove a name from
 * a path that has no name would result in this kind of exception thrown.
 *
 * It inherits from std::runtime_error and has the same methods and behavior.
 */
struct path_error: std::runtime_error {
    using std::runtime_error::runtime_error;

    /* empty, for vtable placement */
    virtual ~path_error();
};

/** @brief A structure representing a file system path.
 *
 * Libostd uses this to represent paths as then it can keep track of
 * the format.
 *
 * A path may be composed of a drive, a root (drive and root together make
 * an anchor) and any number of components with an appropriate separator.
 *
 * Two possible path encodings are supported, POSIX style and Windows style.
 * POSIX style paths use a forward slash (`/`) as a separator, Windows paths
 * use a backslash (`\`). Windows style paths support a drive component,
 * POSIX style paths do not.
 *
 * Windows style paths may take on the following formats:
 *
 * - Absolute paths:
 *   - `C:\foo\bar\file.ext`
 *   - `\\host\share\foo\bar\file.ext`
 * - Relative paths:
 *   - `foo\bar`
 *   - `C:foo\bar`
 *   - `\foo\bar`
 *
 * Absolute paths always have a drive. The drive is either a letter followed
 * by a colon, such as `C:`, or a host+share sequence, `\\host\share`. The
 * drive is followed by a root, `\`, followed by any number of path elemnets.
 *
 * Relative paths may or may not have a drive. If they do, it's always lettered
 * and is not followed by a root. Additionally, POSIX-style `\foo\bar` is
 * considered a relative path in the Windows encoding, and do have a root
 * but not a drive.
 *
 * POSIX style paths may take on the following formats:
 *
 * - Absolute paths:
 *   - `/foo/bar/file.ext`
 * - Relative paths:
 *   - `foo/bar/file.ext`
 *
 * POSIX style paths never contain a drive, but they contain a root if
 * absolute.
 *
 * With both encodings, an empty path is represented as a period (`.`).
 * However, a non-empty path will never contain any period components.
 * Therefore, if your input is a path `./foo`, the normalization will
 * turn it into a simple `foo`. Inner components like this (e.h. `foo/./bar`)
 * are also normalized properly (e.g. into `foo/bar`).
 *
 * The normalization also strips any trailing separators, so e.g. `foo/bar/`
 * is made into `foo/bar`.
 *
 * It is possible to convert from one format to another by copy constructing
 * with the appropriate format. Keep in mind that this conversion does not
 * always make perfect sense, e.g. absolute Windows path `C:\foo` is converted
 * to a relative POSIX path `C:/foo`. The important part is that they are
 * technically lossless, if you convert there and back you should get the
 * same path.
 *
 * All operations on paths outside the `ostd::fs` namespace are purely lexical
 * and involve no system calls.
 *
 * Paths always use a multibyte encoding regardless of operating system. On
 * Windows, file system operations with paths assume Unicode (UTF-8), on
 * POSIX they will assume an 8-bit encoding based on your system locale
 * or filesystem encoding. As the Windows native encoding is UTF-16/UCS-2,
 * libostd uses WTF-8/16 style loose encoding to allow for lossless
 * manipulation of ill-formed UTF-16.
 */
struct path {
#ifdef OSTD_PLATFORM_WIN32
    static constexpr char const native_separator = '\\';
#else
    /** @brief The preferred separator for your OS. */
    static constexpr char const native_separator = '/';
#endif

    /** @brief Describes the format used for a path.
     *
     * The `native` format is only used with constructors (being the default
     * format) and represents either POSIX or Windows style paths depending
     * on the system you're on. Keep in mind that the path does not retain
     * this format, if you try to request the format back it will return
     * the concrete OS specific value.
     *
     * You can use any of the 3 values on any operating system - it is
     * valid to manipulate Windows paths on Unix-like systems. However,
     * you cannot expect actual filesystem operations to accept paths
     * in both encodings (though Windows does support POSIX style separators).
     */
    enum class format {
        native = 0,
        posix,
        windows
    };

#ifdef OSTD_PLATFORM_WIN32
    static constexpr format const native_format = format::windows;
#else
    /** @brief The preferred format for your OS. */
    static constexpr format const native_format = format::posix;
#endif

    /** @brief The range type to iterate over elements of a path.
     *
     * For relative paths, the elements are simply the individual separated
     * pieces. For absolute paths, the first element is the anchor(), i.e.
     * drive (if possible) plus root.
     *
     * The range is an ostd::forward_range_tag and has ostd::string_range
     * as a value and reference type.
     */
    using range = detail::path_range;

    /** @brief Constructs a path using a range or a string.
     *
     * If said `range` is convertible to a string (e.g. ostd::string_range)
     * or if it's a string (like std::string), it is converted to a string,
     * assumed to be a path and normalized.
     *
     * Otherwise, it is assumed to be a range (at least ostd::input_range_tag)
     * with elements being convertible to strings. Each element is normalized
     * and those elements are then concatenated with the directory separator,
     * making sure the actual path stays normalized.
     *
     * If the path has a drive or a root, the previously built contents of
     * the path are reset and the path is started over.
     */
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

    /** @brief Constructs an empty path (`.`) */
    path(format fmt = format::native): path(".", path_fmt(fmt)) {}

    /** @brief Constructs a path using an initializer list.
     *
     * The initializer list components must be convertible to strings.
     * Then, the initializer list is used exactly like a range.
     */
    template<typename T>
    path(std::initializer_list<T> init, format fmt = format::native):
        path(ostd::iter(init), path_fmt(fmt))
    {}

    /** @brief Path copy constructor.
     *
     * No changes are made, the path is copied as is.
     */
    path(path const &p):
        p_path(p.p_path), p_fmt(p.p_fmt)
    {}

    /** @brief Path copy constructor with format.
     *
     * If the new format differs, the path is re-encoded.
     */
    path(path const &p, format fmt):
        p_path(p.p_path), p_fmt(path_fmt(fmt))
    {
        convert_path(p);
    }

    /** @brief Path move constructor.
     *
     * No changes are made, the path is moved as is and the previous
     * path is left empty, but valid.
     */
    path(path &&p) noexcept:
        p_path(std::move(p.p_path)), p_fmt(p.p_fmt)
    {
        p.p_path = ".";
    }

    /** @brief Path move constructor with format.
     *
     * The other path is left empty, but valid. The new path will be
     * reeencoded if necessary.
     */
    path(path &&p, format fmt):
        p_path(std::move(p.p_path)), p_fmt(path_fmt(fmt))
    {
        p.p_path = ".";
        convert_path(p);
    }

    /** @brief Path copy assign.
     *
     * No changes are made.
     */
    path &operator=(path const &p) {
        p_path = p.p_path;
        p_fmt = p.p_fmt;
        return *this;
    }

    /** @brief Path move assign.
     *
     * The other path is left empty, but valid.
     */
    path &operator=(path &&p) noexcept {
        swap(p);
        p.clear();
        return *this;
    }

    /** @brief Gets the currently used separator.
     *
     * Unlike `native_separator`, this is not a constant and represents
     * encoding specific path separator.
     */
    char separator() const noexcept {
        static const char seps[] = { native_separator, '/', '\\' };
        return seps[std::size_t(p_fmt)];
    }

    /** @brief Gets the drive of the path.
     *
     * For POSIX-encoded paths, this returns an empty range. For Windows,
     * this returns the drive portion of the path, if present. Drive may
     * be present in both relative and absolute paths.
     *
     * @see has_drive()
     * @see root()
     * @see anchor()
     */
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

    /** @brief Checks if a path has a drive.
     *
     * @see drive()
     */
    bool has_drive() const noexcept {
        if (is_win()) {
            return (has_letter(p_path) || has_dslash(p_path));
        }
        return false;
    }

    /** @brief Gets the root of the path.
     *
     * On POSIX, absolute paths have a root. On Windows, both relative
     * and absolute paths may have a root.
     *
     * @see has_root()
     * @see drive()
     * @see anchor()
     */
    string_range root() const noexcept {
        char const *rootp = get_rootp();
        if (rootp) {
            return string_range{rootp, rootp + 1};
        }
        return nullptr;
    }

    /** @brief Checks if a path has a root.
     *
     * @see root()
     */
    bool has_root() const noexcept {
        return !!get_rootp();
    }

    /** @brief Gets the anchor of the path.
     *
     * This is the concatenation of a drive and a root. A path has an anchor
     * if it has either a drive or a root.
     *
     * @see has_anchor()
     * @see drive()
     * @see root()
     */
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

    /** @brief Checks if a path has an anchor.
     *
     * @see anchor()
     */
    bool has_anchor() const noexcept {
        return has_root() || has_drive();
    }

    /** @brief Gets the parent path of a path.
     *
     * Keep in mind this is purely lexical, so for e.g. `foo/bar/../baz`
     * this is `foo/bar/..`. Absolute paths have no parent if they're just
     * an anchor. Relative paths have no parent if they contain no separators.
     *
     * This returns a copy of the path for parentless paths, or the parent.
     * Use has_parent() if you need to check, or compare the path strings,
     * but that is slower.
     *
     * @see has_parent()
     */
    path parent() const {
        string_range sep;
        if (is_absolute()) {
            sep = ostd::find_last(relative_to_str(anchor()), separator());
            if (sep.empty()) {
                return path{anchor(), p_fmt};
            }
        } else {
            sep = ostd::find_last(string_range{p_path}, separator());
            if (sep.empty()) {
                return *this;
            }
        }
        return path{ostd::string_range{p_path.data(), sep.data()}, p_fmt};
    }

    /** @brief Checks if a path has a parent.
     *
     * This is true if parent() would return a different path.
     *
     * @see parent()
     */
    bool has_parent() const noexcept {
        if (is_absolute()) {
            return (string() != anchor());
        }
        return !ostd::find(string_range{p_path}, separator()).empty();
    }

    /** @brief Gets a range containing all parents of a path.
     *
     * For example, for `foo/bar/baz`, this is `{'foo/bar', 'foo'}`. The
     * range is an ostd::forward_range_tag with each element being a path,
     * including the reference type.
     */
    detail::path_parent_range parents() const;

    /** @brief Equivalent to `relative_to(anchor())`. */
    path relative() const {
        return relative_to(anchor());
    }

    /** @brief Gets the name component of the path.
     *
     * A name component is the last component of the path that is not an
     * anchor.
     *
     * @see has_name()
     * @see anchor()
     * @see stem()
     * @see suffix()
     */
    string_range name() const noexcept {
        string_range rel = relative_to_str(anchor());
        string_range sep = ostd::find_last(rel, separator());
        if (sep.empty()) {
            return rel;
        }
        sep.pop_front();
        return sep;
    }

    /** @brief Checks if the path has a name.
     *
     * @see name()
     */
    bool has_name() const noexcept {
        return !name().empty();
    }

    /** @brief Gets the suffix of the name component.
     *
     * This is the last part of the name after a period, including the
     * period. For example, for `foo.tar.gz` it's `.gz`. It can be empty.
     *
     * @see has_suffix()
     * @see name()
     * @see stem()
     * @see suffixes()
     */
    string_range suffix() const noexcept {
        return ostd::find_last(relative_to_str(anchor()), '.');
    }

    /** @brief Gets the suffixes of the name component.
     *
     * Like suffix(), but gets all the suffixes, so for example for
     * `foo.tar.gz` this is `.tar.gz`.
     *
     * @see suffix()
     * @see has_suffix()
     */
    string_range suffixes() const noexcept {
        return ostd::find(name(), '.');
    }

    /** @brief Checks if the name has one or more suffixes.
     *
     * @see suffix()
     */
    bool has_suffix() const noexcept {
        return !suffixes().empty();
    }

    /** @brief Gets the stem part of the name.
     *
     * If the name has no suffix, this is equivalent to the name. Otherwise,
     * it's the name but without the suffix (`foo` in `foo.tar.gz`).
     *
     * @see has_stem()
     * @see suffix()
     */
    string_range stem() const noexcept {
        auto nm = name();
        return nm.slice(0, nm.size() - ostd::find(nm, '.').size());
    }

    /** @brief Checks if the path has a stem.
     *
     * @see stem()
     */
    bool has_stem() const noexcept {
        return !stem().empty();
    }

    /** @brief Checks if a path is absolute.
     *
     * You can find an exact description in the documentation for ostd::path.
     * In short, on POSIX an absolute path has a root, on Windows it has both
     * a root and a drive.
     *
     * @see is_relative()
     * @see root()
     * @see drive()
     */
    bool is_absolute() const noexcept {
        if (is_win()) {
            if (has_dslash(p_path)) {
                return true;
            }
            return (has_letter(p_path) && (p_path.data()[2] == '\\'));
        }
        return (p_path.data()[0] == '/');
    }

    /** @brief Checks if a path is relative.
     *
     * A path is relative if it's not absolute.
     *
     * @see is_absolute()
     */
    bool is_relative() const noexcept {
        return !is_absolute();
    }

    /** @brief Gets a path lexically relative to `other`.
     *
     * This is, like every other operation, purely lexical. For example,
     * if this path is `/foo/bar/baz` and the given path is `/foo`, the
     * result is `bar/baz`. Basically, this works by stripping the first
     * part of the path equal to `other` and returning the rest.
     *
     * The other path is converted to the same format as this path if needed.
     *
     * If this is not possible, ostd::path_error is thrown.
     *
     * @throws ostd::path_error
     */
    path relative_to(path const &other) const {
        string_range rto;
        if (other.p_fmt != p_fmt) {
            rto = relative_to_str(path{other, p_fmt}.p_path);
        } else {
            rto = relative_to_str(other.p_path);
        }
        if (rto.empty()) {
            throw path_error{"non-matching paths"};
        }
        return path{rto, p_fmt};
    }

    /** @brief Removes the name component of the path.
     *
     * If the path has no name component, ostd::path_error is thrown.
     *
     * @see without_name()
     * @see with_name()
     * @see replace_name()
     *
     * @throws ostd::path_error
     */
    path &remove_name() {
        auto nm = name();
        if (nm.empty()) {
            throw path_error{"path has no name"};
        }
        p_path.erase(p_path.size() - nm.size() - 1, nm.size() + 1);
        return *this;
    }

    /** @brief Gets the path except without the name component.
     *
     * Same as remove_name(). except it doesn't modify the path.
     *
     * @see name()
     * @see with_name()
     * @see replace_name()
     *
     * @throws ostd::path_error
     */
    path without_name() const {
        path ret{*this};
        ret.remove_name();
        return ret;
    }

    /** @brief Replaces the name component with another.
     *
     * This removes the name first using remove_name() and then appends
     * the new one.
     *
     * @see name()
     * @see with_name()
     * @see without_name()
     *
     * @throws ostd::path_error
     */
    path &replace_name(string_range name) {
        remove_name();
        append_str(std::string{name});
        return *this;
    }

    /** @brief Gets the path with the name replaced.
     *
     * This is like replace_name(), except it doesn't modify this path.
     *
     * @see name()
     * @see with_name()
     * @see replace_name()
     *
     * @throws ostd::path_error
     */
    path with_name(string_range name) const {
        path ret{*this};
        ret.replace_name(name);
        return ret;
    }

    /** @brief Replaces the last suffix with a new one.
     *
     * If there is no suffix, it's not an error, the suffix is simply
     * added. For example, `foo.tar.gz` with `.bz2` becomees `foo.tar.bz2`,
     * and `foo` with `.zip` becomes `foo.zip`.
     *
     * If you want to replace all suffixes, use replace_suffixes().
     *
     * @see replace_suffixes()
     * @see with_suffix()
     */
    path &replace_suffix(string_range sfx = string_range{}) {
        auto osfx = suffix();
        if (!osfx.empty()) {
            p_path.erase(p_path.size() - osfx.size(), osfx.size());
        }
        p_path.append(sfx);
        return *this;
    }

    /** @brief Replaces all suffixes with a new one.
     *
     * If there is no suffix, it's not an error, the suffix is simply
     * added. For example, `foo.tar.gz` with `.zip` becomees `foo.zip`,
     * and `foo` with `.txt` becomes `foo.txt`.
     *
     * If you want to replace just the last suffix, use replace_suffix().
     *
     * @see replace_suffix()
     * @see with_suffixes()
     */
    path &replace_suffixes(string_range sfx = string_range{}) {
        auto sfxs = suffixes();
        if (!sfxs.empty()) {
            p_path.erase(p_path.size() - sfxs.size(), sfxs.size());
        }
        p_path.append(sfx);
        return *this;
    }

    /** @brief Gets the path, but with another suffix.
     *
     * This is just like replace_suffix() but does not modify the path.
     *
     * @see replace_suffix()
     * @see with_suffixes()
     */
    path with_suffix(string_range sfx = string_range{}) const {
        path ret{*this};
        ret.replace_suffix(sfx);
        return ret;
    }

    /** @brief Gets the path, but with different suffixes.
     *
     * This is just like replace_suffixes() but does not modify the path.
     *
     * @see replace_suffixes()
     * @see with_suffix()
     */
    path with_suffixes(string_range sfx = string_range{}) const {
        path ret{*this};
        ret.replace_suffixes(sfx);
        return ret;
    }

    /** @brief Joins the path with another one and returns the result.
     *
     * This is just like append() but does not modify the path.
     *
     * Also available as an operator overload `/`.
     *
     * @see append()
     */
    path join(path const &p) const {
        path ret{*this};
        ret.append(p);
        return ret;
    }

    /** @brief Appends another component to the path.
     *
     * The component is normalized. It may represent multiple components
     * if it contains separators. If it contains a root (on POSIX), the
     * previous contents of the path will be deleted first.
     *
     * If you do not wish to put a separator before the new component,
     * use append_concat().
     *
     * Also available as an operator overload `/=`.
     *
     * @see join()
     * @see append_concat()
     */
    path &append(path const &p) {
        append_str(p.p_path, p.p_fmt == p_fmt);
        return *this;
    }

    /** @brief Appends a sequence without adding a separator.
     *
     * This is handy for e.g. extending names. The given component still
     * undergoes normalization though.
     *
     * Also available as an operator overload `+=`.
     *
     * @see append()
     * @see concat()
     */
    path &append_concat(path const &p) {
        append_concat_str(p.p_path);
        return *this;
    }

    /** @brief Joins two paths together without a separator.
     *
     * This is just like append_concat() but does not modify the path.
     * Also available as an operator overload `+`.
     *
     * @see append_concat()
     */
    path concat(path const &p) const {
        path ret{*this};
        ret.append_concat(p);
        return ret;
    }

    /** @brief Like append(). */
    path &operator/=(path const &p) {
        return append(p);
    }

    /** @brief Like append_concat(). */
    path &operator+=(path const &p) {
        return append_concat(p);
    }

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
    bool match(path const &pattern) noexcept {
        return detail::glob_match_path_impl(
            string().data(), pattern.string().data()
        );
    }

    /** @brief Gets the path as a string.
     *
     * For maximum compatibility, this is returned as a const reference
     * to std::string, not ostd::string_range. A range can be easily
     * constructed from it.
     *
     * The same is also available via implicit conversion operators.
     */
    std::string const &string() const noexcept {
        return p_path;
    }

    /** @brief Implicitly converts to std::string. */
    operator std::string() const {
        return p_path;
    }

    /** @brief Implicitly converts to ostd::string_range. */
    operator string_range() const noexcept {
        return p_path;
    }

    /** @brief Gets the format of the path.
     *
     * This always returns either `format::posix` or `format::windows`,
     * never `format::native`. If you want to check what the native format
     * is, use path::native_format.
     */
    format path_format() const noexcept {
        return p_fmt;
    }

    /** @brief Clears the path.
     *
     * This makes the path empty, i.e. `.`.
     */
    void clear() {
        p_path = ".";
    }

    /** @brief Checks if the path is empty.
     *
     * The path is actually never truly empty, but `.` is considered an
     * empty path.
     */
    bool empty() const noexcept {
        return (p_path == ".");
    }

    /** @brief Swaps the contents with another path. */
    void swap(path &other) noexcept {
        p_path.swap(other.p_path);
        std::swap(p_fmt, other.p_fmt);
    }

    /** @brief Iterates the path by components.
     *
     * See path::range for behavior.
     */
    range iter() const noexcept;

private:
    static format path_fmt(format f) noexcept {
        static const format fmts[] = {
            native_format, format::posix, format::windows
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

/** @brief Joins two paths.
 *
 * Equivalent to `p1.join(p2)`.
 */
inline path operator/(path const &p1, path const &p2) {
    return p1.join(p2);
}

/** @brief Concatenates two paths.
 *
 * Equivalent to `p1.concat(p2)`.
 */
inline path operator+(path const &p1, path const &p2) {
    return p1.concat(p2);
}

/** @brief Checks if two paths are the same. */
inline bool operator==(path const &p1, path const &p2) {
    return (p1.string() == p2.string());
}

/** @brief Checks if two paths are not the same. */
inline bool operator!=(path const &p1, path const &p2) {
    return !(p1 == p2);
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

/** @brief ostd::format_traits specialization for paths.
 *
 * This allows paths to be formatted as strings. The path is formatted as
 * in `path.string()` was formatted, using the exact ostd::format_spec.
 */
template<>
struct format_traits<path> {
    /** @brief Formats the path's string value.
     *
     * This is exactly `fs.format_value(writer, p.string())`.
     */
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

/** @addtogroup Filesystem
 * @{
 */

/** @brief The time point used to represent file access times. */
using file_time_t = std::chrono::time_point<std::chrono::system_clock>;

/** @brief An exception thrown by filesystem operations.
 *
 * Unlike ostd::path_error, this is thrown by operations doing syscalls.
 * It is like std::system_error and represents OS specific error codes.
 *
 * The API is identical to std::system_error except it also contains
 * up to two paths involved in the operation as ostd::path. Whether those
 * are used depends on the operation.
 */
struct fs_error: std::system_error {
    /** @brief Constructs the error without paths. */
    fs_error(std::string const &warg, std::error_code ec):
        std::system_error::system_error(ec, warg)
    {}

    /** @brief Constructs the error with one path. */
    fs_error(std::string const &warg, path const &p1, std::error_code ec):
        std::system_error::system_error(ec, warg), p_p1(p1)
    {}

    /** @brief Constructs the error with two paths. */
    fs_error(
        std::string const &warg, path const &p1,
        path const &p2, std::error_code ec
    ):
        std::system_error::system_error(ec, warg), p_p1(p1), p_p2(p2)
    {}

    /* empty, for vtable placement */
    virtual ~fs_error();

    /** @brief Gets the first path involved in the operation. */
    path const &path1() const noexcept {
        return p_p1;
    }

    /** @brief Gets the second path involved in the operation. */
    path const &path2() const noexcept {
        return p_p1;
    }

private:
    path p_p1{}, p_p2{};
};

/** @brief An enumeration representing file types. */
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

/** @brief An enumeration representing permissions based on POSIX. */
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

/** @brief Allows bitwise OR on permissions. */
inline perms operator|(perms a, perms b) {
    return perms(int(a) | int(b));
}

/** @brief Allows bitwise AND on permissions. */
inline perms operator&(perms a, perms b) {
    return perms(int(a) & int(b));
}

/** @brief Allows bitwise XOR on permissions. */
inline perms operator^(perms a, perms b) {
    return perms(int(a) ^ int(b));
}

/** @brief Allows bitwise NOT on permissions. */
inline perms operator~(perms v) {
    return perms(~int(v));
}

/** @brief Allows bitwise OR on permissions. */
inline perms &operator|=(perms &a, perms b) {
    a = (a | b);
    return a;
}

/** @brief Allows bitwise AND on permissions. */
inline perms &operator&=(perms &a, perms b) {
    a = (a & b);
    return a;
}

/** @brief Allows bitwise XOR on permissions. */
inline perms &operator^=(perms &a, perms b) {
    a = (a ^ b);
    return a;
}

/** @brief Represents file mode as in `st_mode` of `struct stat`.
 *
 * This structure encodes two things, file type and permissions, using
 * fs::file_type and fs::perms. These are internally encoded within a
 * single integer and represent the `st_mode` field of `struct stat`
 * when on POSIX systems.
 */
struct file_mode {
private:
    using UT = std::uint_least32_t;
    UT p_val;

public:
    /** @brief Constructs the mode as in with file_type::none. */
    file_mode() noexcept: file_mode(file_type::none) {}

    /** @brief Constructs the file mode. */
    file_mode(file_type type, perms permissions = perms::unknown) noexcept:
        p_val(UT(permissions) | (UT(type) << 16))
    {}

    /** @brief Gets the file type. */
    file_type type() const noexcept {
        return file_type(p_val >> 16);
    }

    /** @brief Gets the permissions. */
    perms permissions() const noexcept {
        return perms(p_val & 0xFFFF);
    }
};

/** @brief Represents file status as in `struct stat`.
 *
 * This is similar to `struct stat` on POSIX systems, except it has fewer
 * fields and is meant to be portable. Its contents represent the file
 * mode (fs::file_mode), last modification time (fs::file_time_t),
 * file size (if possible, can be unspecified) and the hard link count.
 *
 * There are standalone functions to get these attributes on their own
 * instead of retrieving all. On POSIX systems, these work by simply
 * calling fs::status(), so they need to retrieve all the information
 * anyway, but they may be more efficient on other systems.
 */
struct file_status {
    /** @brief Constructs the status. */
    file_status(
        file_mode mode, file_time_t mtime,
        std::uintmax_t size, std::uintmax_t nlinks
    ):
        p_links(nlinks), p_size(size), p_mtime(mtime), p_mode(mode)
    {}

    /** @brief Gets the file mode. */
    file_mode mode() const noexcept {
        return p_mode;
    }

    /** @brief Gets the file modification time. */
    file_time_t last_write_time() const noexcept {
        return p_mtime;
    }

    /** @brief Gets the hard link count. */
    std::uintmax_t hard_link_count() const noexcept {
        return p_links;
    }

    /** @brief Gets the file size.
     *
     * Keep in mind that this is only guaranteed to be define for
     * regular files (the file size in bytes) and symbolic links
     * (the length of the encoded path). Otherwise, it is system
     * dependent and you need to refer to the documentation for
     * those systems (e.g. `struct stat`).
     */
    std::uintmax_t size() const noexcept {
        return p_size;
    }

private:
    std::uintmax_t p_links;
    std::uintmax_t p_size;
    file_time_t p_mtime;
    file_mode p_mode;
};

/** @brief Gets the file status for the given path.
 *
 * This works as in POSIX `stat`. For symbolic links, it refers to the
 * target of the link, not the symbolic link itself. IF you need to
 * refer to the symbolic link, use symlink_status().
 *
 * It may fail with ostd::fs_error.
 *
 * @see symlink_status()
 * @see mode()
 * @see file_size()
 * @see last_write_time()
 * @see hard_link_count()
 */
OSTD_EXPORT file_status status(path const &p);

/** @brief Gets the file or symlink status.
 *
 * This is like status(), but when `p` refers to a symlink, the
 * result refers to the symlink itself rather than the target.
 *
 * @see status()
 */
OSTD_EXPORT file_status symlink_status(path const &p);

/** @brief Gets the file mode.
 *
 * This is effectively equivalent to `status(p).mode()`,
 * but may be more efficient on some systems.
 *
 * @see symlink_mode()
 * @see status()
 */
OSTD_EXPORT file_mode mode(path const &p);

/** @brief Gets the file or symlink mode.
 *
 * This is like mode(), but when `p` refers to a symlink, the
 * result refers to the symlink itself rather than the target.
 *
 * @see mode()
 * @see symlink_status()
 */
OSTD_EXPORT file_mode symlink_mode(path const &p);

/** @brief Gets the file size.
 *
 * This may not be specified on all systems (see file_status::size()).
 * This is effectively equivalent to `status(p).file_size()`,
 * but may be more efficient on some systems.
 *
 * @see status()
 * @see file_status::size()
 */
OSTD_EXPORT std::uintmax_t file_size(path const &p);

/** @brief Gets the file hard link count.
 *
 * This is effectively equivalent to `status(p).hard_link_count()`,
 * but may be more efficient on some systems.
 *
 * @see status()
 * @see file_status::hard_linl_count()
 */
OSTD_EXPORT std::uintmax_t hard_link_count(path const &p);

/** @brief Gets the file modification time.
 *
 * This is effectively equivalent to `status(p).last_write_time()`,
 * but may be more efficient on some systems.
 *
 * @see status()
 * @see last_write_time(path const &, file_time_t)
 * @see file_status::last_write_time()
 */
OSTD_EXPORT file_time_t last_write_time(path const &p);

/** @brief Sets the file modification time.
 *
 * This changes the file modification time as in POSIX `utimensat()`.
 * It may or may not support nanosecond precision depending on the
 * operating system.
 *
 * @see last_write_time(path const &)
 */
OSTD_EXPORT void last_write_time(path const &p, file_time_t new_time);

/** @brief Checks if file mode is file_type::block. */
inline bool is_block_file(file_mode st) noexcept {
    return st.type() == file_type::block;
}

/** @brief Like `is_block_file(mode(p))`. */
inline bool is_block_file(path const &p) {
    return is_block_file(mode(p));
}

/** @brief Checks if file mode is file_type::character. */
inline bool is_character_file(file_mode st) noexcept {
    return st.type() == file_type::character;
}

/** @brief Like `is_character_file(mode(p))`. */
inline bool is_character_file(path const &p) {
    return is_character_file(mode(p));
}

/** @brief Checks if file mode is file_type::directory. */
inline bool is_directory(file_mode st) noexcept {
    return st.type() == file_type::directory;
}

/** @brief Like `is_directory(mode(p))`. */
inline bool is_directory(path const &p) {
    return is_directory(mode(p));
}

/** @brief Checks if file mode is file_type::regular. */
inline bool is_regular_file(file_mode st) noexcept {
    return st.type() == file_type::regular;
}

/** @brief Like `is_regular_file(mode(p))`. */
inline bool is_regular_file(path const &p) {
    return is_regular_file(mode(p));
}

/** @brief Checks if file mode is file_type::fifo. */
inline bool is_fifo(file_mode st) noexcept {
    return st.type() == file_type::fifo;
}

/** @brief Like `is_fifo(mode(p))`. */
inline bool is_fifo(path const &p) {
    return is_fifo(mode(p));
}

/** @brief Checks if file mode is file_type::symlink. */
inline bool is_symlink(file_mode st) noexcept {
    return st.type() == file_type::symlink;
}

/** @brief Like `is_symlink(mode(p))`. */
inline bool is_symlink(path const &p) {
    return is_symlink(mode(p));
}

/** @brief Checks if file mode is file_type::socket. */
inline bool is_socket(file_mode st) noexcept {
    return st.type() == file_type::socket;
}

/** @brief Like `is_socket(mode(p))`. */
inline bool is_socket(path const &p) {
    return is_socket(mode(p));
}

/** @brief Checks if file mode is file_type::unknown. */
inline bool is_other(file_mode st) noexcept {
    return st.type() == file_type::unknown;
}

/** @brief Like `is_other(mode(p))`. */
inline bool is_other(path const &p) {
    return is_other(mode(p));
}

/** @brief Checks if file mode is known (not file_type::none). */
inline bool mode_known(file_mode st) noexcept {
    return st.type() != file_type::none;
}

/** @brief Like `mode_known(mode(p))`. */
inline bool mode_known(path const &p) {
    return mode_known(mode(p));
}

namespace detail {
    struct dir_range_impl;
    struct rdir_range_impl;
} /* namespace detail */

/** @brief Represents an entry in a directory.
 *
 * This will typically be an element of a directory range like
 * fs::directory_range or fs::recursive_directory_range. You can
 * use it to retrieve the path of the entry as well as check its type,
 * which is usually more efficient than checking it manually, because
 * it is retrieved during the directory traversal. This is also why the
 * checking functions never throw.
 */
struct directory_entry {
    /** @brief Constructs an empty directory entry. */
    directory_entry() {}

    /** @brief COnstructs a directory entry from a path.
     *
     * This calls refresh(), so it may throw. Directory ranges will
     * not use this constructor and instead construct the entry
     * internally with their own info.
     */
    directory_entry(ostd::path const &p): p_path(p) {
        refresh();
    }

    /** @brief Gets the path to the entry.
     *
     * When constructed manually, this will be the same path. When
     * constructed through iteration, this will be the path provided
     * to the range constructor joined with the entry name.
     */
    ostd::path const &path() const noexcept {
        return p_path;
    }

    /** @brief Implicit conversion to path.
     *
     * See path().
     */
    operator ostd::path const &() const noexcept {
        return p_path;
    }

    /** @brief Refreshes the entry type.
     *
     * Like `symlink_mode(path())`.
     *
     * @see symlink_mode()
     * @see path()
     */
    void refresh() {
        p_type = symlink_mode(p_path);
    }

    /** @brief Effectively like `is_block_file(path())`. */
    bool is_block_file() const noexcept {
        return fs::is_block_file(p_type);
    }

    /** @brief Effectively like `is_character_file(path())`. */
    bool is_character_file() const noexcept {
        return fs::is_character_file(p_type);
    }

    /** @brief Effectively like `is_directory(path())`. */
    bool is_directory() const noexcept {
        return fs::is_directory(p_type);
    }

    /** @brief Effectively like `is_fifo(path())`. */
    bool is_fifo() const noexcept {
        return fs::is_fifo(p_type);
    }

    /** @brief Effectively like `is_other(path())`. */
    bool is_other() const noexcept {
        return fs::is_other(p_type);
    }

    /** @brief Effectively like `is_regular_file(path())`. */
    bool is_regular_file() const noexcept {
        return fs::is_regular_file(p_type);
    }

    /** @brief Effectively like `is_socket(path())`. */
    bool is_socket() const noexcept {
        return fs::is_socket(p_type);
    }

    /** @brief Effectively like `is_symlink(path())`. */
    bool is_symlink() const noexcept {
        return fs::is_symlink(p_type);
    }

    /** @brief Checks if the entry exists.
     *
     * The type most not be file_type::none or file_type::not_found.
     */
    bool exists() const noexcept {
        return (mode_known(p_type) && (p_type.type() != file_type::not_found));
    }

private:
    friend struct detail::dir_range_impl;
    friend struct detail::rdir_range_impl;

    directory_entry(ostd::path &&p, file_mode tp):
        p_path(p), p_type(tp)
    {}

    ostd::path p_path{};
    file_mode p_type{};
};

/** @brief A simple range for traversal of a directory.
 *
 * The range is an ostd::input_range_path, so it is only suitable for
 * single-pass algorithms. Its elements are fs::directory_entry and
 * they are not mutable. This simply goes over the directory and
 * gives you access to each field.
 *
 * If you need recursive traversal, use fs::recursive_directory_range.
 *
 * Th elements will be only valid entries and never `.` or `..` like
 * with POSIX `readdir()`. The iteration order is not defined. It is
 * also not defined whether entries added to the directory during
 * iteration will be represented in the range.
 **/
struct OSTD_EXPORT directory_range: input_range<directory_range> {
    using range_category = input_range_tag;
    using value_type = directory_entry;
    using reference = directory_entry const &;
    using size_type = std::size_t;

    directory_range() = delete;

    /** @brief Constructs a directory range using a path.
     *
     * Upon failure, this will throw fs::fs_error.
     *
     * @throws fs::fs_error
     */
    directory_range(path const &p);

    /** @brief Checks if there are any entries in the range. */
    bool empty() const noexcept;

    /** @brief Pops out the current entry and retrieves the next one.
     *
     * On failure, this will throw fs::fs_error.
     *
     * @throws fs::fs_error
     */
    void pop_front();

    /** @brief Retrieves the current entry. */
    reference front() const noexcept;

private:
    std::shared_ptr<detail::dir_range_impl> p_impl;
};

/** @brief A recursive range for traversal of a directory.
 *
 * This is mostly like fs::directory_range, except it is also recursive,
 * so it will go through any sub-directories. The iteration order within
 * a directory is still undefined, but the range does guarantee that if
 * th entry is a directory, it will be listed before its contents.
 */
struct OSTD_EXPORT recursive_directory_range:
    input_range<recursive_directory_range>
{
    using range_category = input_range_tag;
    using value_type = directory_entry;
    using reference = directory_entry const &;
    using size_type = std::size_t;

    recursive_directory_range() = delete;

    /** @brief Constructs a recursive directory range using a path.
     *
     * Upon failure, this will throw fs::fs_error.
     *
     * @throws fs::fs_error
     */
    recursive_directory_range(path const &p);

    /** @brief Checks if there are any entries in the range. */
    bool empty() const noexcept;

    /** @brief Pops out the current entry and retrieves the next one.
     *
     * On failure, this will throw fs::fs_error.
     *
     * @throws fs::fs_error
     */
    void pop_front();

    /** @brief Retrieves the current entry. */
    reference front() const noexcept;

private:
    std::shared_ptr<detail::rdir_range_impl> p_impl;
};

/** @brief Gets the current working directory.
 *
 * This will return an absolute path referring to the current
 * working directory. On failure, it will throw fs::fs_error.
 *
 * it is equivalent to POSIX `getcwd()`.
 *
 * @see current_path(path const &)
 * @see home_path()
 * @see temp_path()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT path current_path();

/** @brief Gets the home directory path.
 *
 * On POSIX, this will attempt to use the environment variahle `HOME`
 * and if that is not possible, it will look up the path from the
 * passwd database.
 *
 * On Windows, this will attempt `HOME` and `USERPROFILE`, otherwise
 * a combination of `HOMEPATH` and `HOMEDRIVE` will be used.
 *
 * On failure, it will throw fs::fs_error.
 *
 * @see current_path()
 * @see temp_path()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT path home_path();

/** @brief Gets the temp directory path.
 *
 * On failure, it may throw fs::fs_error, but in practice it will never
 * throw that exceotuib on POSIX systems, because it will only try
 * environment variables (`TMPDIR`, `TMP`, `TEMP` and `TEMPDIR` in
 * that order) and return the `/tmp` fallback if none exist.
 *
 * On Windows, this will use `GetTempPath()`, which is allowed to fail.
 *
 * @see current_path()
 * @see home_path()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT path temp_path();

/** @brief Sets the current working directory.
 *
 * This works as in POSIX `chdir()`.
 *
 * @see current_path()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT void current_path(path const &p);

/** @brief Returns an absolute path for the given path.
 *
 * If the path is already absolute, it will just copy it.
 * Otherwise, the result will be `current_path() / p`.
 *
 * @see current_path()
 * @see relative()
 */
OSTD_EXPORT path absolute(path const &p);

/** @brief Canonicalizes the given path.
 *
 * This is equivalent to calling POSIX `realpath()`. The result is
 * a path that is absolute, fully expanded and refers to the same
 * file as the given path.
 *
 * @see weakly_canonical()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT path canonical(path const &p);

/** @brief Weakly canonicalizes the given path.
 *
 * Unlike canonical(), this also works for paths that do not exist.
 * The function will try stripping the last component of the given
 * path until it reaches an existing file; then it canonicalizes
 * that path and concatenates it with the rest that it stripped off.
 *
 * @see canonical()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT path weakly_canonical(path const &p);

/** @brief Makes `p` relative to `base`.
 *
 * This is much like path::relative_to(), but it works with weakly
 * canonicalized `p` and `base`, so it is equivalent to simply
 * calling `weakly_canonical(p).relative_tp(weakly_canonical(base))`.
 *
 * @see weakly_canonical()
 * @see path::relative_to()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT path relative(path const &p, path const &base = current_path());

/** @brief Checks if the file mode refers to an existing file.
 *
 * The type must be mode_known() and must not be file_type::not_found.
 *
 * @see exists(path const &)
 */
inline bool exists(file_mode s) noexcept {
    return (mode_known(s) && (s.type() != file_type::not_found));
}

/** @brief Checks if a file exists.
 *
 * This merely checks existence, not whether you have the
 * permissions to actually read, write or execute the file.
 *
 * @see exists(file_mode)
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT bool exists(path const &p);

/** @brief Checks if two paths are equivalent.
 *
 * On POSIX, paths are equivalent if the devices (`st_dev`)
 * and inodes (`st_ino`) when calling `stat()` are the same.
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT bool equivalent(path const &p1, path const &p2);

/** @brief Creates a directory.
 *
 * This will fail if the path already exists and is not a directory.
 * It may also fail in other system specific cases. On failure, it
 * will throw fs::fs_error.
 *
 * If the path exists and is a directory, it will return `false`.
 * Otherwise, `true` is returned.
 *
 * The parent directory must exist. The created directory will have
 * permissions `0777` on POSIX.
 *
 * @see create_directory(path const &, path const &)
 * @see create_directories()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT bool create_directory(path const &p);

/** @brief Creates a directory.
 *
 * This is like create_directory(path const &), but it will take
 * attributes from the reference path `ep`. The attributes taken
 * depend on the OS, on POSIX it will take the permissions.
 *
 * @see create_directory(path const &)
 * @see create_directories()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT bool create_directory(path const &p, path const &ep);

/** @brief Creates directories.
 *
 * Given a path, this will attempt to create a directory and if
 * any of the directories within that path do not exist, it will
 * also create those on the way. It is similar to `mkdir -p` on
 * Unix-like systems.
 *
 * @see create_directory()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT bool create_directories(path const &p);

/** @brief Remvoes a file or a directory.
 *
 * If it's a directory, it must be empty. It follows POSIX `remove()` behavior.
 *
 * If the path does not exist, it will not throw but it will return `false`.
 *
 * @see remove_all()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT bool remove(path const &p);

/** @brief Remvoes a file or a directory including contents.
 *
 * The number of removed files/directories is returned.
 *
 * @see remove()
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT std::uintmax_t remove_all(path const &p);

/** @brief Moves or renames a path.
 *
 * This is equivalent to POSIX `rename()`. This means the following:
 *
 * If `op` is a file and not a directory:
 *
 * - If `np` is the same file or a hard link, nothing happens.
 * - If `np` is a non-file directory, it is removed before `op` is renamed.
 * - If `np` does not exist and its parent directory does, `op` is renamed.
 * - Every other case will fail.
 *
 * If `op` is a directory:
 *
 * - If `np` is the same directory or a hard link, nothing happens.
 * - If `np` is an existing directory, it is removed on POSIX, but may
 *   fail elsewhere. The `op` is then renamed.
 * - If `np` does not exist and its parent directory does, `op` is renamed.
 * - Every other case will fail.
 *
 * Write permissions are necessary in all cases. The `op` must not be
 * an ancestor of `np` and `np` must not end with `..`.
 *
 * @throws fs::fs_error
 */
OSTD_EXPORT void rename(path const &op, path const &np);

namespace detail {
    OSTD_EXPORT void glob_match_impl(
        void (*out)(path const &, void *),
        typename path::range r, path pre, void *data
    );
} /* namespace detail */

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
 * @throws fs::fs_error if a filesystem error occurs.
 * @returns The forwarded `out`.
 */
template<typename OutputRange>
inline OutputRange &&glob_match(OutputRange &&out, path const &pattern) {
    detail::glob_match_impl([](path const &p, void *outp) {
        static_cast<std::remove_reference_t<OutputRange> *>(outp)->put(p);
    }, pattern.iter(), path{}, &out);
    return std::forward<OutputRange>(out);
}

/** @} */

} /* namesapce fs */
} /* namesapce ostd */

#endif

/** @} */
