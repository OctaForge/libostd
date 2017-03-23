/* Concurrency C implementation bits.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#include "ostd/concurrency.hh"

namespace ostd {
namespace detail {

OSTD_EXPORT sched_iface current_sched_iface;
OSTD_EXPORT thread_local csched_task *current_csched_task = nullptr;

} /* namespace detail */
} /* namespace ostd */
