/* Utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_UTILITY_H
#define OCTA_UTILITY_H

#include <stddef.h>

/* must be in std namespace otherwise the compiler won't know about it */
namespace std {
    template<typename T>
    class initializer_list {
        const T *p_buf;
        size_t p_len;

        initializer_list(const T *v, size_t n): p_buf(v), p_len(n) {}
    public:
        struct type {
            typedef       T   value;
            typedef       T  &reference;
            typedef const T  &const_reference;
            typedef       T  *pointer;
            typedef const T  *const_pointer;
            typedef size_t    size;
            typedef ptrdiff_t difference;
        };

        initializer_list(): p_buf(nullptr), p_len(0) {}

        size_t length() const { return p_len; }

        const T *get() const { return p_buf; }
    };
}

namespace octa {
    template<typename T> using InitializerList = std::initializer_list<T>;

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
}

#endif