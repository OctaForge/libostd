/* Environment handling.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_ENVIRON_HH
#define OSTD_ENVIRON_HH

#include <stdlib.h>
#include <string.h>

#include "ostd/maybe.hh"
#include "ostd/string.hh"

namespace ostd {
namespace environ {

inline Maybe<ConstCharRange> get(ConstCharRange name) {
    char buf[256];
    const char *ret = getenv(to_temp_cstr(name, buf, sizeof(buf)).get());
    if (!ret) return ostd::nothing;
    return ostd::move(ConstCharRange(ret));
}

inline bool set(ConstCharRange name, ConstCharRange value,
                bool update = true) {
    char sbuf[2048];
    char *buf = sbuf;
    bool alloc = (name.size() + value.size() + 2) > sizeof(sbuf);
    if (alloc)
        buf = new char[name.size() + value.size() + 2];
    memcpy(buf, name.data(), name.size());
    buf[name.size()] = '\0';
    memcpy(&buf[name.size() + 1], value.data(), value.size());
    buf[name.size() + value.size() + 1] = '\0';
    bool ret = !setenv(buf, &buf[name.size() + 1], update);
    if (alloc)
        delete[] buf;
    return ret;
}

inline bool unset(ConstCharRange name) {
    char buf[256];
    if (name.size() < sizeof(buf)) {
        memcpy(buf, name.data(), name.size());
        buf[name.size()] = '\0';
        return !unsetenv(buf);
    }
    return !unsetenv(String(name).data());
}

} /* namespace environ */
} /* namespace ostd */

#endif
