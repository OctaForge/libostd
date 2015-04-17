/* Utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_UTILITY_H
#define OCTA_UTILITY_H

#include <stddef.h>

#include "octa/traits.h"

/* must be in std namespace otherwise the compiler won't know about it */
namespace std {
    template<typename T>
    class initializer_list {
        const T *p_buf;
        size_t p_len;

        initializer_list(const T *v, size_t n): p_buf(v), p_len(n) {}
    public:
        struct type {
            typedef       T   value;
            typedef       T  &reference;
            typedef const T  &const_reference;
            typedef       T  *pointer;
            typedef const T  *const_pointer;
            typedef size_t    size;
            typedef ptrdiff_t difference;
        };

        initializer_list(): p_buf(nullptr), p_len(0) {}

        size_t length() const { return p_len; }

        const T *get() const { return p_buf; }
    };
}

namespace octa {
    template<typename T> using InitializerList = std::initializer_list<T>;

    template<typename T> void swap(T &a, T &b) {
        T c(move(a));
        a = move(b);
        b = move(c);
    }
    template<typename T, size_t N> void swap(T (&a)[N], T (&b)[N]) {
        for (size_t i = 0; i < N; ++i) {
            swap(a[i], b[i]);
        }
    }
}

#endif