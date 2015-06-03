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
    template<typename _T, size_t _N>
    struct Array {
        typedef size_t                  Size;
        typedef ptrdiff_t               Difference;
        typedef       _T                Value;
        typedef       _T               &Reference;
        typedef const _T               &ConstReference;
        typedef       _T               *Pointer;
        typedef const _T               *ConstPointer;
        typedef PointerRange<      _T>  Range;
        typedef PointerRange<const _T>  ConstRange;

        _T &operator[](size_t __i) { return __buf[__i]; }
        const _T &operator[](size_t __i) const { return __buf[__i]; }

        _T &at(size_t __i) { return __buf[__i]; }
        const _T &at(size_t __i) const { return __buf[__i]; }

        _T &front() { return __buf[0]; }
        const _T &front() const { return __buf[0]; }

        _T &back() { return __buf[(_N > 0) ? (_N - 1) : 0]; }
        const _T &back() const { return __buf[(_N > 0) ? (_N - 1) : 0]; }

        size_t size() const { return _N; }

        bool empty() const { return _N == 0; }

        bool in_range(size_t __idx) { return __idx < _N; }
        bool in_range(int __idx) { return __idx >= 0 && size_t(__idx) < _N; }
        bool in_range(const _T *__ptr) {
            return __ptr >= &__buf[0] && __ptr < &__buf[_N];
        }

        _T *data() { return __buf; }
        const _T *data() const { return __buf; }

        Range each() {
            return octa::PointerRange<_T>(__buf, __buf + _N);
        }
        ConstRange each() const {
            return octa::PointerRange<const _T>(__buf, __buf + _N);
        }

        void swap(Array &__v) {
            octa::swap_ranges(each(), __v.each());
        }

        _T __buf[(_N > 0) ? _N : 1];
    };
}

#endif