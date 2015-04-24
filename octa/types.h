/* Core typedefs for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TYPES_H
#define OCTA_TYPES_H

#include <stdint.h>
#include <stddef.h>

namespace octa {
    typedef signed char schar;
    typedef unsigned char uchar;
    typedef unsigned short ushort;
    typedef unsigned int  uint;
    typedef unsigned long ulong;
    typedef unsigned long long ullong;
    typedef long long llong;

    typedef long double ldouble;

    typedef decltype(nullptr) nullptr_t;

#if defined(__CLANG_MAX_ALIGN_T_DEFINED) || defined(_GCC_MAX_ALIGN_T)
    using ::max_align_t;
#else
    typedef long double max_align_t;
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