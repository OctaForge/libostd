/** @defgroup Containers
 *
 * @brief New containers and extensions to standard containers.
 *
 * libostd adds various new utilities for standard containers that allow
 * besides other things construction of those containers from ranges.
 *
 * Integration of ranges for iteration is however not necessary because
 * there is already a fully generic integration for anything that provides
 * an iterator interface.
 *
 * New containers will also be implemented where necessary.
 *
 * @{
 */

/** @file vector.hh
 *
 * @brief Extensions for std::vector.
 *
 * This file provides extensions for the standard std::vector container.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_VECTOR_HH
#define OSTD_VECTOR_HH

#include <vector>
#include <memory>
#include <type_traits>

#include "ostd/range.hh"

namespace ostd {

/** @addtogroup Containers
 * @{
 */

/** @brief Creates a vector using a range.
 *
 * Given a range `range` and optionally an allocator, this constructs
 * an std::vector, adding each item of the given range to it.
 *
 * You have to manually specify the type of the vector's values. There
 * is also another version with the type decided from the range.
 *
 * @tparam T The value type of the vector.
 */
template<typename T, typename A = std::allocator<T>, typename R>
inline std::vector<T, A> make_vector(R range, A const &alloc = A{}) {
    std::vector<T, A> ret{alloc};
    using C = range_category_t<R>;
    if constexpr(std::is_convertible_v<C, contiguous_range_tag>) {
        ret.reserve(range.size());
        ret.insert(ret.end(), range.data(), range.data() + range.size());
    } else {
        for (; !range.empty(); range.pop_front()) {
            ret.push_back(range.front());
        }
    }
    return ret;
}

/** @brief Creates a vector using a range.
 *
 * Calls into make_vector() using the range value type as the vector
 * value type.
 */
template<typename R, typename A = std::allocator<range_value_t<R>>>
inline std::vector<range_value_t<R>, A> make_vector(
    R &&range, A const &alloc = A{}
) {
    return make_vector<range_value_t<R>, A>(std::forward<R>(range), alloc);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
