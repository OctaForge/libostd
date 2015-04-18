/* Algorithms for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ALGORITHM_H
#define OCTA_ALGORITHM_H

#include "octa/functional.h"
#include "octa/range.h"

namespace octa {
    template<typename R, typename U>
    R partition(R range, U pred) {
        R ret = range;
        for (; !range.empty(); range.pop_first()) {
            if (pred(range.first())) {
                swap(range.first(), ret.first());
                ret.pop_first();
            }
        }
        return ret;
    }

    template<typename R, typename C>
    void insertion_sort(R range, C compare) {
        size_t rlen = range.length();
        for (size_t i = 1; i < rlen; ++i) {
            size_t j = i, v = range[i];
            while (j > 0 && !compare(range[j - 1], v)) {
                range[j] = range[j - 1];
                --j;
            }
            range[j] = v;
        }
    }

    template<typename R>
    void insertion_sort(R range) {
        insertion_sort(range, Less<typename RangeTraits<R>::value>());
    }

    template<typename R, typename C>
    void quicksort(R range, C compare) {
        if (range.length() <= 10) {
            insertion_sort(range, compare);
            return;
        }
        typename RangeTraits<R>::reference p = range[range.length() / 2];
        swap(p, range.last());
        R r = partition(range, [p, compare](decltype(p) v) {
            return compare(v, p);
        });
        R l = range.slice(0, range.length() - r.length());
        swap(r.first(), r.last());
        quicksort(l, compare);
        quicksort(r, compare);
    }

    template<typename R>
    void quicksort(R range) {
        quicksort(range, Less<typename RangeTraits<R>::value>());
    }

    template<typename R, typename C>
    void sort(R range, C compare) {
        quicksort(range, compare);
    }

    template<typename R>
    void sort(R range) {
        sort(range, Less<typename RangeTraits<R>::value>());
    }
}

#endif