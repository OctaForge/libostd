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

    template<typename _T>
    static inline constexpr RemoveReference<_T> &&move(_T &&__v) {
        return static_cast<RemoveReference<_T> &&>(__v);
    }

    /* forward */

    template<typename _T>
    static inline constexpr _T &&forward(RemoveReference<_T> &__v) {
        return static_cast<_T &&>(__v);
    }

    template<typename _T>
    static inline constexpr _T &&forward(RemoveReference<_T> &&__v) {
        return static_cast<_T &&>(__v);
    }

    /* declval */

    template<typename _T> AddRvalueReference<_T> declval();

    /* swap */

    template<typename _T>
    struct __OctaSwapTest {
        template<typename _U, void (_U::*)(_U &)> struct __Test {};
        template<typename _U> static char __test(__Test<_U, &_U::swap> *);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == sizeof(char));
    };

    template<typename _T> inline void __octa_swap(_T &__a, _T &__b, EnableIf<
        __OctaSwapTest<_T>::value, bool
    > = true) {
        __a.swap(__b);
    }

    template<typename _T> inline void __octa_swap(_T &__a, _T &__b, EnableIf<
        !__OctaSwapTest<_T>::value, bool
    > = true) {
        _T __c(octa::move(__a));
        __a = octa::move(__b);
        __b = octa::move(__c);
    }

    template<typename _T> void swap(_T &__a, _T &__b) {
        __octa_swap(__a, __b);
    }

    template<typename _T, size_t _N> void swap(_T (&__a)[_N], _T (&__b)[_N]) {
        for (size_t __i = 0; __i < _N; ++__i) {
            octa::swap(__a[__i], __b[__i]);
        }
    }

    /* pair */

    template<typename _T, typename _U>
    struct Pair {
        _T first;
        _U second;

        Pair() = default;
        ~Pair() = default;

        Pair(const Pair &) = default;
        Pair(Pair &&) = default;

        Pair(const _T &__x, const _U &__y): first(__x), second(__y) {}

        template<typename _TT, typename _UU>
        Pair(_TT &&__x, _UU &&__y):
            first(octa::forward<_TT>(__x)), second(octa::forward<_UU>(__y)) {}

        template<typename _TT, typename _UU>
        Pair(const Pair<_TT, _UU> &__v): first(__v.first), second(__v.second) {}

        template<typename _TT, typename _UU>
        Pair(Pair<_TT, _UU> &&__v):
            first(octa::move(__v.first)), second(octa::move(__v.second)) {}

        Pair &operator=(const Pair &__v) {
            first = __v.first;
            second = __v.second;
            return *this;
        }

        template<typename _TT, typename _UU>
        Pair &operator=(const Pair<_TT, _UU> &__v) {
            first = __v.first;
            second = __v.second;
            return *this;
        }

        Pair &operator=(Pair &&__v) {
            first = octa::move(__v.first);
            second = octa::move(__v.second);
            return *this;
        }

        template<typename _TT, typename _UU>
        Pair &operator=(Pair<_TT, _UU> &&__v) {
            first = octa::forward<_TT>(__v.first);
            second = octa::forward<_UU>(__v.second);
            return *this;
        }

        void swap(Pair &__v) {
            octa::swap(first, __v.first);
            octa::swap(second, __v.second);
        }
    };
}

#endif