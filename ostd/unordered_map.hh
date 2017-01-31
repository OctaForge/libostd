/* OctaSTD extensions for std::unordered_map.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_UNORDERED_MAP_HH
#define OSTD_UNORDERED_MAP_HH

#include <unordered_map>

#include "ostd/range.hh"

namespace ostd {

template<typename K, typename T>
struct ranged_traits<std::unordered_map<K, T>> {
    using Range = IteratorRange<typename std::unordered_map<K, T>::iterator>;
    static Range iter(std::unordered_map<K, T> &v) {
        return Range{v.begin(), v.end()};
    }
};

template<typename K, typename T>
struct ranged_traits<std::unordered_map<K, T> const> {
    using Range = IteratorRange<typename std::unordered_map<K, T>::const_iterator>;
    static Range iter(std::unordered_map<K, T> const &v) {
        return Range{v.cbegin(), v.cend()};
    }
};

template<typename K, typename T, typename R>
inline std::unordered_map<K, T> make_unordered_map(R range) {
    /* TODO: specialize for contiguous ranges and matching value types */
    std::unordered_map<K, T> ret;
    for (; !range.empty(); range.pop_front()) {
        ret.emplace(range.front());
    }
    return ret;
}

template<typename R>
inline std::unordered_map<
    typename RangeValue<R>::first_type,
    typename RangeValue<R>::second_type
> make_unordered_map(R range) {
    return make_unordered_map<
        typename RangeValue<R>::first_type,
        typename RangeValue<R>::second_type
    >(std::move(range));
}

} /* namespace ostd */

#endif
