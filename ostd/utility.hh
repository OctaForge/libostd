/* Utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_UTILITY_HH
#define OSTD_UTILITY_HH

#include <stddef.h>

#include <utility>
#include <tuple>
#include <type_traits>

#include "ostd/types.hh"

namespace ostd {

namespace detail {
    template<typename T, typename U,
        bool = std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>,
        bool = std::is_empty_v<T>, bool = std::is_empty_v<U>
    >
    constexpr size_t CompressedPairSwitch = detail::Undefined<T>();

    /* neither empty */
    template<typename T, typename U, bool Same>
    constexpr size_t CompressedPairSwitch<T, U, Same, false, false> = 0;

    /* first empty */
    template<typename T, typename U, bool Same>
    constexpr size_t CompressedPairSwitch<T, U, Same, true, false> = 1;

    /* second empty */
    template<typename T, typename U, bool Same>
    constexpr size_t CompressedPairSwitch<T, U, Same, false, true> = 2;

    /* both empty, not the same */
    template<typename T, typename U>
    constexpr size_t CompressedPairSwitch<T, U, false, true, true> = 3;

    /* both empty and same */
    template<typename T, typename U>
    constexpr size_t CompressedPairSwitch<T, U, true, true, true> = 1;

    template<typename T, typename U, size_t = CompressedPairSwitch<T, U>>
    struct CompressedPairBase;

    template<typename T, typename U>
    struct CompressedPairBase<T, U, 0> {
        T p_first;
        U p_second;

        template<typename TT, typename UU>
        CompressedPairBase(TT &&a, UU &&b):
            p_first(std::forward<TT>(a)), p_second(std::forward<UU>(b))
        {}

        template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
        CompressedPairBase(
            std::piecewise_construct_t,
            std::tuple<A1...> &fa, std::tuple<A2...> &sa,
            std::index_sequence<I1...>, std::index_sequence<I2...>
        );

        T &first() { return p_first; }
        T const &first() const { return p_first; }

        U &second() { return p_second; }
        U const &second() const { return p_second; }

        void swap(CompressedPairBase &v) {
            swap_adl(p_first, v.p_first);
            swap_adl(p_second, v.p_second);
        }
    };

    template<typename T, typename U>
    struct CompressedPairBase<T, U, 1>: T {
        U p_second;

        template<typename TT, typename UU>
        CompressedPairBase(TT &&a, UU &&b):
            T(std::forward<TT>(a)), p_second(std::forward<UU>(b))
        {}

        template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
        CompressedPairBase(
            std::piecewise_construct_t,
            std::tuple<A1...> &fa, std::tuple<A2...> &sa,
            std::index_sequence<I1...>, std::index_sequence<I2...>
        );

        T &first() { return *this; }
        T const &first() const { return *this; }

        U &second() { return p_second; }
        U const &second() const { return p_second; }

        void swap(CompressedPairBase &v) {
            swap_adl(p_second, v.p_second);
        }
    };

    template<typename T, typename U>
    struct CompressedPairBase<T, U, 2>: U {
        T p_first;

        template<typename TT, typename UU>
        CompressedPairBase(TT &&a, UU &&b):
            U(std::forward<UU>(b)), p_first(std::forward<TT>(a))
        {}

        template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
        CompressedPairBase(
            std::piecewise_construct_t,
            std::tuple<A1...> &fa, std::tuple<A2...> &sa,
            std::index_sequence<I1...>, std::index_sequence<I2...>
        );

        T &first() { return p_first; }
        T const &first() const { return p_first; }

        U &second() { return *this; }
        U const &second() const { return *this; }

        void swap(CompressedPairBase &v) {
            swap_adl(p_first, v.p_first);
        }
    };

    template<typename T, typename U>
    struct CompressedPairBase<T, U, 3>: T, U {
        template<typename TT, typename UU>
        CompressedPairBase(TT &&a, UU &&b):
            T(std::forward<TT>(a)), U(std::forward<UU>(b))
        {}

        template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
        CompressedPairBase(
            std::piecewise_construct_t,
            std::tuple<A1...> &fa, std::tuple<A2...> &sa,
            std::index_sequence<I1...>, std::index_sequence<I2...>
        );

        T &first() { return *this; }
        T const &first() const { return *this; }

        U &second() { return *this; }
        U const &second() const { return *this; }

        void swap(CompressedPairBase &) {}
    };

    template<typename T, typename U>
    struct CompressedPair: CompressedPairBase<T, U> {
        using Base = CompressedPairBase<T, U>;

        template<typename TT, typename UU>
        CompressedPair(TT &&a, UU &&b):
            Base(std::forward<TT>(a), std::forward<UU>(b))
        {}

        template<typename ...A1, typename ...A2>
        CompressedPair(
            std::piecewise_construct_t pc,
            std::tuple<A1...> fa, std::tuple<A2...> sa
        ):
            Base(
                pc, fa, sa,
                std::make_index_sequence<sizeof...(A1)>(),
                std::make_index_sequence<sizeof...(A2)>()
            )
        {}

        T &first() { return Base::first(); }
        T const &first() const { return Base::first(); }

        U &second() { return Base::second(); }
        U const &second() const { return Base::second(); }

        void swap(CompressedPair &v) {
            Base::swap(v);
        }
    };

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
    CompressedPairBase<T, U, 0>::CompressedPairBase(
        std::piecewise_construct_t, std::tuple<A1...> &fa, std::tuple<A2...> &sa,
        std::index_sequence<I1...>, std::index_sequence<I2...>
    ):
        p_first(std::forward<A1>(std::get<I1>(fa))...),
        p_second(std::forward<A2>(std::get<I2>(sa))...)
    {}

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
    CompressedPairBase<T, U, 1>::CompressedPairBase(
        std::piecewise_construct_t, std::tuple<A1...> &fa, std::tuple<A2...> &sa,
        std::index_sequence<I1...>, std::index_sequence<I2...>
    ):
        T(std::forward<A1>(std::get<I1>(fa))...),
        p_second(std::forward<A2>(std::get<I2>(sa))...)
    {}

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
    CompressedPairBase<T, U, 2>::CompressedPairBase(
        std::piecewise_construct_t, std::tuple<A1...> &fa, std::tuple<A2...> &sa,
        std::index_sequence<I1...>, std::index_sequence<I2...>
    ):
        U(std::forward<A2>(std::get<I2>(sa))...),
        p_first(std::forward<A1>(std::get<I1>(fa))...)
    {}

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, size_t ...I1, size_t ...I2>
    CompressedPairBase<T, U, 3>::CompressedPairBase(
        std::piecewise_construct_t, std::tuple<A1...> &fa, std::tuple<A2...> &sa,
        std::index_sequence<I1...>, std::index_sequence<I2...>
    ):
        T(std::forward<A1>(std::get<I1>(fa))...),
        U(std::forward<A2>(std::get<I2>(sa))...)
    {}
} /* namespace detail */

} /* namespace ostd */

#endif
