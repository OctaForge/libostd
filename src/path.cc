/* Decides between POSIX and Windows for path.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#if defined(OSTD_PLATFORM_WIN32)
#  include "src/win32/path.cc"
#elif defined(OSTD_PLATFORM_POSIX)
#  include "src/posix/path.cc"
#else
#  error "Unsupported platform"
#endif
