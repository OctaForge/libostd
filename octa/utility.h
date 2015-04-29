/* Utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_UTILITY_H
#define OCTA_UTILITY_H

#include <stddef.h>

#include "octa/type_traits.h"

namespace octa {
    /* move */

    template<typename T>
    static inline constexpr RemoveReference<T> &&
    move(T &&v) noexcept {
        return static_cast<RemoveReference<T> &&>(v);
    }

    /* forward */

    template<typename T>
    static inline constexpr T &&
    forward(RemoveReference<T> &v) noexcept {
        return static_cast<T &&>(v);
    }

    template<typename T>
    static inline constexpr T &&
    forward(RemoveReference<T> &&v) noexcept {
        return static_cast<T &&>(v);
    }

    /* declval */

    template<typename T> AddRvalueReference<T> declval();

    /* swap */

    template<typename T> void swap(T &a, T &b) noexcept(
        IsNothrowMoveConstructible<T>::value
     && IsNothrowMoveAssignable<T>::value
    ) {
        T c(move(a));
        a = move(b);
        b = move(c);
    }

    template<typename T, size_t N> void swap(T (&a)[N], T (&b)[N]) noexcept(
        noexcept(swap(*a, *b))
    ) {
        for (size_t i = 0; i < N; ++i) {
            swap(a[i], b[i]);
        }
    }

    /* pair */

    template<typename T, typename U>
    struct Pair {
        T first;
        U second;

        Pair() noexcept(
            IsNothrowDefaultConstructible<T>::value
         && IsNothrowDefaultConstructible<U>::value
        ) = default;
        ~Pair() = default;

        Pair(const Pair &) noexcept(
            IsNothrowCopyConstructible<T>::value
         && IsNothrowCopyConstructible<U>::value
        ) = default;
        Pair(Pair &&) noexcept(
            IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveConstructible<U>::value
        ) = default;

        Pair(const T &x, const U &y) noexcept(
            IsNothrowCopyConstructible<T>::value
         && IsNothrowCopyConstructible<U>::value
        ): first(x), second(y) {}

        template<typename TT, typename UU>
        Pair(TT &&x, UU &&y): first(forward<TT>(x)), second(forward<UU>(y)) {}

        template<typename TT, typename UU>
        Pair(const Pair<TT, UU> &v) noexcept(
            IsNothrowCopyConstructible<T>::value
         && IsNothrowCopyConstructible<U>::value
        ): first(v.first), second(v.second) {}

        template<typename TT, typename UU>
        Pair(Pair<TT, UU> &&v) noexcept(
            IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveConstructible<U>::value
        ): first(move(v.first)), second(move(v.second)) {}

        Pair &operator=(const Pair &v) noexcept(
            IsNothrowCopyAssignable<T>::value
         && IsNothrowCopyAssignable<U>::value
        ) {
            first = v.first;
            second = v.second;
            return *this;
        }

        template<typename TT, typename UU>
        Pair &operator=(const Pair<TT, UU> &v) noexcept(
            IsNothrowCopyAssignable<T>::value
         && IsNothrowCopyAssignable<U>::value
        ) {
            first = v.first;
            second = v.second;
            return *this;
        }

        Pair &operator=(Pair &&v) noexcept(
            IsNothrowMoveAssignable<T>::value
         && IsNothrowMoveAssignable<U>::value
        ) {
            first = move(v.first);
            second = move(v.second);
            return *this;
        }

        template<typename TT, typename UU>
        Pair &operator=(Pair<TT, UU> &&v) {
            first = forward<TT>(v.first);
            second = forward<UU>(v.second);
            return *this;
        }

        void swap(Pair &v) noexcept(
            noexcept(octa::swap(first, v.first))
         && noexcept(octa::swap(second, v.second))
        ) {
            octa::swap(first, v.first);
            octa::swap(second, v.second);
        }
    };

    template<typename T, typename U>
    void swap(Pair<T, U> &a, Pair<T, U> &b) noexcept(noexcept(a.swap(b))) {
        a.swap(b);
    }
}

#endif