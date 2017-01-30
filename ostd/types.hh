/* Core type aliases for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_TYPES_HH
#define OSTD_TYPES_HH

#include <stdint.h>
#include <cstddef>

namespace ostd {

/* "builtin" types */

using sbyte = signed char;
using byte = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;

using ldouble = long double;

/* used occasionally for template variables */

namespace detail {
    template<typename> struct Undefined;
}

}

#endif
