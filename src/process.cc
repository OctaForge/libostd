/* Decides between POSIX and Windows for process.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#if defined(OSTD_PLATFORM_WIN32)
#  include "src/win32/process.cc"
#elif defined(OSTD_PLATFORM_POSIX)
#  include "src/posix/process.cc"
#else
#  error "Unsupported platform"
#endif

namespace ostd {

/* place the vtable in here */
word_error::~word_error() {}
subprocess_error::~subprocess_error() {}

} /* namespace ostd */
