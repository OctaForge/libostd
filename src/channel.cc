/* Channel implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/generic_condvar.hh"
#include "ostd/channel.hh"

namespace ostd {

/* place the vtable in here */
channel_error::~channel_error() {}

namespace detail {
    /* vtable placement for generic_condvar */
    cond_iface::~cond_iface() {}
} /* namespace detail */

} /* namespace ostd */
