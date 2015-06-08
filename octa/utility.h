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

/* exchange */

template<typename T, typename U = T>
T exchange(T &v, U &&nv) {
    T old = move(v);
    v = forward<U>(nv);
    return old;
}

/* declval */

template<typename T> AddRvalueReference<T> declval();

/* swap */

namespace detail {
    template<typename T>
    struct SwapTest {
        template<typename U, void (U::*)(U &)> struct Test {};
        template<typename U> static char test(Test<U, &U::swap> *);
        template<typename U> static  int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char));
    };

    template<typename T> inline void swap(T &a, T &b, EnableIf<
        octa::detail::SwapTest<T>::value, bool
    > = true) {
        a.swap(b);
    }

    template<typename T> inline void swap(T &a, T &b, EnableIf<
        !octa::detail::SwapTest<T>::value, bool
    > = true) {
        T c(octa::move(a));
        a = octa::move(b);
        b = octa::move(c);
    }
}

template<typename T> void swap(T &a, T &b) {
   octa::detail::swap(a, b);
}

template<typename T, octa::Size N> void swap(T (&a)[N], T (&b)[N]) {
    for (octa::Size i = 0; i < N; ++i) {
        octa::swap(a[i], b[i]);
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
    Pair(TT &&x, UU &&y):
        first(octa::forward<TT>(x)), second(octa::forward<UU>(y)) {}

    template<typename TT, typename UU>
    Pair(const Pair<TT, UU> &v): first(v.first), second(v.second) {}

    template<typename TT, typename UU>
    Pair(Pair<TT, UU> &&v):
        first(octa::move(v.first)), second(octa::move(v.second)) {}

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
        first = octa::move(v.first);
        second = octa::move(v.second);
        return *this;
    }

    template<typename TT, typename UU>
    Pair &operator=(Pair<TT, UU> &&v) {
        first = octa::forward<TT>(v.first);
        second = octa::forward<UU>(v.second);
        return *this;
    }

    void swap(Pair &v) {
        octa::swap(first, v.first);
        octa::swap(second, v.second);
    }
};

template<typename T> struct ReferenceWrapper;

namespace detail {
    template<typename T>
    struct MakePairRetBase {
        using Type = T;
    };

    template<typename T>
    struct MakePairRetBase<ReferenceWrapper<T>> {
        using Type = T &;
    };

    template<typename T>
    struct MakePairRet {
        using Type = typename octa::detail::MakePairRetBase<octa::Decay<T>>::Type;
    };
} /* namespace detail */

template<typename T, typename U>
Pair<typename octa::detail::MakePairRet<T>::Type,
     typename octa::detail::MakePairRet<U>::Type
 > make_pair(T &&a, U &&b) {
    return Pair<typename octa::detail::MakePairRet<T>::Type,
                typename octa::detail::MakePairRet<U>::Type
    >(forward<T>(a), forward<U>(b));;
}

} /* namespace octa */

#endif