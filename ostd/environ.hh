/** @defgroup Utilities
 *
 * @brief Miscellaneous utilities.
 *
 * @{
 */

/** @file environ.hh
 *
 * @brief A portable environment variable interface.
 *
 * Provides utility functions to portably get, set and unset environment
 * variables.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_ENVIRON_HH
#define OSTD_ENVIRON_HH

#include "ostd/platform.hh"
#include "ostd/internal/win32.hh"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <optional>

#include "ostd/string.hh"

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

/** @brief Gets an environment variable.
 *
 * @returns std::nullopt when the variable doesn't exist, and an std::string
 * containing the environment variable's value if it does.
 *
 * This function is thread-safe as long as the environment is not modified
 * within the program. Calling to lower level functions to set environment
 * vars as well as env_set() or env_unset() at the same time would introduce
 * potential data races.
 *
 * @see env_set(), env_unset()
 */
inline std::optional<std::string> env_get(string_range name) {
    char buf[256];
    auto tbuf = to_temp_cstr(name, buf, sizeof(buf));
    char const *ret = std::getenv(tbuf.get());
    if (!ret) {
        return std::nullopt;
    }
    return std::string{ret};
}

/** @brief Sets an environment variable.
 *
 * If `update` is false, the environment variable will not be overwritten
 * if it already exists. Keep in mind that true is still returned if the
 * variable already exists and it's not being updated.
 *
 * This function is not thread safe. Do not call it from multiple threads
 * and do not call it if a call to env_get() might be done from another
 * thread at the time.
 *
 * @returns true on success, false on failure.
 *
 * @see env_get(), env_unset()
 */
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

/** @brief Unsets an environment varible.
 *
 * This function is not thread safe. Do not call it from multiple threads
 * and do not call it if a call to env_get() might be done from another
 * thread at the time.
 *
 * @returns true on success, false on failure.
 *
 * @see env_get(), env_set()
 */
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

/** @} */

} /* namespace ostd */

#endif

/** @} */
