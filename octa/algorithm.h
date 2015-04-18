/* Algorithms for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ALGORITHM_H
#define OCTA_ALGORITHM_H

#include "math.h"

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

    namespace internal {
        template<typename T, typename U>
        struct UnaryCompare {
            const T &val;
            U comp;
            bool operator()(const T &v) const { return comp(v, val); }
        };

        template<typename R, typename C>
        void hs_sift_down(R range, size_t s, size_t e, C compare) {
            size_t r = s;
            while ((r * 2 + 1) <= e) {
                size_t ch = r * 2 + 1;
                size_t sw = r;
                if (compare(range[sw], range[ch]))
                    sw = ch;
                if (((ch + 1) <= e) && compare(range[sw], range[ch + 1]))
                    sw = ch + 1;
                if (sw != r) {
                    swap(range[r], range[sw]);
                    r = sw;
                } else return;
            }
        }

        template<typename R, typename C>
        void heapsort(R range, C compare) {
            size_t len = range.length();
            size_t st = (len - 2) / 2;
            for (;;) {
                hs_sift_down(range, st, len - 1, compare);
                if (st-- == 0) break;
            }
            size_t e = len - 1;
            while (e > 0) {
                swap(range[e], range[0]);
                --e;
                hs_sift_down(range, 0, e, compare);
            }
        }

        template<typename R, typename C>
        void introloop(R range, C compare, size_t depth) {
            if (range.length() <= 10) {
                insertion_sort(range, compare);
                return;
            }
            if (depth == 0) {
                heapsort(range, compare);
                return;
            }
            typename RangeTraits<R>::reference p = range[range.length() / 2];
            swap(p, range.last());
            R r = partition(range, UnaryCompare<decltype(p), C>{ p, compare });
            R l = range.slice(0, range.length() - r.length());
            swap(r.first(), r.last());
            introloop(l, compare, depth - 1);
            introloop(r, compare, depth - 1);
        }

        template<typename R, typename C>
        void introsort(R range, C compare) {
            introloop(range, compare, size_t(2
                * (log(range.length()) / log(2))));
        }
    }

    template<typename R, typename C>
    void sort(R range, C compare) {
        internal::introsort(range, compare);
    }

    template<typename R>
    void sort(R range) {
        sort(range, Less<typename RangeTraits<R>::value>());
    }
}

#endif