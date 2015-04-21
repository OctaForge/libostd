/* Static array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ARRAY_H
#define OCTA_ARRAY_H

#include <stddef.h>

#include "octa/range.h"

namespace octa {
    template<typename T, size_t N>
    struct Array {
        struct type {
            typedef       T                value;
            typedef       T               &reference;
            typedef const T               &const_reference;
            typedef       T               *pointer;
            typedef const T               *const_pointer;
            typedef size_t                 size;
            typedef ptrdiff_t              difference;
            typedef PointerRange<      T>  range;
            typedef PointerRange<const T>  const_range;
        };

        T &operator[](size_t i) { return p_buf[i]; }
        const T &operator[](size_t i) const { return p_buf[i]; }

        T &at(size_t i) { return p_buf[i]; }
        const T &at(size_t i) const { return p_buf[i]; }

        T &first() { return p_buf[0]; }
        const T &first() const { return p_buf[0]; }

        T &last() { return p_buf[N - 1]; }
        const T &last() const { return p_buf[N - 1]; }

        bool empty() const { return (N > 0); }
        size_t length() const { return N; }

        T *get() { return p_buf; }
        const T *get() const { return p_buf; }

        void swap(Array &v) {
            swap(p_buf, v.p_buf);
        }

        typename type::range each() {
            return PointerRange<T>(p_buf, p_buf + N);
        }
        typename type::const_range each() const {
            return PointerRange<const T>(p_buf, p_buf + N);
        }

        T p_buf[N];
    };

    template<typename T, size_t N>
    void swap(Array<T, N> &a, Array<T, N> &b) {
        a.swap(b);
    }
}

#endif