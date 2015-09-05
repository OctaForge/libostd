/* Filesystem API for OctaSTD. Currently POSIX only.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_FILESYSTEM_HH
#define OSTD_FILESYSTEM_HH

#include <dirent.h>
#include <sys/stat.h>

#include "ostd/types.hh"
#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/string.hh"
#include "ostd/array.hh"
#include "ostd/algorithm.hh"
#include "ostd/platform.hh"

namespace ostd {

enum class FileType {
    unknown, regular, fifo, character, directory, block, symlink, socket
};

struct FileInfo;

#ifdef OSTD_PLATFORM_WIN32
static constexpr char PATH_SEPARATOR = '\\';
#else
static constexpr char PATH_SEPARATOR = '/';
#endif

inline void path_normalize(CharRange) {
    /* TODO */
}

struct FileInfo {
    FileInfo() {}

    FileInfo(const FileInfo &i):
        p_slash(i.p_slash), p_dot(i.p_dot), p_type(i.p_type),
        p_path(i.p_path), p_atime(i.p_atime), p_mtime(i.p_mtime),
        p_ctime(i.p_ctime) {}

    FileInfo(FileInfo &&i):
        p_slash(i.p_slash), p_dot(i.p_dot), p_type(i.p_type),
        p_path(move(i.p_path)), p_atime(i.p_atime), p_mtime(i.p_mtime),
        p_ctime(i.p_ctime) {
        i.p_slash = i.p_dot = npos;
        i.p_type = FileType::unknown;
        i.p_atime = i.p_ctime = i.p_mtime = 0;
    }

    FileInfo(ConstCharRange path) {
        init_from_str(path);
    }

    FileInfo &operator=(const FileInfo &i) {
        p_slash = i.p_slash;
        p_dot = i.p_dot;
        p_type = i.p_type;
        p_path = i.p_path;
        p_atime = i.p_atime;
        p_mtime = i.p_mtime;
        p_ctime = i.p_ctime;
        return *this;
    }

    FileInfo &operator=(FileInfo &&i) {
        swap(i);
        return *this;
    }

    ConstCharRange path() const { return p_path.iter(); }

    ConstCharRange filename() const {
        return path().slice((p_slash == npos) ? 0 : (p_slash + 1),
                            p_path.size());
    }

    ConstCharRange stem() const {
        return path().slice((p_slash == npos) ? 0 : (p_slash + 1),
                            (p_dot == npos) ? p_path.size() : p_dot);
    }

    ConstCharRange extension() const {
        return (p_dot == npos) ? ConstCharRange()
                               : path().slice(p_dot, p_path.size());
    }

    FileType type() const { return p_type; }

    void normalize() {
        path_normalize(p_path.iter());
        init_from_str(p_path.iter());
    }

    time_t atime() const { return p_atime; }
    time_t mtime() const { return p_mtime; }
    time_t ctime() const { return p_ctime; }

    void swap(FileInfo &i) {
        detail::swap_adl(i.p_slash, p_slash);
        detail::swap_adl(i.p_dot, p_dot);
        detail::swap_adl(i.p_type, p_type);
        detail::swap_adl(i.p_path, p_path);
        detail::swap_adl(i.p_atime, p_atime);
        detail::swap_adl(i.p_mtime, p_mtime);
        detail::swap_adl(i.p_ctime, p_ctime);
    }

private:
    void init_from_str(ConstCharRange path) {
        struct stat st;
        if (stat(String(path).data(), &st) < 0) {
            p_slash = p_dot = npos;
            p_type = FileType::unknown;
            p_path.clear();
            p_atime = p_mtime = p_ctime = 0;
            return;
        }
        p_path = path;
        ConstCharRange r = p_path.iter();

        ConstCharRange found = find_last(r, '/');
        if (found.empty())
            p_slash = npos;
        else
            p_slash = r.distance_front(found);

        found = find(filename(), '.');
        if (found.empty())
            p_dot = npos;
        else
            p_dot = r.distance_front(found);

        if (S_ISREG(st.st_mode))
            p_type = FileType::regular;
        else if (S_ISDIR(st.st_mode))
            p_type = FileType::directory;
        else if (S_ISCHR(st.st_mode))
            p_type = FileType::character;
        else if (S_ISBLK(st.st_mode))
            p_type = FileType::block;
        else if (S_ISFIFO(st.st_mode))
            p_type = FileType::fifo;
        else if (S_ISLNK(st.st_mode))
            p_type = FileType::symlink;
        else if (S_ISSOCK(st.st_mode))
            p_type = FileType::socket;
        else
            p_type = FileType::unknown;

        p_atime = st.st_atime;
        p_mtime = st.st_mtime;
        p_ctime = st.st_ctime;
    }

    Size p_slash = npos, p_dot = npos;
    FileType p_type = FileType::unknown;
    String p_path;

    time_t p_atime = 0, p_mtime = 0, p_ctime = 0;
};

struct DirectoryRange;

struct DirectoryStream {
    friend struct DirectoryRange;

    DirectoryStream(): p_d(), p_path(), p_owned(false) {}
    DirectoryStream(const DirectoryStream &) = delete;
    DirectoryStream(DirectoryStream &&s): p_d(s.p_d), p_path(move(s.p_path)),
                                          p_owned(s.p_owned) {
        s.p_d = nullptr;
        s.p_owned = false;
    }

    DirectoryStream(ConstCharRange path): p_d() {
        open(path);
    }

    ~DirectoryStream() { close(); }

    DirectoryStream &operator=(const DirectoryStream &) = delete;
    DirectoryStream &operator=(DirectoryStream &&s) {
        close();
        swap(s);
        return *this;
    }

    bool open(ConstCharRange path) {
        if (p_d || (path.size() > FILENAME_MAX)) return false;
        char buf[FILENAME_MAX + 1];
        memcpy(buf, &path[0], path.size());
        buf[path.size()] = '\0';
        p_d = opendir(buf);
        p_path = path;
        p_owned = true;
        return is_open();
    }

    bool is_open() const { return p_d != nullptr; }
    bool is_owned() const { return p_owned; }

    void close() {
        if (p_d && p_owned) closedir(p_d);
        p_d = nullptr;
        p_owned = false;
    }

    long size() const {
        if (!p_d) return -1;
        long cs = telldir(p_d);
        if (cs < 0) return cs;
        seekdir(p_d, 0);
        long ret = 0;
        struct dirent *rd = nullptr;
        while ((rd = readdir(p_d)))
            ret += (strcmp(rd->d_name, ".") && strcmp(rd->d_name, ".."));
        seekdir(p_d, cs);
        return ret;
    }

    bool rewind() {
        if (!p_d) return false;
        rewinddir(p_d);
        return true;
    }

    FileInfo read() {
        if (!p_d) return FileInfo();
        auto rd = readdir(p_d);
        if (!rd) return FileInfo();
        if (!strcmp(rd->d_name, ".") || !strcmp(rd->d_name, ".."))
            return read();
        String ap = p_path;
        ap += PATH_SEPARATOR;
        ap += (const char *)rd->d_name;
        return FileInfo(ap);
    }

    void swap(DirectoryStream &s) {
        detail::swap_adl(p_d, s.p_d);
        detail::swap_adl(p_path, s.p_path);
        detail::swap_adl(p_owned, s.p_owned);
    }

    DirectoryRange iter();

private:
    bool compare(const DirectoryStream &ds) {
        if (!p_d) return !ds.p_d;
        return (p_d == ds.p_d) && (telldir(p_d) == telldir(ds.p_d));
    }

    bool empty(long n) const {
        return !p_d || (telldir(p_d) >= n);
    }

    bool pop_front() const {
        if (!p_d) return false;
        long cs = telldir(p_d);
        if (cs < 0) return false;
        seekdir(p_d, cs + 1);
        return telldir(p_d) == (cs + 1);
    }

    void push_front() const {
        if (!p_d) return;
        long cs = telldir(p_d);
        if (cs < 0) return;
        seekdir(p_d, cs - 1);
    }

    DIR *p_d;
    String p_path;
    bool p_owned;
};

struct DirectoryRange: InputRange<
    DirectoryRange, InputRangeTag, FileInfo, FileInfo, Size, long
> {
    DirectoryRange() = delete;
    DirectoryRange(DirectoryStream &s): p_stream(&s), p_ssize(s.size()) {}
    DirectoryRange(const DirectoryRange &r):
        p_stream(r.p_stream), p_ssize(r.p_ssize) {}

    DirectoryRange &operator=(const DirectoryRange &r) {
        p_stream = r.p_stream;
        p_ssize = r.p_ssize;
        return *this;
    }

    bool empty() const {
        return p_stream->empty(p_ssize);
    }

    bool pop_front() {
        return p_stream->pop_front();
    }

    FileInfo front() const {
        FileInfo ret(p_stream->read());
        p_stream->push_front();
        return ret;
    }

    bool equals_front(const DirectoryRange &s) const {
        if (!p_stream) return !s.p_stream;
        return p_stream->compare(*s.p_stream);
    }

private:
    DirectoryStream *p_stream;
    long p_ssize;
};

DirectoryRange DirectoryStream::iter() {
    return DirectoryRange(*this);
}

namespace detail {
    template<Size I> struct PathJoin {
        template<typename T, typename ...A>
        static void join(String &s, const T &a, const A &...b) {
            s += a;
            s += PATH_SEPARATOR;
            PathJoin<I - 1>::join(s, b...);
        }
    };

    template<> struct PathJoin<1> {
        template<typename T>
        static void join(String &s, const T &a) {
            s += a;
        }
    };
}

template<typename ...A>
inline FileInfo path_join(const A &...args) {
    String path;
    detail::PathJoin<sizeof...(A)>::join(path, args...);
    path_normalize(path.iter());
    return FileInfo(path);
}

} /* namespace ostd */

#endif