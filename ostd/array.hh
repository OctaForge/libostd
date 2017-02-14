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
struct ranged_traits<std::array<T, N>> {
    static IteratorRange<T *> iter(std::array<T, N> &v) {
        return IteratorRange<T *>{v.data(), v.data() + N};
    }
};

template<typename T, size_t N>
struct ranged_traits<std::array<T, N> const> {
    static IteratorRange<T const *> iter(std::array<T, N> const &v) {
        return IteratorRange<T const *>{v.data(), v.data() + N};
    }
};

} /* namespace ostd */

#endif
