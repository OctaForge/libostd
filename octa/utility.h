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
    static inline constexpr RemoveReference<T> &&
    move(T &&v) noexcept {
        return static_cast<RemoveReference<T> &&>(v);
    }

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

    template<typename T> AddRvalueReference<T> declval();

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