/* Decides between POSIX and Windows for process.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#ifdef OSTD_PLATFORM_WIN32
#  include "src/win32/process.cc"
#else
#ifdef OSTD_PLATFORM_POSIX
#  include "src/posix/process.cc"
#else
#  error "Unsupported platform"
#endif
#endif
