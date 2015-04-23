/* Utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_UTILITY_H
#define OCTA_UTILITY_H

#include <stddef.h>

#include "octa/traits.h"

namespace octa {
    template<typename T>
    static inline constexpr typename RemoveReference<T>::type &&
    move(T &&v) noexcept {
        return static_cast<typename RemoveReference<T>::type &&>(v);
    }

    template<typename T>
    static inline constexpr T &&
    forward(typename RemoveReference<T>::type &v) noexcept {
        return static_cast<T &&>(v);
    }

    template<typename T>
    static inline constexpr T &&
    forward(typename RemoveReference<T>::type &&v) noexcept {
        return static_cast<T &&>(v);
    }

    template<typename T> typename AddRvalueReference<T>::type declval();

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
}

#endif