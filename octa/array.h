/* Static array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ARRAY_H
#define OCTA_ARRAY_H

#include <stddef.h>

#include "octa/algorithm.h"
#include "octa/range.h"
#include "octa/string.h"

namespace octa {

template<typename T, size_t N>
struct Array {
    using Size = size_t;
    using Difference = ptrdiff_t;
    using Value = T;
    using Reference = T &;
    using ConstReference = const T &;
    using Pointer = T *;
    using ConstPointer = const T *;
    using Range = PointerRange<T>;
    using ConstRange = PointerRange<const T>;

    T &operator[](size_t i) { return p_buf[i]; }
    const T &operator[](size_t i) const { return p_buf[i]; }

    T &at(size_t i) { return p_buf[i]; }
    const T &at(size_t i) const { return p_buf[i]; }

    T &front() { return p_buf[0]; }
    const T &front() const { return p_buf[0]; }

    T &back() { return p_buf[(N > 0) ? (N - 1) : 0]; }
    const T &back() const { return p_buf[(N > 0) ? (N - 1) : 0]; }

    size_t size() const { return N; }

    bool empty() const { return N == 0; }

    bool in_range(size_t idx) { return idx < N; }
    bool in_range(int idx) { return idx >= 0 && size_t(idx) < N; }
    bool in_range(const T *ptr) {
        return ptr >= &p_buf[0] && ptr < &p_buf[N];
    }

    T *data() { return p_buf; }
    const T *data() const { return p_buf; }

    Range each() {
        return octa::PointerRange<T>(p_buf, p_buf + N);
    }
    ConstRange each() const {
        return octa::PointerRange<const T>(p_buf, p_buf + N);
    }

    void swap(Array &v) {
        octa::swap_ranges(each(), v.each());
    }

    T p_buf[(N > 0) ? N : 1];
};

} /* namespace octa */

#endif