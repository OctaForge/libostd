/* Some tuple internals for inclusion from various headers. Partially
 * taken from the libc++ project.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_TUPLE_HH
#define OSTD_INTERNAL_TUPLE_HH

#include <tuple>
#include <utility>

namespace ostd {

/* is tuple-like */

template<typename T>
constexpr bool IsTupleLike = false;

template<typename T>
constexpr bool IsTupleLike<const T> = IsTupleLike<T>;
template<typename T>
constexpr bool IsTupleLike<volatile T> = IsTupleLike<T>;
template<typename T>
constexpr bool IsTupleLike<const volatile T> = IsTupleLike<T>;

template<typename ...A>
constexpr bool IsTupleLike<std::tuple<A...>> = true;

template<typename T, typename U>
constexpr bool IsTupleLike<std::pair<T, U>> = true;

} /* namespace ostd */

#endif
