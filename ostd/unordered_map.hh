/* OctaSTD extensions for std::unordered_map.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_UNORDERED_MAP_HH
#define OSTD_UNORDERED_MAP_HH

#include <unordered_map>
#include <type_traits>

#include "ostd/range.hh"

namespace ostd {

template<typename K, typename T, typename H, typename E, typename A>
struct ranged_traits<std::unordered_map<K, T, H, E, A>> {
    using Range = IteratorRange<
        typename std::unordered_map<K, T, H, E, A>::iterator
    >;
    static Range iter(std::unordered_map<K, T, H, E, A> &v) {
        return Range{v.begin(), v.end()};
    }
};

template<typename K, typename T, typename H, typename E, typename A>
struct ranged_traits<std::unordered_map<K, T, H, E, A> const> {
    using Range = IteratorRange<
        typename std::unordered_map<K, T, H, E, A>::const_iterator
    >;
    static Range iter(std::unordered_map<K, T, H, E, A> const &v) {
        return Range{v.cbegin(), v.cend()};
    }
};

namespace detail {
    template<typename T>
    std::integral_constant<
        bool, std::tuple_size<T>::value == 2
    > tuple2_like_test(typename std::tuple_size<T>::type *);

    template<typename>
    std::false_type tuple2_like_test(...);

    template<typename T>
    constexpr bool is_2tuple_like = decltype(tuple2_like_test<T>(0))::value;
}

template<
    typename K, typename T, typename H = std::hash<K>,
    typename E = std::equal_to<K>,
    typename A = std::allocator<std::pair<K const, T>>, typename R
>
inline std::unordered_map<K, T, H, E, A> make_unordered_map(
    R range, size_t bcount = 1, H const &hash = H{},
    E const &kequal = E{}, A const &alloc = A{}
) {
    static_assert(
        detail::is_2tuple_like<RangeValue<R>>,
        "the range element must be a pair/2-tuple"
    );

    using MP = std::pair<K const, T>;
    using AK = std::tuple_element_t<0, RangeValue<R>>;
    using AV = std::tuple_element_t<1, RangeValue<R>>;

    static_assert(
        std::is_constructible_v<K const, AK> && std::is_constructible_v<T, AV>,
        "incompatible range element type"
    );

    std::unordered_map<K, T, H, E, A> ret{bcount, hash, kequal, alloc};

    using C = RangeCategory<R>;
    if constexpr(std::is_convertible_v<C, FiniteRandomAccessRangeTag>) {
        /* at least try to preallocate here... */
        ret.reserve(range.size());
    }

    for (; !range.empty(); range.pop_front()) {
        if constexpr(std::is_constructible_v<MP, RangeValue<R>>) {
            ret.emplace(range.front());
        } else {
            /* store a temporary to prevent calling front() twice; however,
             * for values that can be used to construct the pair directly
             * we can just do the above
             */
            RangeValue<R> v{range.front()};
            ret.emplace(std::move(std::get<0>(v)), std::move(std::get<1>(v)));
        }
    }
    return ret;
}

template<
    typename R,
    typename H = std::hash<typename RangeValue<R>::first_type>,
    typename E = std::equal_to<typename RangeValue<R>::first_type>,
    typename A = std::allocator<std::pair<
        std::tuple_element_t<0, RangeValue<R>>,
        std::tuple_element_t<1, RangeValue<R>>
    >>
>
inline std::unordered_map<
    std::tuple_element_t<0, RangeValue<R>>,
    std::tuple_element_t<1, RangeValue<R>>, H, E, A
> make_unordered_map(
    R &&range, size_t bcount = 1, H const &hash = H{},
    E const &kequal = E{}, A const &alloc = A{}
) {
    static_assert(
        detail::is_2tuple_like<RangeValue<R>>,
        "the range element must be a pair/2-tuple"
    );
    return make_unordered_map<
        std::tuple_element_t<0, RangeValue<R>>,
        std::tuple_element_t<1, RangeValue<R>>, H, E, A
    >(std::forward<R>(range), bcount, hash, kequal, alloc);
}

} /* namespace ostd */

#endif
