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
    char const *ret = std::getenv(std::string{name}.data());
    if (!ret) {
        return std::nullopt;
    }
    return std::string{ret};
}

OSTD_EXPORT bool env_set(string_range name, string_range value, bool update) {
#ifndef OSTD_PLATFORM_WIN32
    return !setenv(std::string{name}.data(), std::string{value}.data(), update);
#else
    std::string nstr{name};
    if (!update && GetEnvironmentVariable(nstr.data(), nullptr, 0)) {
        return true;
    }
    return !!SetEnvironmentVariable(nstr.data(), std::string{value}.data());
#endif
}

OSTD_EXPORT bool env_unset(string_range name) {
#ifndef OSTD_PLATFORM_WIN32
    return !unsetenv(std::string{name}.data());
#else
    return !!SetEnvironmentVariable(std::string{name}.data(), nullptr);
#endif
}

} /* namespace ostd */
