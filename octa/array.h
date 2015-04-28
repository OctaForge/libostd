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
        typedef size_t                 size_type;
        typedef ptrdiff_t              difference_type;
        typedef       T                value_type;
        typedef       T               &reference;
        typedef const T               &const_reference;
        typedef       T               *pointer;
        typedef const T               *const_pointer;
        typedef PointerRange<      T>  range;
        typedef PointerRange<const T>  const_range;

        T &operator[](size_t i) noexcept { return p_buf[i]; }
        const T &operator[](size_t i) const noexcept { return p_buf[i]; }

        T &at(size_t i) noexcept { return p_buf[i]; }
        const T &at(size_t i) const noexcept { return p_buf[i]; }

        T &first() noexcept { return p_buf[0]; }
        const T &first() const noexcept { return p_buf[0]; }

        T &last() noexcept { return p_buf[N - 1]; }
        const T &last() const noexcept { return p_buf[N - 1]; }

        bool empty() const noexcept { return (N > 0); }
        size_t length() const noexcept { return N; }

        T *get() noexcept { return p_buf; }
        const T *get() const noexcept { return p_buf; }

        void swap(Array &v) noexcept {
            swap(p_buf, v.p_buf);
        }

        range each() noexcept {
            return PointerRange<T>(p_buf, p_buf + N);
        }
        const_range each() const noexcept {
            return PointerRange<const T>(p_buf, p_buf + N);
        }

        T p_buf[N];
    };

    template<typename T, size_t N>
    void swap(Array<T, N> &a, Array<T, N> &b) noexcept {
        a.swap(b);
    }
}

#endif