/* Algorithms for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_ALGORITHM_HH
#define OSTD_ALGORITHM_HH

#include <math.h>

#include "ostd/functional.hh"
#include "ostd/range.hh"
#include "ostd/utility.hh"
#include "ostd/initializer_list.hh"

namespace ostd {

/* partitioning */

template<typename R, typename U>
inline R partition(R range, U pred) {
    R ret = range;
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            detail::swap_adl(range.front(), ret.front());
            ret.pop_front();
        }
    }
    return ret;
}

template<typename F>
inline auto partition(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return partition(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename P>
inline bool is_partitioned(R range, P pred) {
    for (; !range.empty() && pred(range.front()); range.pop_front());
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return false;
    return true;
}

template<typename F>
inline auto is_partitioned(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return is_partitioned(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

/* sorting */

namespace detail {
    template<typename R, typename C>
    static void insort(R range, C compare) {
        RangeSize<R> rlen = range.size();
        for (RangeSize<R> i = 1; i < rlen; ++i) {
            RangeSize<R> j = i;
            RangeValue<R> v(move(range[i]));
            while (j > 0 && !compare(range[j - 1], v)) {
                range[j] = range[j - 1];
                --j;
            }
            range[j] = move(v);
        }
    }

    template<typename R, typename C>
    static void hs_sift_down(R range, RangeSize<R> s,
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
                detail::swap_adl(range[r], range[sw]);
                r = sw;
            } else return;
        }
    }

    template<typename R, typename C>
    static void heapsort(R range, C compare) {
        RangeSize<R> len = range.size();
        RangeSize<R> st = (len - 2) / 2;
        for (;;) {
            detail::hs_sift_down(range, st, len - 1, compare);
            if (st-- == 0) break;
        }
        RangeSize<R> e = len - 1;
        while (e > 0) {
            detail::swap_adl(range[e], range[0]);
            --e;
            detail::hs_sift_down(range, 0, e, compare);
        }
    }

    template<typename R, typename C>
    static void introloop(R range, C compare, RangeSize<R> depth) {
        if (range.size() <= 10) {
            detail::insort(range, compare);
            return;
        }
        if (depth == 0) {
            detail::heapsort(range, compare);
            return;
        }
        detail::swap_adl(range[range.size() / 2], range.back());
        RangeSize<R> pi = 0;
        R pr = range;
        pr.pop_back();
        for (; !pr.empty(); pr.pop_front()) {
            if (compare(pr.front(), range.back()))
                detail::swap_adl(pr.front(), range[pi++]);
        }
        detail::swap_adl(range[pi], range.back());
        detail::introloop(range.slice(0, pi), compare, depth - 1);
        detail::introloop(range.slice(pi + 1, range.size()), compare,
            depth - 1);
    }

    template<typename R, typename C>
    inline void introsort(R range, C compare) {
        detail::introloop(range, compare, RangeSize<R>(2
            * (log(range.size()) / log(2))));
    }
} /* namespace detail */

template<typename R, typename C>
inline R sort_cmp(R range, C compare) {
    detail::introsort(range, compare);
    return range;
}
template<typename C>
inline auto sort_cmp(C &&compare) {
    return [compare = forward<C>(compare)](auto &&obj) mutable {
        return sort_cmp(forward<decltype(obj)>(obj), forward<C>(compare));
    };
}

template<typename R>
inline R sort(R range) {
    return sort_cmp(range, Less<RangeValue<R>>());
}
inline auto sort() {
    return [](auto &&obj) { return sort(forward<decltype(obj)>(obj)); };
}

/* min/max(_element) */

template<typename T>
inline const T &min(const T &a, const T &b) {
    return (a < b) ? a : b;
}
template<typename T, typename C>
inline const T &min_cmp(const T &a, const T &b, C compare) {
    return compare(a, b) ? a : b;
}

template<typename T>
inline const T &max(const T &a, const T &b) {
    return (a < b) ? b : a;
}
template<typename T, typename C>
inline const T &max_cmp(const T &a, const T &b, C compare) {
    return compare(a, b) ? b : a;
}

template<typename R>
inline R min_element(R range) {
    R r = range;
    for (; !range.empty(); range.pop_front())
        if (ostd::min(r.front(), range.front()) == range.front())
            r = range;
    return r;
}
template<typename R, typename C>
inline R min_element_cmp(R range, C compare) {
    R r = range;
    for (; !range.empty(); range.pop_front())
        if (ostd::min_cmp(r.front(), range.front(), compare) == range.front())
            r = range;
    return r;
}
inline auto min_element() {
    return [](auto &&obj) { return min_element(forward<decltype(obj)>(obj)); };
}
template<typename C>
inline auto min_element_cmp(C &&compare) {
    return [compare = forward<C>(compare)](auto &&obj) mutable {
        return min_element_cmp(forward<decltype(obj)>(obj), forward<C>(compare));
    };
}

template<typename R>
inline R max_element(R range) {
    R r = range;
    for (; !range.empty(); range.pop_front())
        if (ostd::max(r.front(), range.front()) == range.front())
            r = range;
    return r;
}
template<typename R, typename C>
inline R max_element_cmp(R range, C compare) {
    R r = range;
    for (; !range.empty(); range.pop_front())
        if (ostd::max_cmp(r.front(), range.front(), compare) == range.front())
            r = range;
    return r;
}
inline auto max_element() {
    return [](auto &&obj) { return max_element(forward<decltype(obj)>(obj)); };
}
template<typename C>
inline auto max_element_cmp(C &&compare) {
    return [compare = forward<C>(compare)](auto &&obj) mutable {
        return max_element_cmp(forward<decltype(obj)>(obj), forward<C>(compare));
    };
}

template<typename T>
inline T min(std::initializer_list<T> il) {
    return ostd::min_element(ostd::iter(il)).front();
}
template<typename T, typename C>
inline T min_cmp(std::initializer_list<T> il, C compare) {
    return ostd::min_element_cmp(ostd::iter(il), compare).front();
}

template<typename T>
inline T max(std::initializer_list<T> il) {
    return ostd::max_element(ostd::iter(il)).front();
}

template<typename T, typename C>
inline T max_cmp(std::initializer_list<T> il, C compare) {
    return ostd::max_element_cmp(ostd::iter(il), compare).front();
}

/* clamp */

template<typename T, typename U>
inline T clamp(const T &v, const U &lo, const U &hi) {
    return ostd::max(T(lo), ostd::min(v, T(hi)));
}

template<typename T, typename U, typename C>
inline T clamp(const T &v, const U &lo, const U &hi, C compare) {
    return ostd::max_cmp(T(lo), ostd::min_cmp(v, T(hi), compare), compare);
}

/* lexicographical compare */

template<typename R1, typename R2>
inline bool lexicographical_compare(R1 range1, R2 range2) {
    while (!range1.empty() && !range2.empty()) {
        if (range1.front() < range2.front()) return true;
        if (range2.front() < range1.front()) return false;
        range1.pop_front();
        range2.pop_front();
    }
    return (range1.empty() && !range2.empty());
}
template<typename R>
inline auto lexicographical_compare(R &&range) {
    return [range = forward<R>(range)](auto &&obj) mutable {
        return lexicographical_compare(forward<decltype(obj)>(obj), forward<R>(range));
    };
}

template<typename R1, typename R2, typename C>
inline bool lexicographical_compare_cmp(R1 range1, R2 range2, C compare) {
    while (!range1.empty() && !range2.empty()) {
        if (compare(range1.front(), range2.front())) return true;
        if (compare(range2.front(), range1.front())) return false;
        range1.pop_front();
        range2.pop_front();
    }
    return (range1.empty() && !range2.empty());
}
template<typename R, typename C>
inline auto lexicographical_compare_cmp(R &&range, C &&compare) {
    return [range = forward<R>(range), compare = forward<C>(compare)](auto &&obj) mutable {
        return lexicographical_compare_cmp(forward<decltype(obj)>(obj),
            forward<R>(range), forward<C>(compare));
    };
}

/* algos that don't change the range */

template<typename R, typename F>
inline F for_each(R range, F func) {
    for (; !range.empty(); range.pop_front())
        func(range.front());
    return move(func);
}

template<typename F>
inline auto for_each(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return for_each(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename P>
inline bool all_of(R range, P pred) {
    for (; !range.empty(); range.pop_front())
        if (!pred(range.front())) return false;
    return true;
}

template<typename F>
inline auto all_of(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return all_of(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename P>
inline bool any_of(R range, P pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return true;
    return false;
}

template<typename F>
inline auto any_of(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return any_of(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename P>
inline bool none_of(R range, P pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return false;
    return true;
}

template<typename F>
inline auto none_of(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return none_of(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename T>
inline R find(R range, const T &v) {
    for (; !range.empty(); range.pop_front())
        if (range.front() == v)
            break;
    return range;
}

template<typename T>
inline auto find(T &&v) {
    return [v = forward<T>(v)](auto &&obj) mutable {
        return find(forward<decltype(obj)>(obj), forward<T>(v));
    };
}

template<typename R, typename T>
inline R find_last(R range, const T &v) {
    range = find(range, v);
    if (!range.empty()) for (;;) {
        R prev = range;
        prev.pop_front();
        R r = find(prev, v);
        if (r.empty())
            break;
        range = r;
    }
    return range;
}

template<typename T>
inline auto find_last(T &&v) {
    return [v = forward<T>(v)](auto &&obj) mutable {
        return find_last(forward<decltype(obj)>(obj), forward<T>(v));
    };
}

template<typename R, typename P>
inline R find_if(R range, P pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front()))
            break;
    return range;
}

template<typename F>
inline auto find_if(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return find_if(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename P>
inline R find_if_not(R range, P pred) {
    for (; !range.empty(); range.pop_front())
        if (!pred(range.front()))
            break;
    return range;
}

template<typename F>
inline auto find_if_not(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return find_if_not(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R1, typename R2, typename C>
inline R1 find_one_of_cmp(R1 range, R2 values, C compare) {
    for (; !range.empty(); range.pop_front())
        for (R2 rv = values; !rv.empty(); rv.pop_front())
            if (compare(range.front(), rv.front()))
                return range;
    return range;
}
template<typename R, typename C>
inline auto find_one_of_cmp(R &&values, C &&compare) {
    return [values = forward<R>(values), compare = forward<C>(compare)](auto &&obj) mutable {
        return find_one_of_cmp(forward<decltype(obj)>(obj),
            forward<R>(values), forward<C>(compare));
    };
}

template<typename R1, typename R2>
inline R1 find_one_of(R1 range, R2 values) {
    for (; !range.empty(); range.pop_front())
        for (R2 rv = values; !rv.empty(); rv.pop_front())
            if (range.front() == rv.front())
                return range;
    return range;
}
template<typename R>
inline auto find_one_of(R &&values) {
    return [values = forward<R>(values)](auto &&obj) mutable {
        return find_one_of(forward<decltype(obj)>(obj), forward<R>(values));
    };
}

template<typename R, typename T>
inline RangeSize<R> count(R range, const T &v) {
    RangeSize<R> ret = 0;
    for (; !range.empty(); range.pop_front())
        if (range.front() == v)
            ++ret;
    return ret;
}

template<typename T>
inline auto count(T &&v) {
    return [v = forward<T>(v)](auto &&obj) mutable {
        return count(forward<decltype(obj)>(obj), forward<T>(v));
    };
}

template<typename R, typename P>
inline RangeSize<R> count_if(R range, P pred) {
    RangeSize<R> ret = 0;
    for (; !range.empty(); range.pop_front())
        if (pred(range.front()))
            ++ret;
    return ret;
}

template<typename F>
inline auto count_if(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return count_if(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R, typename P>
inline RangeSize<R> count_if_not(R range, P pred) {
    RangeSize<R> ret = 0;
    for (; !range.empty(); range.pop_front())
        if (!pred(range.front()))
            ++ret;
    return ret;
}

template<typename F>
inline auto count_if_not(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return count_if_not(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename R>
inline bool equal(R range1, R range2) {
    for (; !range1.empty(); range1.pop_front()) {
        if (range2.empty() || (range1.front() != range2.front()))
            return false;
        range2.pop_front();
    }
    return range2.empty();
}

template<typename R>
inline auto equal(R &&range) {
    return [range = forward<R>(range)](auto &&obj) mutable {
        return equal(forward<decltype(obj)>(obj), forward<R>(range));
    };
}

template<typename R>
R slice_until(R range1, R range2) {
    return range1.slice(0, range1.distance_front(range2));
}

template<typename R>
inline auto slice_until(R &&range) {
    return [range = forward<R>(range)](auto &&obj) mutable {
        return slice_until(forward<decltype(obj)>(obj), forward<R>(range));
    };
}

/* algos that modify ranges or work with output ranges */

template<typename R1, typename R2>
inline R2 copy(R1 irange, R2 orange) {
    for (; !irange.empty(); irange.pop_front())
        orange.put(irange.front());
    return orange;
}

template<typename R1, typename R2, typename P>
inline R2 copy_if(R1 irange, R2 orange, P pred) {
    for (; !irange.empty(); irange.pop_front())
        if (pred(irange.front()))
            orange.put(irange.front());
    return orange;
}

template<typename R1, typename R2, typename P>
inline R2 copy_if_not(R1 irange, R2 orange, P pred) {
    for (; !irange.empty(); irange.pop_front())
        if (!pred(irange.front()))
            orange.put(irange.front());
    return orange;
}

template<typename R1, typename R2>
inline R2 move(R1 irange, R2 orange) {
    for (; !irange.empty(); irange.pop_front())
        orange.put(move(irange.front()));
    return orange;
}

template<typename R>
inline void reverse(R range) {
    while (!range.empty()) {
        detail::swap_adl(range.front(), range.back());
        range.pop_front();
        range.pop_back();
    }
}

template<typename R1, typename R2>
inline R2 reverse_copy(R1 irange, R2 orange) {
    for (; !irange.empty(); irange.pop_back())
        orange.put(irange.back());
    return orange;
}

template<typename R, typename T>
inline void fill(R range, const T &v) {
    for (; !range.empty(); range.pop_front())
        range.front() = v;
}

template<typename R, typename F>
inline void generate(R range, F gen) {
    for (; !range.empty(); range.pop_front())
        range.front() = gen();
}

template<typename R1, typename R2>
inline Pair<R1, R2> swap_ranges(R1 range1, R2 range2) {
    while (!range1.empty() && !range2.empty()) {
        detail::swap_adl(range1.front(), range2.front());
        range1.pop_front();
        range2.pop_front();
    }
    return ostd::make_pair(range1, range2);
}

template<typename R, typename T>
inline void iota(R range, T value) {
    for (; !range.empty(); range.pop_front())
        range.front() = value++;
}

template<typename R, typename T>
inline T foldl(R range, T init) {
    for (; !range.empty(); range.pop_front())
        init = init + range.front();
    return init;
}

template<typename R, typename T, typename F>
inline T foldl_f(R range, T init, F func) {
    for (; !range.empty(); range.pop_front())
        init = func(init, range.front());
    return init;
}

template<typename T>
inline auto foldl(T &&init) {
    return [init = forward<T>(init)](auto &&obj) mutable {
        return foldl(forward<decltype(obj)>(obj), forward<T>(init));
    };
}
template<typename T, typename F>
inline auto foldl_f(T &&init, F &&func) {
    return [init = forward<T>(init), func = forward<F>(func)](auto &&obj) mutable {
        return foldl_f(forward<decltype(obj)>(obj), forward<T>(init), forward<F>(func));
    };
}

template<typename R, typename T>
inline T foldr(R range, T init) {
    for (; !range.empty(); range.pop_back())
        init = init + range.back();
    return init;
}

template<typename R, typename T, typename F>
inline T foldr_f(R range, T init, F func) {
    for (; !range.empty(); range.pop_back())
        init = func(init, range.back());
    return init;
}

template<typename T>
inline auto foldr(T &&init) {
    return [init = forward<T>(init)](auto &&obj) mutable {
        return foldr(forward<decltype(obj)>(obj), forward<T>(init));
    };
}
template<typename T, typename F>
inline auto foldr_f(T &&init, F &&func) {
    return [init = forward<T>(init), func = forward<F>(func)](auto &&obj) mutable {
        return foldr_f(forward<decltype(obj)>(obj), forward<T>(init), forward<F>(func));
    };
}

template<typename T, typename F, typename R>
struct MapRange: InputRange<
    MapRange<T, F, R>, RangeCategory<T>, R, R, RangeSize<T>
> {
private:
    T p_range;
    Decay<F> p_func;

public:
    MapRange() = delete;
    template<typename FF>
    MapRange(const T &range, FF &&func):
        p_range(range), p_func(forward<FF>(func)) {}
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
    RangeSize<T> size() const { return p_range.size(); }

    bool equals_front(const MapRange &r) const {
        return p_range.equals_front(r.p_range);
    }
    bool equals_back(const MapRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    RangeDifference<T> distance_front(const MapRange &r) const {
        return p_range.distance_front(r.p_range);
    }
    RangeDifference<T> distance_back(const MapRange &r) const {
        return p_range.distance_back(r.p_range);
    }

    bool pop_front() { return p_range.pop_front(); }
    bool pop_back() { return p_range.pop_back(); }

    bool push_front() { return p_range.pop_front(); }
    bool push_back() { return p_range.push_back(); }

    RangeSize<T> pop_front_n(RangeSize<T> n) {
        p_range.pop_front_n(n);
    }
    RangeSize<T> pop_back_n(RangeSize<T> n) {
        p_range.pop_back_n(n);
    }

    RangeSize<T> push_front_n(RangeSize<T> n) {
        return p_range.push_front_n(n);
    }
    RangeSize<T> push_back_n(RangeSize<T> n) {
        return p_range.push_back_n(n);
    }

    R front() const { return p_func(p_range.front()); }
    R back() const { return p_func(p_range.back()); }

    R operator[](RangeSize<T> idx) const {
        return p_func(p_range[idx]);
    }

    MapRange slice(RangeSize<T> start, RangeSize<T> end) {
        return MapRange(p_range.slice(start, end), p_func);
    }
};

namespace detail {
    template<typename R, typename F> using MapReturnType
        = decltype(declval<F>()(declval<RangeReference<R>>()));
}

template<typename R, typename F>
inline MapRange<R, F, detail::MapReturnType<R, F>> map(R range, F func) {
    return MapRange<R, F, detail::MapReturnType<R, F>>(range, move(func));
}

template<typename F>
inline auto map(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return map(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

template<typename T, typename F>
struct FilterRange: InputRange<
    FilterRange<T, F>, CommonType<RangeCategory<T>, ForwardRangeTag>,
    RangeValue<T>, RangeReference<T>, RangeSize<T>
> {
private:
    T p_range;
    Decay<F> p_pred;

    void advance_valid() {
        while (!p_range.empty() && !p_pred(front())) p_range.pop_front();
    }

public:
    FilterRange() = delete;
    template<typename P>
    FilterRange(const T &range, P &&pred): p_range(range),
    p_pred(forward<P>(pred)) {
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

    RangeReference<T> front() const { return p_range.front(); }
};

namespace detail {
    template<typename R, typename P> using FilterPred
        = EnableIf<IsSame<
            decltype(declval<P>()(declval<RangeReference<R>>())), bool
        >, P>;
}

template<typename R, typename P>
inline FilterRange<R, detail::FilterPred<R, P>> filter(R range, P pred) {
    return FilterRange<R, P>(range, move(pred));
}

template<typename F>
inline auto filter(F &&func) {
    return [func = forward<F>(func)](auto &&obj) mutable {
        return filter(forward<decltype(obj)>(obj), forward<F>(func));
    };
}

} /* namespace ostd */

#endif
