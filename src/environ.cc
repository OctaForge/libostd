/* Environment handling implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdlib>
#include <cstring>
#include <new>

#include "ostd/environ.hh"

#ifdef OSTD_PLATFORM_WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace ostd {

OSTD_EXPORT std::optional<std::string> env_get(string_range name) {
    char buf[256];
    auto tbuf = to_temp_cstr(name, buf, sizeof(buf));
    char const *ret = std::getenv(tbuf.get());
    if (!ret) {
        return std::nullopt;
    }
    return std::string{ret};
}

OSTD_EXPORT bool env_set(string_range name, string_range value, bool update) {
    char sbuf[2048];
    char *buf = sbuf;
    bool alloc = (name.size() + value.size() + 2) > sizeof(sbuf);
    if (alloc) {
        buf = new char[name.size() + value.size() + 2];
    }
    std::memcpy(buf, name.data(), name.size());
    buf[name.size()] = '\0';
    std::memcpy(&buf[name.size() + 1], value.data(), value.size());
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

OSTD_EXPORT bool env_unset(string_range name) {
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
