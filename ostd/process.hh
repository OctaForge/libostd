/** @addtogroup Utilities
 * @{
 */

/** @file process.hh
 *
 * @brief Portable extensions to process handling.
 *
 * Provides POSIX and Windows abstractions for process creation and more.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_PROCESS_HH
#define OSTD_PROCESS_HH

#include <stdexcept>

#include "ostd/platform.hh"
#include "ostd/string.hh"

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

struct word_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {
    OSTD_EXPORT void split_args_impl(
        string_range const &str, void (*func)(string_range, void *), void *data
    );
}

/** @brief Splits command line argument string into individual arguments.
 *
 * The split is done in a platform specific manner, using wordexp on POSIX
 * systems and CommandLineToArgvW on Windows. On Windows, the input string
 * is assumed to be UTF-8 and the output strings are always UTF-8, on POSIX
 * the wordexp implementation is followed (so it's locale specific).
 *
 * The `out` argument is an output range that takes a single argument at
 * a time. The value type is any that can be explicitly constructed from
 * an ostd::string_range. However, the string range that is used internally
 * during the conversions is just temporary and freed at the end of this
 * function, so it's important that the string type holds its own memory;
 * an std::string will usually suffice.
 *
 * The ostd::word_error exception is used to handle failures of this
 * function itself. It may also throw other exceptions though, particularly
 * those thrown by `out` when putting the strings into it and also
 * std::bad_alloc on allocation failures.
 *
 * @returns The forwarded `out`.
 *
 * @throws ostd::word_error on failure or anything thrown by `out`.
 * @throws std::bad_alloc on alloation failures.
 */
template<typename OutputRange>
OutputRange &&split_args(OutputRange &&out, string_range str) {
    detail::split_args_impl(str, [](string_range val, void *outp) {
        static_cast<std::decay_t<OutputRange> *>(outp)->put(
            range_value_t<std::decay_t<OutputRange>>{val}
        );
    }, &out);
    return std::forward<OutputRange>(out);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
