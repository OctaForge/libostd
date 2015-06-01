/* Memory utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_MEMORY_H
#define OCTA_MEMORY_H

#include <stddef.h>

#include "octa/new.h"
#include "octa/utility.h"
#include "octa/type_traits.h"

namespace octa {
    /* address of */

    template<typename _T> constexpr _T *address_of(_T &__v) {
        return reinterpret_cast<_T *>(&const_cast<char &>
            (reinterpret_cast<const volatile char &>(__v)));
    }

    /* pointer traits */

    template<typename _T>
    struct __OctaHasElementType {
        template<typename _U>
        static int  __test(...);
        template<typename _U>
        static char __test(typename _U::ElementType * = 0);

        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, bool = __OctaHasElementType<_T>::value>
    struct __OctaPtrTraitsElementType;

    template<typename _T> struct __OctaPtrTraitsElementType<_T, true> {
        typedef typename _T::ElementType Type;
    };

    template<template<typename, typename...> class _T, typename _U, typename ..._A>
    struct __OctaPtrTraitsElementType<_T<_U, _A...>, true> {
        typedef typename _T<_U, _A...>::ElementType Type;
    };

    template<template<typename, typename...> class _T, typename _U, typename ..._A>
    struct __OctaPtrTraitsElementType<_T<_U, _A...>, false> {
        typedef _U Type;
    };

    template<typename _T>
    struct __OctaHasDiffType {
        template<typename _U>
        static int  __test(...);
        template<typename _U>
        static char __test(typename _U::DiffType * = 0);

        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, bool = __OctaHasDiffType<_T>::value>
    struct __OctaPtrTraitsDiffType {
        typedef ptrdiff_t Type;
    };

    template<typename _T> struct __OctaPtrTraitsDiffType<_T, true> {
        typedef typename _T::DiffType Type;
    };

    template<typename _T, typename _U>
    struct __OctaHasRebind {
        template<typename _V>
        static int  __test(...);
        template<typename _V>
        static char __test(typename _V::template rebind<_U> * = 0);

        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, typename _U, bool = __OctaHasRebind<_T, _U>::value>
    struct __OctaPtrTraitsRebindType {
        typedef typename _T::template rebind<_U> Type;
    };

    template<template<typename, typename...> class _T, typename _U,
        typename ..._A, typename _V
    >
    struct __OctaPtrTraitsRebindType<_T<_U, _A...>, _V, true> {
        typedef typename _T<_U, _A...>::template rebind<_V> Type;
    };

    template<template<typename, typename...> class _T, typename _U,
        typename ..._A, typename _V
    >
    struct __OctaPtrTraitsRebindType<_T<_U, _A...>, _V, false> {
        typedef _T<_V, _A...> Type;
    };

    template<typename _T>
    struct __OctaPtrTraitsPointer {
        typedef _T Type;
    };

    template<typename _T>
    struct __OctaPtrTraitsPointer<_T *> {
        typedef _T *Type;
    };

    template<typename _T>
    using PtrType = typename __OctaPtrTraitsPointer<_T>::Type;

    template<typename _T>
    struct __OctaPtrTraitsElement {
        typedef typename __OctaPtrTraitsElementType<_T>::Type Type;
    };

    template<typename _T>
    struct __OctaPtrTraitsElement<_T *> {
        typedef _T Type;
    };

    template<typename _T>
    using PointerElement = typename __OctaPtrTraitsElement<_T>::Type;

    template<typename _T>
    struct __OctaPtrTraitsDifference {
        typedef typename __OctaPtrTraitsDiffType<_T>::Type Type;
    };


    template<typename _T>
    struct __OctaPtrTraitsDifference<_T *> {
        typedef ptrdiff_t DiffType;
    };

    template<typename _T>
    using PointerDifference = typename __OctaPtrTraitsDifference<_T>::Type;

    template<typename _T, typename _U>
    struct __OctaPtrTraitsRebind {
        using type = typename __OctaPtrTraitsRebindType<_T, _U>::Type;
    };

    template<typename _T, typename _U>
    struct __OctaPtrTraitsRebind<_T *, _U> {
        using type = _U *;
    };

    template<typename _T, typename _U>
    using PointerRebind = typename __OctaPtrTraitsRebind<_T, _U>::Type;

    struct __OctaPtrTraitsNat {};

    template<typename _T>
    struct __OctaPtrTraitsPointerTo {
        static _T pointer_to(octa::Conditional<
            octa::IsVoid<PointerElement<_T>>::value,
            __OctaPtrTraitsNat, PointerElement<_T>
        > &__r) {
            return _T::pointer_to(__r);
        }
    };

    template<typename _T>
    struct __OctaPtrTraitsPointerTo<_T *> {
        static _T pointer_to(octa::Conditional<
            octa::IsVoid<_T>::value, __OctaPtrTraitsNat, _T
        > &__r) {
            return octa::address_of(__r);
        }
    };

    template<typename _T>
    static _T pointer_to(octa::Conditional<
        octa::IsVoid<PointerElement<_T>>::value,
        __OctaPtrTraitsNat, PointerElement<_T>
    > &__r) {
        return __OctaPtrTraitsPointerTo<_T>::pointer_to(__r);
    }

    /* default deleter */

    template<typename _T>
    struct DefaultDelete {
        constexpr DefaultDelete() = default;

        template<typename _U> DefaultDelete(const DefaultDelete<_U> &) {};

        void operator()(_T *__p) const {
            delete __p;
        }
    };

    template<typename _T>
    struct DefaultDelete<_T[]> {
        constexpr DefaultDelete() = default;

        template<typename _U> DefaultDelete(const DefaultDelete<_U[]> &) {};

        void operator()(_T *__p) const {
            delete[] __p;
        }
        template<typename _U> void operator()(_U *) const = delete;
    };

    /* box */

    template<typename _T, typename _U, bool = octa::IsEmpty<_U>::value>
    struct __OctaBoxPair;

    template<typename _T, typename _U>
    struct __OctaBoxPair<_T, _U, false> { /* non-empty deleter */
        _T *__ptr;
    private:
        _U  __del;

    public:
        template<typename _D>
        __OctaBoxPair(_T *__ptr, _D &&__dltr): __ptr(__ptr),
            __del(octa::forward<_D>(__dltr)) {}

        _U &get_deleter() { return __del; }
        const _U &get_deleter() const { return __del; }

        void swap(__OctaBoxPair &__v) {
            octa::swap(__ptr, __v.__ptr);
            octa::swap(__del, __v.__del);
        }
    };

    template<typename _T, typename _U>
    struct __OctaBoxPair<_T, _U, true>: _U { /* empty deleter */
        _T *__ptr;

        template<typename _D>
        __OctaBoxPair(_T *__ptr, _D &&__dltr):
            _U(octa::forward<_D>(__dltr)), __ptr(__ptr) {}

        _U &get_deleter() { return *this; }
        const _U &get_deleter() const { return *this; }

        void swap(__OctaBoxPair &__v) {
            octa::swap(__ptr, __v.__ptr);
        }
    };

    template<typename _T>
    static int __octa_ptr_test(...);
    template<typename _T>
    static char __octa_ptr_test(typename _T::PtrType * = 0);

    template<typename _T> struct __OctaHasPtr: octa::IntegralConstant<bool,
        (sizeof(__octa_ptr_test<_T>(0)) == 1)
    > {};

    template<typename _T, typename _D, bool = __OctaHasPtr<_D>::value>
    struct __OctaPtrTypeBase {
        typedef typename _D::PtrType Type;
    };

    template<typename _T, typename _D> struct __OctaPtrTypeBase<_T, _D, false> {
        typedef _T *Type;
    };

    template<typename _T, typename _D> struct __OctaPtrType {
        typedef typename __OctaPtrTypeBase<_T, octa::RemoveReference<_D>>::Type Type;
    };

    template<typename _T, typename _D = DefaultDelete<_T>>
    struct Box {
        typedef _T ElementType;
        typedef _D DeleterType;
        typedef typename __OctaPtrType<_T, _D>::Type PtrType;

    private:
        struct __OctaNat { int __x; };

        typedef RemoveReference<_D> &_D_ref;
        typedef const RemoveReference<_D> &_D_cref;

    public:
        constexpr Box(): __stor(nullptr, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }
        constexpr Box(nullptr_t): __stor(nullptr, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }

        explicit Box(PtrType __p): __stor(__p, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }

        Box(PtrType __p, octa::Conditional<octa::IsReference<_D>::value,
            _D, octa::AddLvalueReference<const _D>
        > __d): __stor(__p, __d) {}

        Box(PtrType __p, octa::RemoveReference<_D> &&__d):
        __stor(__p, octa::move(__d)) {
            static_assert(!octa::IsReference<_D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&__u): __stor(__u.release(),
            octa::forward<_D>(__u.get_deleter())) {}

        template<typename _TT, typename _DD>
        Box(Box<_TT, _DD> &&__u, octa::EnableIf<!octa::IsArray<_TT>::value
            && octa::IsConvertible<typename Box<_TT, _DD>::PtrType, PtrType>::value
            && octa::IsConvertible<_DD, _D>::value
            && (!octa::IsReference<_D>::value || octa::IsSame<_D, _DD>::value)
        > = __OctaNat()): __stor(__u.release(), octa::forward<_DD>(__u.get_deleter())) {}

        Box &operator=(Box &&__u) {
            reset(__u.release());
            __stor.get_deleter() = octa::forward<_D>(__u.get_deleter());
            return *this;
        }

        template<typename _TT, typename _DD>
        EnableIf<!octa::IsArray<_TT>::value
            && octa::IsConvertible<typename Box<_TT, _DD>::PtrType, PtrType>::value
            && octa::IsAssignable<_D &, _DD &&>::value,
            Box &
        > operator=(Box<_TT, _DD> &&__u) {
            reset(__u.release());
            __stor.get_deleter() = octa::forward<_DD>(__u.get_deleter());
            return *this;
        }

        Box &operator=(nullptr_t) {
            reset();
            return *this;
        }

        ~Box() { reset(); }

        octa::AddLvalueReference<_T> operator*() const { return *__stor.__ptr; }
        PtrType operator->() const { return __stor.__ptr; }

        explicit operator bool() const {
            return __stor.__ptr != nullptr;
        }

        PtrType get() const { return __stor.__ptr; }

        _D_ref  get_deleter()       { return __stor.get_deleter(); }
        _D_cref get_deleter() const { return __stor.get_deleter(); }

        PtrType release() {
            PtrType __p = __stor.__ptr;
            __stor.__ptr = nullptr;
            return __p;
        }

        void reset(PtrType __p = nullptr) {
            PtrType __tmp = __stor.__ptr;
            __stor.__ptr = __p;
            if (__tmp) __stor.get_deleter()(__tmp);
        }

        void swap(Box &__u) {
            __stor.swap(__u.__stor);
        }

    private:
        __OctaBoxPair<_T, _D> __stor;
    };

    template<typename _T, typename _U, bool = octa::IsSame<
        octa::RemoveCv<PointerElement<_T>>,
        octa::RemoveCv<PointerElement<_U>>
    >::value> struct __OctaSameOrLessCvQualifiedBase: octa::IsConvertible<_T, _U> {};

    template<typename _T, typename _U>
    struct __OctaSameOrLessCvQualifiedBase<_T, _U, false>: octa::False {};

    template<typename _T, typename _U, bool = octa::IsPointer<_T>::value
        || octa::IsSame<_T, _U>::value || __OctaHasElementType<_T>::value
    > struct __OctaSameOrLessCvQualified: __OctaSameOrLessCvQualifiedBase<_T, _U> {};

    template<typename _T, typename _U>
    struct __OctaSameOrLessCvQualified<_T, _U, false>: octa::False {};

    template<typename _T, typename _D>
    struct Box<_T[], _D> {
        typedef _T ElementType;
        typedef _D DeleterType;
        typedef typename __OctaPtrType<_T, _D>::Type PtrType;

    private:
        struct __OctaNat { int __x; };

        typedef RemoveReference<_D> &_D_ref;
        typedef const RemoveReference<_D> &_D_cref;

    public:
        constexpr Box(): __stor(nullptr, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }
        constexpr Box(nullptr_t): __stor(nullptr, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename _U> explicit Box(_U __p, octa::EnableIf<
            __OctaSameOrLessCvQualified<_U, PtrType>::value, __OctaNat
        > = __OctaNat()): __stor(__p, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename _U> Box(_U __p, octa::Conditional<
            octa::IsReference<_D>::value,
            _D, AddLvalueReference<const _D>
        > __d, octa::EnableIf<__OctaSameOrLessCvQualified<_U, PtrType>::value,
        __OctaNat> = __OctaNat()): __stor(__p, __d) {}

        Box(nullptr_t, octa::Conditional<octa::IsReference<_D>::value,
            _D, AddLvalueReference<const _D>
        > __d): __stor(nullptr, __d) {}

        template<typename _U> Box(_U __p, octa::RemoveReference<_D> &&__d,
        octa::EnableIf<
            __OctaSameOrLessCvQualified<_U, PtrType>::value, __OctaNat
        > = __OctaNat()): __stor(__p, octa::move(__d)) {
            static_assert(!octa::IsReference<_D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(nullptr_t, octa::RemoveReference<_D> &&__d):
        __stor(nullptr, octa::move(__d)) {
            static_assert(!octa::IsReference<_D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&__u): __stor(__u.release(),
            octa::forward<_D>(__u.get_deleter())) {}

        template<typename _TT, typename _DD>
        Box(Box<_TT, _DD> &&__u, EnableIf<IsArray<_TT>::value
            && __OctaSameOrLessCvQualified<typename Box<_TT, _DD>::PtrType,
                                           PtrType>::value
            && octa::IsConvertible<_DD, _D>::value
            && (!octa::IsReference<_D>::value ||
                 octa::IsSame<_D, _DD>::value)> = __OctaNat()
        ): __stor(__u.release(), octa::forward<_DD>(__u.get_deleter())) {}

        Box &operator=(Box &&__u) {
            reset(__u.release());
            __stor.get_deleter() = octa::forward<_D>(__u.get_deleter());
            return *this;
        }

        template<typename _TT, typename _DD>
        EnableIf<octa::IsArray<_TT>::value
            && __OctaSameOrLessCvQualified<typename Box<_TT, _DD>::PtrType,
                                           PtrType>::value
            && IsAssignable<_D &, _DD &&>::value,
            Box &
        > operator=(Box<_TT, _DD> &&__u) {
            reset(__u.release());
            __stor.get_deleter() = octa::forward<_DD>(__u.get_deleter());
            return *this;
        }

        Box &operator=(nullptr_t) {
            reset();
            return *this;
        }

        ~Box() { reset(); }

        octa::AddLvalueReference<_T> operator[](size_t __idx) const {
            return __stor.__ptr[__idx];
        }

        explicit operator bool() const {
            return __stor.__ptr != nullptr;
        }

        PtrType get() const { return __stor.__ptr; }

        _D_ref  get_deleter()       { return __stor.get_deleter(); }
        _D_cref get_deleter() const { return __stor.get_deleter(); }

        PtrType release() {
            PtrType __p = __stor.__ptr;
            __stor.__ptr = nullptr;
            return __p;
        }

        template<typename _U> EnableIf<
            __OctaSameOrLessCvQualified<_U, PtrType>::value, void
        > reset(_U __p) {
            PtrType __tmp = __stor.__ptr;
            __stor.__ptr = __p;
            if (__tmp) __stor.get_deleter()(__tmp);
        }

        void reset(nullptr_t) {
            PtrType __tmp = __stor.__ptr;
            __stor.__ptr = nullptr;
            if (__tmp) __stor.get_deleter()(__tmp);
        }

        void reset() {
            reset(nullptr);
        }

        void swap(Box &__u) {
            __stor.swap(__u.__stor);
        }

    private:
        __OctaBoxPair<_T, _D> __stor;
    };

    template<typename _T> struct __OctaBoxIf {
        typedef Box<_T> __OctaBox;
    };

    template<typename _T> struct __OctaBoxIf<_T[]> {
        typedef Box<_T[]> __OctaBoxUnknownSize;
    };

    template<typename _T, size_t _N> struct __OctaBoxIf<_T[_N]> {
        typedef void __OctaBoxKnownSize;
    };

    template<typename _T, typename ..._A>
    typename __OctaBoxIf<_T>::__OctaBox make_box(_A &&...__args) {
        return Box<_T>(new _T(octa::forward<_A>(__args)...));
    }

    template<typename _T>
    typename __OctaBoxIf<_T>::__OctaBoxUnknownSize make_box(size_t __n) {
        return Box<_T>(new octa::RemoveExtent<_T>[__n]());
    }

    template<typename _T, typename ..._A>
    typename __OctaBoxIf<_T>::__OctaBoxKnownSize make_box(_A &&...__args) = delete;

    /* allocator */

    template<typename> struct Allocator;

    template<> struct Allocator<void> {
        typedef void        ValType;
        typedef void       *PtrType;
        typedef const void *ConstPtrType;

        template<typename _U> using Rebind = Allocator<_U>;
    };

    template<> struct Allocator<const void> {
        typedef const void  ValType;
        typedef const void *PtrType;
        typedef const void *ConstPtrType;

        template<typename _U> using Rebind = Allocator<_U>;
    };

    template<typename _T> struct Allocator {
        typedef size_t     SizeType;
        typedef ptrdiff_t  DiffType;
        typedef _T         ValType;
        typedef _T        &RefType;
        typedef const _T  &ConstRefType;
        typedef _T        *PtrType;
        typedef const _T  *ConstPtrType;

        template<typename _U> using Rebind = Allocator<_U>;

        PtrType address(RefType __v) const {
            return address_of(__v);
        };
        ConstPtrType address(ConstRefType __v) const {
            return address_of(__v);
        };

        SizeType max_size() const { return SizeType(~0) / sizeof(_T); }

        PtrType allocate(SizeType __n, Allocator<void>::ConstPtrType = nullptr) {
            return (PtrType) ::new uchar[__n * sizeof(_T)];
        }

        void deallocate(PtrType __p, SizeType) { ::delete[] (uchar *) __p; }

        template<typename _U, typename ..._A>
        void construct(_U *__p, _A &&...__args) {
            ::new((void *)__p) _U(octa::forward<_A>(__args)...);
        }

        void destroy(PtrType __p) { __p->~_T(); }
    };

    template<typename _T> struct Allocator<const _T> {
        typedef size_t     SizeType;
        typedef ptrdiff_t  DiffType;
        typedef const _T   ValType;
        typedef const _T  &RefType;
        typedef const _T  &ConstRefType;
        typedef const _T  *PtrType;
        typedef const _T  *ConstPtrType;

        template<typename _U> using Rebind = Allocator<_U>;

        ConstPtrType address(ConstRefType __v) const {
            return address_of(__v);
        };

        SizeType max_size() const { return SizeType(~0) / sizeof(_T); }

        PtrType allocate(SizeType __n, Allocator<void>::ConstPtrType = nullptr) {
            return (PtrType) ::new uchar[__n * sizeof(_T)];
        }

        void deallocate(PtrType __p, SizeType) { ::delete[] (uchar *) __p; }

        template<typename _U, typename ..._A>
        void construct(_U *__p, _A &&...__args) {
            ::new((void *)__p) _U(octa::forward<_A>(__args)...);
        }

        void destroy(PtrType __p) { __p->~_T(); }
    };

    template<typename _T, typename _U>
    bool operator==(const Allocator<_T> &, const Allocator<_U> &) {
        return true;
    }

    template<typename _T, typename _U>
    bool operator!=(const Allocator<_T> &, const Allocator<_U> &) {
        return false;
    }
}

#endif