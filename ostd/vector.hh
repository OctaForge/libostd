/* OctaSTD extensions for std::vector.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_VECTOR_HH
#define OSTD_VECTOR_HH

#include <vector>
#include <memory>
#include <type_traits>

#include "ostd/range.hh"

namespace ostd {

template<typename T, typename A = std::allocator<T>, typename R>
inline std::vector<T, A> make_vector(R range, A const &alloc = A{}) {
    std::vector<T, A> ret{alloc};
    using C = range_category_t<R>;
    if constexpr(std::is_convertible_v<C, finite_random_access_range_tag>) {
        /* finite random access or contiguous */
        auto h = range.half();
        ret.reserve(range.size());
        ret.insert(ret.end(), h, h + range.size());
    } else {
        /* infinite random access and below */
        for (; !range.empty(); range.pop_front()) {
            ret.push_back(range.front());
        }
    }
    return ret;
}

template<typename R, typename A = std::allocator<range_value_t<R>>>
inline std::vector<range_value_t<R>, A> make_vector(
    R &&range, A const &alloc = A{}
) {
    return make_vector<range_value_t<R>, A>(std::forward<R>(range), alloc);
}

} /* namespace ostd */

#endif
