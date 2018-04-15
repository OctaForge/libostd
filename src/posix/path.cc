/* Path implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdlib>
#include <dirent.h>

#include "ostd/path.hh"

namespace ostd {
namespace fs {

void directory_range::open(path const &p) {
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

void directory_range::close() noexcept {
    if (!p_owned) {
        return;
    }
    closedir(static_cast<DIR *>(p_handle));
    p_owned = false;
}

void directory_range::read_next() {
    struct dirent d;
    struct dirent *o;
    DIR *dh = static_cast<DIR *>(p_handle);
    for (;;) {
        if (readdir_r(dh, &d, &o)) {
            /* FIXME: throw */
            abort();
        }
        if (!o) {
            p_current.clear();
            return;
        }
        string_range nm{static_cast<char const *>(o->d_name)};
        if ((nm == ".") || (nm == "..")) {
            continue;
        }
        path p{p_dir};
        p.append(nm);
        p_current.assign(std::move(p));
        break;
    }
}

} /* namesapce fs */
} /* namespace ostd */
