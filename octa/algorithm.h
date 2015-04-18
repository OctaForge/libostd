/* Algorithms for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ALGORITHM_H
#define OCTA_ALGORITHM_H

#include "octa/functional.h"
#include "octa/range.h"

namespace octa {
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
    void sort(R range, C compare) {
        insertion_sort(range, compare);
    }

    template<typename R>
    void sort(R range) {
        sort(range, Less<typename RangeTraits<R>::value>());
    }
}

#endif