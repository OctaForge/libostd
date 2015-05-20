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
        typedef size_t                 SizeType;
        typedef ptrdiff_t              DiffType;
        typedef       T                ValType;
        typedef       T               &RefType;
        typedef const T               &ConstRefType;
        typedef       T               *PtrType;
        typedef const T               *ConstPtrType;
        typedef PointerRange<      T>  RangeType;
        typedef PointerRange<const T>  ConstRangeType;

        T &operator[](size_t i) { return p_buf[i]; }
        const T &operator[](size_t i) const { return p_buf[i]; }

        T &at(size_t i) { return p_buf[i]; }
        const T &at(size_t i) const { return p_buf[i]; }

        T &first() { return p_buf[0]; }
        const T &first() const { return p_buf[0]; }

        T &last() { return p_buf[N - 1]; }
        const T &last() const { return p_buf[N - 1]; }

        size_t length() const { return N; }

        bool empty() const { return N == 0; }

        bool in_range(size_t idx) { return idx < N; }
        bool in_range(int idx) { return idx >= 0 && idx < N; }
        bool in_range(const T *ptr) {
            return ptr >= &p_buf[0] && ptr < &p_buf[N];
        }

        T *get() { return p_buf; }
        const T *get() const { return p_buf; }

        void swap(Array &v)(swap(declval<T &>(), declval<T &>())) {
            swap(p_buf, v.p_buf);
        }

        RangeType each() {
            return PointerRange<T>(p_buf, p_buf + N);
        }
        ConstRangeType each() const {
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