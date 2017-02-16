/* Environment handling.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_ENVIRON_HH
#define OSTD_ENVIRON_HH

#include "ostd/platform.hh"
#include "ostd/internal/win32.hh"

#include <stdlib.h>
#include <string.h>

#include <vector>
#include <optional>

#include "ostd/string.hh"

/* TODO: make POSIX version thread safe, the Windows version is... */

namespace ostd {

inline std::optional<std::string> env_get(string_range name) {
    char buf[256];
    auto tbuf = to_temp_cstr(name, buf, sizeof(buf));
#ifndef OSTD_PLATFORM_WIN32
    char const *ret = getenv(tbuf.get());
    if (!ret) {
        return std::nullopt;
    }
    return std::string{ret};
#else
    std::vector<char> rbuf;
    DWORD sz;
    for (;;) {
        sz = GetEnvironmentVariable(tbuf.get(), rbuf.data(), rbuf.capacity());
        if (!sz) {
            return std::nullopt;
        }
        if (sz < rbuf.capacity()) {
            break;
        }
        rbuf.reserve(sz);
    }
    return std::string{rbuf.data(), sz};
#endif
}

inline bool env_set(
    string_range name, string_range value, bool update = true
) {
    char sbuf[2048];
    char *buf = sbuf;
    bool alloc = (name.size() + value.size() + 2) > sizeof(sbuf);
    if (alloc) {
        buf = new char[name.size() + value.size() + 2];
    }
    memcpy(buf, name.data(), name.size());
    buf[name.size()] = '\0';
    memcpy(&buf[name.size() + 1], value.data(), value.size());
    buf[name.size() + value.size() + 1] = '\0';
#ifndef OSTD_PLATFORM_WIN32
    bool ret = !setenv(buf, &buf[name.size() + 1], update);
#else
    if (!update && GetEnvironmentVariable(buf, nullptr, 0)) {
        return true;
    }
    bool ret = !!SetEnvironmentVariable(buf, &buf[name.size() + 1]);
#endif
    if (alloc) {
        delete[] buf;
    }
    return ret;
}

inline bool env_unset(string_range name) {
    char buf[256];
    if (name.size() < sizeof(buf)) {
        memcpy(buf, name.data(), name.size());
        buf[name.size()] = '\0';
#ifndef OSTD_PLATFORM_WIN32
        return !unsetenv(buf);
#else
        return !!SetEnvironmentVariable(buf, nullptr);
#endif
    }
#ifndef OSTD_PLATFORM_WIN32
    return !unsetenv(std::string{name}.data());
#else
    return !!SetEnvironmentVariable(std::string{name}.data(), nullptr);
#endif
}

} /* namespace ostd */

#endif
