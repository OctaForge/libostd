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

    template<typename T> constexpr T *address_of(T &v) noexcept {
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
    struct __OctaPtrTraitsRebindType {
        typedef typename T::template rebind<U> type;
    };

    template<template<typename, typename...> class T, typename U,
        typename ...A, typename V
    >
    struct __OctaPtrTraitsRebindType<T<U, A...>, V, true> {
        typedef typename T<U, A...>::template rebind<V> type;
    };

    template<template<typename, typename...> class T, typename U,
        typename ...A, typename V
    >
    struct __OctaPtrTraitsRebindType<T<U, A...>, V, false> {
        typedef T<V, A...> type;
    };

    template<typename T>
    struct __OctaPtrTraitsPointer {
        typedef T type;
    };

    template<typename T>
    struct __OctaPtrTraitsPointer<T *> {
        typedef T *type;
    };

    template<typename T>
    using PointerType = typename __OctaPtrTraitsPointer<T>::type;

    template<typename T>
    struct __OctaPtrTraitsElement {
        typedef typename __OctaPtrTraitsElementType<T>::type type;
    };

    template<typename T>
    struct __OctaPtrTraitsElement<T *> {
        typedef T type;
    };

    template<typename T>
    using PointerElement = typename __OctaPtrTraitsElement<T>::type;

    template<typename T>
    struct __OctaPtrTraitsDifference {
        typedef typename __OctaPtrTraitsDifferenceType<T>::type type;
    };


    template<typename T>
    struct __OctaPtrTraitsDifference<T *> {
        typedef ptrdiff_t difference_type;
    };

    template<typename T>
    using PointerDifference = typename __OctaPtrTraitsDifference<T>::type;

    template<typename T, typename U>
    struct __OctaPtrTraitsRebind {
        using type = typename __OctaPtrTraitsRebindType<T, U>::type;
    };

    template<typename T, typename U>
    struct __OctaPtrTraitsRebind<T *, U> {
        using type = U *;
    };

    template<typename T, typename U>
    using PointerRebind = typename __OctaPtrTraitsRebind<T, U>::type;

    struct __OctaPtrTraitsNat {};

    template<typename T>
    struct __OctaPtrTraitsPointerTo {
        static T pointer_to(Conditional<IsVoid<PointerElement<T>>::value,
            __OctaPtrTraitsNat, PointerElement<T>
        > &r) noexcept(noexcept(T::pointer_to(r))) {
            return T::pointer_to(r);
        }
    };

    template<typename T>
    struct __OctaPtrTraitsPointerTo<T *> {
        static T pointer_to(Conditional<IsVoid<T>::value, __OctaPtrTraitsNat, T> &r)
        noexcept {
            return octa::address_of(r);
        }
    };

    template<typename T>
    static T pointer_to(Conditional<IsVoid<PointerElement<T>>::value,
        __OctaPtrTraitsNat, PointerElement<T>
    > &r)  noexcept(noexcept(__OctaPtrTraitsPointerTo<T>::pointer_to(r))) {
        return __OctaPtrTraitsPointerTo<T>::pointer_to(r);
    }

    /* default deleter */

    template<typename T>
    struct DefaultDelete {
        constexpr DefaultDelete() = default;

        template<typename U> DefaultDelete(const DefaultDelete<U> &) {};

        void operator()(T *p) const noexcept {
            delete p;
        }
    };

    template<typename T>
    struct DefaultDelete<T[]> {
        constexpr DefaultDelete() = default;

        template<typename U> DefaultDelete(const DefaultDelete<U[]> &) {};

        void operator()(T *p) const noexcept {
            delete[] p;
        }
        template<typename U> void operator()(U *) const = delete;
    };

    /* box */

    template<typename T, typename U, bool = IsEmpty<U>::value>
    struct __OctaBoxPair;

    template<typename T, typename U>
    struct __OctaBoxPair<T, U, false> { /* non-empty deleter */
        T *i_ptr;
    private:
        U  p_del;

    public:
        template<typename D>
        __OctaBoxPair(T *ptr, D &&dltr) noexcept:
            i_ptr(ptr), p_del(forward<D>(dltr)) {}

        U &get_deleter() noexcept { return p_del; }
        const U &get_deleter() const noexcept { return p_del; }

        void swap(__OctaBoxPair &v) noexcept {
            octa::swap(i_ptr, v.i_ptr);
            octa::swap(p_del, v.p_del);
        }
    };

    template<typename T, typename U>
    struct __OctaBoxPair<T, U, true>: U { /* empty deleter */
        T *i_ptr;

        template<typename D>
        __OctaBoxPair(T *ptr, D &&dltr) noexcept:
            U(forward<D>(dltr)), i_ptr(ptr) {}

        U &get_deleter() noexcept { return *this; }
        const U &get_deleter() const noexcept { return *this; }

        void swap(__OctaBoxPair &v) noexcept {
            octa::swap(i_ptr, v.i_ptr);
        }
    };

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
        constexpr Box() noexcept: p_stor(nullptr, D()) {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }
        constexpr Box(nullptr_t) noexcept: p_stor(nullptr, D()) {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        explicit Box(pointer p) noexcept: p_stor(p, D()) {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        Box(pointer p, Conditional<IsReference<D>::value,
            D, AddLvalueReference<const D>
        > d) noexcept: p_stor(p, d) {}

        Box(pointer p, RemoveReference<D> &&d) noexcept: p_stor(p, move(d)) {
            static_assert(!IsReference<D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&u) noexcept: p_stor(u.release(), forward<D>(u.get_deleter())) {}

        template<typename TT, typename DD>
        Box(Box<TT, DD> &&u, EnableIf<!IsArray<TT>::value
            && IsConvertible<typename Box<TT, DD>::pointer, pointer>::value
            && IsConvertible<DD, D>::value
            && (!IsReference<D>::value || IsSame<D, DD>::value)
        > = __OctaNat()) noexcept: p_stor(u.release(),
            forward<DD>(u.get_deleter())) {}

        Box &operator=(Box &&u) noexcept {
            reset(u.release());
            p_stor.get_deleter() = forward<D>(u.get_deleter());
            return *this;
        }

        template<typename TT, typename DD>
        EnableIf<!IsArray<TT>::value
            && IsConvertible<typename Box<TT, DD>::pointer, pointer>::value
            && IsAssignable<D &, DD &&>::value,
            Box &
        > operator=(Box<TT, DD> &&u) noexcept {
            reset(u.release());
            p_stor.get_deleter() = forward<DD>(u.get_deleter());
            return *this;
        }

        Box &operator=(nullptr_t) noexcept {
            reset();
            return *this;
        }

        ~Box() { reset(); }

        AddLvalueReference<T> operator*() const { return *p_stor.i_ptr; }
        pointer operator->() const noexcept { return p_stor.i_ptr; }

        explicit operator bool() const noexcept {
            return p_stor.i_ptr != nullptr;
        }

        pointer get() const noexcept { return p_stor.i_ptr; }

        D_ref  get_deleter() noexcept       { return p_stor.get_deleter(); }
        D_cref get_deleter() const noexcept { return p_stor.get_deleter(); }

        pointer release() noexcept {
            pointer p = p_stor.i_ptr;
            p_stor.i_ptr = nullptr;
            return p;
        }

        void reset(pointer p = nullptr) noexcept {
            pointer tmp = p_stor.i_ptr;
            p_stor.i_ptr = p;
            if (tmp) p_stor.get_deleter()(tmp);
        }

        void swap(Box &u) noexcept {
            p_stor.swap(u.p_stor);
        }

    private:
        __OctaBoxPair<T, D> p_stor;
    };

    template<typename T, typename U, bool = IsSame<
        RemoveCv<PointerElement<T>>,
        RemoveCv<PointerElement<U>>
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
        constexpr Box() noexcept: p_stor(nullptr, D()) {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }
        constexpr Box(nullptr_t) noexcept: p_stor(nullptr, D()) {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename U> explicit Box(U p, EnableIf<
            __OctaSameOrLessCvQualified<U, pointer>::value, __OctaNat
        > = __OctaNat()) noexcept: p_stor(p, D()) {
            static_assert(!IsPointer<D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename U> Box(U p, Conditional<IsReference<D>::value,
            D, AddLvalueReference<const D>
        > d, EnableIf<__OctaSameOrLessCvQualified<U, pointer>::value, __OctaNat
        > = __OctaNat()) noexcept: p_stor(p, d) {}

        Box(nullptr_t, Conditional<IsReference<D>::value,
            D, AddLvalueReference<const D>
        > d) noexcept: p_stor(nullptr, d) {}

        template<typename U> Box(U p, RemoveReference<D> &&d, EnableIf<
            __OctaSameOrLessCvQualified<U, pointer>::value, __OctaNat
        > = __OctaNat()) noexcept: p_stor(p, move(d)) {
            static_assert(!IsReference<D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(nullptr_t, RemoveReference<D> &&d) noexcept: p_stor(nullptr, move(d)) {
            static_assert(!IsReference<D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&u) noexcept: p_stor(u.release(), forward<D>(u.get_deleter())) {}

        template<typename TT, typename DD>
        Box(Box<TT, DD> &&u, EnableIf<IsArray<TT>::value
            && __OctaSameOrLessCvQualified<typename Box<TT, DD>::pointer,
                                           pointer>::value
            && IsConvertible<DD, D>::value
            && (!IsReference<D>::value || IsSame<D, DD>::value)> = __OctaNat()
        ) noexcept: p_stor(u.release(), forward<DD>(u.get_deleter())) {}

        Box &operator=(Box &&u) noexcept {
            reset(u.release());
            p_stor.get_deleter() = forward<D>(u.get_deleter());
            return *this;
        }

        template<typename TT, typename DD>
        EnableIf<IsArray<TT>::value
            && __OctaSameOrLessCvQualified<typename Box<TT, DD>::pointer,
                                           pointer>::value
            && IsAssignable<D &, DD &&>::value,
            Box &
        > operator=(Box<TT, DD> &&u) noexcept {
            reset(u.release());
            p_stor.get_deleter() = forward<DD>(u.get_deleter());
            return *this;
        }

        Box &operator=(nullptr_t) noexcept {
            reset();
            return *this;
        }

        ~Box() { reset(); }

        AddLvalueReference<T> operator[](size_t idx) const {
            return p_stor.i_ptr[idx];
        }

        explicit operator bool() const noexcept {
            return p_stor.i_ptr != nullptr;
        }

        pointer get() const noexcept { return p_stor.i_ptr; }

        D_ref  get_deleter() noexcept       { return p_stor.get_deleter(); }
        D_cref get_deleter() const noexcept { return p_stor.get_deleter(); }

        pointer release() noexcept {
            pointer p = p_stor.i_ptr;
            p_stor.i_ptr = nullptr;
            return p;
        }

        template<typename U> EnableIf<
            __OctaSameOrLessCvQualified<U, pointer>::value, void
        > reset(U p) noexcept {
            pointer tmp = p_stor.i_ptr;
            p_stor.i_ptr = p;
            if (tmp) p_stor.get_deleter()(tmp);
        }

        void reset(nullptr_t) noexcept {
            pointer tmp = p_stor.i_ptr;
            p_stor.i_ptr = nullptr;
            if (tmp) p_stor.get_deleter()(tmp);
        }

        void reset() noexcept {
            reset(nullptr);
        }

        void swap(Box &u) noexcept {
            p_stor.swap(u.p_stor);
        }

    private:
        __OctaBoxPair<T, D> p_stor;
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