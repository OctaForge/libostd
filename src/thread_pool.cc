/* Thread pool implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/thread_pool.hh"
#include "ostd/channel.hh"

namespace ostd {
namespace detail {

/* place the vtable here */
tpool_func_base::~tpool_func_base() {}

} /* namespace detail */
} /* namespace ostd */
