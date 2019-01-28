/* Path implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

/* mainly only for glibc, but it's harmless elsewhere */

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

/* only because glibc is awful, does not apply to other libcs */
#define _FILE_OFFSET_BITS 64

#define _POSIX_C_SOURCE 200809L

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE 1
#endif

#include <cstdlib>
#include <ctime>
#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vector>
#include <stack>
#include <list>

#include "ostd/path.hh"

namespace ostd {
namespace fs {

static file_type mode_to_type(mode_t mode) {
    switch (mode & S_IFMT) {
        case S_IFBLK: return file_type::block;
        case S_IFCHR: return file_type::character;
        case S_IFIFO: return file_type::fifo;
        case S_IFREG: return file_type::regular;
        case S_IFDIR: return file_type::directory;
        case S_IFLNK: return file_type::symlink;
        case S_IFSOCK: return file_type::socket;
    }
    return file_type::unknown;
}

static std::error_code errno_ec() {
    return std::error_code{errno, std::system_category()};
}

static std::error_code ec_from_int(int v) {
    return std::error_code(v, std::system_category());
}

/* ugly test for whether nanosecond precision is available in stat
 * could check for existence of st_mtime macro, but this is more reliable
 */

template<typename T>
struct test_mtim {
    template<typename TT, TT> struct test_stat;

    struct fake_stat {
        struct timespec st_mtim;
    };

    struct stat_test: fake_stat, T {};

    template<typename TT>
    static char test(test_stat<struct timespec fake_stat::*, &TT::st_mtim> *);

    template<typename>
    static int test(...);

    static constexpr bool value = (sizeof(test<stat_test>(0)) == sizeof(int));
};

template<bool B>
struct mtime_impl {
    template<typename S>
    static file_time_t get(S const &st) {
        return std::chrono::system_clock::from_time_t(st.st_mtime);
    }
};

template<>
struct mtime_impl<true> {
    template<typename S>
    static file_time_t get(S const &st) {
        struct timespec ts = st.st_mtim;
        auto d = std::chrono::seconds{ts.tv_sec} +
                 std::chrono::nanoseconds{ts.tv_nsec};
        return file_time_t{
            std::chrono::duration_cast<std::chrono::system_clock::duration>(d)
        };
    }
};

using mtime = mtime_impl<test_mtim<struct stat>::value>;

static file_status status_get(path const &p, int ret, struct stat &sb) {
    if (ret < 0) {
        if (errno == ENOENT) {
            return file_status{
                file_mode{file_type::not_found, perms::none},
                file_time_t{}, 0, 0
            };
        }
        throw fs_error{"stat failure", p, errno_ec()};
    }
    return file_status{
        file_mode{mode_to_type(sb.st_mode), perms{int(sb.st_mode) & 07777}},
        mtime::get(sb),
        std::uintmax_t(sb.st_size),
        std::uintmax_t(sb.st_nlink)
    };
}

OSTD_EXPORT file_status status(path const &p) {
    struct stat sb;
    return status_get(p, stat(p.string().data(), &sb), sb);
}

OSTD_EXPORT file_status symlink_status(path const &p) {
    struct stat sb;
    return status_get(p, lstat(p.string().data(), &sb), sb);
}

OSTD_EXPORT file_mode mode(path const &p) {
    return status(p).mode();
}

OSTD_EXPORT file_mode symlink_mode(path const &p) {
    return symlink_status(p).mode();
}

OSTD_EXPORT file_time_t last_write_time(path const &p) {
    return status(p).last_write_time();
}

OSTD_EXPORT std::uintmax_t file_size(path const &p) {
    return status(p).size();
}

OSTD_EXPORT std::uintmax_t hard_link_count(path const &p) {
    return status(p).hard_link_count();
}

/* TODO: somehow feature-test for utimensat and fallback to utimes */
OSTD_EXPORT void last_write_time(path const &p, file_time_t new_time) {
    auto d = new_time.time_since_epoch();
    auto sec = std::chrono::floor<std::chrono::seconds>(d);
    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d - sec);
    struct timespec times[2] = {
        {0, UTIME_OMIT}, {time_t(sec.count()), long(nsec.count())}
    };
    if (utimensat(0, p.string().data(), times, 0)) {
        throw fs_error{"utimensat failure", p, errno_ec()};
    }
}

} /* namespace fs */
} /* namespace ostd */

namespace ostd {
namespace fs {
namespace detail {

static void dir_read_next(
    DIR *dh, path &cur, file_mode &tp, path const &base
) {
    struct dirent d;
    struct dirent *o;
    for (;;) {
        if (readdir_r(dh, &d, &o)) {
            throw fs_error{"readdir_r failure", base, errno_ec()};
        }
        if (!o) {
            cur = path{};
            return;
        }
        string_range nm{static_cast<char const *>(o->d_name)};
        if ((nm == ".") || (nm == "..")) {
            continue;
        }
        path p{base};
        p.append(nm);
#ifdef DT_UNKNOWN
        /* most systems have d_type */
        file_mode md{mode_to_type(DTTOIF(o->d_type))};
#else
        /* fallback mainly for legacy */
        file_mode md = symlink_mode(p);
#endif
        tp = md;
        cur = std::move(p);
        break;
    }
}

struct dir_range_impl {
    void open(path const &p) {
        DIR *d = opendir(p.string().data());
        if (!d) {
            throw fs_error{"opendir failure", p, errno_ec()};
        }
        p_dir = p;
        p_handle = d;
        read_next();
    }

    void close() noexcept {
        closedir(static_cast<DIR *>(p_handle));
    }

    void read_next() {
        path cur;
        file_mode tp;
        dir_read_next(static_cast<DIR *>(p_handle), cur, tp, p_dir);
        p_current = directory_entry{std::move(cur), tp};
    }

    bool empty() const noexcept {
        return p_current.path().empty();
    }

    directory_entry const &front() const noexcept {
        return p_current;
    }

    ~dir_range_impl() {
        close();
    }

    directory_entry p_current{};
    path p_dir{};
    void *p_handle = nullptr;
};

struct rdir_range_impl {
    using hstack = std::stack<void *, std::list<void *>>;

    void open(path const &p) {
        DIR *d = opendir(p.string().data());
        if (!d) {
            throw fs_error{"opendir failure", p, errno_ec()};
        }
        p_dir = p;
        p_handles.push(d);
        read_next();
    }

    void close() noexcept {
        while (!p_handles.empty()) {
            closedir(static_cast<DIR *>(p_handles.top()));
            p_handles.pop();
        }
    }

    void read_next() {
        if (p_handles.empty()) {
            return;
        }
        path curd;
        file_mode tp;
        /* can't reuse info from dirent because we need to expand symlinks */
        if (p_current.is_directory()) {
            /* directory, recurse into it and if it contains stuff, return */
            DIR *nd = opendir(p_current.path().string().data());
            if (!nd) {
                throw fs_error{"opendir failure", p_current, errno_ec()};
            }
            directory_entry based = p_current;
            dir_read_next(nd, curd, tp, based);
            if (!curd.empty()) {
                p_dir = based;
                p_handles.push(nd);
                p_current = directory_entry{std::move(curd), tp};
                return;
            } else {
                closedir(nd);
            }
        }
        /* didn't recurse into a directory, go to next file */
        dir_read_next(static_cast<DIR *>(p_handles.top()), curd, tp, p_dir);
        p_current = directory_entry{std::move(curd), tp};
        /* end of dir, pop while at it */
        if (p_current.path().empty()) {
            closedir(static_cast<DIR *>(p_handles.top()));
            p_handles.pop();
            if (!p_handles.empty()) {
                /* got back to another dir, read next so it's valid */
                p_dir.remove_name();
                dir_read_next(static_cast<DIR *>(p_handles.top()), curd, tp, p_dir);
                p_current = directory_entry{std::move(curd), tp};
            }
        }
    }

    bool empty() const noexcept {
        return p_current.path().empty();
    }

    directory_entry const &front() const noexcept {
        return p_current;
    }

    ~rdir_range_impl() {
        close();
    }

    directory_entry p_current{};
    path p_dir{};
    hstack p_handles{};
};

} /* namespace detail */
} /* namesapce fs */
} /* namespace ostd */

namespace ostd {
namespace fs {

OSTD_EXPORT directory_range::directory_range(path const &p):
    p_impl{std::make_shared<detail::dir_range_impl>()}
{
    p_impl->open(p);
}

OSTD_EXPORT bool directory_range::empty() const noexcept {
    return p_impl->empty();
}

OSTD_EXPORT void directory_range::pop_front() {
    p_impl->read_next();
}

OSTD_EXPORT directory_range::reference directory_range::front() const noexcept {
    return p_impl->front();
}

OSTD_EXPORT recursive_directory_range::recursive_directory_range(path const &p):
    p_impl{std::make_shared<detail::rdir_range_impl>()}
{
    p_impl->open(p);
}

OSTD_EXPORT bool recursive_directory_range::empty() const noexcept {
    return p_impl->empty();
}

OSTD_EXPORT void recursive_directory_range::pop_front() {
    p_impl->read_next();
}

OSTD_EXPORT recursive_directory_range::reference
recursive_directory_range::front() const noexcept {
    return p_impl->front();
}

} /* namespace fs */
} /* namespace ostd */

namespace ostd {
namespace fs {

OSTD_EXPORT path current_path() {
    auto pmax = pathconf(".", _PC_PATH_MAX);
    std::vector<char> rbuf;
    if (pmax > 0) {
        rbuf.reserve((pmax > 1024) ? 1024 : pmax);
    }
    for (;;) {
        auto p = getcwd(rbuf.data(), rbuf.capacity());
        if (!p) {
            if (errno != ERANGE) {
                throw fs_error{"getcwd failure", errno_ec()};
            }
            rbuf.reserve(rbuf.capacity() * 2);
            continue;
        }
        break;
    }
    return path{string_range{rbuf.data()}};
}

OSTD_EXPORT path home_path() {
    char const *hdir = getenv("HOME");
    if (hdir) {
        return path{string_range{hdir}};
    }
    struct passwd pwd;
    struct passwd *ret;
    std::vector<char> buf;
    long bufs = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufs < 0) {
        bufs = 2048;
    }
    buf.reserve(bufs);
    getpwuid_r(getuid(), &pwd, buf.data(), buf.capacity(), &ret);
    if (!ret) {
        throw fs_error{"getpwuid_r failure", errno_ec()};
    }
    return path{string_range{pwd.pw_dir}};
}

OSTD_EXPORT path temp_path() {
    char const *envs[] = { "TMPDIR", "TMP", "TEMP", "TEMPDIR", NULL };
    for (char const **p = envs; *p; ++p) {
        char const *tdir = getenv(*p);
        if (tdir) {
            return path{string_range{tdir}};
        }
    }
    return path{"/tmp"};
}

OSTD_EXPORT void current_path(path const &p) {
    if (chdir(p.string().data())) {
        throw fs_error{"chdir failure", p, errno_ec()};
    }
}

OSTD_EXPORT path absolute(path const &p) {
    if (p.is_absolute()) {
        return p;
    }
    return current_path() / p;
}

OSTD_EXPORT path canonical(path const &p) {
    /* TODO: legacy system support */
    char *rp = realpath(p.string().data(), nullptr);
    if (!rp) {
        throw fs_error{"realpath failure", p, errno_ec()};
    }
    path ret{string_range{rp}};
    free(rp);
    return ret;
}

static inline bool try_access(path const &p) {
    bool ret = !access(p.string().data(), F_OK);
    if (!ret && (errno != ENOENT)) {
        throw fs_error{"access failure", p, errno_ec()};
    }
    return ret;
}

OSTD_EXPORT path weakly_canonical(path const &p) {
    if (try_access(p)) {
        return canonical(p);
    }
    path cp{p};
    for (;;) {
        if (!cp.has_name()) {
            /* went all the way back to root */
            return p;
        }
        cp.remove_name();
        if (try_access(cp)) {
            break;
        }
    }
    /* cp refers to an existing section, canonicalize */
    path ret = canonical(cp);
    string_range pstr = p;
    /*a append the unresolved rest */
    ret.append(pstr.slice(cp.string().size(), pstr.size()));
    return ret;
}

OSTD_EXPORT path relative(path const &p, path const &base) {
    return weakly_canonical(p).relative_to(weakly_canonical(base));
}

OSTD_EXPORT bool exists(path const &p) {
    return try_access(p);
}

OSTD_EXPORT bool equivalent(path const &p1, path const &p2) {
    struct stat sb;
    if (stat(p1.string().data(), &sb) < 0) {
        throw fs_error{"stat failure", p1, p2, errno_ec()};
    }
    auto stdev = sb.st_dev;
    auto stino = sb.st_ino;
    if (stat(p2.string().data(), &sb) < 0) {
        throw fs_error{"stat failure", p1, p2, errno_ec()};
    }
    return ((sb.st_dev == stdev) && (sb.st_ino == stino));
}

static bool mkdir_p(path const &p, mode_t mode) {
    if (mkdir(p.string().data(), mode)) {
        if (errno != EEXIST) {
            throw fs_error{"mkdir failure", p, errno_ec()};
        }
        int eno = errno;
        auto tp = fs::mode(p);
        if (tp.type() != file_type::directory) {
            throw fs_error{"mkdir failure", p, ec_from_int(eno)};
            abort();
        }
        return false;
    }
    return true;
}

OSTD_EXPORT bool create_directory(path const &p) {
    return mkdir_p(p, 0777);
}

OSTD_EXPORT bool create_directory(path const &p, path const &ep) {
    return mkdir_p(p, mode_t(fs::mode(ep).permissions()));
}

OSTD_EXPORT bool create_directories(path const &p) {
    if (p.has_parent()) {
        create_directories(p.parent());
    }
    return create_directory(p);
}

OSTD_EXPORT bool remove(path const &p) {
    if (!exists(p)) {
        return false;
    }
    if (::remove(p.string().data())) {
        throw fs_error{"remove failure", p, errno_ec()};
    }
    return true;
}

OSTD_EXPORT std::uintmax_t remove_all(path const &p) {
    std::uintmax_t ret = 0;
    if (is_directory(p)) {
        fs::directory_range ds{p};
        for (auto &v: ds) {
            ret += remove_all(v.path());
        }
    }
    ret += remove(p);
    return ret;
}

OSTD_EXPORT void rename(path const &op, path const &np) {
    if (::rename(op.string().data(), np.string().data())) {
        throw fs_error{"rename failure", op, np, errno_ec()};
    }
}

} /* namespace fs */
} /* namespace ostd */
