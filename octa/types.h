/* Core type aliases for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TYPES_H
#define OCTA_TYPES_H

#include <stdint.h>
#include <stddef.h>

namespace octa {

using schar = signed char;
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;

using ldouble = long double;

using Nullptr = decltype(nullptr);

#if defined(__CLANG_MAX_ALIGN_T_DEFINED) || defined(_GCC_MAX_ALIGN_T)
using MaxAlign = ::max_align_t;
#else
using MaxAlign = long double;
#endif

}

using octa::schar;
using octa::uchar;
using octa::ushort;
using octa::uint;
using octa::ulong;
using octa::ullong;
using octa::llong;
using octa::ldouble;

#endif