/* Default new/delete operator overloads for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.txt for futher information.
 */

#ifndef OCTASTD_NEW_H
#define OCTASTD_NEW_H

#include <stdlib.h>

inline void *operator new(size_t size) {
    void  *p = malloc(size);
    if   (!p) abort();
    return p;
}

inline void *operator new[](size_t size) {
    void  *p = malloc(size);
    if   (!p) abort();
    return p;
}

inline void operator delete(void *p) {
    free(p);
}

inline void operator delete[](void *p) {
    free(p);
}

inline void *operator new     (size_t, void *p) { return p; }
inline void *operator new   [](size_t, void *p) { return p; }
inline void  operator delete  (void *, void *)  {}
inline void  operator delete[](void *, void *)  {}

#endif