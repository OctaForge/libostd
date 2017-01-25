/* OctaSTD extensions for std::array.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_ARRAY_HH
#define OSTD_ARRAY_HH

#include <array>

#include "ostd/range.hh"

namespace ostd {

template<typename T, size_t N>
inline PointerRange<T> iter(std::array<T, N> &v) {
    return PointerRange<T>{v.data(), N};
}

template<typename T, size_t N>
inline PointerRange<T const> iter(std::array<T, N> const &v) {
    return PointerRange<T const>{v.data(), N};
}

template<typename T, size_t N>
inline PointerRange<T const> citer(std::array<T, N> const &v) {
    return PointerRange<T const>{v.data(), N};
}

} /* namespace ostd */

#endif
