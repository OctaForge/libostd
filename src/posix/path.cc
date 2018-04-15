/* Path implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>

#include "ostd/path.hh"

namespace ostd {
namespace fs {
namespace detail {

static void dir_read_next(DIR *dh, directory_entry &cur, path const &base) {
    struct dirent d;
    struct dirent *o;
    for (;;) {
        if (readdir_r(dh, &d, &o)) {
            /* FIXME: throw */
            abort();
        }
        if (!o) {
            cur.clear();
            return;
        }
        string_range nm{static_cast<char const *>(o->d_name)};
        if ((nm == ".") || (nm == "..")) {
            continue;
        }
        path p{base};
        p.append(nm);
        cur.assign(std::move(p));
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
    dir_read_next(static_cast<DIR *>(p_handle), p_current, p_dir);
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
    struct stat sb;
    if (stat(p_current.path().string().data(), &sb) < 0) {
        /* FIXME: throw */
        abort();
    }
    if (S_ISDIR(sb.st_mode)) {
        /* directory, recurse into it and if it contains stuff, return */
        DIR *nd = opendir(p_current.path().string().data());
        if (!nd) {
            abort();
        }
        directory_entry based = p_current, curd;
        dir_read_next(nd, curd, based);
        if (!curd.path().empty()) {
            p_dir = based;
            p_handles.push(nd);
            p_current = curd;
            return;
        } else {
            closedir(nd);
        }
    }
    /* didn't recurse into a directory, go to next file */
    dir_read_next(static_cast<DIR *>(p_handles.top()), p_current, p_dir);
    /* end of dir, pop while at it */
    if (p_current.path().empty()) {
        closedir(static_cast<DIR *>(p_handles.top()));
        p_handles.pop();
        if (!p_handles.empty()) {
            /* got back to another dir, read next so it's valid */
            p_dir.remove_name();
            dir_read_next(static_cast<DIR *>(p_handles.top()), p_current, p_dir);
        } else {
            p_current.clear();
        }
    }
}

} /* namespace detail */
} /* namesapce fs */
} /* namespace ostd */
