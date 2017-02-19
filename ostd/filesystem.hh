/* Filesystem API for OctaSTD. Currently POSIX only.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_FILESYSTEM_HH
#define OSTD_FILESYSTEM_HH

#include "ostd/platform.hh"
#include "ostd/internal/win32.hh"

#ifdef OSTD_PLATFORM_POSIX
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#else
#include <direct.h>
#endif

#include <utility>

#include "ostd/types.hh"
#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/string.hh"
#include "ostd/algorithm.hh"

namespace ostd {

enum class file_type {
    UNKNOWN, REGULAR, FIFO, CHARACTER, DIRECTORY, BLOCK, SYMLINK, SOCKET
};

struct file_info;

#ifdef OSTD_PLATFORM_WIN32
static constexpr char PathSeparator = '\\';
#else
static constexpr char PathSeparator = '/';
#endif

#ifdef OSTD_PLATFORM_WIN32
namespace detail {
    inline time_t filetime_to_time_t(FILETIME const &ft) {
        ULARGE_INTEGER ul;
        ul.LowPart  = ft.dwLowDateTime;
        ul.HighPart = ft.dwHighDateTime;
        return static_cast<time_t>((ul.QuadPart / 10000000ULL) - 11644473600ULL);
    }
}
#endif

inline void path_normalize(char_range) {
    /* TODO */
}

struct file_info {
    file_info() {}

    file_info(file_info const &i):
        p_slash(i.p_slash), p_dot(i.p_dot), p_type(i.p_type),
        p_path(i.p_path), p_atime(i.p_atime), p_mtime(i.p_mtime),
        p_ctime(i.p_ctime)
    {}

    file_info(file_info &&i):
        p_slash(i.p_slash), p_dot(i.p_dot), p_type(i.p_type),
        p_path(std::move(i.p_path)), p_atime(i.p_atime), p_mtime(i.p_mtime),
        p_ctime(i.p_ctime)
    {
        i.p_slash = i.p_dot = std::string::npos;
        i.p_type = file_type::UNKNOWN;
        i.p_atime = i.p_ctime = i.p_mtime = 0;
    }

    file_info(string_range path) {
        init_from_str(path);
    }

    file_info &operator=(file_info const &i) {
        p_slash = i.p_slash;
        p_dot = i.p_dot;
        p_type = i.p_type;
        p_path = i.p_path;
        p_atime = i.p_atime;
        p_mtime = i.p_mtime;
        p_ctime = i.p_ctime;
        return *this;
    }

    file_info &operator=(file_info &&i) {
        swap(i);
        return *this;
    }

    string_range path() const { return ostd::iter(p_path); }

    string_range filename() const {
        return path().slice(
            (p_slash == std::string::npos) ? 0 : (p_slash + 1), p_path.size()
        );
    }

    string_range stem() const {
        return path().slice(
            (p_slash == std::string::npos) ? 0 : (p_slash + 1),
            (p_dot == std::string::npos) ? p_path.size() : p_dot
        );
    }

    string_range extension() const {
        return (p_dot == std::string::npos)
            ? string_range()
            : path().slice(p_dot, p_path.size());
    }

    file_type type() const { return p_type; }

    void normalize() {
        path_normalize(ostd::iter(p_path));
        init_from_str(ostd::iter(p_path));
    }

    time_t atime() const { return p_atime; }
    time_t mtime() const { return p_mtime; }
    time_t ctime() const { return p_ctime; }

    void swap(file_info &i) {
        using std::swap;
        swap(i.p_slash, p_slash);
        swap(i.p_dot, p_dot);
        swap(i.p_type, p_type);
        swap(i.p_path, p_path);
        swap(i.p_atime, p_atime);
        swap(i.p_mtime, p_mtime);
        swap(i.p_ctime, p_ctime);
    }

private:
    void init_from_str(string_range path) {
        /* TODO: implement a version that will follow symbolic links */
        p_path = path;
#ifdef OSTD_PLATFORM_WIN32
        WIN32_FILE_ATTRIBUTE_DATA attr;
        if (!GetFileAttributesEx(p_path.data(), GetFileExInfoStandard, &attr) ||
            attr.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
#else
        struct stat st;
        if (lstat(p_path.data(), &st) < 0)
#endif
        {
            p_slash = p_dot = std::string::npos;
            p_type = file_type::UNKNOWN;
            p_path.clear();
            p_atime = p_mtime = p_ctime = 0;
            return;
        }
        string_range r = p_path;

        string_range found = find_last(r, PathSeparator);
        if (found.empty()) {
            p_slash = std::string::npos;
        } else {
            p_slash = r.distance_front(found);
        }

        found = find(filename(), '.');
        if (found.empty()) {
            p_dot = std::string::npos;
        } else {
            p_dot = r.distance_front(found);
        }

#ifdef OSTD_PLATFORM_WIN32
        if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            p_type = file_type::DIRECTORY;
        } else if (attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            p_type = file_type::SYMLINK;
        } else if (attr.dwFileAttributes & (
            FILE_ATTRIBUTE_ARCHIVE     | FILE_ATTRIBUTE_COMPRESSED |
            FILE_ATTRIBUTE_HIDDEN      | FILE_ATTRIBUTE_NORMAL     |
            FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_TEMPORARY
        )) {
            p_type = file_type::REGULAR;
        } else {
            p_type = file_type::UNKNOWN;
        }

        p_atime = detail::filetime_to_time_t(attr.ftLastAccessTime);
        p_mtime = detail::filetime_to_time_t(attr.ftLastWriteTime);
        p_ctime = detail::filetime_to_time_t(attr.ftCreationTime);
#else
        if (S_ISREG(st.st_mode)) {
            p_type = file_type::REGULAR;
        } else if (S_ISDIR(st.st_mode)) {
            p_type = file_type::DIRECTORY;
        } else if (S_ISCHR(st.st_mode)) {
            p_type = file_type::CHARACTER;
        } else if (S_ISBLK(st.st_mode)) {
            p_type = file_type::BLOCK;
        } else if (S_ISFIFO(st.st_mode)) {
            p_type = file_type::FIFO;
        } else if (S_ISLNK(st.st_mode)) {
            p_type = file_type::SYMLINK;
        } else if (S_ISSOCK(st.st_mode)) {
            p_type = file_type::SOCKET;
        } else {
            p_type = file_type::UNKNOWN;
        }

        p_atime = st.st_atime;
        p_mtime = st.st_mtime;
        p_ctime = st.st_ctime;
#endif
    }

    size_t p_slash = std::string::npos, p_dot = std::string::npos;
    file_type p_type = file_type::UNKNOWN;
    std::string p_path;

    time_t p_atime = 0, p_mtime = 0, p_ctime = 0;
};

inline void swap(file_info &a, file_info &b) {
    a.swap(b);
}

struct directory_range;

#ifndef OSTD_PLATFORM_WIN32
struct directory_stream {
    friend struct directory_range;

    directory_stream(): p_d(), p_de(), p_path() {}
    directory_stream(directory_stream const &) = delete;
    directory_stream(directory_stream &&s):
        p_d(s.p_d), p_de(s.p_de), p_path(std::move(s.p_path))
    {
        s.p_d = nullptr;
        s.p_de = nullptr;
    }

    directory_stream(string_range path): directory_stream() {
        open(path);
    }

    ~directory_stream() { close(); }

    directory_stream &operator=(directory_stream const &) = delete;
    directory_stream &operator=(directory_stream &&s) {
        close();
        swap(s);
        return *this;
    }

    bool open(string_range path) {
        if (p_d || (path.size() > FILENAME_MAX)) {
            return false;
        }
        char buf[FILENAME_MAX + 1];
        memcpy(buf, &path[0], path.size());
        buf[path.size()] = '\0';
        p_d = opendir(buf);
        if (!pop_front()) {
            close();
            return false;
        }
        p_path = path;
        return true;
    }

    bool is_open() const { return p_d != nullptr; }

    void close() {
        if (p_d) closedir(p_d);
        p_d = nullptr;
        p_de = nullptr;
    }

    long size() const {
        if (!p_d) {
            return -1;
        }
        DIR *td = opendir(p_path.data());
        if (!td) {
            return -1;
        }
        long ret = 0;
        struct dirent *rd;
        while (pop_front(td, &rd)) {
            ret += strcmp(rd->d_name, ".") && strcmp(rd->d_name, "..");
        }
        closedir(td);
        return ret;
    }

    bool rewind() {
        if (!p_d) {
            return false;
        }
        rewinddir(p_d);
        if (!pop_front()) {
            close();
            return false;
        }
        return true;
    }

    bool empty() const {
        return !p_de;
    }

    file_info read() {
        if (!pop_front()) {
            return file_info();
        }
        return front();
    }

    void swap(directory_stream &s) {
        using std::swap;
        swap(p_d, s.p_d);
        swap(p_de, s.p_de);
        swap(p_path, s.p_path);
    }

    directory_range iter();

private:
    static bool pop_front(DIR *d, struct dirent **de) {
        if (!d) return false;
        if (!(*de = readdir(d)))
            return false;
        /* order of . and .. in the stream is not guaranteed, apparently...
         * gotta check every time because of that
         */
        while (*de && (
            !strcmp((*de)->d_name, ".") || !strcmp((*de)->d_name, "..")
        )) {
            if (!(*de = readdir(d))) {
                return false;
            }
        }
        return !!*de;
    }

    bool pop_front() {
        return pop_front(p_d, &p_de);
    }

    file_info front() const {
        if (!p_de) {
            return file_info();
        }
        std::string ap = p_path;
        ap += PathSeparator;
        ap += static_cast<char const *>(p_de->d_name);
        return file_info(ap);
    }

    DIR *p_d;
    struct dirent *p_de;
    std::string p_path;
};

#else /* OSTD_PLATFORM_WIN32 */

struct directory_stream {
    friend struct directory_range;

    directory_stream(): p_handle(INVALID_HANDLE_VALUE), p_data(), p_path() {}
    directory_stream(directory_stream const &) = delete;
    directory_stream(directory_stream &&s):
        p_handle(s.p_handle), p_data(s.p_data), p_path(std::move(s.p_path))
    {
        s.p_handle = INVALID_HANDLE_VALUE;
        memset(&s.p_data, 0, sizeof(s.p_data));
    }

    directory_stream(string_range path): directory_stream() {
        open(path);
    }

    ~directory_stream() { close(); }

    directory_stream &operator=(directory_stream const &) = delete;
    directory_stream &operator=(directory_stream &&s) {
        close();
        swap(s);
        return *this;
    }

    bool open(string_range path) {
        if (p_handle != INVALID_HANDLE_VALUE) {
            return false;
        }
        if ((path.size() >= 1024) || !path.size()) {
            return false;
        }
        char buf[1026];
        memcpy(buf, &path[0], path.size());
        char *bptr = &buf[path.size()];
        /* if path ends with a slash, replace it */
        bptr -= ((*(bptr - 1) == '\\') || (*(bptr - 1) == '/'));
        /* include trailing zero */
        memcpy(bptr, "\\*", 3);
        p_handle = FindFirstFile(buf, &p_data);
        if (p_handle == INVALID_HANDLE_VALUE) {
            return false;
        }
        while (
            !strcmp(p_data.cFileName, ".") || !strcmp(p_data.cFileName, "..")
        ) {
            if (!FindNextFile(p_handle, &p_data)) {
                FindClose(p_handle);
                p_handle = INVALID_HANDLE_VALUE;
                p_data.cFileName[0] = '\0';
                return false;
            }
        }
        p_path = path;
        return true;
    }

    bool is_open() const { return p_handle != INVALID_HANDLE_VALUE; }

    void close() {
        if (p_handle != INVALID_HANDLE_VALUE) {
            FindClose(p_handle);
        }
        p_handle = INVALID_HANDLE_VALUE;
        p_data.cFileName[0] = '\0';
    }

    long size() const {
        if (p_handle == INVALID_HANDLE_VALUE) {
            return -1;
        }
        WIN32_FIND_DATA wfd;
        HANDLE td = FindFirstFile(p_path.data(), &wfd);
        if (td == INVALID_HANDLE_VALUE) {
            return -1;
        }
        while (!strcmp(wfd.cFileName, ".") && !strcmp(wfd.cFileName, "..")) {
            if (!FindNextFile(td, &wfd)) {
                FindClose(td);
                return 0;
            }
        }
        long ret = 1;
        while (FindNextFile(td, &wfd)) {
            ++ret;
        }
        FindClose(td);
        return ret;
    }

    bool rewind() {
        if (p_handle != INVALID_HANDLE_VALUE) {
            FindClose(p_handle);
        }
        p_handle = FindFirstFile(p_path.data(), &p_data);
        if (p_handle == INVALID_HANDLE_VALUE) {
            p_data.cFileName[0] = '\0';
            return false;
        }
        while (
            !strcmp(p_data.cFileName, ".") || !strcmp(p_data.cFileName, "..")
        ) {
            if (!FindNextFile(p_handle, &p_data)) {
                FindClose(p_handle);
                p_handle = INVALID_HANDLE_VALUE;
                p_data.cFileName[0] = '\0';
                return false;
            }
        }
        return true;
    }

    bool empty() const {
        return p_data.cFileName[0] == '\0';
    }

    file_info read() {
        if (!pop_front()) {
            return file_info();
        }
        return front();
    }

    void swap(directory_stream &s) {
        using std::swap;
        swap(p_handle, s.p_handle);
        swap(p_data, s.p_data);
        swap(p_path, s.p_path);
    }

    directory_range iter();

private:
    bool pop_front() {
        if (!is_open()) {
            return false;
        }
        if (!FindNextFile(p_handle, &p_data)) {
            p_data.cFileName[0] = '\0';
            return false;
        }
        return true;
    }

    file_info front() const {
        if (empty()) {
            return file_info();
        }
        std::string ap = p_path;
        ap += PathSeparator;
        ap += static_cast<char const *>(p_data.cFileName);
        return file_info(ap);
    }

    HANDLE p_handle;
    WIN32_FIND_DATA p_data;
    std::string p_path;
};
#endif /* OSTD_PLATFORM_WIN32 */

inline void swap(directory_stream &a, directory_stream &b) {
    a.swap(b);
}

struct directory_range: input_range<directory_range> {
    using range_category  = input_range_tag;
    using value_type      = file_info;
    using reference       = file_info;
    using size_type       = size_t;
    using difference_type = long;

    directory_range() = delete;
    directory_range(directory_stream &s): p_stream(&s) {}
    directory_range(directory_range const &r): p_stream(r.p_stream) {}

    directory_range &operator=(directory_range const &r) {
        p_stream = r.p_stream;
        return *this;
    }

    bool empty() const {
        return p_stream->empty();
    }

    void pop_front() {
        p_stream->pop_front();
    }

    file_info front() const {
        return p_stream->front();
    }

    bool equals_front(directory_range const &s) const {
        return p_stream == s.p_stream;
    }

private:
    directory_stream *p_stream;
};

inline directory_range directory_stream::iter() {
    return directory_range(*this);
}

namespace detail {
    template<size_t I>
    struct path_join {
        template<typename T, typename ...A>
        static void join(std::string &s, T const &a, A const &...b) {
            s += a;
            s += PathSeparator;
            path_join<I - 1>::join(s, b...);
        }
    };

    template<>
    struct path_join<1> {
        template<typename T>
        static void join(std::string &s, T const &a) {
            s += a;
        }
    };
}

template<typename ...A>
inline file_info path_join(A const &...args) {
    std::string path;
    detail::path_join<sizeof...(A)>::join(path, args...);
    path_normalize(ostd::iter(path));
    return file_info(path);
}

inline bool directory_change(string_range path) {
    char buf[1024];
    if (path.size() >= 1024) {
        return false;
    }
    memcpy(buf, path.data(), path.size());
    buf[path.size()] = '\0';
#ifndef OSTD_PLATFORM_WIN32
    return !chdir(buf);
#else
    return !_chdir(buf);
#endif
}

} /* namespace ostd */

#endif
