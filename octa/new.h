/* Default new/delete operator overloads for OctaSTD. Also has an impl file.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_NEW_H
#define OCTA_NEW_H

#include <stddef.h>

#ifndef OCTA_ALLOW_CXXSTD
inline void *operator new     (size_t, void *p) { return p; }
inline void *operator new   [](size_t, void *p) { return p; }
inline void  operator delete  (void *, void *)  {}
inline void  operator delete[](void *, void *)  {}
#endif

#endif