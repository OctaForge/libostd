/* Concurrency C implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/concurrency.hh"

namespace ostd {

/* place the vtable in here */
coroutine_error::~coroutine_error() {}

namespace detail {
    /* place the vtable in here */
    stack_free_iface::~stack_free_iface() {}
} /* namespace detail */

/* non-inline for vtable placement */
coroutine_context::~coroutine_context() {
    unwind();
    free_stack();
}

namespace detail {
    /* place the vtable here, derived from coroutine_context */
    csched_task::~csched_task() {}

    OSTD_EXPORT scheduler *current_scheduler = nullptr;
    OSTD_EXPORT thread_local csched_task *current_csched_task = nullptr;
} /* namespace detail */

scheduler::~scheduler() {}

} /* namespace ostd */
