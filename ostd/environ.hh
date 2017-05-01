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
#include "ostd/string.hh"

#include <optional>
#include <string>

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
OSTD_EXPORT std::optional<std::string> env_get(string_range name);

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
OSTD_EXPORT bool env_set(string_range name, string_range value, bool update = true);

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
OSTD_EXPORT bool env_unset(string_range name);

/** @} */

} /* namespace ostd */

#endif

/** @} */
