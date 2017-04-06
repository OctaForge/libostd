/* Concurrency C implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/concurrency.hh"

namespace ostd {
namespace detail {

OSTD_EXPORT scheduler *current_scheduler = nullptr;
OSTD_EXPORT thread_local csched_task *current_csched_task = nullptr;

} /* namespace detail */
} /* namespace ostd */
