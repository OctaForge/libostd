/* Utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_UTILITY_H
#define OCTA_UTILITY_H

#include <stddef.h>

namespace octa {
    /* aliased in traits.h later */
    namespace internal {
        template<typename T> struct RemoveReference      { typedef T type; };
        template<typename T> struct RemoveReference<T&>  { typedef T type; };
        template<typename T> struct RemoveReference<T&&> { typedef T type; };

        template<typename T> struct AddRvalueReference       { typedef T &&type; };
        template<typename T> struct AddRvalueReference<T  &> { typedef T &&type; };
        template<typename T> struct AddRvalueReference<T &&> { typedef T &&type; };
        template<> struct AddRvalueReference<void> {
            typedef void type;
        };
        template<> struct AddRvalueReference<const void> {
            typedef const void type;
        };
        template<> struct AddRvalueReference<volatile void> {
            typedef volatile void type;
        };
        template<> struct AddRvalueReference<const volatile void> {
            typedef const volatile void type;
        };
    }

    template<typename T>
    static inline constexpr typename internal::RemoveReference<T>::type &&
    move(T &&v) noexcept {
        return static_cast<typename internal::RemoveReference<T>::type &&>(v);
    }

    template<typename T>
    static inline constexpr T &&
    forward(typename internal::RemoveReference<T>::type &v) noexcept {
        return static_cast<T &&>(v);
    }

    template<typename T>
    static inline constexpr T &&
    forward(typename internal::RemoveReference<T>::type &&v) noexcept {
        return static_cast<T &&>(v);
    }

    template<typename T> typename internal::AddRvalueReference<T>::type declval();

    namespace internal {
        template<typename, typename> struct MemTypes;
        template<typename T, typename R, typename ...A>
        struct MemTypes<T, R(A...)> {
            typedef R result;
            typedef T argument;
        };
        template<typename T, typename R, typename A>
        struct MemTypes<T, R(A)> {
            typedef R result;
            typedef T argument;
            typedef T first;
            typedef A second;
        };
        template<typename T, typename R, typename ...A>
        struct MemTypes<T, R(A...) const> {
            typedef R result;
            typedef const T argument;
        };
        template<typename T, typename R, typename A>
        struct MemTypes<T, R(A) const> {
            typedef R result;
            typedef const T argument;
            typedef const T first;
            typedef A second;
        };

        template<typename R, typename T>
        class MemFn {
            R T::*p_ptr;
        public:
            struct type: MemTypes<T, R> {};

            MemFn(R T::*ptr): p_ptr(ptr) {}
            template<typename... A>
            auto operator()(T &obj, A &&...args) ->
              decltype(((obj).*(p_ptr))(args...)) {
                return ((obj).*(p_ptr))(args...);
            }
            template<typename... A>
            auto operator()(const T &obj, A &&...args) ->
              decltype(((obj).*(p_ptr))(args...)) const {
                return ((obj).*(p_ptr))(args...);
            }
            template<typename... A>
            auto operator()(T *obj, A &&...args) ->
              decltype(((obj)->*(p_ptr))(args...)) {
                return ((obj)->*(p_ptr))(args...);
            }
            template<typename... A>
            auto operator()(const T *obj, A &&...args) ->
              decltype(((obj)->*(p_ptr))(args...)) const {
                return ((obj)->*(p_ptr))(args...);
            }
        };
    }

    template<typename R, typename T>
    internal::MemFn<R, T> mem_fn(R T:: *ptr) {
        return internal::MemFn<R, T>(ptr);
    }

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