/* Path implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdlib>
#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vector>

#include "ostd/path.hh"

namespace ostd {
namespace fs {

static perms mode_to_perms(mode_t mode) {
    perms ret = perms::none;
    switch (mode & S_IRWXU) {
        case S_IRUSR: ret |= perms::owner_read;
        case S_IWUSR: ret |= perms::owner_write;
        case S_IXUSR: ret |= perms::owner_exec;
        case S_IRWXU: ret |= perms::owner_all;
    }
    switch (mode & S_IRWXG) {
        case S_IRGRP: ret |= perms::group_read;
        case S_IWGRP: ret |= perms::group_write;
        case S_IXGRP: ret |= perms::group_exec;
        case S_IRWXG: ret |= perms::group_all;
    }
    switch (mode & S_IRWXO) {
        case S_IROTH: ret |= perms::others_read;
        case S_IWOTH: ret |= perms::others_write;
        case S_IXOTH: ret |= perms::others_exec;
        case S_IRWXO: ret |= perms::others_all;
    }
    if (mode & S_ISUID) {
        ret |= perms::set_uid;
    }
    if (mode & S_ISGID) {
        ret |= perms::set_gid;
    }
    if (mode & S_ISVTX) {
        ret |= perms::sticky_bit;
    }
    return ret;
}

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

OSTD_EXPORT file_mode mode(path const &p) {
    struct stat sb;
    if (stat(p.string().data(), &sb) < 0) {
        if (errno == ENOENT) {
            return file_mode{file_type::not_found, perms::none};
        }
        /* FIXME: throw */
        abort();
    }
    return file_mode{mode_to_type(sb.st_mode), mode_to_perms(sb.st_mode)};
}

OSTD_EXPORT file_mode symlink_mode(path const &p) {
    struct stat sb;
    if (lstat(p.string().data(), &sb) < 0) {
        if (errno == ENOENT) {
            return file_mode{file_type::not_found, perms::none};
        }
        /* FIXME: throw */
        abort();
    }
    return file_mode{mode_to_type(sb.st_mode), mode_to_perms(sb.st_mode)};
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
            /* FIXME: throw */
            abort();
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

/* dir range */

OSTD_EXPORT void dir_range_impl::open(path const &p) {
    DIR *d = opendir(p.string().data());
    if (!d) {
        /* FIXME: throw */
        abort();
    }
    p_dir = p;
    p_handle = d;
    read_next();
}

OSTD_EXPORT void dir_range_impl::close() noexcept {
    closedir(static_cast<DIR *>(p_handle));
}

OSTD_EXPORT void dir_range_impl::read_next() {
    path cur;
    file_mode tp;
    dir_read_next(static_cast<DIR *>(p_handle), cur, tp, p_dir);
    p_current = directory_entry{std::move(cur), tp};
}

/* recursive dir range */

OSTD_EXPORT void rdir_range_impl::open(path const &p) {
    DIR *d = opendir(p.string().data());
    if (!d) {
        /* FIXME: throw */
        abort();
    }
    p_dir = p;
    p_handles.push(d);
    read_next();
}

OSTD_EXPORT void rdir_range_impl::close() noexcept {
    while (!p_handles.empty()) {
        closedir(static_cast<DIR *>(p_handles.top()));
        p_handles.pop();
    }
}

OSTD_EXPORT void rdir_range_impl::read_next() {
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
            abort();
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

} /* namespace detail */
} /* namesapce fs */
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
                abort();
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
        abort();
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
        abort();
    }
    path ret{string_range{rp}};
    free(rp);
    return ret;
}

static inline bool try_access(path const &p) {
    bool ret = !access(p.string().data(), F_OK);
    if (!ret && (errno != ENOENT)) {
        abort();
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
    auto pstr = p.string();
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
        abort();
    }
    auto stdev = sb.st_dev;
    auto stino = sb.st_ino;
    if (stat(p2.string().data(), &sb) < 0) {
        abort();
    }
    return ((sb.st_dev == stdev) && (sb.st_ino == stino));
}

} /* namespace fs */
} /* namespace ostd */
