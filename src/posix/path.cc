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

OSTD_EXPORT void directory_range::open(path const &p) {
    DIR *d = opendir(p.string().data());
    if (!d) {
        /* FIXME: throw */
        abort();
    }
    p_dir = p;
    p_handle = d;
    p_owned = true;
    read_next();
}

OSTD_EXPORT void directory_range::close() noexcept {
    if (!p_owned) {
        return;
    }
    closedir(static_cast<DIR *>(p_handle));
    p_owned = false;
}

OSTD_EXPORT void directory_range::read_next() {
    dir_read_next(static_cast<DIR *>(p_handle), p_current, p_dir);
}

/* recursive dir range */

OSTD_EXPORT void recursive_directory_range::open(path const &p) {
    DIR *d = opendir(p.string().data());
    if (!d) {
        /* FIXME: throw */
        abort();
    }
    p_dir = p;
    p_stack = &p_handles;
    p_stack->push(d);
    p_owned = true;
    read_next();
}

OSTD_EXPORT void recursive_directory_range::close() noexcept {
    if (!p_owned) {
        return;
    }
    while (!p_handles.empty()) {
        closedir(static_cast<DIR *>(p_handles.top()));
        p_handles.pop();
    }
    p_owned = false;
}

OSTD_EXPORT void recursive_directory_range::read_next() {
    directory_entry curd;
    if (p_stack->empty()) {
        return;
    }
    dir_read_next(static_cast<DIR *>(p_stack->top()), curd, p_dir);
    struct stat sb;
    /* check if dir and recurse maybe */
    while (!curd.path().empty()) {
        if (stat(curd.path().string().data(), &sb) < 0) {
            /* FIXME: throw */
            abort();
        }
        if (S_ISDIR(sb.st_mode)) {
            /* directory, recurse into it */
            DIR *nd = opendir(curd.path().string().data());
            if (!nd) {
                abort();
            }
            p_dir = curd;
            p_stack->push(nd);
            dir_read_next(nd, curd, p_dir);
            p_current = curd;
            if (curd.path().empty()) {
                break;
            } else {
                return;
            }
        } else {
            /* not a dir, stick with it */
            p_current = curd;
            return;
        }
    }
    /* empty result - done with a dir, pop */
    closedir(static_cast<DIR *>(p_stack->top()));
    p_stack->pop();
    if (!p_stack->empty()) {
        p_current.assign(p_dir);
        p_dir.remove_name();
    } else {
        p_current.clear();
    }
}

} /* namesapce fs */
} /* namespace ostd */
