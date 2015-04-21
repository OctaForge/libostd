/* Algorithms for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ALGORITHM_H
#define OCTA_ALGORITHM_H

#include <math.h>

#include "octa/functional.h"
#include "octa/range.h"
#include "octa/utility.h"
#include "octa/initializer_list.h"

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

    template<typename R, typename P>
    bool is_partitioned(R range, P pred) {
        for (; !range.empty() && pred(range.first()); range.pop_first());
        for (; !range.empty(); range.pop_first())
            if (pred(range.first())) return false;
        return true;
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

    template<typename T>
    inline const T &min(const T &a, const T &b) {
        return (a < b) ? a : b;
    }
    template<typename T, typename C>
    inline const T &min(const T &a, const T &b, C compare) {
        return compare(a, b) ? a : b;
    }

    template<typename T>
    inline const T &max(const T &a, const T &b) {
        return (a < b) ? b : a;
    }
    template<typename T, typename C>
    inline const T &max(const T &a, const T &b, C compare) {
        return compare(a, b) ? b : a;
    }

    template<typename R>
    inline R min_element(R range) {
        R r = range;
        for (; !range.empty(); range.pop_first())
            if (min(r.first(), range.first()) == range.first())
                r = range;
        return r;
    }
    template<typename R, typename C>
    inline R min_element(R range, C compare) {
        R r = range;
        for (; !range.empty(); range.pop_first())
            if (min(r.first(), range.first(), compare) == range.first())
                r = range;
        return r;
    }

    template<typename R>
    inline R max_element(R range) {
        R r = range;
        for (; !range.empty(); range.pop_first())
            if (max(r.first(), range.first()) == range.first())
                r = range;
        return r;
    }
    template<typename R, typename C>
    inline R max_element(R range, C compare) {
        R r = range;
        for (; !range.empty(); range.pop_first())
            if (max(r.first(), range.first(), compare) == range.first())
                r = range;
        return r;
    }

    template<typename T>
    inline T min(InitializerList<T> il) {
        return min_element(il.range()).first();
    }
    template<typename T, typename C>
    inline T min(InitializerList<T> il, C compare) {
        return min_element(il.range(), compare).first();
    }

    template<typename T>
    inline T max(InitializerList<T> il) {
        return max_element(il.range()).first();
    }

    template<typename T, typename C>
    inline T max(InitializerList<T> il, C compare) {
        return max_element(il.range(), compare).first();
    }

    template<typename R, typename F>
    F for_each(R range, F func) {
        for (; !range.empty(); range.pop_first())
            func(range.first());
        return move(func);
    }

    template<typename R, typename P>
    bool all_of(R range, P pred) {
        for (; !range.empty(); range.pop_first())
            if (!pred(range.first())) return false;
        return true;
    }

    template<typename R, typename P>
    bool any_of(R range, P pred) {
        for (; !range.empty(); range.pop_first())
            if (pred(range.first())) return true;
        return false;
    }

    template<typename R, typename P>
    bool none_of(R range, P pred) {
        for (; !range.empty(); range.pop_first())
            if (pred(range.first())) return false;
        return true;
    }

    template<typename R, typename T>
    R find(R range, const T &v) {
        for (; !range.empty(); range.pop_first())
            if (range.first() == v)
                break;
        return range;
    }

    template<typename R, typename P>
    R find_if(R range, P pred) {
        for (; !range.empty(); range.pop_first())
            if (pred(range.first()))
                break;
        return range;
    }

    template<typename R, typename P>
    R find_if_not(R range, P pred) {
        for (; !range.empty(); range.pop_first())
            if (!pred(range.first()))
                break;
        return range;
    }

    template<typename R, typename T>
    size_t count(R range, const T &v) {
        size_t ret = 0;
        for (; !range.empty(); range.pop_first())
            if (range.first() == v)
                ++ret;
        return ret;
    }

    template<typename R, typename P>
    size_t count_if(R range, P pred) {
        size_t ret = 0;
        for (; !range.empty(); range.pop_first())
            if (pred(range.first()))
                ++ret;
        return ret;
    }

    template<typename R, typename P>
    size_t count_if_not(R range, P pred) {
        size_t ret = 0;
        for (; !range.empty(); range.pop_first())
            if (!pred(range.first()))
                ++ret;
        return ret;
    }

    template<typename R>
    bool equal(R range1, R range2) {
        for (; !range1.empty(); range1.pop_first()) {
            if (range2.empty() || (range1.first() != range2.first()))
                return false;
            range2.pop_first();
        }
        return range2.empty();
    }

    template<typename R1, typename R2>
    R2 copy(R1 irange, R2 orange) {
        for (; !irange.empty(); irange.pop_first())
            orange.put(irange.first());
        return orange;
    }

    template<typename R1, typename R2, typename P>
    R2 copy_if(R1 irange, R2 orange, P pred) {
        for (; !irange.empty(); irange.pop_first())
            if (pred(irange.first()))
                orange.put(irange.first());
        return orange;
    }

    template<typename R1, typename R2, typename P>
    R2 copy_if_not(R1 irange, R2 orange, P pred) {
        for (; !irange.empty(); irange.pop_first())
            if (!pred(irange.first()))
                orange.put(irange.first());
        return orange;
    }

    template<typename R1, typename R2>
    R2 move(R1 irange, R2 orange) {
        for (; !irange.empty(); irange.pop_first())
            orange.put(move(irange.first()));
        return orange;
    }

    template<typename R>
    void reverse(R range) {
        while (!range.empty()) {
            swap(range.first(), range.last());
            range.pop_first();
            range.pop_last();
        }
    }

    template<typename R1, typename R2>
    R2 reverse_copy(R1 irange, R2 orange) {
        for (; !irange.empty(); irange.pop_last())
            orange.put(irange.last());
        return orange;
    }

    template<typename R, typename T>
    void fill(R range, const T &v) {
        for (; !range.empty(); range.pop_first())
            range.first() = v;
    }

    template<typename R, typename F>
    void generate(R range, F gen) {
        for (; !range.empty(); range.pop_first())
            range.first() = gen();
    }

    template<typename R1, typename R2>
    R2 swap_ranges(R1 range1, R2 range2) {
        for (; !range1.empty(); range1.pop_first()) {
            swap(range1.first(), range2.first());
            range2.pop_first();
        }
        return range2;
    }

    template<typename R, typename T>
    void iota(R range, T value) {
        for (; !range.empty(); range.pop_first())
            range.first() = value++;
    }
}

#endif