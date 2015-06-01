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
    _R partition(_R __range, _U __pred) {
        _R __ret = __range;
        for (; !__range.empty(); __range.pop_front()) {
            if (__pred(__range.front())) {
                octa::swap(__range.front(), __ret.front());
                __ret.pop_front();
            }
        }
        return __ret;
    }

    template<typename _R, typename _P>
    bool is_partitioned(_R __range, _P __pred) {
        for (; !__range.empty() && __pred(__range.front()); __range.pop_front());
        for (; !__range.empty(); __range.pop_front())
            if (__pred(__range.front())) return false;
        return true;
    }

    /* sorting */

    template<typename _R, typename _C>
    void __octa_insort(_R __range, _C __compare) {
        octa::RangeSize<_R> __rlen = __range.size();
        for (octa::RangeSize<_R> __i = 1; __i < __rlen; ++__i) {
            octa::RangeSize<_R> __j = __i;
            octa::RangeReference<_R> __v = __range[__i];
            while (__j > 0 && !__compare(__range[__j - 1], __v)) {
                __range[__j] = __range[__j - 1];
                --__j;
            }
            __range[__j] = __v;
        }
    }

    template<typename _T, typename _U>
    struct __OctaUnaryCompare {
        const _T &__val;
        _U __comp;
        bool operator()(const _T &__v) const { return __comp(__v, __val); }
    };

    template<typename _R, typename _C>
    void __octa_hs_sift_down(_R __range, octa::RangeSize<_R> __s,
    octa::RangeSize<_R> __e, _C __compare) {
        octa::RangeSize<_R> __r = __s;
        while ((__r * 2 + 1) <= __e) {
            octa::RangeSize<_R> __ch = __r * 2 + 1;
            octa::RangeSize<_R> __sw = __r;
            if (__compare(__range[__sw], __range[__ch]))
                __sw = __ch;
            if (((__ch + 1) <= __e) && __compare(__range[__sw], __range[__ch + 1]))
                __sw = __ch + 1;
            if (__sw != __r) {
                octa::swap(__range[__r], __range[__sw]);
                __r = __sw;
            } else return;
        }
    }

    template<typename _R, typename _C>
    void __octa_heapsort(_R __range, _C __compare) {
        octa::RangeSize<_R> __len = __range.size();
        octa::RangeSize<_R> __st = (__len - 2) / 2;
        for (;;) {
            __octa_hs_sift_down(__range, __st, __len - 1, __compare);
            if (__st-- == 0) break;
        }
        octa::RangeSize<_R> __e = __len - 1;
        while (__e > 0) {
            octa::swap(__range[__e], __range[0]);
            --__e;
            __octa_hs_sift_down(__range, 0, __e, __compare);
        }
    }

    template<typename _R, typename _C>
    void __octa_introloop(_R __range, _C __compare, RangeSize<_R> __depth) {
        if (__range.size() <= 10) {
            __octa_insort(__range, __compare);
            return;
        }
        if (__depth == 0) {
            __octa_heapsort(__range, __compare);
            return;
        }
        octa::RangeReference<_R> __p = __range[__range.size() / 2];
        octa::swap(__p, __range.back());
        _R __r = octa::partition(__range,
            __OctaUnaryCompare<decltype(__p), _C>{ __p, __compare });
        _R __l = __range.slice(0, __range.size() - __r.size());
        octa::swap(__r.front(), __r.back());
        __octa_introloop(__l, __compare, __depth - 1);
        __octa_introloop(__r, __compare, __depth - 1);
    }

    template<typename _R, typename _C>
    void __octa_introsort(_R __range, _C __compare) {
        __octa_introloop(__range, __compare, octa::RangeSize<_R>(2
            * (log(__range.size()) / log(2))));
    }

    template<typename _R, typename _C>
    void sort(_R __range, _C __compare) {
        __octa_introsort(__range, __compare);
    }

    template<typename _R>
    void sort(_R __range) {
        sort(__range, octa::Less<RangeValue<_R>>());
    }

    /* min/max(_element) */

    template<typename _T>
    inline const _T &min(const _T &__a, const _T &__b) {
        return (__a < __b) ? __a : __b;
    }
    template<typename _T, typename _C>
    inline const _T &min(const _T &__a, const _T &__b, _C __compare) {
        return __compare(__a, __b) ? __a : __b;
    }

    template<typename _T>
    inline const _T &max(const _T &__a, const _T &__b) {
        return (__a < __b) ? __b : __a;
    }
    template<typename _T, typename _C>
    inline const _T &max(const _T &__a, const _T &__b, _C __compare) {
        return __compare(__a, __b) ? __b : __a;
    }

    template<typename _R>
    inline _R min_element(_R __range) {
        _R __r = __range;
        for (; !__range.empty(); __range.pop_front())
            if (octa::min(__r.front(), __range.front()) == __range.front())
                __r = __range;
        return __r;
    }
    template<typename _R, typename _C>
    inline _R min_element(_R __range, _C __compare) {
        _R __r = __range;
        for (; !__range.empty(); __range.pop_front())
            if (octa::min(__r.front(), __range.front(), __compare) == __range.front())
                __r = __range;
        return __r;
    }

    template<typename _R>
    inline _R max_element(_R __range) {
        _R __r = __range;
        for (; !__range.empty(); __range.pop_front())
            if (octa::max(__r.front(), __range.front()) == __range.front())
                __r = __range;
        return __r;
    }
    template<typename _R, typename _C>
    inline _R max_element(_R __range, _C __compare) {
        _R __r = __range;
        for (; !__range.empty(); __range.pop_front())
            if (octa::max(__r.front(), __range.front(), __compare) == __range.front())
                __r = __range;
        return __r;
    }

    template<typename _T>
    inline _T min(std::initializer_list<_T> __il) {
        return octa::min_element(octa::each(__il)).front();
    }
    template<typename _T, typename _C>
    inline _T min(std::initializer_list<_T> __il, _C __compare) {
        return octa::min_element(octa::each(__il), __compare).front();
    }

    template<typename _T>
    inline _T max(std::initializer_list<_T> __il) {
        return octa::max_element(octa::each(__il)).front();
    }

    template<typename _T, typename _C>
    inline _T max(std::initializer_list<_T> __il, _C __compare) {
        return octa::max_element(octa::each(__il), __compare).front();
    }

    /* clamp */

    template<typename _T, typename _U>
    inline _T clamp(const _T &__v, const _U &__lo, const _U &__hi) {
        return octa::max(_T(__lo), octa::min(__v, _T(__hi)));
    }

    template<typename _T, typename _U, typename _C>
    inline _T clamp(const _T &__v, const _U &__lo, const _U &__hi, _C __compare) {
        return octa::max(_T(__lo), octa::min(__v, _T(__hi), __compare), __compare);
    }

    /* algos that don't change the range */

    template<typename _R, typename _F>
    _F for_each(_R __range, _F __func) {
        for (; !__range.empty(); __range.pop_front())
            __func(__range.front());
        return octa::move(__func);
    }

    template<typename _R, typename _P>
    bool all_of(_R __range, _P __pred) {
        for (; !__range.empty(); __range.pop_front())
            if (!__pred(__range.front())) return false;
        return true;
    }

    template<typename _R, typename _P>
    bool any_of(_R __range, _P __pred) {
        for (; !__range.empty(); __range.pop_front())
            if (__pred(__range.front())) return true;
        return false;
    }

    template<typename _R, typename _P>
    bool none_of(_R __range, _P __pred) {
        for (; !__range.empty(); __range.pop_front())
            if (__pred(__range.front())) return false;
        return true;
    }

    template<typename _R, typename _T>
    _R find(_R __range, const _T &__v) {
        for (; !__range.empty(); __range.pop_front())
            if (__range.front() == __v)
                break;
        return __range;
    }

    template<typename _R, typename _P>
    _R find_if(_R __range, _P __pred) {
        for (; !__range.empty(); __range.pop_front())
            if (__pred(__range.front()))
                break;
        return __range;
    }

    template<typename _R, typename _P>
    _R find_if_not(_R __range, _P __pred) {
        for (; !__range.empty(); __range.pop_front())
            if (!__pred(__range.front()))
                break;
        return __range;
    }

    template<typename _R, typename _T>
    RangeSize<_R> count(_R __range, const _T &__v) {
        RangeSize<_R> __ret = 0;
        for (; !__range.empty(); __range.pop_front())
            if (__range.front() == __v)
                ++__ret;
        return __ret;
    }

    template<typename _R, typename _P>
    RangeSize<_R> count_if(_R __range, _P __pred) {
        RangeSize<_R> __ret = 0;
        for (; !__range.empty(); __range.pop_front())
            if (__pred(__range.front()))
                ++__ret;
        return __ret;
    }

    template<typename _R, typename _P>
    RangeSize<_R> count_if_not(_R __range, _P __pred) {
        RangeSize<_R> __ret = 0;
        for (; !__range.empty(); __range.pop_front())
            if (!__pred(__range.front()))
                ++__ret;
        return __ret;
    }

    template<typename _R>
    bool equal(_R __range1, _R __range2) {
        for (; !__range1.empty(); __range1.pop_front()) {
            if (__range2.empty() || (__range1.front() != __range2.front()))
                return false;
            __range2.pop_front();
        }
        return __range2.empty();
    }

    /* algos that modify ranges or work with output ranges */

    template<typename _R1, typename _R2>
    _R2 copy(_R1 __irange, _R2 __orange) {
        for (; !__irange.empty(); __irange.pop_front())
            __orange.put(__irange.front());
        return __orange;
    }

    template<typename _R1, typename _R2, typename _P>
    _R2 copy_if(_R1 __irange, _R2 __orange, _P __pred) {
        for (; !__irange.empty(); __irange.pop_front())
            if (__pred(__irange.front()))
                __orange.put(__irange.front());
        return __orange;
    }

    template<typename _R1, typename _R2, typename _P>
    _R2 copy_if_not(_R1 __irange, _R2 __orange, _P __pred) {
        for (; !__irange.empty(); __irange.pop_front())
            if (!__pred(__irange.front()))
                __orange.put(__irange.front());
        return __orange;
    }

    template<typename _R1, typename _R2>
    _R2 move(_R1 __irange, _R2 __orange) {
        for (; !__irange.empty(); __irange.pop_front())
            __orange.put(octa::move(__irange.front()));
        return __orange;
    }

    template<typename _R>
    void reverse(_R __range) {
        while (!__range.empty()) {
            octa::swap(__range.front(), __range.back());
            __range.pop_front();
            __range.pop_back();
        }
    }

    template<typename _R1, typename _R2>
    _R2 reverse_copy(_R1 __irange, _R2 __orange) {
        for (; !__irange.empty(); __irange.pop_back())
            __orange.put(__irange.back());
        return __orange;
    }

    template<typename _R, typename _T>
    void fill(_R __range, const _T &__v) {
        for (; !__range.empty(); __range.pop_front())
            __range.front() = __v;
    }

    template<typename _R, typename _F>
    void generate(_R __range, _F __gen) {
        for (; !__range.empty(); __range.pop_front())
            __range.front() = __gen();
    }

    template<typename _R1, typename _R2>
    octa::Pair<_R1, _R2> swap_ranges(_R1 __range1, _R2 __range2) {
        while (!__range1.empty() && !__range2.empty()) {
            octa::swap(__range1.front(), __range2.front());
            __range1.pop_front();
            __range2.pop_front();
        }
        return octa::Pair<_R1, _R2>(__range1, __range2);
    }

    template<typename _R, typename _T>
    void iota(_R __range, _T __value) {
        for (; !__range.empty(); __range.pop_front())
            __range.front() = __value++;
    }

    template<typename _R, typename _T>
    _T foldl(_R __range, _T __init) {
        for (; !__range.empty(); __range.pop_front())
            __init = __init + __range.front();
        return __init;
    }

    template<typename _R, typename _T, typename _F>
    _T foldl(_R __range, _T __init, _F __func) {
        for (; !__range.empty(); __range.pop_front())
            __init = __func(__init, __range.front());
        return __init;
    }

    template<typename _R, typename _T>
    _T foldr(_R __range, _T __init) {
        for (; !__range.empty(); __range.pop_back())
            __init = __init + __range.back();
        return __init;
    }

    template<typename _R, typename _T, typename _F>
    _T foldr(_R __range, _T __init, _F __func) {
        for (; !__range.empty(); __range.pop_back())
            __init = __func(__init, __range.back());
        return __init;
    }

    template<typename _T, typename _R>
    struct MapRange: InputRange<
        MapRange<_T, _R>, octa::RangeCategory<_T>, _R, _R, octa::RangeSize<_T>
    > {
    private:
        _T __range;
        octa::Function<_R(octa::RangeReference<_T>)> __func;

    public:
        MapRange(): __range(), __func() {}
        template<typename _F>
        MapRange(const _T &__range, const _F &__func):
            __range(__range), __func(__func) {}
        MapRange(const MapRange &__it):
            __range(__it.__range), __func(__it.__func) {}
        MapRange(MapRange &&__it):
            __range(move(__it.__range)), __func(move(__it.__func)) {}

        MapRange &operator=(const MapRange &__v) {
            __range = __v.__range;
            __func  = __v.__func;
            return *this;
        }
        MapRange &operator=(MapRange &&__v) {
            __range = move(__v.__range);
            __func  = move(__v.__func);
            return *this;
        }

        bool empty() const { return __range.empty(); }
        octa::RangeSize<_T> size() const { return __range.size(); }

        bool equals_front(const MapRange &__r) const {
            return __range.equals_front(__r.__range);
        }
        bool equals_back(const MapRange &__r) const {
            return __range.equals_front(__r.__range);
        }

        octa::RangeDifference<_T> distance_front(const MapRange &__r) const {
            return __range.distance_front(__r.__range);
        }
        octa::RangeDifference<_T> distance_back(const MapRange &__r) const {
            return __range.distance_back(__r.__range);
        }

        bool pop_front() { return __range.pop_front(); }
        bool pop_back() { return __range.pop_back(); }

        bool push_front() { return __range.pop_front(); }
        bool push_back() { return __range.push_back(); }

        octa::RangeSize<_T> pop_front_n(octa::RangeSize<_T> __n) {
            __range.pop_front_n(__n);
        }
        octa::RangeSize<_T> pop_back_n(octa::RangeSize<_T> __n) {
            __range.pop_back_n(__n);
        }

        octa::RangeSize<_T> push_front_n(octa::RangeSize<_T> __n) {
            return __range.push_front_n(__n);
        }
        octa::RangeSize<_T> push_back_n(octa::RangeSize<_T> __n) {
            return __range.push_back_n(__n);
        }

        _R front() const { return __func(__range.front()); }
        _R back() const { return __func(__range.back()); }

        _R operator[](octa::RangeSize<_T> __idx) const {
            return __func(__range[__idx]);
        }

        MapRange<_T, _R> slice(octa::RangeSize<_T> __start,
                               octa::RangeSize<_T> __end) {
            return MapRange<_T, _R>(__range.slice(__start, __end), __func);
        }
    };

    template<typename _R, typename _F> using __OctaMapReturnType
        = decltype(declval<_F>()(octa::declval<octa::RangeReference<_R>>()));

    template<typename _R, typename _F>
    MapRange<_R, __OctaMapReturnType<_R, _F>> map(_R __range, _F __func) {
        return octa::MapRange<_R, __OctaMapReturnType<_R, _F>>(__range, __func);
    }

    template<typename _T>
    struct FilterRange: InputRange<
        FilterRange<_T>, octa::CommonType<octa::RangeCategory<_T>,
                                          octa::ForwardRangeTag>,
        octa::RangeValue<_T>, octa::RangeReference<_T>, octa::RangeSize<_T>
    > {
    private:
        _T __range;
        octa::Function<bool(octa::RangeReference<_T>)> __pred;

        void advance_valid() {
            while (!__range.empty() && !__pred(front())) __range.pop_front();
        }

    public:
        FilterRange(): __range(), __pred() {}

        template<typename _P>
        FilterRange(const _T &__range, const _P &__pred): __range(__range),
        __pred(__pred) {
            advance_valid();
        }
        FilterRange(const FilterRange &__it): __range(__it.__range),
        __pred(__it.__pred) {
            advance_valid();
        }
        FilterRange(FilterRange &&__it): __range(move(__it.__range)),
        __pred(move(__it.__pred)) {
            advance_valid();
        }

        FilterRange &operator=(const FilterRange &__v) {
            __range = __v.__range;
            __pred  = __v.__pred;
            advance_valid();
            return *this;
        }
        FilterRange &operator=(FilterRange &&__v) {
            __range = move(__v.__range);
            __pred  = move(__v.__pred);
            advance_valid();
            return *this;
        }

        bool empty() const { return __range.empty(); }

        bool equals_front(const FilterRange &__r) const {
            return __range.equals_front(__r.__range);
        }

        bool pop_front() {
            bool __ret = __range.pop_front();
            advance_valid();
            return __ret;
        }
        bool push_front() {
            _T __tmp = __range;
            if (!__tmp.push_front()) return false;
            while (!pred(__tmp.front()))
                if (!__tmp.push_front())
                    return false;
            __range = __tmp;
            return true;
        }

        octa::RangeReference<_T> front() const { return __range.front(); }
    };

    template<typename _R, typename _P>
    FilterRange<_R> filter(_R __range, _P __pred) {
        return octa::FilterRange<_R>(__range, __pred);
    }
}

#endif