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
    static inline constexpr RemoveReference<T> &&move(T &&v) {
        return static_cast<RemoveReference<T> &&>(v);
    }

    /* forward */

    template<typename T>
    static inline constexpr T &&forward(RemoveReference<T> &v) {
        return static_cast<T &&>(v);
    }

    template<typename T>
    static inline constexpr T &&forward(RemoveReference<T> &&v) {
        return static_cast<T &&>(v);
    }

    /* declval */

    template<typename T> AddRvalueReference<T> declval();

    /* swap */

    template<typename T> void swap(T &a, T &b) {
        T c(move(a));
        a = move(b);
        b = move(c);
    }

    template<typename T, size_t N> void swap(T (&a)[N], T (&b)[N]) {
        for (size_t i = 0; i < N; ++i) {
            swap(a[i], b[i]);
        }
    }

    /* pair */

    template<typename T, typename U>
    struct Pair {
        T first;
        U second;

        Pair() = default;
        ~Pair() = default;

        Pair(const Pair &) = default;
        Pair(Pair &&) = default;

        Pair(const T &x, const U &y): first(x), second(y) {}

        template<typename TT, typename UU>
        Pair(TT &&x, UU &&y): first(forward<TT>(x)), second(forward<UU>(y)) {}

        template<typename TT, typename UU>
        Pair(const Pair<TT, UU> &v): first(v.first), second(v.second) {}

        template<typename TT, typename UU>
        Pair(Pair<TT, UU> &&v): first(move(v.first)), second(move(v.second)) {}

        Pair &operator=(const Pair &v) {
            first = v.first;
            second = v.second;
            return *this;
        }

        template<typename TT, typename UU>
        Pair &operator=(const Pair<TT, UU> &v) {
            first = v.first;
            second = v.second;
            return *this;
        }

        Pair &operator=(Pair &&v) {
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

        void swap(Pair &v) {
            octa::swap(first, v.first);
            octa::swap(second, v.second);
        }
    };

    template<typename T, typename U>
    void swap(Pair<T, U> &a, Pair<T, U> &b) {
        a.swap(b);
    }
}

#endif