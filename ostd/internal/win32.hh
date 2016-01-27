/* Windows includes.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_WIN32_HH
#define OSTD_INTERNAL_WIN32_HH

#include "ostd/platform.hh"

#ifdef OSTD_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#endif