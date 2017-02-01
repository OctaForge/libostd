/* OctaSTD extensions for std::vector.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_VECTOR_HH
#define OSTD_VECTOR_HH

#include <vector>
#include <type_traits>

#include "ostd/range.hh"

namespace ostd {

template<typename T>
struct ranged_traits<std::vector<T>> {
    static PointerRange<T> iter(std::vector<T> &v) {
        return PointerRange<T>{v.data(), v.size()};
    }
};

template<typename T>
struct ranged_traits<std::vector<T> const> {
    static PointerRange<T const> iter(std::vector<T> const &v) {
        return PointerRange<T const>{v.data(), v.size()};
    }
};

template<typename T, typename R>
inline std::vector<T> make_vector(R range) {
    std::vector<T> ret;
    using C = RangeCategory<R>;
    if constexpr(std::is_convertible_v<C, FiniteRandomAccessRangeTag>) {
        /* finite random access or contiguous */
        auto h = range.half();
        ret.insert(ret.end(), h, h + range.size());
    } else {
        /* infinite random access and below */
        for (; !range.empty(); range.pop_front()) {
            ret.push_back(range.front());
        }
    }
    return ret;
}

template<typename R>
inline std::vector<RangeValue<R>> make_vector(R &&range) {
    return make_vector<RangeValue<R>>(std::forward<R>(range));
}

} /* namespace ostd */

#endif
