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

template<typename _R, typename _U>
_R partition(_R range, _U pred) {
    _R ret = range;
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            octa::swap(range.front(), ret.front());
            ret.pop_front();
        }
    }
    return ret;
}

template<typename _R, typename _P>
bool is_partitioned(_R range, _P pred) {
    for (; !range.empty() && pred(range.front()); range.pop_front());
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return false;
    return true;
}

/* sorting */

namespace detail {
    template<typename _R, typename _C>
    void insort(_R range, _C compare) {
        octa::RangeSize<_R> rlen = range.size();
        for (octa::RangeSize<_R> i = 1; i < rlen; ++i) {
            octa::RangeSize<_R> j = i;
            octa::RangeReference<_R> v = range[i];
            while (j > 0 && !compare(range[j - 1], v)) {
                range[j] = range[j - 1];
                --j;
            }
            range[j] = v;
        }
    }

    template<typename _T, typename _U>
    struct UnaryCompare {
        const _T &val;
        _U comp;
        bool operator()(const _T &v) const { return comp(v, val); }
    };

    template<typename _R, typename _C>
    void hs_sift_down(_R range, octa::RangeSize<_R> s,
    octa::RangeSize<_R> e, _C compare) {
        octa::RangeSize<_R> r = s;
        while ((r * 2 + 1) <= e) {
            octa::RangeSize<_R> ch = r * 2 + 1;
            octa::RangeSize<_R> sw = r;
            if (compare(range[sw], range[ch]))
                sw = ch;
            if (((ch + 1) <= e) && compare(range[sw], range[ch + 1]))
                sw = ch + 1;
            if (sw != r) {
                octa::swap(range[r], range[sw]);
                r = sw;
            } else return;
        }
    }

    template<typename _R, typename _C>
    void heapsort(_R range, _C compare) {
        octa::RangeSize<_R> len = range.size();
        octa::RangeSize<_R> st = (len - 2) / 2;
        for (;;) {
            octa::detail::hs_sift_down(range, st, len - 1, compare);
            if (st-- == 0) break;
        }
        octa::RangeSize<_R> e = len - 1;
        while (e > 0) {
            octa::swap(range[e], range[0]);
            --e;
            octa::detail::hs_sift_down(range, 0, e, compare);
        }
    }

    template<typename _R, typename _C>
    void introloop(_R range, _C compare, RangeSize<_R> depth) {
        if (range.size() <= 10) {
            octa::detail::insort(range, compare);
            return;
        }
        if (depth == 0) {
            octa::detail::heapsort(range, compare);
            return;
        }
        octa::RangeReference<_R> p = range[range.size() / 2];
        octa::swap(p, range.back());
        _R r = octa::partition(range,
            octa::detail::UnaryCompare<decltype(p), _C>{ p, compare });
        _R l = range.slice(0, range.size() - r.size());
        octa::swap(r.front(), r.back());
        octa::detail::introloop(l, compare, depth - 1);
        octa::detail::introloop(r, compare, depth - 1);
    }

    template<typename _R, typename _C>
    void introsort(_R range, _C compare) {
        octa::detail::introloop(range, compare, octa::RangeSize<_R>(2
            * (log(range.size()) / log(2))));
    }
} /* namespace detail */

template<typename _R, typename _C>
void sort(_R range, _C compare) {
    octa::detail::introsort(range, compare);
}

template<typename _R>
void sort(_R range) {
    sort(range, octa::Less<RangeValue<_R>>());
}

/* min/max(_element) */

template<typename _T>
inline const _T &min(const _T &a, const _T &b) {
    return (a < b) ? a : b;
}
template<typename _T, typename _C>
inline const _T &min(const _T &a, const _T &b, _C compare) {
    return compare(a, b) ? a : b;
}

template<typename _T>
inline const _T &max(const _T &a, const _T &b) {
    return (a < b) ? b : a;
}
template<typename _T, typename _C>
inline const _T &max(const _T &a, const _T &b, _C compare) {
    return compare(a, b) ? b : a;
}

template<typename _R>
inline _R min_element(_R range) {
    _R r = range;
    for (; !range.empty(); range.pop_front())
        if (octa::min(r.front(), range.front()) == range.front())
            r = range;
    return r;
}
template<typename _R, typename _C>
inline _R min_element(_R range, _C compare) {
    _R r = range;
    for (; !range.empty(); range.pop_front())
        if (octa::min(r.front(), range.front(), compare) == range.front())
            r = range;
    return r;
}

template<typename _R>
inline _R max_element(_R range) {
    _R r = range;
    for (; !range.empty(); range.pop_front())
        if (octa::max(r.front(), range.front()) == range.front())
            r = range;
    return r;
}
template<typename _R, typename _C>
inline _R max_element(_R range, _C compare) {
    _R r = range;
    for (; !range.empty(); range.pop_front())
        if (octa::max(r.front(), range.front(), compare) == range.front())
            r = range;
    return r;
}

template<typename _T>
inline _T min(std::initializer_list<_T> il) {
    return octa::min_element(octa::each(il)).front();
}
template<typename _T, typename _C>
inline _T min(std::initializer_list<_T> il, _C compare) {
    return octa::min_element(octa::each(il), compare).front();
}

template<typename _T>
inline _T max(std::initializer_list<_T> il) {
    return octa::max_element(octa::each(il)).front();
}

template<typename _T, typename _C>
inline _T max(std::initializer_list<_T> il, _C compare) {
    return octa::max_element(octa::each(il), compare).front();
}

/* clamp */

template<typename _T, typename _U>
inline _T clamp(const _T &v, const _U &lo, const _U &hi) {
    return octa::max(_T(lo), octa::min(v, _T(hi)));
}

template<typename _T, typename _U, typename _C>
inline _T clamp(const _T &v, const _U &lo, const _U &hi, _C compare) {
    return octa::max(_T(lo), octa::min(v, _T(hi), compare), compare);
}

/* algos that don't change the range */

template<typename _R, typename _F>
_F for_each(_R range, _F func) {
    for (; !range.empty(); range.pop_front())
        func(range.front());
    return octa::move(func);
}

template<typename _R, typename _P>
bool all_of(_R range, _P pred) {
    for (; !range.empty(); range.pop_front())
        if (!pred(range.front())) return false;
    return true;
}

template<typename _R, typename _P>
bool any_of(_R range, _P pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return true;
    return false;
}

template<typename _R, typename _P>
bool none_of(_R range, _P pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return false;
    return true;
}

template<typename _R, typename _T>
_R find(_R range, const _T &v) {
    for (; !range.empty(); range.pop_front())
        if (range.front() == v)
            break;
    return range;
}

template<typename _R, typename _P>
_R find_if(_R range, _P pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front()))
            break;
    return range;
}

template<typename _R, typename _P>
_R find_if_not(_R range, _P pred) {
    for (; !range.empty(); range.pop_front())
        if (!pred(range.front()))
            break;
    return range;
}

template<typename _R, typename _T>
RangeSize<_R> count(_R range, const _T &v) {
    RangeSize<_R> ret = 0;
    for (; !range.empty(); range.pop_front())
        if (range.front() == v)
            ++ret;
    return ret;
}

template<typename _R, typename _P>
RangeSize<_R> count_if(_R range, _P pred) {
    RangeSize<_R> ret = 0;
    for (; !range.empty(); range.pop_front())
        if (pred(range.front()))
            ++ret;
    return ret;
}

template<typename _R, typename _P>
RangeSize<_R> count_if_not(_R range, _P pred) {
    RangeSize<_R> ret = 0;
    for (; !range.empty(); range.pop_front())
        if (!pred(range.front()))
            ++ret;
    return ret;
}

template<typename _R>
bool equal(_R range1, _R range2) {
    for (; !range1.empty(); range1.pop_front()) {
        if (range2.empty() || (range1.front() != range2.front()))
            return false;
        range2.pop_front();
    }
    return range2.empty();
}

/* algos that modify ranges or work with output ranges */

template<typename _R1, typename _R2>
_R2 copy(_R1 irange, _R2 orange) {
    for (; !irange.empty(); irange.pop_front())
        orange.put(irange.front());
    return orange;
}

template<typename _R1, typename _R2, typename _P>
_R2 copy_if(_R1 irange, _R2 orange, _P pred) {
    for (; !irange.empty(); irange.pop_front())
        if (pred(irange.front()))
            orange.put(irange.front());
    return orange;
}

template<typename _R1, typename _R2, typename _P>
_R2 copy_if_not(_R1 irange, _R2 orange, _P pred) {
    for (; !irange.empty(); irange.pop_front())
        if (!pred(irange.front()))
            orange.put(irange.front());
    return orange;
}

template<typename _R1, typename _R2>
_R2 move(_R1 irange, _R2 orange) {
    for (; !irange.empty(); irange.pop_front())
        orange.put(octa::move(irange.front()));
    return orange;
}

template<typename _R>
void reverse(_R range) {
    while (!range.empty()) {
        octa::swap(range.front(), range.back());
        range.pop_front();
        range.pop_back();
    }
}

template<typename _R1, typename _R2>
_R2 reverse_copy(_R1 irange, _R2 orange) {
    for (; !irange.empty(); irange.pop_back())
        orange.put(irange.back());
    return orange;
}

template<typename _R, typename _T>
void fill(_R range, const _T &v) {
    for (; !range.empty(); range.pop_front())
        range.front() = v;
}

template<typename _R, typename _F>
void generate(_R range, _F gen) {
    for (; !range.empty(); range.pop_front())
        range.front() = gen();
}

template<typename _R1, typename _R2>
octa::Pair<_R1, _R2> swap_ranges(_R1 range1, _R2 range2) {
    while (!range1.empty() && !range2.empty()) {
        octa::swap(range1.front(), range2.front());
        range1.pop_front();
        range2.pop_front();
    }
    return octa::make_pair(range1, range2);
}

template<typename _R, typename _T>
void iota(_R range, _T value) {
    for (; !range.empty(); range.pop_front())
        range.front() = value++;
}

template<typename _R, typename _T>
_T foldl(_R range, _T init) {
    for (; !range.empty(); range.pop_front())
        init = init + range.front();
    return init;
}

template<typename _R, typename _T, typename _F>
_T foldl(_R range, _T init, _F func) {
    for (; !range.empty(); range.pop_front())
        init = func(init, range.front());
    return init;
}

template<typename _R, typename _T>
_T foldr(_R range, _T init) {
    for (; !range.empty(); range.pop_back())
        init = init + range.back();
    return init;
}

template<typename _R, typename _T, typename _F>
_T foldr(_R range, _T init, _F func) {
    for (; !range.empty(); range.pop_back())
        init = func(init, range.back());
    return init;
}

template<typename _T, typename _R>
struct MapRange: InputRange<
    MapRange<_T, _R>, octa::RangeCategory<_T>, _R, _R, octa::RangeSize<_T>
> {
private:
    _T p_range;
    octa::Function<_R(octa::RangeReference<_T>)> p_func;

public:
    MapRange(): p_range(), p_func() {}
    template<typename _F>
    MapRange(const _T &range, const _F &func):
        p_range(range), p_func(func) {}
    MapRange(const MapRange &it):
        p_range(it.p_range), p_func(it.p_func) {}
    MapRange(MapRange &&it):
        p_range(move(it.p_range)), p_func(move(it.p_func)) {}

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
    octa::RangeSize<_T> size() const { return p_range.size(); }

    bool equals_front(const MapRange &r) const {
        return p_range.equals_front(r.p_range);
    }
    bool equals_back(const MapRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    octa::RangeDifference<_T> distance_front(const MapRange &r) const {
        return p_range.distance_front(r.p_range);
    }
    octa::RangeDifference<_T> distance_back(const MapRange &r) const {
        return p_range.distance_back(r.p_range);
    }

    bool pop_front() { return p_range.pop_front(); }
    bool pop_back() { return p_range.pop_back(); }

    bool push_front() { return p_range.pop_front(); }
    bool push_back() { return p_range.push_back(); }

    octa::RangeSize<_T> pop_front_n(octa::RangeSize<_T> n) {
        p_range.pop_front_n(n);
    }
    octa::RangeSize<_T> pop_back_n(octa::RangeSize<_T> n) {
        p_range.pop_back_n(n);
    }

    octa::RangeSize<_T> push_front_n(octa::RangeSize<_T> n) {
        return p_range.push_front_n(n);
    }
    octa::RangeSize<_T> push_back_n(octa::RangeSize<_T> n) {
        return p_range.push_back_n(n);
    }

    _R front() const { return p_func(p_range.front()); }
    _R back() const { return p_func(p_range.back()); }

    _R operator[](octa::RangeSize<_T> idx) const {
        return p_func(p_range[idx]);
    }

    MapRange<_T, _R> slice(octa::RangeSize<_T> start,
                           octa::RangeSize<_T> end) {
        return MapRange<_T, _R>(p_range.slice(start, end), p_func);
    }
};

namespace detail {
    template<typename _R, typename _F> using MapReturnType
        = decltype(declval<_F>()(octa::declval<octa::RangeReference<_R>>()));
}

template<typename _R, typename _F>
MapRange<_R, octa::detail::MapReturnType<_R, _F>> map(_R range,
                                                      _F func) {
    return octa::MapRange<_R, octa::detail::MapReturnType<_R, _F>>(range,
        func);
}

template<typename _T>
struct FilterRange: InputRange<
    FilterRange<_T>, octa::CommonType<octa::RangeCategory<_T>,
                                      octa::ForwardRangeTag>,
    octa::RangeValue<_T>, octa::RangeReference<_T>, octa::RangeSize<_T>
> {
private:
    _T p_range;
    octa::Function<bool(octa::RangeReference<_T>)> p_pred;

    void advance_valid() {
        while (!p_range.empty() && !p_pred(front())) p_range.pop_front();
    }

public:
    FilterRange(): p_range(), p_pred() {}

    template<typename _P>
    FilterRange(const _T &range, const _P &pred): p_range(range),
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

    bool equals_front(const FilterRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() {
        bool ret = p_range.pop_front();
        advance_valid();
        return ret;
    }
    bool push_front() {
        _T tmp = p_range;
        if (!tmp.push_front()) return false;
        while (!p_pred(tmp.front()))
            if (!tmp.push_front())
                return false;
        p_range = tmp;
        return true;
    }

    octa::RangeReference<_T> front() const { return p_range.front(); }
};

template<typename _R, typename _P>
FilterRange<_R> filter(_R range, _P pred) {
    return octa::FilterRange<_R>(range, pred);
}

} /* namespace octa */

#endif