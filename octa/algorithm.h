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

    /* partitioning */

    template<typename R, typename U>
    R partition(R range, U pred) {
        R ret = range;
        for (; !range.empty(); range.pop_front()) {
            if (pred(range.front())) {
                swap(range.front(), ret.front());
                ret.pop_front();
            }
        }
        return ret;
    }

    template<typename R, typename P>
    bool is_partitioned(R range, P pred) {
        for (; !range.empty() && pred(range.front()); range.pop_front());
        for (; !range.empty(); range.pop_front())
            if (pred(range.front())) return false;
        return true;
    }

    /* sorting */

    template<typename R, typename C>
    void __octa_insort(R range, C compare) {
        RangeSize<R> rlen = range.size();
        for (RangeSize<R> i = 1; i < rlen; ++i) {
            RangeSize<R> j = i;
            RangeReference<R> v = range[i];
            while (j > 0 && !compare(range[j - 1], v)) {
                range[j] = range[j - 1];
                --j;
            }
            range[j] = v;
        }
    }

    template<typename T, typename U>
    struct __OctaUnaryCompare {
        const T &val;
        U comp;
        bool operator()(const T &v) const { return comp(v, val); }
    };

    template<typename R, typename C>
    void __octa_hs_sift_down(R range, RangeSize<R> s,
    RangeSize<R> e, C compare) {
        RangeSize<R> r = s;
        while ((r * 2 + 1) <= e) {
            RangeSize<R> ch = r * 2 + 1;
            RangeSize<R> sw = r;
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
    void __octa_heapsort(R range, C compare) {
        RangeSize<R> len = range.size();
        RangeSize<R> st = (len - 2) / 2;
        for (;;) {
            __octa_hs_sift_down(range, st, len - 1, compare);
            if (st-- == 0) break;
        }
        RangeSize<R> e = len - 1;
        while (e > 0) {
            swap(range[e], range[0]);
            --e;
            __octa_hs_sift_down(range, 0, e, compare);
        }
    }

    template<typename R, typename C>
    void __octa_introloop(R range, C compare, RangeSize<R> depth) {
        if (range.size() <= 10) {
            __octa_insort(range, compare);
            return;
        }
        if (depth == 0) {
            __octa_heapsort(range, compare);
            return;
        }
        RangeReference<R> p = range[range.size() / 2];
        swap(p, range.back());
        R r = partition(range, __OctaUnaryCompare<decltype(p), C>{ p, compare });
        R l = range.slice(0, range.size() - r.size());
        swap(r.front(), r.back());
        __octa_introloop(l, compare, depth - 1);
        __octa_introloop(r, compare, depth - 1);
    }

    template<typename R, typename C>
    void __octa_introsort(R range, C compare) {
        __octa_introloop(range, compare, RangeSize<R>(2
            * (log(range.size()) / log(2))));
    }

    template<typename R, typename C>
    void sort(R range, C compare) {
        __octa_introsort(range, compare);
    }

    template<typename R>
    void sort(R range) {
        sort(range, Less<RangeValue<R>>());
    }

    /* min/max(_element) */

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
        for (; !range.empty(); range.pop_front())
            if (min(r.front(), range.front()) == range.front())
                r = range;
        return r;
    }
    template<typename R, typename C>
    inline R min_element(R range, C compare) {
        R r = range;
        for (; !range.empty(); range.pop_front())
            if (min(r.front(), range.front(), compare) == range.front())
                r = range;
        return r;
    }

    template<typename R>
    inline R max_element(R range) {
        R r = range;
        for (; !range.empty(); range.pop_front())
            if (max(r.front(), range.front()) == range.front())
                r = range;
        return r;
    }
    template<typename R, typename C>
    inline R max_element(R range, C compare) {
        R r = range;
        for (; !range.empty(); range.pop_front())
            if (max(r.front(), range.front(), compare) == range.front())
                r = range;
        return r;
    }

    using std::initializer_list;

    template<typename T>
    inline T min(initializer_list<T> il) {
        return min_element(each(il)).front();
    }
    template<typename T, typename C>
    inline T min(initializer_list<T> il, C compare) {
        return min_element(each(il), compare).front();
    }

    template<typename T>
    inline T max(initializer_list<T> il) {
        return max_element(each(il)).front();
    }

    template<typename T, typename C>
    inline T max(initializer_list<T> il, C compare) {
        return max_element(each(il), compare).front();
    }

    /* clamp */

    template<typename T, typename U>
    inline T clamp(const T &v, const U &lo, const U &hi) {
        return max(T(lo), min(v, T(hi)));
    }

    template<typename T, typename U, typename C>
    inline T clamp(const T &v, const U &lo, const U &hi, C compare) {
        return max(T(lo), min(v, T(hi), compare), compare);
    }

    /* algos that don't change the range */

    template<typename R, typename F>
    F for_each(R range, F func) {
        for (; !range.empty(); range.pop_front())
            func(range.front());
        return move(func);
    }

    template<typename R, typename P>
    bool all_of(R range, P pred) {
        for (; !range.empty(); range.pop_front())
            if (!pred(range.front())) return false;
        return true;
    }

    template<typename R, typename P>
    bool any_of(R range, P pred) {
        for (; !range.empty(); range.pop_front())
            if (pred(range.front())) return true;
        return false;
    }

    template<typename R, typename P>
    bool none_of(R range, P pred) {
        for (; !range.empty(); range.pop_front())
            if (pred(range.front())) return false;
        return true;
    }

    template<typename R, typename T>
    R find(R range, const T &v) {
        for (; !range.empty(); range.pop_front())
            if (range.front() == v)
                break;
        return range;
    }

    template<typename R, typename P>
    R find_if(R range, P pred) {
        for (; !range.empty(); range.pop_front())
            if (pred(range.front()))
                break;
        return range;
    }

    template<typename R, typename P>
    R find_if_not(R range, P pred) {
        for (; !range.empty(); range.pop_front())
            if (!pred(range.front()))
                break;
        return range;
    }

    template<typename R, typename T>
    RangeSize<R> count(R range, const T &v) {
        RangeSize<R> ret = 0;
        for (; !range.empty(); range.pop_front())
            if (range.front() == v)
                ++ret;
        return ret;
    }

    template<typename R, typename P>
    RangeSize<R> count_if(R range, P pred) {
        RangeSize<R> ret = 0;
        for (; !range.empty(); range.pop_front())
            if (pred(range.front()))
                ++ret;
        return ret;
    }

    template<typename R, typename P>
    RangeSize<R> count_if_not(R range, P pred) {
        RangeSize<R> ret = 0;
        for (; !range.empty(); range.pop_front())
            if (!pred(range.front()))
                ++ret;
        return ret;
    }

    template<typename R>
    bool equal(R range1, R range2) {
        for (; !range1.empty(); range1.pop_front()) {
            if (range2.empty() || (range1.front() != range2.front()))
                return false;
            range2.pop_front();
        }
        return range2.empty();
    }

    /* algos that modify ranges or work with output ranges */

    template<typename R1, typename R2>
    R2 copy(R1 irange, R2 orange) {
        for (; !irange.empty(); irange.pop_front())
            orange.put(irange.front());
        return orange;
    }

    template<typename R1, typename R2, typename P>
    R2 copy_if(R1 irange, R2 orange, P pred) {
        for (; !irange.empty(); irange.pop_front())
            if (pred(irange.front()))
                orange.put(irange.front());
        return orange;
    }

    template<typename R1, typename R2, typename P>
    R2 copy_if_not(R1 irange, R2 orange, P pred) {
        for (; !irange.empty(); irange.pop_front())
            if (!pred(irange.front()))
                orange.put(irange.front());
        return orange;
    }

    template<typename R1, typename R2>
    R2 move(R1 irange, R2 orange) {
        for (; !irange.empty(); irange.pop_front())
            orange.put(move(irange.front()));
        return orange;
    }

    template<typename R>
    void reverse(R range) {
        while (!range.empty()) {
            swap(range.front(), range.back());
            range.pop_front();
            range.pop_back();
        }
    }

    template<typename R1, typename R2>
    R2 reverse_copy(R1 irange, R2 orange) {
        for (; !irange.empty(); irange.pop_back())
            orange.put(irange.back());
        return orange;
    }

    template<typename R, typename T>
    void fill(R range, const T &v) {
        for (; !range.empty(); range.pop_front())
            range.front() = v;
    }

    template<typename R, typename F>
    void generate(R range, F gen) {
        for (; !range.empty(); range.pop_front())
            range.front() = gen();
    }

    template<typename R1, typename R2>
    Pair<R1, R2> swap_ranges(R1 range1, R2 range2) {
        while (!range1.empty() && !range2.empty()) {
            swap(range1.front(), range2.front());
            range1.pop_front();
            range2.pop_front();
        }
        return Pair<R1, R2>(range1, range2);
    }

    template<typename R, typename T>
    void iota(R range, T value) {
        for (; !range.empty(); range.pop_front())
            range.front() = value++;
    }

    template<typename R, typename T>
    T foldl(R range, T init) {
        for (; !range.empty(); range.pop_front())
            init = init + range.front();
        return init;
    }

    template<typename R, typename T, typename F>
    T foldl(R range, T init, F func) {
        for (; !range.empty(); range.pop_front())
            init = func(init, range.front());
        return init;
    }

    template<typename R, typename T>
    T foldr(R range, T init) {
        for (; !range.empty(); range.pop_back())
            init = init + range.back();
        return init;
    }

    template<typename R, typename T, typename F>
    T foldr(R range, T init, F func) {
        for (; !range.empty(); range.pop_back())
            init = func(init, range.back());
        return init;
    }

    template<typename T, typename R>
    struct MapRange: InputRange<
        MapRange<T, R>, RangeCategory<T>, R, R, RangeSize<T>
    > {
    private:
        T p_range;
        Function<R(RangeReference<T>)> p_func;

    public:
        MapRange(): p_range(), p_func() {}
        template<typename F>
        MapRange(const T &range, const F &func): p_range(range), p_func(func) {}
        MapRange(const MapRange &it): p_range(it.p_range), p_func(it.p_func) {}
        MapRange(MapRange &&it): p_range(move(it.p_range)), p_func(move(it.p_func)) {}

        MapRange &operator=(const MapRange &v) {
            p_range = v.p_range;
            p_func  = v.p_func;
            return *this;
        }
        MapRange &operator=(MapRange &&v) {
            p_range = move(v.p_range);
            p_func  = move(v.p_func);
            return *this;
        }

        bool empty() const { return p_range.empty(); }
        RangeSize<T> size() const { return p_range.size(); }

        bool equals_front(const MapRange &range) const {
            return p_range.equals_front(range.p_range);
        }
        bool equals_back(const MapRange &range) const {
            return p_range.equals_front(range.p_range);
        }

        RangeDifference<T> distance_front(const MapRange &range) const {
            return p_range.distance_front(range.p_range);
        }
        RangeDifference<T> distance_back(const MapRange &range) const {
            return p_range.distance_back(range.p_range);
        }

        bool pop_front() { return p_range.pop_front(); }
        bool pop_back() { return p_range.pop_back(); }

        bool push_front() { return p_range.pop_front(); }
        bool push_back() { return p_range.push_back(); }

        RangeSize<T> pop_front_n(RangeSize<T> n) { p_range.pop_front_n(n); }
        RangeSize<T> pop_back_n(RangeSize<T> n) { p_range.pop_back_n(n); }

        RangeSize<T> push_front_n(RangeSize<T> n) { return p_range.push_front_n(n); }
        RangeSize<T> push_back_n(RangeSize<T> n) { return p_range.push_back_n(n); }

        R front() const { return p_func(p_range.front()); }
        R back() const { return p_func(p_range.back()); }

        R operator[](RangeSize<T> idx) const {
            return p_func(p_range[idx]);
        }

        MapRange<T, R> slice(RangeSize<T> start, RangeSize<T> end) {
            return MapRange<T, R>(p_range.slice(start, end), p_func);
        }
    };

    template<typename R, typename F>
    using __OctaMapReturnType = decltype(declval<F>()(declval<RangeReference<R>>()));

    template<typename R, typename F>
    MapRange<R, __OctaMapReturnType<R, F>> map(R range, F func) {
        return MapRange<R, __OctaMapReturnType<R, F>>(range, func);
    }

    template<typename T>
    struct FilterRange: InputRange<
        FilterRange<T>, CommonType<RangeCategory<T>, ForwardRangeTag>,
        RangeValue<T>, RangeReference<T>, RangeSize<T>
    > {
    private:
        T p_range;
        Function<bool(RangeReference<T>)> p_pred;

        void advance_valid() {
            while (!p_range.empty() && !p_pred(front())) p_range.pop_front();
        }

    public:
        FilterRange(): p_range(), p_pred() {}

        template<typename P>
        FilterRange(const T &range, const P &pred): p_range(range),
        p_pred(pred) {
            advance_valid();
        }
        FilterRange(const FilterRange &it): p_range(it.p_range),
        p_pred(it.p_pred) {
            advance_valid();
        }
        FilterRange(FilterRange &&it): p_range(move(it.p_range)),
        p_pred(move(it.p_pred)) {
            advance_valid();
        }

        FilterRange &operator=(const FilterRange &v) {
            p_range = v.p_range;
            p_pred  = v.p_pred;
            advance_valid();
            return *this;
        }
        FilterRange &operator=(FilterRange &&v) {
            p_range = move(v.p_range);
            p_pred  = move(v.p_pred);
            advance_valid();
            return *this;
        }

        bool empty() const { return p_range.empty(); }

        bool equals_front(const FilterRange &range) const {
            return p_range.equals_front(range.p_range);
        }

        bool pop_front() {
            bool ret = p_range.pop_front();
            advance_valid();
            return ret;
        }
        bool push_front() {
            T tmp = p_range;
            if (!tmp.push_front()) return false;
            while (!pred(tmp.front()))
                if (!tmp.push_front())
                    return false;
            p_range = tmp;
            return true;
        }

        RangeReference<T> front() const { return p_range.front(); }
    };

    template<typename R, typename P>
    FilterRange<R> filter(R range, P pred) {
        return FilterRange<R>(range, pred);
    }
}

#endif