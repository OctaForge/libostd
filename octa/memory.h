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
        static char __test(typename _V::template Rebind<_U> * = 0);

        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, typename _U, bool = __OctaHasRebind<_T, _U>::value>
    struct __OctaPtrTraitsRebindType {
        typedef typename _T::template Rebind<_U> Type;
    };

    template<template<typename, typename...> class _T, typename _U,
        typename ..._A, typename _V
    >
    struct __OctaPtrTraitsRebindType<_T<_U, _A...>, _V, true> {
        typedef typename _T<_U, _A...>::template Rebind<_V> Type;
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

    /* allocator traits - modeled after libc++ */

    template<typename _T>
    struct __OctaConstPtrTest {
        template<typename _U> static char __test(
            typename _U::ConstPtrType * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, typename _P, typename _A,
        bool = __OctaConstPtrTest<_A>::value>
    struct __OctaConstPtrType {
        typedef typename _A::ConstPtrType Type;
    };

    template<typename _T, typename _P, typename _A>
    struct __OctaConstPtrType<_T, _P, _A, false> {
        typedef PointerRebind<_P, const _T> Type;
    };

    template<typename _T>
    struct __OctaVoidPtrTest {
        template<typename _U> static char __test(
            typename _U::VoidPtrType * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _P, typename _A, bool = __OctaVoidPtrTest<_A>::value>
    struct __OctaVoidPtrType {
        typedef typename _A::VoidPtrType Type;
    };

    template<typename _P, typename _A>
    struct __OctaVoidPtrType<_P, _A, false> {
        typedef PointerRebind<_P, void> Type;
    };

    template<typename _T>
    struct __OctaConstVoidPtrTest {
        template<typename _U> static char __test(
            typename _U::ConstVoidPtrType * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _P, typename _A, bool = __OctaConstVoidPtrTest<_A>::value>
    struct __OctaConstVoidPtrType {
        typedef typename _A::ConstVoidPtrType Type;
    };

    template<typename _P, typename _A>
    struct __OctaConstVoidPtrType<_P, _A, false> {
        typedef PointerRebind<_P, const void> Type;
    };

    template<typename _T>
    struct __OctaSizeTest {
        template<typename _U> static char __test(typename _U::SizeType * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _A, typename _D, bool = __OctaSizeTest<_A>::value>
    struct __OctaSizeType {
        typedef octa::MakeUnsigned<_D> Type;
    };

    template<typename _A, typename _D>
    struct __OctaSizeType<_A, _D, true> {
        typedef typename _A::SizeType Type;
    };

    /* allocator type traits */

    template<typename _A>
    using AllocatorType = _A;

    template<typename _A>
    using AllocatorValue = typename AllocatorType<_A>::ValType;

    template<typename _A>
    using AllocatorPointer = typename __OctaPtrType<
        AllocatorValue<_A>, AllocatorType <_A>
    >::Type;

    template<typename _A>
    using AllocatorConstPointer = typename __OctaConstPtrType<
        AllocatorValue<_A>, AllocatorPointer<_A>, AllocatorType<_A>
    >::Type;

    template<typename _A>
    using AllocatorVoidPointer = typename __OctaVoidPtrType<
        AllocatorPointer<_A>, AllocatorType<_A>
    >::Type;

    template<typename _A>
    using AllocatorConstVoidPointer = typename __OctaConstVoidPtrType<
        AllocatorPointer<_A>, AllocatorType<_A>
    >::Type;

    /* allocator difference */

    template<typename _T>
    struct __OctaDiffTest {
        template<typename _U> static char __test(typename _U::DiffType * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _A, typename _P, bool = __OctaDiffTest<_A>::value>
    struct __OctaAllocDiffType {
        typedef PointerDifference<_P> Type;
    };

    template<typename _A, typename _P>
    struct __OctaAllocDiffType<_A, _P, true> {
        typedef typename _A::DiffType Type;
    };

    template<typename _A>
    using AllocatorDifference = typename __OctaAllocDiffType<
        _A, AllocatorPointer<_A>
    >::Type;

    /* allocator size */

    template<typename _A>
    using AllocatorSize = typename __OctaSizeType<
        _A, AllocatorDifference<_A>
    >::Type;

    /* allocator rebind */

    template<typename _T, typename _U, bool = __OctaHasRebind<_T, _U>::value>
    struct __OctaAllocTraitsRebindType {
        typedef typename _T::template Rebind<_U> Type;
    };

    template<template<typename, typename...> class _A, typename _T,
        typename ..._Args, typename _U
    >
    struct __OctaAllocTraitsRebindType<_A<_T, _Args...>, _U, true> {
        typedef typename _A<_T, _Args...>::template Rebind<_U> Type;
    };

    template<template<typename, typename...> class _A, typename _T,
        typename ..._Args, typename _U
    >
    struct __OctaAllocTraitsRebindType<_A<_T, _Args...>, _U, false> {
        typedef _A<_U, _Args...> Type;
    };

    template<typename _A, typename _T>
    using AllocatorRebind = typename __OctaAllocTraitsRebindType<
        AllocatorType<_A>, _T
    >::Type;

    /* allocator propagate on container copy assignment */

    template<typename _T>
    struct __OctaPropagateOnContainerCopyAssignmentTest {
        template<typename _U> static char __test(
            typename _U::PropagateOnContainerCopyAssignment * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _A, bool = __OctaPropagateOnContainerCopyAssignmentTest<
        _A
    >::value> struct __OctaPropagateOnContainerCopyAssignment {
        typedef octa::False Type;
    };

    template<typename _A>
    struct __OctaPropagateOnContainerCopyAssignment<_A, true> {
        typedef typename _A::PropagateOnContainerCopyAssignment Type;
    };

    template<typename _A>
    using PropagateOnContainerCopyAssignment
        = typename __OctaPropagateOnContainerCopyAssignment<_A>::Type;

    /* allocator propagate on container move assignment */

    template<typename _T>
    struct __OctaPropagateOnContainerMoveAssignmentTest {
        template<typename _U> static char __test(
            typename _U::PropagateOnContainerMoveAssignment * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _A, bool = __OctaPropagateOnContainerMoveAssignmentTest<
        _A
    >::value> struct __OctaPropagateOnContainerMoveAssignment {
        typedef octa::False Type;
    };

    template<typename _A>
    struct __OctaPropagateOnContainerMoveAssignment<_A, true> {
        typedef typename _A::PropagateOnContainerMoveAssignment Type;
    };

    template<typename _A>
    using PropagateOnContainerMoveAssignment
        = typename __OctaPropagateOnContainerMoveAssignment<_A>::Type;

    /* allocator propagate on container swap */

    template<typename _T>
    struct __OctaPropagateOnContainerSwapTest {
        template<typename _U> static char __test(
            typename _U::PropagateOnContainerSwap * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _A, bool = __OctaPropagateOnContainerSwapTest<_A>::value>
    struct __OctaPropagateOnContainerSwap {
        typedef octa::False Type;
    };

    template<typename _A>
    struct __OctaPropagateOnContainerSwap<_A, true> {
        typedef typename _A::PropagateOnContainerSwap Type;
    };

    template<typename _A>
    using PropagateOnContainerSwap
        = typename __OctaPropagateOnContainerSwap<_A>::Type;

    /* allocator is always equal */

    template<typename _T>
    struct __OctaIsAlwaysEqualTest {
        template<typename _U> static char __test(
            typename _U::IsAlwaysEqual * = 0);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _A, bool = __OctaIsAlwaysEqualTest<_A>::value>
    struct __OctaIsAlwaysEqual {
        typedef typename octa::IsEmpty<_A>::Type Type;
    };

    template<typename _A>
    struct __OctaIsAlwaysEqual<_A, true> {
        typedef typename _A::IsAlwaysEqual Type;
    };

    template<typename _A>
    using IsAlwaysEqual = typename __OctaIsAlwaysEqual<_A>::Type;

    /* allocator allocate */

    template<typename _A>
    inline AllocatorPointer<_A>
    allocator_allocate(_A &__a, AllocatorSize<_A> __n) {
        return __a.allocate(__n);
    }

    template<typename _A, typename _S, typename _CVP>
    auto __octa_allocate_hint_test(_A &&__a, _S &&__sz, _CVP &&__p)
        -> decltype(__a.allocate(__sz, __p), octa::True());

    template<typename _A, typename _S, typename _CVP>
    auto __octa_allocate_hint_test(const _A &__a, _S &&__sz, _CVP &&__p)
        -> octa::False;

    template<typename _A, typename _S, typename _CVP>
    struct __OctaAllocateHintTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(__octa_allocate_hint_test(octa::declval<_A>(),
                                               octa::declval<_S>(),
                                               octa::declval<_CVP>())),
            octa::True
        >::value
    > {};

    template<typename _A>
    inline AllocatorPointer<_A>
    __octa_allocate(_A &__a, AllocatorSize<_A> __n,
                    AllocatorConstVoidPointer<_A> __h, octa::True) {
        return __a.allocate(__n, __h);
    }

    template<typename _A>
    inline AllocatorPointer<_A>
    __octa_allocate(_A &__a, AllocatorSize<_A> __n,
                    AllocatorConstVoidPointer<_A>, octa::False) {
        return __a.allocate(__n);
    }

    template<typename _A>
    inline AllocatorPointer<_A>
    allocator_allocate(_A &__a, AllocatorSize<_A> __n,
                       AllocatorConstVoidPointer<_A> __h) {
        return __octa_allocate(__a, __n, __h, __OctaAllocateHintTest<
            _A, AllocatorSize<_A>, AllocatorConstVoidPointer<_A>
        >());
    }

    /* allocator deallocate */

    template<typename _A>
    inline void allocator_deallocate(_A &__a, AllocatorPointer<_A> __p,
                                     AllocatorSize<_A> __n) {
        __a.deallocate(__p, __n);
    }

    /* allocator construct */

    template<typename _A, typename _T, typename ..._Args>
    auto __octa_construct_test(_A &&__a, _T *__p, _Args &&...__args)
        -> decltype(__a.construct(__p, octa::forward<_Args>(__args)...),
            octa::True());

    template<typename _A, typename _T, typename ..._Args>
    auto __octa_construct_test(const _A &__a, _T *__p, _Args &&...__args)
        -> octa::False;

    template<typename _A, typename _T, typename ..._Args>
    struct __OctaConstructTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(__octa_construct_test(octa::declval<_A>(),
                                           octa::declval<_T>(),
                                           octa::declval<_Args>()...)),
            octa::True
        >::value
    > {};

    template<typename _A, typename _T, typename ..._Args>
    inline void
    __octa_construct(octa::True, _A &__a, _T *__p, _Args &&...__args) {
        __a.construct(__p, octa::forward<_Args>(__args)...);
    }

    template<typename _A, typename _T, typename ..._Args>
    inline void
    __octa_construct(octa::False, _A &, _T *__p, _Args &&...__args) {
        ::new ((void *)__p) _T(octa::forward<_Args>(__args)...);
    }

    template<typename _A, typename _T, typename ..._Args>
    inline void allocator_construct(_A &__a, _T *__p, _Args &&...__args) {
        __octa_construct(__OctaConstructTest<_A, _T *, _Args...>(),
                         __a, __p, octa::forward<_Args>(__args)...);
    }

    /* allocator destroy */

    template<typename _A, typename _P>
    auto __octa_destroy_test(_A &&__a, _P &&__p)
        -> decltype(__a.destroy(__p), octa::True());

    template<typename _A, typename _P>
    auto __octa_destroy_test(const _A &__a, _P &&__p) -> octa::False;

    template<typename _A, typename _P>
    struct __OctaDestroyTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(__octa_destroy_test(octa::declval<_A>(),
                                         octa::declval<_P>())),
            octa::True
        >::value
    > {};

    template<typename _A, typename _T>
    inline void __octa_destroy(octa::True, _A &__a, _T *__p) {
        __a.destroy(__p);
    }

    template<typename _A, typename _T>
    inline void __octa_destroy(octa::False, _A &, _T *__p) {
        __p->~_T();
    }

    template<typename _A, typename _T>
    inline void allocator_destroy(_A &__a, _T *__p) {
        __octa_destroy(__OctaDestroyTest<_A, _T *>(), __a, __p);
    }

    /* allocator max size */

    template<typename _A>
    auto __octa_alloc_max_size_test(_A &&__a)
        -> decltype(__a.max_size(), octa::True());

    template<typename _A>
    auto __octa_alloc_max_size_test(const volatile _A &__a) -> octa::False;

    template<typename _A>
    struct __OctaAllocMaxSizeTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(__octa_alloc_max_size_test(octa::declval<_A &>())),
            octa::True
        >::value
    > {};

    template<typename _A>
    inline AllocatorSize<_A> __octa_alloc_max_size(octa::True, const _A &__a) {
        return __a.max_size();
    }

    template<typename _A>
    inline AllocatorSize<_A> __octa_alloc_max_size(octa::False, const _A &) {
        return AllocatorSize<_A>(~0);
    }

    template<typename _A>
    inline AllocatorSize<_A> allocator_max_size(const _A &__a) {
        return __octa_alloc_max_size(__OctaAllocMaxSizeTest<const _A>(), __a);
    }

    /* allocator container copy */

    template<typename _A>
    auto __octa_alloc_copy_test(_A &&__a)
        -> decltype(__a.container_copy(), octa::True());

    template<typename _A>
    auto __octa_alloc_copy_test(const volatile _A &__a) -> octa::False;

    template<typename _A>
    struct __OctaAllocCopyTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(__octa_alloc_copy_test(octa::declval<_A &>())),
            octa::True
        >::value
    > {};

    template<typename _A>
    inline AllocatorType<_A>
    __octa_alloc_container_copy(octa::True, const _A &__a) {
        return __a.container_copy();
    }

    template<typename _A>
    inline AllocatorType<_A>
    __octa_alloc_container_copy(octa::False, const _A &__a) {
        return __a;
    }

    template<typename _A>
    inline AllocatorType<_A> allocator_container_copy(const _A &__a) {
        return __octa_alloc_container_copy(__OctaAllocCopyTest<const _A>(),
            __a);
    }
}

#endif