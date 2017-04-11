/* Algorithms for libostd.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#ifndef OSTD_ALGORITHM_HH
#define OSTD_ALGORITHM_HH

#include <cmath>
#include <utility>
#include <functional>
#include <type_traits>
#include <algorithm>

#include "ostd/range.hh"

namespace ostd {

/* partitioning */

template<typename R, typename U>
inline R partition(R range, U pred) {
    R ret = range;
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            using std::swap;
            swap(range.front(), ret.front());
            ret.pop_front();
        }
    }
    return ret;
}

template<typename F>
inline auto partition(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return partition(obj, std::forward<F>(func));
    };
}

template<typename R, typename P>
inline bool is_partitioned(R range, P pred) {
    for (; !range.empty() && pred(range.front()); range.pop_front());
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            return false;
        }
    }
    return true;
}

template<typename F>
inline auto is_partitioned(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return is_partitioned(obj, std::forward<F>(func));
    };
}

/* sorting */

namespace detail {
    template<typename R, typename C>
    static void insort(R range, C &compare) {
        range_size_t<R> rlen = range.size();
        for (range_size_t<R> i = 1; i < rlen; ++i) {
            range_size_t<R> j = i;
            range_value_t<R> v{std::move(range[i])};
            while (j > 0 && !compare(range[j - 1], v)) {
                range[j] = std::move(range[j - 1]);
                --j;
            }
            range[j] = std::move(v);
        }
    }

    template<typename R, typename C>
    static void hs_sift_down(
        R range, range_size_t<R> s, range_size_t<R> e, C &compare
    ) {
        range_size_t<R> r = s;
        while ((r * 2 + 1) <= e) {
            range_size_t<R> ch = r * 2 + 1;
            range_size_t<R> sw = r;
            if (compare(range[sw], range[ch])) {
                sw = ch;
            }
            if (((ch + 1) <= e) && compare(range[sw], range[ch + 1])) {
                sw = ch + 1;
            }
            if (sw != r) {
                using std::swap;
                swap(range[r], range[sw]);
                r = sw;
            } else {
                return;
            }
        }
    }

    template<typename R, typename C>
    static void heapsort(R range, C &compare) {
        range_size_t<R> len = range.size();
        range_size_t<R> st = (len - 2) / 2;
        for (;;) {
            detail::hs_sift_down(range, st, len - 1, compare);
            if (st-- == 0) {
                break;
            }
        }
        range_size_t<R> e = len - 1;
        while (e > 0) {
            using std::swap;
            swap(range[e], range[0]);
            --e;
            detail::hs_sift_down(range, 0, e, compare);
        }
    }

    template<typename R, typename C>
    static void introloop(R range, C &compare, range_size_t<R> depth) {
        using std::swap;
        if (range.size() <= 10) {
            detail::insort(range, compare);
            return;
        }
        if (depth == 0) {
            detail::heapsort(range, compare);
            return;
        }
        swap(range[range.size() / 2], range.back());
        range_size_t<R> pi = 0;
        R pr = range;
        pr.pop_back();
        for (; !pr.empty(); pr.pop_front()) {
            if (compare(pr.front(), range.back())) {
                swap(pr.front(), range[pi++]);
            }
        }
        swap(range[pi], range.back());
        detail::introloop(range.slice(0, pi), compare, depth - 1);
        detail::introloop(range.slice(pi + 1), compare, depth - 1);
    }

    template<typename R, typename C>
    inline void introsort(R range, C &compare) {
        detail::introloop(range, compare, static_cast<range_size_t<R>>(
            2 * (std::log(range.size()) / std::log(2))
        ));
    }
} /* namespace detail */

template<typename R, typename C>
inline R sort_cmp(R range, C compare) {
    detail::introsort(range, compare);
    return range;
}
template<typename C>
inline auto sort_cmp(C &&compare) {
    return [compare = std::forward<C>(compare)](auto &obj) mutable {
        return sort_cmp(obj, std::forward<C>(compare));
    };
}

template<typename R>
inline R sort(R range) {
    return sort_cmp(range, std::less<range_value_t<R>>());
}
inline auto sort() {
    return [](auto &obj) { return sort(obj); };
}

/* min/max(_element) */

template<typename R>
inline R min_element(R range) {
    R r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::min(r.front(), range.front()) == range.front()) {
            r = range;
        }
    }
    return r;
}
template<typename R, typename C>
inline R min_element_cmp(R range, C compare) {
    R r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::min(r.front(), range.front(), compare) == range.front()) {
            r = range;
        }
    }
    return r;
}
inline auto min_element() {
    return [](auto &obj) {
        return min_element(obj);
    };
}
template<typename C>
inline auto min_element_cmp(C &&compare) {
    return [compare = std::forward<C>(compare)](auto &obj) mutable {
        return min_element_cmp(obj, std::forward<C>(compare));
    };
}

template<typename R>
inline R max_element(R range) {
    R r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::max(r.front(), range.front()) == range.front()) {
            r = range;
        }
    }
    return r;
}
template<typename R, typename C>
inline R max_element_cmp(R range, C compare) {
    R r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::max(r.front(), range.front(), compare) == range.front()) {
            r = range;
        }
    }
    return r;
}
inline auto max_element() {
    return [](auto &obj) {
        return max_element(obj);
    };
}
template<typename C>
inline auto max_element_cmp(C &&compare) {
    return [compare = std::forward<C>(compare)](auto &obj) mutable {
        return max_element_cmp(obj, std::forward<C>(compare));
    };
}

/* lexicographical compare */

template<typename R1, typename R2>
inline bool lexicographical_compare(R1 range1, R2 range2) {
    while (!range1.empty() && !range2.empty()) {
        if (range1.front() < range2.front()) {
            return true;
        }
        if (range2.front() < range1.front()) {
            return false;
        }
        range1.pop_front();
        range2.pop_front();
    }
    return (range1.empty() && !range2.empty());
}
template<typename R>
inline auto lexicographical_compare(R &&range) {
    return [range = std::forward<R>(range)](auto &obj) mutable {
        return lexicographical_compare(obj, std::forward<R>(range));
    };
}

template<typename R1, typename R2, typename C>
inline bool lexicographical_compare_cmp(R1 range1, R2 range2, C compare) {
    while (!range1.empty() && !range2.empty()) {
        if (compare(range1.front(), range2.front())) {
            return true;
        }
        if (compare(range2.front(), range1.front())) {
            return false;
        }
        range1.pop_front();
        range2.pop_front();
    }
    return (range1.empty() && !range2.empty());
}
template<typename R, typename C>
inline auto lexicographical_compare_cmp(R &&range, C &&compare) {
    return [
        range = std::forward<R>(range), compare = std::forward<C>(compare)
    ](auto &obj) mutable {
        return lexicographical_compare_cmp(
            obj, std::forward<R>(range), std::forward<C>(compare)
        );
    };
}

/* algos that don't change the range */

template<typename R, typename F>
inline F for_each(R range, F func) {
    for (; !range.empty(); range.pop_front()) {
        func(range.front());
    }
    return std::move(func);
}

template<typename F>
inline auto for_each(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return for_each(obj, std::forward<F>(func));
    };
}

template<typename R, typename P>
inline bool all_of(R range, P pred) {
    for (; !range.empty(); range.pop_front()) {
        if (!pred(range.front())) {
            return false;
        }
    }
    return true;
}

template<typename F>
inline auto all_of(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return all_of(obj, std::forward<F>(func));
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
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return any_of(obj, std::forward<F>(func));
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
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return none_of(obj, std::forward<F>(func));
    };
}

template<typename R, typename T>
inline R find(R range, T const &v) {
    for (; !range.empty(); range.pop_front()) {
        if (range.front() == v) {
            break;
        }
    }
    return range;
}

template<typename T>
inline auto find(T &&v) {
    return [v = std::forward<T>(v)](auto &obj) mutable {
        return find(obj, std::forward<T>(v));
    };
}

template<typename R, typename T>
inline R find_last(R range, T const &v) {
    range = find(range, v);
    if (!range.empty()) {
        for (;;) {
            R prev = range;
            prev.pop_front();
            R r = find(prev, v);
            if (r.empty()) {
                break;
            }
            range = r;
        }
    }
    return range;
}

template<typename T>
inline auto find_last(T &&v) {
    return [v = std::forward<T>(v)](auto &obj) mutable {
        return find_last(obj, std::forward<T>(v));
    };
}

template<typename R, typename P>
inline R find_if(R range, P pred) {
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            break;
        }
    }
    return range;
}

template<typename F>
inline auto find_if(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return find_if(obj, std::forward<F>(func));
    };
}

template<typename R, typename P>
inline R find_if_not(R range, P pred) {
    for (; !range.empty(); range.pop_front()) {
        if (!pred(range.front())) {
            break;
        }
    }
    return range;
}

template<typename F>
inline auto find_if_not(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return find_if_not(obj, std::forward<F>(func));
    };
}

template<typename R1, typename R2, typename C>
inline R1 find_one_of_cmp(R1 range, R2 values, C compare) {
    for (; !range.empty(); range.pop_front()) {
        for (R2 rv = values; !rv.empty(); rv.pop_front()) {
            if (compare(range.front(), rv.front())) {
                return range;
            }
        }
    }
    return range;
}
template<typename R, typename C>
inline auto find_one_of_cmp(R &&values, C &&compare) {
    return [
        values = std::forward<R>(values), compare = std::forward<C>(compare)
    ](auto &obj) mutable {
        return find_one_of_cmp(
            obj, std::forward<R>(values), std::forward<C>(compare)
        );
    };
}

template<typename R1, typename R2>
inline R1 find_one_of(R1 range, R2 values) {
    for (; !range.empty(); range.pop_front()) {
        for (R2 rv = values; !rv.empty(); rv.pop_front()) {
            if (range.front() == rv.front()) {
                return range;
            }
        }
    }
    return range;
}
template<typename R>
inline auto find_one_of(R &&values) {
    return [values = std::forward<R>(values)](auto &obj) mutable {
        return find_one_of(obj, std::forward<R>(values));
    };
}

template<typename R, typename T>
inline range_size_t<R> count(R range, T const &v) {
    range_size_t<R> ret = 0;
    for (; !range.empty(); range.pop_front()) {
        if (range.front() == v) {
            ++ret;
        }
    }
    return ret;
}

template<typename T>
inline auto count(T &&v) {
    return [v = std::forward<T>(v)](auto &obj) mutable {
        return count(obj, std::forward<T>(v));
    };
}

template<typename R, typename P>
inline range_size_t<R> count_if(R range, P pred) {
    range_size_t<R> ret = 0;
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            ++ret;
        }
    }
    return ret;
}

template<typename F>
inline auto count_if(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return count_if(obj, std::forward<F>(func));
    };
}

template<typename R, typename P>
inline range_size_t<R> count_if_not(R range, P pred) {
    range_size_t<R> ret = 0;
    for (; !range.empty(); range.pop_front()) {
        if (!pred(range.front())) {
            ++ret;
        }
    }
    return ret;
}

template<typename F>
inline auto count_if_not(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return count_if_not(obj, std::forward<F>(func));
    };
}

template<typename R>
inline bool equal(R range1, R range2) {
    for (; !range1.empty(); range1.pop_front()) {
        if (range2.empty() || (range1.front() != range2.front())) {
            return false;
        }
        range2.pop_front();
    }
    return range2.empty();
}

template<typename R>
inline auto equal(R &&range) {
    return [range = std::forward<R>(range)](auto &obj) mutable {
        return equal(obj, std::forward<R>(range));
    };
}

/* algos that modify ranges or work with output ranges */

template<typename R1, typename R2>
inline R2 copy(R1 irange, R2 orange) {
    range_put_all(orange, irange);
    return orange;
}

template<typename R1, typename R2, typename P>
inline R2 copy_if(R1 irange, R2 orange, P pred) {
    for (; !irange.empty(); irange.pop_front()) {
        if (pred(irange.front())) {
            orange.put(irange.front());
        }
    }
    return orange;
}

template<typename R1, typename R2, typename P>
inline R2 copy_if_not(R1 irange, R2 orange, P pred) {
    for (; !irange.empty(); irange.pop_front()) {
        if (!pred(irange.front())) {
            orange.put(irange.front());
        }
    }
    return orange;
}

template<typename R1, typename R2>
inline R2 move(R1 irange, R2 orange) {
    for (; !irange.empty(); irange.pop_front()) {
        orange.put(std::move(irange.front()));
    }
    return orange;
}

template<typename R>
inline void reverse(R range) {
    while (!range.empty()) {
        using std::swap;
        swap(range.front(), range.back());
        range.pop_front();
        range.pop_back();
    }
}

template<typename R1, typename R2>
inline R2 reverse_copy(R1 irange, R2 orange) {
    for (; !irange.empty(); irange.pop_back()) {
        orange.put(irange.back());
    }
    return orange;
}

template<typename R, typename T>
inline void fill(R range, T const &v) {
    for (; !range.empty(); range.pop_front()) {
        range.front() = v;
    }
}

template<typename R, typename F>
inline void generate(R range, F gen) {
    for (; !range.empty(); range.pop_front()) {
        range.front() = gen();
    }
}

template<typename R1, typename R2>
inline std::pair<R1, R2> swap_ranges(R1 range1, R2 range2) {
    while (!range1.empty() && !range2.empty()) {
        using std::swap;
        swap(range1.front(), range2.front());
        range1.pop_front();
        range2.pop_front();
    }
    return std::make_pair(range1, range2);
}

template<typename R, typename T>
inline void iota(R range, T value) {
    for (; !range.empty(); range.pop_front()) {
        range.front() = value++;
    }
}

template<typename R, typename T>
inline T foldl(R range, T init) {
    for (; !range.empty(); range.pop_front()) {
        init = init + range.front();
    }
    return init;
}

template<typename R, typename T, typename F>
inline T foldl_f(R range, T init, F func) {
    for (; !range.empty(); range.pop_front()) {
        init = func(init, range.front());
    }
    return init;
}

template<typename T>
inline auto foldl(T &&init) {
    return [init = std::forward<T>(init)](auto &obj) mutable {
        return foldl(obj, std::forward<T>(init));
    };
}
template<typename T, typename F>
inline auto foldl_f(T &&init, F &&func) {
    return [
        init = std::forward<T>(init), func = std::forward<F>(func)
    ](auto &obj) mutable {
        return foldl_f(obj, std::forward<T>(init), std::forward<F>(func));
    };
}

template<typename R, typename T>
inline T foldr(R range, T init) {
    for (; !range.empty(); range.pop_back()) {
        init = init + range.back();
    }
    return init;
}

template<typename R, typename T, typename F>
inline T foldr_f(R range, T init, F func) {
    for (; !range.empty(); range.pop_back()) {
        init = func(init, range.back());
    }
    return init;
}

template<typename T>
inline auto foldr(T &&init) {
    return [init = std::forward<T>(init)](auto &obj) mutable {
        return foldr(obj, std::forward<T>(init));
    };
}
template<typename T, typename F>
inline auto foldr_f(T &&init, F &&func) {
    return [
        init = std::forward<T>(init), func = std::forward<F>(func)
    ](auto &obj) mutable {
        return foldr_f(obj, std::forward<T>(init), std::forward<F>(func));
    };
}

template<typename T, typename F, typename R>
struct map_range: input_range<map_range<T, F, R>> {
    using range_category  = std::common_type_t<
        range_category_t<T>, finite_random_access_range_tag
    >;
    using value_type      = R;
    using reference       = R;
    using size_type       = range_size_t<T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;
    std::decay_t<F> p_func;

public:
    map_range() = delete;
    template<typename FF>
    map_range(T const &range, FF &&func):
        p_range(range), p_func(std::forward<FF>(func)) {}

    bool empty() const { return p_range.empty(); }
    size_type size() const { return p_range.size(); }

    void pop_front() { p_range.pop_front(); }
    void pop_back() { p_range.pop_back(); }

    R front() const { return p_func(p_range.front()); }
    R back() const { return p_func(p_range.back()); }

    R operator[](size_type idx) const {
        return p_func(p_range[idx]);
    }

    map_range slice(size_type start, size_type end) const {
        return map_range(p_range.slice(start, end), p_func);
    }
    map_range slice(size_type start) const {
        return slice(start, size());
    }
};

namespace detail {
    template<typename R, typename F>
    using MapReturnType = decltype(
        std::declval<F>()(std::declval<range_reference_t<R>>())
    );
}

template<typename R, typename F>
inline map_range<R, F, detail::MapReturnType<R, F>> map(R range, F func) {
    return map_range<R, F, detail::MapReturnType<R, F>>(range, std::move(func));
}

template<typename F>
inline auto map(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return map(obj, std::forward<F>(func));
    };
}

template<typename T, typename F>
struct filter_range: input_range<filter_range<T, F>> {
    using range_category  = std::common_type_t<
        range_category_t<T>, forward_range_tag
    >;
    using value_type      = range_value_t<T>;
    using reference       = range_reference_t<T>;
    using size_type       = range_size_t<T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;
    std::decay_t<F> p_pred;

    void advance_valid() {
        while (!p_range.empty() && !p_pred(front())) {
            p_range.pop_front();
        }
    }

public:
    filter_range() = delete;
    template<typename P>
    filter_range(T const &range, P &&pred):
        p_range(range), p_pred(std::forward<P>(pred))
    {
        advance_valid();
    }

    bool empty() const { return p_range.empty(); }

    void pop_front() {
        p_range.pop_front();
        advance_valid();
    }

    range_reference_t<T> front() const { return p_range.front(); }
};

namespace detail {
    template<typename R, typename P>
    using FilterPred = std::enable_if_t<std::is_same_v<
        decltype(std::declval<P>()(std::declval<range_reference_t<R>>())), bool
    >, P>;
}

template<typename R, typename P>
inline filter_range<R, detail::FilterPred<R, P>> filter(R range, P pred) {
    return filter_range<R, P>(range, std::move(pred));
}

template<typename F>
inline auto filter(F &&func) {
    return [func = std::forward<F>(func)](auto &obj) mutable {
        return filter(obj, std::forward<F>(func));
    };
}

} /* namespace ostd */

#endif
