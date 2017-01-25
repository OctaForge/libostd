/* OctaSTD extensions for std::vector.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_VECTOR_HH
#define OSTD_VECTOR_HH

#include <vector>

#include "ostd/range.hh"

namespace ostd {

template<typename T>
inline PointerRange<T> iter(std::vector<T> &v) {
    return PointerRange<T>{v.data(), v.size()};
}

template<typename T>
inline PointerRange<T const> iter(std::vector<T> const &v) {
    return PointerRange<T const>{v.data(), v.size()};
}

template<typename T>
inline PointerRange<T const> citer(std::vector<T> const &v) {
    return PointerRange<T const>{v.data(), v.size()};
}

template<typename T, typename R>
inline std::vector<T> make_vector(R range) {
    /* TODO: specialize for contiguous ranges and matching value types */
    std::vector<T> ret;
    for (; !range.empty(); range.pop_front()) {
        ret.push_back(range.front());
    }
    return ret;
}

template<typename R>
inline std::vector<RangeValue<R>> make_vector(R range) {
    return make_vector<RangeValue<R>>(std::move(range));
}

} /* namespace ostd */

#endif
