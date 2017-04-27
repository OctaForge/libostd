/** @addtogroup Ranges
 * @{
 */

/** @file algorithm.hh
 *
 * @brief Generic algorithms for working with ranges.
 *
 * This file provides various algorithms that work with ranges, including
 * partitioning, sorting, comparison, iteration, finding, filling, folding,
 * mapping, filtering and others. It's roughly equivalent to the C++ header
 * `algorithm`, with many of the algorithms being just range-based versions
 * of the iterator ones, but it also provides different custom algorithms.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_ALGORITHM_HH
#define OSTD_ALGORITHM_HH

#include <cmath>
#include <utility>
#include <functional>
#include <type_traits>
#include <algorithm>

#ifdef OSTD_BUILD_TESTS
#include <vector>
#endif

#include "ostd/unit_test.hh"

#include "ostd/range.hh"

#define OSTD_TEST_MODULE libostd_algorithm

namespace ostd {

/** @addtogroup Ranges
 * @{
 */

/* partitioning */

/** @brief Partitions a range.
 *
 * Given a predicate `pred`, this rearranges the range so that items for
 * which the predicate returns true are in the first part of the range.
 *
 * The range must meet the conditions of ostd::is_range_element_swappable.
 *
 * The predicate is applied `N` times and the swap is done at most `N` times.
 *
 * @returns The second part of the range.
 */
template<typename ForwardRange, typename Predicate>
inline ForwardRange partition(ForwardRange range, Predicate pred) {
    static_assert(
        is_range_element_swappable<ForwardRange>,
        "The range element accessors must allow swapping"
    );
    auto ret = range;
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            using std::swap;
            swap(range.front(), ret.front());
            ret.pop_front();
        }
    }
    return ret;
}

/** @brief A pipeable version of ostd::partition().
 *
 * The predicate is forwarded.
 */
template<typename Predicate>
inline auto partition(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return partition(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Checks if a range is partitioned as in ostd::partition().
 *
 * First, all elements matching `pred` are skipped and then if any of
 * the elements following that match `pred`, false is returned. Otherwise,
 * true is returned.
 *
 * The predicate is applied at most `N` times.
 */
template<typename InputRange, typename Predicate>
inline bool is_partitioned(InputRange range, Predicate pred) {
    for (; !range.empty() && pred(range.front()); range.pop_front());
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            return false;
        }
    }
    return true;
}

/** @brief A pipeable version of ostd::is_partitioned().
 *
 * The predicate is forwarded.
 */
template<typename Predicate>
inline auto is_partitioned(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return is_partitioned(obj, std::forward<Predicate>(pred));
    };
}

#ifdef OSTD_BUILD_TESTS
OSTD_UNIT_TEST {
    using ostd::test::fail_if;
    using ostd::test::fail_if_not;
    /* test with two vectors for pipeable and non-pipeable */
    std::vector<int> v1 = { 5, 15, 10, 8, 36, 24 };
    std::vector<int> v2 = v1;
    auto try_test = [](auto &v, auto h) {
        for (auto i: h) {
            fail_if(i < 15);
        }
        for (auto i: iter(v).take(v.size() - h.size())) {
            fail_if(i >= 15);
        }
    };
    /* assume they're not partitioned */
    fail_if(is_partitioned(iter(v1), [](int &i) { return i < 15; }));
    fail_if(iter(v1) | is_partitioned([](int &i) { return i < 15; }));
    fail_if(is_partitioned(iter(v2), [](int &i) { return i < 15; }));
    fail_if(iter(v2) | is_partitioned([](int &i) { return i < 15; }));
    /* partition now */
    try_test(v1, partition(iter(v1), [](int &i) { return i < 15; }));
    try_test(v2, iter(v2) | partition([](int &i) { return i < 15; }));
    /* assume partitioned */
    fail_if_not(is_partitioned(iter(v1), [](int &i) { return i < 15; }));
    fail_if_not(iter(v1) | is_partitioned([](int &i) { return i < 15; }));
    fail_if_not(is_partitioned(iter(v2), [](int &i) { return i < 15; }));
    fail_if_not(iter(v2) | is_partitioned([](int &i) { return i < 15; }));
}
#endif

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

/** @brief Sorts a range given a comparison function.
 *
 * The range must be at least ostd::finite_random_access_range_tag. The
 * comparison function takes two `ostd::range_reference_t<R>` and must
 * return a boolean equivalent to `a < b` for ascending order.
 *
 * The items are swapped in the range, which means the range must also
 * meet the conditions of ostd::is_range_element_swappable.
 *
 * The worst-case and average performance of this algorithm os `O(n log n)`.
 * The best-case performance is `O(n)`. This happens when the range is small
 * enough and already sorted (insertion sort is used for small ranges).
 *
 * The actual algorithm used is a hybrid algorithm between quicksort and
 * heapsort (intosort) with insertion sort for small ranges.
 *
 * @see ostd::sort()
 */
template<typename FiniteRandomRange, typename Compare>
inline FiniteRandomRange sort_cmp(FiniteRandomRange range, Compare compare) {
    static_assert(
        is_range_element_swappable<FiniteRandomRange>,
        "The range element accessors must allow swapping"
    );
    detail::introsort(range, compare);
    return range;
}

/** @brief A pipeable version of ostd::sort_cmp().
 *
 * The comparison function is forwarded.
 */
template<typename Compare>
inline auto sort_cmp(Compare &&compare) {
    return [compare = std::forward<Compare>(compare)](auto &obj) mutable {
        return sort_cmp(obj, std::forward<Compare>(compare));
    };
}

/** @brief Like ostd::sort_cmp() using `std::less<ostd::range_value_t<R>>{}`. */
template<typename FiniteRandomRange>
inline FiniteRandomRange sort(FiniteRandomRange range) {
    static_assert(
        is_range_element_swappable<FiniteRandomRange>,
        "The range element accessors must allow swapping"
    );
    return sort_cmp(range, std::less<range_value_t<FiniteRandomRange>>{});
}

/** @brief A pipeable version of ostd::sort(). */
inline auto sort() {
    return [](auto &obj) { return sort(obj); };
}

/* min/max(_element) */

/** @brief Finds the smallest element in the range.
 *
 * It works like std::min_element(). The range must be at least
 * ostd::forward_range_tag. The `<` operator is used for comparisons.
 *
 * @see ostd::min_element_cmp(), ostd::max_element()
 */
template<typename ForwardRange>
inline ForwardRange min_element(ForwardRange range) {
    ForwardRange r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::min(r.front(), range.front()) == range.front()) {
            r = range;
        }
    }
    return r;
}

/** @brief Finds the smallest element in the range.
 *
 * It works like std::min_element. The range must be at least
 * ostd::forward_range_tag. The `compare` function is used for comparisons.
 *
 * @see ostd::min_element(), ostd::max_element_cmp()
 */
template<typename ForwardRange, typename Compare>
inline ForwardRange min_element_cmp(ForwardRange range, Compare compare) {
    ForwardRange r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::min(r.front(), range.front(), compare) == range.front()) {
            r = range;
        }
    }
    return r;
}

/** @brief A pipeable version of ostd::min_element(). */
inline auto min_element() {
    return [](auto &obj) {
        return min_element(obj);
    };
}

/** @brief A pipeable version of ostd::min_element_cmp().
 *
 * The comparison function is forwarded.
 */
template<typename Compare>
inline auto min_element_cmp(Compare &&compare) {
    return [compare = std::forward<Compare>(compare)](auto &obj) mutable {
        return min_element_cmp(obj, std::forward<Compare>(compare));
    };
}

/** @brief Finds the largest element in the range.
 *
 * It works like std::max_element(). The range must be at least
 * ostd::forward_range_tag. The `<` operator is used for comparisons.
 *
 * @see ostd::max_element_cmp(), ostd::min_element()
 */
template<typename ForwardRange>
inline ForwardRange max_element(ForwardRange range) {
    ForwardRange r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::max(r.front(), range.front()) == range.front()) {
            r = range;
        }
    }
    return r;
}

/** @brief Finds the largest element in the range.
 *
 * It works like std::max_element. The range must be at least
 * ostd::forward_range_tag. The `compare` function is used for comparisons.
 *
 * @see ostd::max_element(), ostd::min_element_cmp()
 */
template<typename ForwardRange, typename Compare>
inline ForwardRange max_element_cmp(ForwardRange range, Compare compare) {
    ForwardRange r = range;
    for (; !range.empty(); range.pop_front()) {
        if (std::max(r.front(), range.front(), compare) == range.front()) {
            r = range;
        }
    }
    return r;
}

/** @brief A pipeable version of ostd::max_element(). */
inline auto max_element() {
    return [](auto &obj) {
        return max_element(obj);
    };
}

/** @brief A pipeable version of ostd::max_element_cmp().
 *
 * The comparison function is forwarded.
 */
template<typename Compare>
inline auto max_element_cmp(Compare &&compare) {
    return [compare = std::forward<Compare>(compare)](auto &obj) mutable {
        return max_element_cmp(obj, std::forward<Compare>(compare));
    };
}

/* lexicographical compare */

/** @brief Like std::lexicographical_compare(), but for ranges.
 *
 * This version uses the `<` operator for comparisons. This algorithm
 * is not multi-pass, so an ostd::input_range_tag is perfectly fine here.
 *
 * @see ostd::lexicographical_compare_cmp()
 */
template<typename InputRange1, typename InputRange2>
inline bool lexicographical_compare(InputRange1 range1, InputRange2 range2) {
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

/** @brief A pipeable version of ostd::lexicographical_compare().
 *
 * The range is forwarded.
 */
template<typename InputRange>
inline auto lexicographical_compare(InputRange &&range) {
    return [range = std::forward<InputRange>(range)](auto &obj) mutable {
        return lexicographical_compare(obj, std::forward<InputRange>(range));
    };
}

/** @brief Like std::lexicographical_compare(), but for ranges.
 *
 * This version uses the `compare` function for comparisons. This algorithm
 * is not multi-pass, so an ostd::input_range_tag is perfectly fine here.
 *
 * @see ostd::lexicographical_compare()
 */
template<typename InputRange1, typename InputRange2, typename Compare>
inline bool lexicographical_compare_cmp(
    InputRange1 range1, InputRange2 range2, Compare compare
) {
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

/** @brief A pipeable version of ostd::lexicographical_compare_cmp().
 *
 * The range and comparison function are forwarded.
 */
template<typename InputRange, typename Compare>
inline auto lexicographical_compare_cmp(InputRange &&range, Compare &&compare) {
    return [
        range = std::forward<InputRange>(range),
        compare = std::forward<Compare>(compare)
    ](auto &obj) mutable {
        return lexicographical_compare_cmp(
            obj, std::forward<InputRange>(range), std::forward<Compare>(compare)
        );
    };
}

/* algos that don't change the range */

/** @brief Executes `func` on each element of `range`.
 *
 * The `func` is called like `func(range.front())`. The algorithm is
 * not multi-pass, so an ostd::input_range_tag is perfectly fine.
 *
 * @returns The `func` by move.
 */
template<typename InputRange, typename UnaryFunction>
inline UnaryFunction for_each(InputRange range, UnaryFunction func) {
    for (; !range.empty(); range.pop_front()) {
        func(range.front());
    }
    return std::move(func);
}

/** @brief A pipeable version of ostd::for_each().
 *
 * The function is forwarded.
 */
template<typename UnaryFunction>
inline auto for_each(UnaryFunction &&func) {
    return [func = std::forward<UnaryFunction>(func)](auto &obj) mutable {
        return for_each(obj, std::forward<UnaryFunction>(func));
    };
}

/** @brief Checks if every element of `range` matches `pred`.
 *
 * The `pred` has to return true when called like `pred(range.front())`.
 * If it doesn't, `false` is returned.
 *
 * The predicate is called at most `N` times, where `N` is the range length.
 * The range is an ostd::input_range_tag or better.
 *
 * @see ostd::any_of(), ostd::none_of()
 */
template<typename InputRange, typename Predicate>
inline bool all_of(InputRange range, Predicate pred) {
    for (; !range.empty(); range.pop_front()) {
        if (!pred(range.front())) {
            return false;
        }
    }
    return true;
}

/** @brief A pipeable version of ostd::all_of().
 *
 * The function is forwarded.
 */
template<typename Predicate>
inline auto all_of(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return all_of(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Checks if any element of `range` matches `pred`.
 *
 * As soon as `pred` returns true when called like `pred(range.front())`,
 * this returns true. Otherwise it returns false.
 *
 * The predicate is called at most `N` times, where `N` is the range length.
 * The range is an ostd::input_range_tag or better.
 *
 * @see ostd::all_of(), ostd::none_of()
 */
template<typename InputRange, typename Predicate>
inline bool any_of(InputRange range, Predicate pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return true;
    return false;
}

/** @brief A pipeable version of ostd::any_of().
 *
 * The function is forwarded.
 */
template<typename Predicate>
inline auto any_of(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return any_of(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Checks if no element of `range` matches `pred`.
 *
 * As soon as `pred` returns true when called like `pred(range.front())`,
 * this returns false. Otherwise it returns true.
 *
 * The predicate is called at most `N` times, where `N` is the range length.
 * The range is an ostd::input_range_tag or better.
 *
 * @see ostd::all_of(), ostd::any_of()
 */
template<typename InputRange, typename Predicate>
inline bool none_of(InputRange range, Predicate pred) {
    for (; !range.empty(); range.pop_front())
        if (pred(range.front())) return false;
    return true;
}

/** @brief A pipeable version of ostd::none_of().
 *
 * The function is forwarded.
 */
template<typename Predicate>
inline auto none_of(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return none_of(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Finds `v` in `range`.
 *
 * Iterates the range and as soon as `range.front()` is equal to `v`,
 * returns `range`. The `range` is at least ostd::input_range_tag.
 *
 * @sse ostd::find_last(), ostd::find_if(), ostd::find_if_not(),
 *      ostd::find_one_of()
 */
template<typename InputRange, typename Value>
inline InputRange find(InputRange range, Value const &v) {
    for (; !range.empty(); range.pop_front()) {
        if (range.front() == v) {
            break;
        }
    }
    return range;
}

/** @brief A pipeable version of ostd::find().
 *
 * The `v` is forwarded.
 */
template<typename Value>
inline auto find(Value &&v) {
    return [v = std::forward<Value>(v)](auto &obj) mutable {
        return find(obj, std::forward<Value>(v));
    };
}

/** @brief Finds the last occurence of `v` in `range`.
 *
 * Keeps attempting ostd::find() from the point of previous ostd::find()
 * until no next matching element is found. As this algorithm has to save
 * the previous result of ostd::find() in case nothing is found next, this
 * algortihm requires `range` to be at least ostd::forward_range_tag.
 *
 * @sse ostd::find(), ostd::find_if(), ostd::find_if_not(),
 *      ostd::find_one_of()
 */
template<typename ForwardRange, typename Value>
inline ForwardRange find_last(ForwardRange range, Value const &v) {
    range = find(range, v);
    if (!range.empty()) {
        for (;;) {
            auto prev = range;
            prev.pop_front();
            auto r = find(prev, v);
            if (r.empty()) {
                break;
            }
            range = r;
        }
    }
    return range;
}

/** @brief A pipeable version of ostd::find_last().
 *
 * The `v` is forwarded.
 */
template<typename Value>
inline auto find_last(Value &&v) {
    return [v = std::forward<Value>(v)](auto &obj) mutable {
        return find_last(obj, std::forward<Value>(v));
    };
}

/** @brief Finds an element matching `pred` in `range`.
 *
 * Iterates the range and as soon as `pred(range.front())` is true,
 * returns `range`. The `range` is at least ostd::input_range_tag.
 *
 * @sse ostd::find(), ostd::find_last(), ostd::find_if_not(),
 *      ostd::find_one_of()
 */
template<typename InputRange, typename Predicate>
inline InputRange find_if(InputRange range, Predicate pred) {
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            break;
        }
    }
    return range;
}

/** @brief A pipeable version of ostd::find_if().
 *
 * The `pred` is forwarded.
 */
template<typename Predicate>
inline auto find_if(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return find_if(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Finds an element not matching `pred` in `range`.
 *
 * Iterates the range and as soon as `!pred(range.front())` is true,
 * returns `range`. The `range` is at least ostd::input_range_tag.
 *
 * @sse ostd::find(), ostd::find_last(), ostd::find_if(), ostd::find_one_of()
 */
template<typename InputRange, typename Predicate>
inline InputRange find_if_not(InputRange range, Predicate pred) {
    for (; !range.empty(); range.pop_front()) {
        if (!pred(range.front())) {
            break;
        }
    }
    return range;
}

/** @brief A pipeable version of ostd::find_if_not().
 *
 * The `pred` is forwarded.
 */
template<typename Predicate>
inline auto find_if_not(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return find_if_not(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Finds the first element matching any element in `values`.
 *
 * The `compare` function is used to compare the values. The `range`
 * is iterated and each item is compared with each item in `values`
 * and once a match is found, `range` is returned.
 *
 * The `range` has to be at least ostd::input_range_tag as it's
 * iterated only once, `values` has to be ostd::forward_range_tag or
 * better.
 *
 * The time complexity is up to `N * M` where `N` is the length of
 * `range` and `M` is the length of `values`.
 *
 * Use ostd::find_one_of() if you want to use the `==` operator
 * instead of calling `compare`.
 *
 * @sse ostd::find(), ostd::find_last(), ostd::find_if(), ostd::find_if_not(),
 *      ostd::find_one_of()
 */
template<typename InputRange, typename ForwardRange, typename Compare>
inline InputRange find_one_of_cmp(
    InputRange range, ForwardRange values, Compare compare
) {
    for (; !range.empty(); range.pop_front()) {
        for (auto rv = values; !rv.empty(); rv.pop_front()) {
            if (compare(range.front(), rv.front())) {
                return range;
            }
        }
    }
    return range;
}

/** @brief A pipeable version of ostd::find_one_of_cmp().
 *
 * The `values` and `compare` are forwarded.
 */
template<typename ForwardRange, typename Compare>
inline auto find_one_of_cmp(ForwardRange &&values, Compare &&compare) {
    return [
        values = std::forward<ForwardRange>(values),
        compare = std::forward<Compare>(compare)
    ](auto &obj) mutable {
        return find_one_of_cmp(
            obj, std::forward<ForwardRange>(values),
            std::forward<Compare>(compare)
        );
    };
}

/** @brief Finds the first element matching any element in `values`.
 *
 * The `==` operator is used to compare the values. The `range`
 * is iterated and each item is compared with each item in `values`
 * and once a match is found, `range` is returned.
 *
 * The `range` has to be at least ostd::input_range_tag as it's
 * iterated only once, `values` has to be ostd::forward_range_tag or
 * better.
 *
 * The time complexity is up to `N * M` where `N` is the length of
 * `range` and `M` is the length of `values`.
 *
 * Use ostd::find_one_of_cmp() if you want to use a comparison
 * function instead of the `==` operator.
 *
 * @sse ostd::find(), ostd::find_last(), ostd::find_if(), ostd::find_if_not(),
 *      ostd::find_one_of_cmp()
 */
template<typename InputRange, typename ForwardRange>
inline InputRange find_one_of(InputRange range, ForwardRange values) {
    for (; !range.empty(); range.pop_front()) {
        for (auto rv = values; !rv.empty(); rv.pop_front()) {
            if (range.front() == rv.front()) {
                return range;
            }
        }
    }
    return range;
}

/** @brief A pipeable version of ostd::find_one_of().
 *
 * The `values` range is forwarded.
 */
template<typename ForwardRange>
inline auto find_one_of(ForwardRange &&values) {
    return [values = std::forward<ForwardRange>(values)](auto &obj) mutable {
        return find_one_of(obj, std::forward<ForwardRange>(values));
    };
}

/** @brief Counts the number of occurences of `v` in `range`.
 *
 * Iterates the range and each time `v` is encountered (using the `==`
 * operator) a counter is incremented. This counter is then returned.
 *
 * The `range` is at least ostd::input_range_tag.
 *
 * @see ostd::count_if(), ostd::count_if_not()
 */
template<typename InputRange, typename Value>
inline range_size_t<InputRange> count(InputRange range, Value const &v) {
    range_size_t<InputRange> ret = 0;
    for (; !range.empty(); range.pop_front()) {
        if (range.front() == v) {
            ++ret;
        }
    }
    return ret;
}

/** @brief A pipeable version of ostd::count().
 *
 * The `v` is forwarded.
 */
template<typename Value>
inline auto count(Value &&v) {
    return [v = std::forward<Value>(v)](auto &obj) mutable {
        return count(obj, std::forward<Value>(v));
    };
}

/** @brief Counts the number of elements matching `pred` in `range.
 *
 * Iterates the range and each time `pred(range.front())` returns true,
 * a counter is incremented. This counter is then returned.
 *
 * The `range` is at least ostd::input_range_tag.
 *
 * @see ostd::count(), ostd::count_if_not()
 */
template<typename InputRange, typename Predicate>
inline range_size_t<InputRange> count_if(InputRange range, Predicate pred) {
    range_size_t<InputRange> ret = 0;
    for (; !range.empty(); range.pop_front()) {
        if (pred(range.front())) {
            ++ret;
        }
    }
    return ret;
}

/** @brief A pipeable version of ostd::count_if().
 *
 * The `pred` is forwarded.
 */
template<typename Predicate>
inline auto count_if(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return count_if(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Counts the number of elements not matching `pred` in `range.
 *
 * Iterates the range and each time `!pred(range.front())` returns true,
 * a counter is incremented. This counter is then returned.
 *
 * The `range` is at least ostd::input_range_tag.
 *
 * @see ostd::count(), ostd::count_if()
 */
template<typename InputRange, typename Predicate>
inline range_size_t<InputRange> count_if_not(InputRange range, Predicate pred) {
    range_size_t<InputRange> ret = 0;
    for (; !range.empty(); range.pop_front()) {
        if (!pred(range.front())) {
            ++ret;
        }
    }
    return ret;
}

/** @brief A pipeable version of ostd::count_if_not().
 *
 * The `pred` is forwarded.
 */
template<typename Predicate>
inline auto count_if_not(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return count_if_not(obj, std::forward<Predicate>(pred));
    };
}

/** @brief Checks if two ranges have equal contents.
 *
 * Both ranges are at least ostd::input_range_tag. If one range becomes
 * empty before the other does, false is returned. If the expression
 * `!(range1.front() == range2.front())` is true in any iteration,
 * false is also returned. Otherwise true is returned.
 */
template<typename InputRange>
inline bool equal(InputRange range1, InputRange range2) {
    for (; !range1.empty(); range1.pop_front()) {
        if (range2.empty() || !(range1.front() == range2.front())) {
            return false;
        }
        range2.pop_front();
    }
    return range2.empty();
}

/** @brief A pipeable version of ostd::equal().
 *
 * The `range` is forwarded as the second range.
 */
template<typename InputRange>
inline auto equal(InputRange &&range) {
    return [range = std::forward<InputRange>(range)](auto &obj) mutable {
        return equal(obj, std::forward<InputRange>(range));
    };
}

/* algos that modify ranges or work with output ranges */

/** @brief Copies all elements from `irange` to `orange`.
 *
 * The `irange` is at least ostd::input_range_tag. The `orange` is
 * an output range. The ostd::range_put_all() function is used to
 * perform the copy. it respects ADL and therefore any per-type
 * overloads of ostd::range_put_all.
 *
 * @see ostd::copy_if(), ostd::copy_if_not()
 */
template<typename InputRange, typename OutputRange>
inline OutputRange copy(InputRange irange, OutputRange orange) {
    range_put_all(orange, irange);
    return orange;
}

/** @brief Copies elements of `irange` matching `pred` into `orange`.
 *
 * The `irange` (at least ostd::input_range_tag) is iterated and if
 * the expression `pred(irange.front())` is true, the element is
 * inserted (like `orange.put(irange.front())`).
 *
 * @see ostd::copy(), ostd::copy_if_not()
 */
template<typename InputRange, typename OutputRange, typename Predicate>
inline OutputRange copy_if(
    InputRange irange, OutputRange orange, Predicate pred
) {
    for (; !irange.empty(); irange.pop_front()) {
        if (pred(irange.front())) {
            orange.put(irange.front());
        }
    }
    return orange;
}

/** @brief Copies elements of `irange` not matching `pred` into `orange`.
 *
 * The `irange` (at least ostd::input_range_tag) is iterated and if
 * the expression `!pred(irange.front())` is true, the element is
 * inserted (like `orange.put(irange.front())`).
 *
 * @see ostd::copy(), ostd::copy_if()
 */
template<typename InputRange, typename OutputRange, typename Predicate>
inline OutputRange copy_if_not(
    InputRange irange, OutputRange orange, Predicate pred
) {
    for (; !irange.empty(); irange.pop_front()) {
        if (!pred(irange.front())) {
            orange.put(irange.front());
        }
    }
    return orange;
}

/** @brief Reverses the order of the given range in-place.
 *
 * The range must be at least ostd::bidirectional_range_tag. The range
 * must also meet the conditions of ostd::is_range_element_swappable.
 *
 * Equivalent to the following:
 *
 * ~~~{.cc}
 * while (!range.empty()) {
 *     using std::swap;
 *     swap(range.front(), range.back());
 *     range.pop_front();
 *     range.pop_back();
 * }
 * ~~~
 *
 * @see ostd::reverse_copy()
 */
template<typename BidirRange>
inline void reverse(BidirRange range) {
    static_assert(
        is_range_element_swappable<BidirRange>,
        "The range element accessors must allow swapping"
    );
    while (!range.empty()) {
        using std::swap;
        swap(range.front(), range.back());
        range.pop_front();
        range.pop_back();
    }
}

/** @brief Reverses the order of the given range into `orange`.
 *
 * Iterates `irange` backwards, putting each item into `orange`. The
 * `irange` has to be at least ostd::bidirectional_range_tag.
 *
 * Equivalent to the following:
 *
 * ~~~{.cc}
 * while (!irange.empty()) {
 *     orange.put(irange.back());
 *     irange.pop_back();
 * }
 *
 * return orange;
 * ~~~
 *
 * @see ostd::reverse()
 */
template<typename BidirRange, typename OutputRange>
inline OutputRange reverse_copy(BidirRange irange, OutputRange orange) {
    for (; !irange.empty(); irange.pop_back()) {
        orange.put(irange.back());
    }
    return orange;
}

/** @brief Fills the given input range with `v`.
 *
 * Iterates over `range` and assigns `v` to each element. The elements
 * must therefore be actual lvalue references so they can be assigned to.
 *
 * @see ostd::generate(), ostd::iota()
 */
template<typename InputRange, typename Value>
inline void fill(InputRange range, Value const &v) {
    for (; !range.empty(); range.pop_front()) {
        range.front() = v;
    }
}

/** @brief Fills the given input range with calls to `gen`.
 *
 * Iterates over `range` and assigns `gen()` to each element. The elements
 * must therefore be actual lvalue references so they can be assigned to.
 *
 * @see ostd::fill(), ostd::iota()
 */
template<typename InputRange, typename UnaryFunction>
inline void generate(InputRange range, UnaryFunction gen) {
    for (; !range.empty(); range.pop_front()) {
        range.front() = gen();
    }
}

/** @brief Swaps the contents of two input ranges.
 *
 * Testing ostd::is_range_element_swappable_with must be true with the given
 * ranges. The ranges must be at least ostd::input_range_tag. The swapping
 * stops as soon as any of the ranges becomes empty.
 *
 * @returns The `range1` and `range2` in a pair after swapping is done.
 */
template<typename InputRange1, typename InputRange2>
inline std::pair<InputRange1, InputRange2> swap_ranges(
    InputRange1 range1, InputRange2 range2
) {
    static_assert(
        is_range_element_swappable_with<InputRange1, InputRange2>,
        "The range element accessors must allow swapping"
    );
    while (!range1.empty() && !range2.empty()) {
        using std::swap;
        swap(range1.front(), range2.front());
        range1.pop_front();
        range2.pop_front();
    }
    return std::make_pair(range1, range2);
}

/** @brief Fills the given input range with `value++`.
 *
 * Iterates over `range` and assigns `value++` to each element. The elements
 * must therefore be actual lvalue references so they can be assigned to.
 *
 * @see ostd::fill(), ostd::generate()
 */
template<typename InputRange, typename Value>
inline void iota(InputRange range, Value value) {
    for (; !range.empty(); range.pop_front()) {
        range.front() = value++;
    }
}

/** @brief Left-folds the `range` into `init`.
 *
 * The `init` is an initial value. The `range` is iterated and each
 * element is added to `init` using the `+` operator. Once that is
 * done, `init` is returned.
 *
 * The `range` must be at least ostd::input_range_tag.
 *
 * A function-based version as well as right-folds are provided as well.
 *
 * @see ostd::foldl_f(), ostd::foldr()
 */
template<typename InputRange, typename Value>
inline Value foldl(InputRange range, Value init) {
    for (; !range.empty(); range.pop_front()) {
        init = init + range.front();
    }
    return init;
}

/** @brief Left-folds the `range` into `init`.
 *
 * The `init` is an initial value. The `range` is iterated and `init`
 * is assigned as `init = func(init, range.front())`. Once that is
 * done, `init` is returned.
 *
 * The `range` must be at least ostd::input_range_tag.
 *
 * A `+` operator-based version as well as right-folds are provided as well.
 *
 * @see ostd::foldl(), ostd::foldr_f()
 */
template<typename InputRange, typename Value, typename BinaryFunction>
inline Value foldl_f(InputRange range, Value init, BinaryFunction func) {
    for (; !range.empty(); range.pop_front()) {
        init = func(init, range.front());
    }
    return init;
}

/** @brief A pipeable version of ostd::foldl().
 *
 * The `init` is forwarded.
 */
template<typename Value>
inline auto foldl(Value &&init) {
    return [init = std::forward<Value>(init)](auto &obj) mutable {
        return foldl(obj, std::forward<Value>(init));
    };
}

/** @brief A pipeable version of ostd::foldl_f().
 *
 * The `init` and `func` are forwarded.
 */
template<typename Value, typename BinaryFunction>
inline auto foldl_f(Value &&init, BinaryFunction &&func) {
    return [
        init = std::forward<Value>(init),
        func = std::forward<BinaryFunction>(func)
    ](auto &obj) mutable {
        return foldl_f(
            obj, std::forward<Value>(init), std::forward<BinaryFunction>(func)
        );
    };
}

/** @brief Right-folds the `range` into `init`.
 *
 * The `init` is an initial value. The `range` is iterated backwards
 * and each element is added to `init` using the `+` operator. Once
 * that is done, `init` is returned.
 *
 * The `range` must be at least ostd::bidirectional_range_tag.
 *
 * A function-based version as well as left-folds are provided as well.
 *
 * @see ostd::foldr_f(), ostd::foldl()
 */
template<typename InputRange, typename Value>
inline Value foldr(InputRange range, Value init) {
    for (; !range.empty(); range.pop_back()) {
        init = init + range.back();
    }
    return init;
}

/** @brief Right-folds the `range` into `init`.
 *
 * The `init` is an initial value. The `range` is iterated backwards
 * and `init` is assigned as `init = func(init, range.back())`. Once
 * that is done, `init` is returned.
 *
 * The `range` must be at least ostd::bidirectional_range_tag.
 *
 * A `+` operator-based version as well as left-folds are provided as well.
 *
 * @see ostd::foldr(), ostd::foldl_f()
 */
template<typename InputRange, typename Value, typename BinaryFunction>
inline Value foldr_f(InputRange range, Value init, BinaryFunction func) {
    for (; !range.empty(); range.pop_back()) {
        init = func(init, range.back());
    }
    return init;
}

/** @brief A pipeable version of ostd::foldr().
 *
 * The `init` is forwarded.
 */
template<typename Value>
inline auto foldr(Value &&init) {
    return [init = std::forward<Value>(init)](auto &obj) mutable {
        return foldr(obj, std::forward<Value>(init));
    };
}

/** @brief A pipeable version of ostd::foldr_f().
 *
 * The `init` and `func` are forwarded.
 */
template<typename Value, typename BinaryFunction>
inline auto foldr_f(Value &&init, BinaryFunction &&func) {
    return [
        init = std::forward<Value>(init),
        func = std::forward<BinaryFunction>(func)
    ](auto &obj) mutable {
        return foldr_f(
            obj, std::forward<Value>(init), std::forward<BinaryFunction>(func)
        );
    };
}

namespace detail {
    template<typename T, typename F, typename R>
    struct map_range: input_range<map_range<T, F, R>> {
        using range_category  = std::common_type_t<
            range_category_t<T>, finite_random_access_range_tag
        >;
        using value_type = std::remove_reference_t<R>;
        using reference  = R;
        using size_type  = range_size_t<T>;

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

    template<typename R, typename F>
    using MapReturnType = decltype(
        std::declval<F>()(std::declval<range_reference_t<R>>())
    );
} /* namespace detail */

/** @brief Gets a wrapper range that maps each item by `func`.
 *
 * The resulting range is at most ostd::finite_random_access_range_tag.
 * There are no restrictions on the input range. The resulting range is
 * not mutable, it's purely input-type.
 *
 * The `reference` member type of the range is `R` where `R` is the return
 * value of `func`. The `value_type` is `std::remove_reference_t<R>`. The
 * size type is preserved.
 *
 * On each access of a range item (front, back, indexing), the `func` is
 * called with the actual wrapped range's item and the result is returned.
 *
 * An example:
 *
 * ~~~{.cc}
 * auto r = ostd::map(ostd::range(5), [](auto v) { return v + 5; });
 * for (auto i: r) {
 *     ostd::writeln(i); // 5, 6, 7, 8, 9
 * }
 * ~~~
 *
 * Because the range is lazy, a new value is computed on each access, but
 * the wrapper range also needs no memory of its own.
 *
 * @see ostd::filter()
 */
template<typename InputRange, typename UnaryFunction>
inline auto map(InputRange range, UnaryFunction func) {
    return detail::map_range<
        InputRange, UnaryFunction,
        detail::MapReturnType<InputRange, UnaryFunction>
    >(range, std::move(func));
}

/** @brief A pipeable version of ostd::map().
 *
 * The `func` is forwarded.
 */
template<typename UnaryFunction>
inline auto map(UnaryFunction &&func) {
    return [func = std::forward<UnaryFunction>(func)](auto &obj) mutable {
        return map(obj, std::forward<UnaryFunction>(func));
    };
}

namespace detail {
    template<typename T, typename F>
    struct filter_range: input_range<filter_range<T, F>> {
        using range_category  = std::common_type_t<
            range_category_t<T>, forward_range_tag
        >;
        using value_type = range_value_t<T>;
        using reference  = range_reference_t<T>;
        using size_type  = range_size_t<T>;

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

    template<typename R, typename P>
    using FilterPred = std::enable_if_t<std::is_same_v<
        decltype(std::declval<P>()(std::declval<range_reference_t<R>>())), bool
    >, P>;
} /* namespace detail */

/** @brief Gets a wrapper range that filters items by `pred`.
 *
 * The resulting range is ostd::forward_range_tag at most. The range will
 * only contains items for which `pred` returns true. What this means is
 * that upon creation, the given range is stored and all iterated until
 * an item matching `pred` is reached. On `pop_front()`, this item is
 * popped out and the same is done, i.e. again iterated until a new
 * item matching `pred` is reached.
 *
 * In other words, this is done:
 *
 * ~~~{.cc}
 * void advance(R &range, P &pred) {
 *     while (!range.empty() && !pred()) {
 *         range.pop_front();
 *     }
 * }
 *
 * constructor(R range, P pred) {
 *     store_range_and_pred(range, pred);
 *     advance(stored_range, stored_pred);
 * }
 *
 * void pop_front() {
 *     stored_range.pop_front();
 *     advance(stored_range, stored_pred);
 * }
 * ~~~
 *
 * The value, reference and size types are preserved, as are
 * calls to `front()` and `empty()`.
 *
 * @see ostd::map()
 */
template<typename InputRange, typename Predicate>
inline auto filter(InputRange range, Predicate pred) {
    return detail::filter_range<InputRange, Predicate>(range, std::move(pred));
}

/** @brief A pipeable version of ostd::filter().
 *
 * The `pred` is forwarded.
 */
template<typename Predicate>
inline auto filter(Predicate &&pred) {
    return [pred = std::forward<Predicate>(pred)](auto &obj) mutable {
        return filter(obj, std::forward<Predicate>(pred));
    };
}

/** @} */

} /* namespace ostd */

#undef OSTD_TEST_MODULE

#endif

/** @} */
