/** @addtogroup Containers
 * @{
 */

/** @file unordered_map.hh
 *
 * @brief Extensions for std::unordered_map.
 *
 * This file provides extensions for the standard std::unordered_map container.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_UNORDERED_MAP_HH
#define OSTD_UNORDERED_MAP_HH

#include <cstddef>
#include <unordered_map>
#include <type_traits>

#include "ostd/range.hh"

namespace ostd {

/** @addtogroup Containers
 * @{
 */

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

/** @brief Creates an unordered map using a range.
 *
 * The range's value type must be either an std::pair or an std::tuple
 * with 2 elements, additionally the key and value types must be constructible
 * using the tuple or pair's first and second element respectively.
 *
 * You need to manually specify the key and value types for this overload.
 *
 * @param[in] range The range.
 * @param[in] bcount The initial bucket count.
 * @param[in] hash The hash function.
 * @param[in] kequal The key equality comparison function.
 * @param[in] alloc The allocator.
 *
 * @tparam K The key type.
 * @tparam V The value type.
 */
template<
    typename K, typename T, typename H = std::hash<K>,
    typename E = std::equal_to<K>,
    typename A = std::allocator<std::pair<K const, T>>, typename R
>
inline std::unordered_map<K, T, H, E, A> make_unordered_map(
    R range, std::size_t bcount = 1, H const &hash = H{},
    E const &kequal = E{}, A const &alloc = A{}
) {
    static_assert(
        detail::is_2tuple_like<range_value_t<R>>,
        "the range element must be a pair/2-tuple"
    );

    using MP = std::pair<K const, T>;
    using AK = std::tuple_element_t<0, range_value_t<R>>;
    using AV = std::tuple_element_t<1, range_value_t<R>>;

    static_assert(
        std::is_constructible_v<K const, AK> && std::is_constructible_v<T, AV>,
        "incompatible range element type"
    );

    std::unordered_map<K, T, H, E, A> ret{bcount, hash, kequal, alloc};

    using C = range_category_t<R>;
    if constexpr(std::is_convertible_v<C, finite_random_access_range_tag>) {
        /* at least try to preallocate here... */
        ret.reserve(range.size());
    }

    for (; !range.empty(); range.pop_front()) {
        if constexpr(std::is_constructible_v<MP, range_value_t<R>>) {
            ret.emplace(range.front());
        } else {
            /* store a temporary to prevent calling front() twice; however,
             * for values that can be used to construct the pair directly
             * we can just do the above
             */
            range_value_t<R> v{range.front()};
            ret.emplace(std::move(std::get<0>(v)), std::move(std::get<1>(v)));
        }
    }
    return ret;
}

/** @brief Creates an unordered map using a range.
 *
 * Calls into make_unordered_map() using the range value type's first and
 * second element types as key and value respectively.
 */
template<
    typename R,
    typename H = std::hash<typename range_value_t<R>::first_type>,
    typename E = std::equal_to<typename range_value_t<R>::first_type>,
    typename A = std::allocator<std::pair<
        std::tuple_element_t<0, range_value_t<R>>,
        std::tuple_element_t<1, range_value_t<R>>
    >>
>
inline std::unordered_map<
    std::tuple_element_t<0, range_value_t<R>>,
    std::tuple_element_t<1, range_value_t<R>>, H, E, A
> make_unordered_map(
    R &&range, std::size_t bcount = 1, H const &hash = H{},
    E const &kequal = E{}, A const &alloc = A{}
) {
    static_assert(
        detail::is_2tuple_like<range_value_t<R>>,
        "the range element must be a pair/2-tuple"
    );
    return make_unordered_map<
        std::tuple_element_t<0, range_value_t<R>>,
        std::tuple_element_t<1, range_value_t<R>>, H, E, A
    >(std::forward<R>(range), bcount, hash, kequal, alloc);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
