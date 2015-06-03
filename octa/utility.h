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
static inline constexpr RemoveReference<_T> &&move(_T &&v) {
    return static_cast<RemoveReference<_T> &&>(v);
}

/* forward */

template<typename _T>
static inline constexpr _T &&forward(RemoveReference<_T> &v) {
    return static_cast<_T &&>(v);
}

template<typename _T>
static inline constexpr _T &&forward(RemoveReference<_T> &&v) {
    return static_cast<_T &&>(v);
}

/* exchange */

template<typename _T, typename _U = _T>
_T exchange(_T &v, _U &&nv) {
    _T old = move(v);
    v = forward<_U>(nv);
    return old;
}

/* declval */

template<typename _T> AddRvalueReference<_T> declval();

/* swap */

namespace detail {
    template<typename _T>
    struct SwapTest {
        template<typename _U, void (_U::*)(_U &)> struct Test {};
        template<typename _U> static char test(Test<_U, &_U::swap> *);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == sizeof(char));
    };

    template<typename _T> inline void swap(_T &a, _T &b, EnableIf<
        octa::detail::SwapTest<_T>::value, bool
    > = true) {
        a.swap(b);
    }

    template<typename _T> inline void swap(_T &a, _T &b, EnableIf<
        !octa::detail::SwapTest<_T>::value, bool
    > = true) {
        _T c(octa::move(a));
        a = octa::move(b);
        b = octa::move(c);
    }
}

template<typename _T> void swap(_T &a, _T &b) {
   octa::detail::swap(a, b);
}

template<typename _T, size_t _N> void swap(_T (&a)[_N], _T (&b)[_N]) {
    for (size_t i = 0; i < _N; ++i) {
        octa::swap(a[i], b[i]);
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

    Pair(const _T &x, const _U &y): first(x), second(y) {}

    template<typename _TT, typename _UU>
    Pair(_TT &&x, _UU &&y):
        first(octa::forward<_TT>(x)), second(octa::forward<_UU>(y)) {}

    template<typename _TT, typename _UU>
    Pair(const Pair<_TT, _UU> &v): first(v.first), second(v.second) {}

    template<typename _TT, typename _UU>
    Pair(Pair<_TT, _UU> &&v):
        first(octa::move(v.first)), second(octa::move(v.second)) {}

    Pair &operator=(const Pair &v) {
        first = v.first;
        second = v.second;
        return *this;
    }

    template<typename _TT, typename _UU>
    Pair &operator=(const Pair<_TT, _UU> &v) {
        first = v.first;
        second = v.second;
        return *this;
    }

    Pair &operator=(Pair &&v) {
        first = octa::move(v.first);
        second = octa::move(v.second);
        return *this;
    }

    template<typename _TT, typename _UU>
    Pair &operator=(Pair<_TT, _UU> &&v) {
        first = octa::forward<_TT>(v.first);
        second = octa::forward<_UU>(v.second);
        return *this;
    }

    void swap(Pair &v) {
        octa::swap(first, v.first);
        octa::swap(second, v.second);
    }
};

template<typename _T> struct ReferenceWrapper;

namespace detail {
    template<typename _T>
    struct MakePairRetBase {
        typedef _T Type;
    };

    template<typename _T>
    struct MakePairRetBase<ReferenceWrapper<_T>> {
        typedef _T &Type;
    };

    template<typename _T>
    struct MakePairRet {
        typedef typename octa::detail::MakePairRetBase<octa::Decay<_T>>::Type Type;
    };
} /* namespace detail */

template<typename _T, typename _U>
Pair<typename octa::detail::MakePairRet<_T>::Type,
     typename octa::detail::MakePairRet<_U>::Type
 > make_pair(_T &&a, _U &&b) {
    return Pair<typename octa::detail::MakePairRet<_T>::Type,
                typename octa::detail::MakePairRet<_U>::Type
    >(forward<_T>(a), forward<_U>(b));;
}

} /* namespace octa */

#endif