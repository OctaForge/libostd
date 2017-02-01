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

template<
    typename K, typename T, typename H = std::hash<K>,
    typename E = std::equal_to<K>,
    typename A = std::allocator<std::pair<K const, T>>, typename R
>
inline std::unordered_map<K, T> make_unordered_map(
    R range, size_t bcount = 1, H const &hash = H{},
    E const &kequal = E{}, A const &alloc = A{}
) {
    std::unordered_map<K, T> ret{bcount, hash, kequal, alloc};
    using C = RangeCategory<R>;
    if constexpr(std::is_convertible_v<C, FiniteRandomAccessRangeTag>) {
        /* at least try to preallocate here... */
        ret.reserve(range.size());
    }
    for (; !range.empty(); range.pop_front()) {
        ret.emplace(range.front());
    }
    return ret;
}

template<
    typename R,
    typename H = std::hash<typename RangeValue<R>::first_type>,
    typename E = std::equal_to<typename RangeValue<R>::first_type>,
    typename A = std::allocator<std::pair<
        typename RangeValue<R>::first_type, typename RangeValue<R>::second_type
    >>
>
inline std::unordered_map<
    typename RangeValue<R>::first_type,
    typename RangeValue<R>::second_type
> make_unordered_map(
    R &&range, size_t bcount = 1, H const &hash = H{},
    E const &kequal = E{}, A const &alloc = A{}
) {
    return make_unordered_map<
        typename RangeValue<R>::first_type,
        typename RangeValue<R>::second_type
    >(std::forward<R>(range), bcount, hash, kequal, alloc);
}

} /* namespace ostd */

#endif
