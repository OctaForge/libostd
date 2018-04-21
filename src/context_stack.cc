/* Decides between POSIX and Windows for context_stack.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#if defined(OSTD_PLATFORM_WIN32)
#  include "src/win32/context_stack.cc"
#elif defined(OSTD_PLATFORM_POSIX)
#  include "src/posix/context_stack.cc"
#else
#  error "Unsupported platform"
#endif

namespace ostd {
struct coroutine_context;
namespace detail {
    OSTD_EXPORT thread_local coroutine_context *coro_current = nullptr;
} /* namespace detail */
} /* namespace ostd */
