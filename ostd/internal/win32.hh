/* Windows includes.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_WIN32_HH
#define OSTD_INTERNAL_WIN32_HH

#if defined(WIN32) || defined(_WIN32) || (defined(__WIN32) && !defined(__CYGWIN__))
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#endif
