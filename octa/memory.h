/* Memory utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_MEMORY_H
#define OCTA_MEMORY_H

#include "octa/new.h"
#include "octa/type_traits.h"

namespace octa {
    /* address of */

    template<typename T> T *address_of(T &v) {
        return reinterpret_cast<T *>(&const_cast<char &>
            (reinterpret_cast<const volatile char &>(v)));
    }

    /* pointer traits */

    template<typename T>
    struct __OctaHasElementType {
        template<typename U>
        static int  __octa_test(...);
        template<typename U>
        static char __octa_test(typename U::element_type * = 0);

        static constexpr bool value = (sizeof(__octa_test<T>(0)) == 1);
    };

    template<typename T, bool = __OctaHasElementType<T>::value>
    struct __OctaPtrTraitsElementType;

    template<typename T> struct __OctaPtrTraitsElementType<T, true> {
        typedef typename T::element_type type;
    };

    template<template<typename, typename...> class T, typename U, typename ...A>
    struct __OctaPtrTraitsElementType<T<U, A...>, true> {
        typedef typename T<U, A...>::element_type type;
    };

    template<template<typename, typename...> class T, typename U, typename ...A>
    struct __OctaPtrTraitsElementType<T<U, A...>, false> {
        typedef U type;
    };

    template<typename T>
    struct __OctaHasDifferenceType {
        template<typename U>
        static int  __octa_test(...);
        template<typename U>
        static char __octa_test(typename U::difference_type * = 0);

        static constexpr bool value = (sizeof(__octa_test<T>(0)) == 1);
    };

    template<typename T, bool = __OctaHasDifferenceType<T>::value>
    struct __OctaPtrTraitsDifferenceType {
        typedef ptrdiff_t type;
    };

    template<typename T> struct __OctaPtrTraitsDifferenceType<T, true> {
        typedef typename T::difference_type type;
    };

    template<typename T, typename U>
    struct __OctaHasRebind {
        template<typename V>
        static int  __octa_test(...);
        template<typename V>
        static char __octa_test(typename V::template rebind<U> * = 0);

        static constexpr bool value = (sizeof(__octa_test<T>(0)) == 1);
    };

    template<typename T, typename U, bool = __OctaHasRebind<T, U>::value>
    struct __OctaPtrTraitsRebind {
        typedef typename T::template rebind<U> type;
    };

    template<template<typename, typename...> class T, typename U,
        typename ...A, typename V
    >
    struct __OctaPtrTraitsRebind<T<U, A...>, V, true> {
        typedef typename T<U, A...>::template rebind<V> type;
    };

    template<template<typename, typename...> class T, typename U,
        typename ...A, typename V
    >
    struct __OctaPtrTraitsRebind<T<U, A...>, V, false> {
        typedef T<V, A...> type;
    };

    template<typename T>
    struct PointerTraits {
        typedef T pointer;

        typedef typename __OctaPtrTraitsElementType   <T>::type element_type;
        typedef typename __OctaPtrTraitsDifferenceType<T>::type difference_type;

        template<typename U>
        using rebind = typename __OctaPtrTraitsRebind<T, U>::type;

    private:
        struct __OctaNat {};

    public:
        static T pointer_to(Conditional<IsVoid<element_type>::value,
            __OctaNat, element_type
        > &r) {
            return T::pointer_to(r);
        }
    };

    template<typename T>
    struct PointerTraits<T *> {
        typedef T *pointer;
        typedef T  element_type;

        typedef ptrdiff_t difference_type;

        template<typename U> using rebind = U *;

    private:
        struct __OctaNat {};

    public:
        static T pointer_to(Conditional<IsVoid<element_type>::value,
            __OctaNat, element_type
        > &r) {
            return octa::address_of(r);
        }
    };

    /* default deleter */

    template<typename T>
    struct DefaultDelete {
        constexpr DefaultDelete() = default;

        template<typename U> DefaultDelete(const DefaultDelete<U> &) {};

        void operator()(T *p) const {
            delete p;
        }
    };

    template<typename T>
    struct DefaultDelete<T[]> {
        constexpr DefaultDelete() = default;

        template<typename U> DefaultDelete(const DefaultDelete<U[]> &) {};

        void operator()(T *p) const {
            delete[] p;
        }
        template<typename U> void operator()(U *) const = delete;
    };

    /* box */

    template<typename T>
    static int __octa_ptr_test(...);
    template<typename T>
    static char __octa_ptr_test(typename T::pointer * = 0);

    template<typename T> struct __OctaHasPtr: IntegralConstant<bool,
        (sizeof(__octa_ptr_test<T>(0)) == 1)
    > {};

    template<typename T, typename D, bool = __OctaHasPtr<D>::value>
    struct __OctaPtrTypeBase {
        typedef typename D::pointer type;
    };

    template<typename T, typename D> struct __OctaPtrTypeBase<T, D, false> {
        typedef T *type;
    };

    template<typename T, typename D> struct __OctaPtrType {
        typedef typename __OctaPtrTypeBase<T, RemoveReference<D>>::type type;
    };

    template<typename T, typename D = DefaultDelete<T>>
    struct Box {
        typedef T element_type;
        typedef D deleter_type;
        typedef typename __OctaPtrType<T, D>::type pointer;

    private:
        struct __OctaNat { int x; };

        typedef RemoveReference<D> &D_ref;
        typedef const RemoveReference<D> &D_cref;

    public:
        constexpr Box(): p_ptr(nullptr), p_del() {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }
        constexpr Box(nullptr_t): p_ptr(nullptr), p_del() {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        explicit Box(pointer p): p_ptr(p), p_del() {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        Box(pointer p, Conditional<IsReference<D>::value,
            D, AddLvalueReference<const D>
        > d): p_ptr(p), p_del(d) {}

        Box(pointer p, RemoveReference<D> &&d): p_ptr(p), p_del(move(d)) {
            static_assert(!IsReference<D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&u): p_ptr(u.release()), p_del(forward<D>(u.get_deleter())) {}

        template<typename TT, typename DD>
        Box(Box<TT, DD> &&u, EnableIf<!IsArray<TT>::value
            && IsConvertible<typename Box<TT, DD>::pointer, pointer>::value
            && IsConvertible<DD, D>::value
            && (!IsReference<D>::value || IsSame<D, DD>::value)
        > = __OctaNat()): p_ptr(u.release()),
            p_del(forward<DD>(u.get_deleter())) {}

        Box &operator=(Box &&u) {
            reset(u.release());
            p_del = forward<D>(u.get_deleter());
            return *this;
        }

        template<typename TT, typename DD>
        EnableIf<!IsArray<TT>::value
            && IsConvertible<typename Box<TT, DD>::pointer, pointer>::value
            && IsAssignable<D &, DD &&>::value,
            Box &
        > operator=(Box<TT, DD> &&u) {
            reset(u.release());
            p_del = forward<DD>(u.get_deleter());
            return *this;
        }

        Box &operator=(nullptr_t) {
            reset();
            return *this;
        }

        ~Box() { reset(); }

        AddLvalueReference<T> operator*() const { return *p_ptr; }
        pointer operator->() const { return p_ptr; }

        explicit operator bool() const { return p_ptr != nullptr; }

        pointer get() const { return p_ptr; }

        D_ref  get_deleter()       { return p_del; }
        D_cref get_deleter() const { return p_del; }

        pointer release() {
            pointer p = p_ptr;
            p_ptr = nullptr;
            return p;
        }

        void reset(pointer p = nullptr) {
            pointer tmp = p_ptr;
            p_ptr = p;
            if (tmp) p_del(tmp);
        }

        void swap(Box &u) {
            octa::swap(p_ptr, u.p_ptr);
            octa::swap(p_del, u.p_del);
        }

    private:
        /* TODO: optimize with pair (so that deleter doesn't take up memory) */
        T *p_ptr;
        D  p_del;
    };

    template<typename T, typename U, bool = IsSame<
        RemoveCv<typename PointerTraits<T>::element_type>,
        RemoveCv<typename PointerTraits<U>::element_type>
    >::value> struct __OctaSameOrLessCvQualifiedBase: IsConvertible<T, U> {};

    template<typename T, typename U>
    struct __OctaSameOrLessCvQualifiedBase<T, U, false>: False {};

    template<typename T, typename U, bool = IsPointer<T>::value
        || IsSame<T, U>::value || __OctaHasElementType<T>::value
    > struct __OctaSameOrLessCvQualified: __OctaSameOrLessCvQualifiedBase<T, U> {};

    template<typename T, typename U>
    struct __OctaSameOrLessCvQualified<T, U, false>: False {};

    template<typename T, typename D>
    struct Box<T[], D> {
        typedef T element_type;
        typedef D deleter_type;
        typedef typename __OctaPtrType<T, D>::type pointer;

    private:
        struct __OctaNat { int x; };

        typedef RemoveReference<D> &D_ref;
        typedef const RemoveReference<D> &D_cref;

    public:
        constexpr Box(): p_ptr(nullptr), p_del() {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }
        constexpr Box(nullptr_t): p_ptr(nullptr), p_del() {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename U> explicit Box(U p, EnableIf<
            __OctaSameOrLessCvQualified<U, pointer>::value, __OctaNat
        > = __OctaNat()): p_ptr(p), p_del() {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename U> Box(U p, Conditional<IsReference<D>::value,
            D, AddLvalueReference<const D>
        > d, EnableIf<__OctaSameOrLessCvQualified<U, pointer>::value, __OctaNat
        > = __OctaNat()): p_ptr(p), p_del(d) {}

        Box(nullptr_t, Conditional<IsReference<D>::value,
            D, AddLvalueReference<const D>
        > d): p_ptr(), p_del(d) {}

        template<typename U> Box(U p, RemoveReference<D> &&d, EnableIf<
            __OctaSameOrLessCvQualified<U, pointer>::value, __OctaNat
        > = __OctaNat()): p_ptr(p), p_del(move(d)) {
            static_assert(!IsReference<D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(nullptr_t, RemoveReference<D> &&d): p_ptr(), p_del(move(d)) {
            static_assert(!IsReference<D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&u): p_ptr(u.release()), p_del(forward<D>(u.get_deleter())) {}

        template<typename TT, typename DD>
        Box(Box<TT, DD> &&u, EnableIf<IsArray<TT>::value
            && __OctaSameOrLessCvQualified<typename Box<TT, DD>::pointer,
                                           pointer>::value
            && IsConvertible<DD, D>::value
            && (!IsReference<D>::value || IsSame<D, DD>::value)> = __OctaNat()
        ): p_ptr(u.release()), p_del(forward<DD>(u.get_deleter())) {}

        Box &operator=(Box &&u) {
            reset(u.release());
            p_del = forward<D>(u.get_deleter());
            return *this;
        }

        template<typename TT, typename DD>
        EnableIf<IsArray<TT>::value
            && __OctaSameOrLessCvQualified<typename Box<TT, DD>::pointer,
                                           pointer>::value
            && IsAssignable<D &, DD &&>::value,
            Box &
        > operator=(Box<TT, DD> &&u) {
            reset(u.release());
            p_del = forward<DD>(u.get_deleter());
            return *this;
        }

        Box &operator=(nullptr_t) {
            reset();
            return *this;
        }

        ~Box() { reset(); }

        AddLvalueReference<T> operator[](size_t idx) const {
            return p_ptr[idx];
        }

        explicit operator bool() const { return p_ptr != nullptr; }

        pointer get() const { return p_ptr; }

        D_ref  get_deleter()       { return p_del; }
        D_cref get_deleter() const { return p_del; }

        pointer release() {
            pointer p = p_ptr;
            p_ptr = nullptr;
            return p;
        }

        template<typename U> EnableIf<
            __OctaSameOrLessCvQualified<U, pointer>::value, void
        > reset(U p) {
            pointer tmp = p_ptr;
            p_ptr = p;
            if (tmp) p_del(tmp);
        }

        void reset(nullptr_t) {
            pointer tmp = p_ptr;
            p_ptr = nullptr;
            if (tmp) p_del(tmp);
        }

        void reset() {
            reset(nullptr);
        }

        void swap(Box &u) {
            octa::swap(p_ptr, u.p_ptr);
            octa::swap(p_del, u.p_del);
        }

    private:
        T *p_ptr;
        D  p_del;
    };

    template<typename T> struct __OctaBoxIf {
        typedef Box<T> __OctaBox;
    };

    template<typename T> struct __OctaBoxIf<T[]> {
        typedef Box<T[]> __OctaBoxUnknownSize;
    };

    template<typename T, size_t N> struct __OctaBoxIf<T[N]> {
        typedef void __OctaBoxKnownSize;
    };

    template<typename T, typename ...A>
    typename __OctaBoxIf<T>::__OctaBox make_box(A &&...args) {
        return Box<T>(new T(forward<A>(args)...));
    }

    template<typename T>
    typename __OctaBoxIf<T>::__OctaBoxUnknownSize make_box(size_t n) {
        return Box<T>(new RemoveExtent<T>[n]());
    }

    template<typename T, typename ...A>
    typename __OctaBoxIf<T>::__OctaBoxKnownSize make_box(A &&...args) = delete;
}

#endif