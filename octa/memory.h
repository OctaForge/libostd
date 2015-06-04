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

    template<typename _T> constexpr _T *address_of(_T &v) {
        return reinterpret_cast<_T *>(&const_cast<char &>
            (reinterpret_cast<const volatile char &>(v)));
    }

    /* pointer traits */

    template<typename _T>
    struct __OctaHasElement {
        template<typename _U>
        static int  __test(...);
        template<typename _U>
        static char __test(typename _U::Element * = 0);

        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, bool = __OctaHasElement<_T>::value>
    struct __OctaPtrTraitsElementType;

    template<typename _T> struct __OctaPtrTraitsElementType<_T, true> {
        typedef typename _T::Element Type;
    };

    template<template<typename, typename...> class _T, typename _U, typename ..._A>
    struct __OctaPtrTraitsElementType<_T<_U, _A...>, true> {
        typedef typename _T<_U, _A...>::Element Type;
    };

    template<template<typename, typename...> class _T, typename _U, typename ..._A>
    struct __OctaPtrTraitsElementType<_T<_U, _A...>, false> {
        typedef _U Type;
    };

    template<typename _T>
    struct __OctaHasDifference {
        template<typename _U>
        static int  __test(...);
        template<typename _U>
        static char __test(typename _U::Difference * = 0);

        static constexpr bool value = (sizeof(__test<_T>(0)) == 1);
    };

    template<typename _T, bool = __OctaHasDifference<_T>::value>
    struct __OctaPtrTraitsDifferenceType {
        typedef ptrdiff_t Type;
    };

    template<typename _T> struct __OctaPtrTraitsDifferenceType<_T, true> {
        typedef typename _T::Difference Type;
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
    using Pointer = typename __OctaPtrTraitsPointer<_T>::Type;

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
        typedef typename __OctaPtrTraitsDifferenceType<_T>::Type Type;
    };


    template<typename _T>
    struct __OctaPtrTraitsDifference<_T *> {
        typedef ptrdiff_t Type;
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

        void operator()(_T *p) const {
            delete p;
        }
    };

    template<typename _T>
    struct DefaultDelete<_T[]> {
        constexpr DefaultDelete() = default;

        template<typename _U> DefaultDelete(const DefaultDelete<_U[]> &) {};

        void operator()(_T *p) const {
            delete[] p;
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
    static char __octa_ptr_test(typename _T::Pointer * = 0);

    template<typename _T> struct __OctaHasPtr: octa::IntegralConstant<bool,
        (sizeof(__octa_ptr_test<_T>(0)) == 1)
    > {};

    template<typename _T, typename _D, bool = __OctaHasPtr<_D>::value>
    struct __OctaPointerBase {
        typedef typename _D::Pointer Type;
    };

    template<typename _T, typename _D> struct __OctaPointerBase<_T, _D, false> {
        typedef _T *Type;
    };

    template<typename _T, typename _D> struct __OctaPointer {
        typedef typename __OctaPointerBase<_T, octa::RemoveReference<_D>>::Type Type;
    };

    template<typename _T, typename _D = DefaultDelete<_T>>
    struct Box {
        typedef _T Element;
        typedef _D Deleter;
        typedef typename __OctaPointer<_T, _D>::Type Pointer;

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

        explicit Box(Pointer __p): __stor(__p, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }

        Box(Pointer __p, octa::Conditional<octa::IsReference<_D>::value,
            _D, octa::AddLvalueReference<const _D>
        > __d): __stor(__p, __d) {}

        Box(Pointer __p, octa::RemoveReference<_D> &&__d):
        __stor(__p, octa::move(__d)) {
            static_assert(!octa::IsReference<_D>::value,
                "rvalue deleter cannot be a ref");
        }

        Box(Box &&__u): __stor(__u.release(),
            octa::forward<_D>(__u.get_deleter())) {}

        template<typename _TT, typename _DD>
        Box(Box<_TT, _DD> &&__u, octa::EnableIf<!octa::IsArray<_TT>::value
            && octa::IsConvertible<typename Box<_TT, _DD>::Pointer, Pointer>::value
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
            && octa::IsConvertible<typename Box<_TT, _DD>::Pointer, Pointer>::value
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
        Pointer operator->() const { return __stor.__ptr; }

        explicit operator bool() const {
            return __stor.__ptr != nullptr;
        }

        Pointer get() const { return __stor.__ptr; }

        _D_ref  get_deleter()       { return __stor.get_deleter(); }
        _D_cref get_deleter() const { return __stor.get_deleter(); }

        Pointer release() {
            Pointer __p = __stor.__ptr;
            __stor.__ptr = nullptr;
            return __p;
        }

        void reset(Pointer __p = nullptr) {
            Pointer __tmp = __stor.__ptr;
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
        || octa::IsSame<_T, _U>::value || __OctaHasElement<_T>::value
    > struct __OctaSameOrLessCvQualified: __OctaSameOrLessCvQualifiedBase<_T, _U> {};

    template<typename _T, typename _U>
    struct __OctaSameOrLessCvQualified<_T, _U, false>: octa::False {};

    template<typename _T, typename _D>
    struct Box<_T[], _D> {
        typedef _T Element;
        typedef _D Deleter;
        typedef typename __OctaPointer<_T, _D>::Type Pointer;

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
            __OctaSameOrLessCvQualified<_U, Pointer>::value, __OctaNat
        > = __OctaNat()): __stor(__p, _D()) {
            static_assert(!octa::IsPointer<_D>::value,
                "Box constructed with null fptr deleter");
        }

        template<typename _U> Box(_U __p, octa::Conditional<
            octa::IsReference<_D>::value,
            _D, AddLvalueReference<const _D>
        > __d, octa::EnableIf<__OctaSameOrLessCvQualified<_U, Pointer>::value,
        __OctaNat> = __OctaNat()): __stor(__p, __d) {}

        Box(nullptr_t, octa::Conditional<octa::IsReference<_D>::value,
            _D, AddLvalueReference<const _D>
        > __d): __stor(nullptr, __d) {}

        template<typename _U> Box(_U __p, octa::RemoveReference<_D> &&__d,
        octa::EnableIf<
            __OctaSameOrLessCvQualified<_U, Pointer>::value, __OctaNat
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
            && __OctaSameOrLessCvQualified<typename Box<_TT, _DD>::Pointer,
                                           Pointer>::value
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
            && __OctaSameOrLessCvQualified<typename Box<_TT, _DD>::Pointer,
                                           Pointer>::value
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

        Pointer get() const { return __stor.__ptr; }

        _D_ref  get_deleter()       { return __stor.get_deleter(); }
        _D_cref get_deleter() const { return __stor.get_deleter(); }

        Pointer release() {
            Pointer __p = __stor.__ptr;
            __stor.__ptr = nullptr;
            return __p;
        }

        template<typename _U> EnableIf<
            __OctaSameOrLessCvQualified<_U, Pointer>::value, void
        > reset(_U __p) {
            Pointer __tmp = __stor.__ptr;
            __stor.__ptr = __p;
            if (__tmp) __stor.get_deleter()(__tmp);
        }

        void reset(nullptr_t) {
            Pointer __tmp = __stor.__ptr;
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
    typedef void        Value;
    typedef void       *Pointer;
    typedef const void *ConstPointer;

    template<typename _U> using Rebind = Allocator<_U>;
};

template<> struct Allocator<const void> {
    typedef const void  Value;
    typedef const void *Pointer;
    typedef const void *ConstPointer;

    template<typename _U> using Rebind = Allocator<_U>;
};

template<typename _T> struct Allocator {
    typedef size_t     Size;
    typedef ptrdiff_t  Difference;
    typedef _T         Value;
    typedef _T        &Reference;
    typedef const _T  &ConstReference;
    typedef _T        *Pointer;
    typedef const _T  *ConstPointer;

    template<typename _U> using Rebind = Allocator<_U>;

    Pointer address(Reference v) const {
        return address_of(v);
    };
    ConstPointer address(ConstReference v) const {
        return address_of(v);
    };

    Size max_size() const { return Size(~0) / sizeof(_T); }

    Pointer allocate(Size n, Allocator<void>::ConstPointer = nullptr) {
        return (Pointer) ::new uchar[n * sizeof(_T)];
    }

    void deallocate(Pointer p, Size) { ::delete[] (uchar *) p; }

    template<typename _U, typename ..._A>
    void construct(_U *p, _A &&...args) {
        ::new((void *)p) _U(octa::forward<_A>(args)...);
    }

    void destroy(Pointer p) { p->~_T(); }
};

template<typename _T> struct Allocator<const _T> {
    typedef size_t     Size;
    typedef ptrdiff_t  Difference;
    typedef const _T   Value;
    typedef const _T  &Reference;
    typedef const _T  &ConstReference;
    typedef const _T  *Pointer;
    typedef const _T  *ConstPointer;

    template<typename _U> using Rebind = Allocator<_U>;

    ConstPointer address(ConstReference v) const {
        return address_of(v);
    };

    Size max_size() const { return Size(~0) / sizeof(_T); }

    Pointer allocate(Size n, Allocator<void>::ConstPointer = nullptr) {
        return (Pointer) ::new uchar[n * sizeof(_T)];
    }

    void deallocate(Pointer p, Size) { ::delete[] (uchar *) p; }

    template<typename _U, typename ..._A>
    void construct(_U *p, _A &&...args) {
        ::new((void *)p) _U(octa::forward<_A>(args)...);
    }

    void destroy(Pointer p) { p->~_T(); }
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

namespace detail {
    template<typename _T>
    struct ConstPtrTest {
        template<typename _U> static char test(
            typename _U::ConstPointer * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _T, typename _P, typename _A,
        bool = ConstPtrTest<_A>::value>
    struct ConstPointer {
        typedef typename _A::ConstPointer Type;
    };

    template<typename _T, typename _P, typename _A>
    struct ConstPointer<_T, _P, _A, false> {
        typedef PointerRebind<_P, const _T> Type;
    };

    template<typename _T>
    struct VoidPtrTest {
        template<typename _U> static char test(
            typename _U::VoidPointer * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _P, typename _A, bool = VoidPtrTest<_A>::value>
    struct VoidPointer {
        typedef typename _A::VoidPointer Type;
    };

    template<typename _P, typename _A>
    struct VoidPointer<_P, _A, false> {
        typedef PointerRebind<_P, void> Type;
    };

    template<typename _T>
    struct ConstVoidPtrTest {
        template<typename _U> static char test(
            typename _U::ConstVoidPointer * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _P, typename _A, bool = ConstVoidPtrTest<_A>::value>
    struct ConstVoidPointer {
        typedef typename _A::ConstVoidPointer Type;
    };

    template<typename _P, typename _A>
    struct ConstVoidPointer<_P, _A, false> {
        typedef PointerRebind<_P, const void> Type;
    };

    template<typename _T>
    struct SizeTest {
        template<typename _U> static char test(typename _U::Size * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _A, typename _D, bool = SizeTest<_A>::value>
    struct SizeBase {
        typedef octa::MakeUnsigned<_D> Type;
    };

    template<typename _A, typename _D>
    struct SizeBase<_A, _D, true> {
        typedef typename _A::Size Type;
    };
} /* namespace detail */

/* allocator type traits */

template<typename _A>
using AllocatorType = _A;

template<typename _A>
using AllocatorValue = typename AllocatorType<_A>::Value;

template<typename _A>
using AllocatorPointer = typename __OctaPointer<
    AllocatorValue<_A>, AllocatorType <_A>
>::Type;

template<typename _A>
using AllocatorConstPointer = typename octa::detail::ConstPointer<
    AllocatorValue<_A>, AllocatorPointer<_A>, AllocatorType<_A>
>::Type;

template<typename _A>
using AllocatorVoidPointer = typename octa::detail::VoidPointer<
    AllocatorPointer<_A>, AllocatorType<_A>
>::Type;

template<typename _A>
using AllocatorConstVoidPointer = typename octa::detail::ConstVoidPointer<
    AllocatorPointer<_A>, AllocatorType<_A>
>::Type;

/* allocator difference */

namespace detail {
    template<typename _T>
    struct DiffTest {
        template<typename _U> static char test(typename _U::Difference * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _A, typename _P, bool = DiffTest<_A>::value>
    struct AllocDifference {
        typedef PointerDifference<_P> Type;
    };

    template<typename _A, typename _P>
    struct AllocDifference<_A, _P, true> {
        typedef typename _A::Difference Type;
    };
}

template<typename _A>
using AllocatorDifference = typename octa::detail::AllocDifference<
    _A, AllocatorPointer<_A>
>::Type;

/* allocator size */

template<typename _A>
using AllocatorSize = typename octa::detail::SizeBase<
    _A, AllocatorDifference<_A>
>::Type;

/* allocator rebind */

namespace detail {
    template<typename _T, typename _U, bool = __OctaHasRebind<_T, _U>::value>
    struct AllocTraitsRebindType {
        typedef typename _T::template Rebind<_U> Type;
    };

    template<template<typename, typename...> class _A, typename _T,
        typename ..._Args, typename _U
    >
    struct AllocTraitsRebindType<_A<_T, _Args...>, _U, true> {
        typedef typename _A<_T, _Args...>::template Rebind<_U> Type;
    };

    template<template<typename, typename...> class _A, typename _T,
        typename ..._Args, typename _U
    >
    struct AllocTraitsRebindType<_A<_T, _Args...>, _U, false> {
        typedef _A<_U, _Args...> Type;
    };
} /* namespace detail */

template<typename _A, typename _T>
using AllocatorRebind = typename octa::detail::AllocTraitsRebindType<
    AllocatorType<_A>, _T
>::Type;

/* allocator propagate on container copy assignment */

namespace detail {
    template<typename _T>
    struct PropagateOnContainerCopyAssignmentTest {
        template<typename _U> static char test(
            typename _U::PropagateOnContainerCopyAssignment * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _A, bool = PropagateOnContainerCopyAssignmentTest<
        _A
    >::value> struct PropagateOnContainerCopyAssignmentBase {
        typedef octa::False Type;
    };

    template<typename _A>
    struct PropagateOnContainerCopyAssignmentBase<_A, true> {
        typedef typename _A::PropagateOnContainerCopyAssignment Type;
    };
} /* namespace detail */

template<typename _A>
using PropagateOnContainerCopyAssignment
    = typename octa::detail::PropagateOnContainerCopyAssignmentBase<_A>::Type;

/* allocator propagate on container move assignment */

namespace detail {
    template<typename _T>
    struct PropagateOnContainerMoveAssignmentTest {
        template<typename _U> static char test(
            typename _U::PropagateOnContainerMoveAssignment * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _A, bool = PropagateOnContainerMoveAssignmentTest<
        _A
    >::value> struct PropagateOnContainerMoveAssignmentBase {
        typedef octa::False Type;
    };

    template<typename _A>
    struct PropagateOnContainerMoveAssignmentBase<_A, true> {
        typedef typename _A::PropagateOnContainerMoveAssignment Type;
    };
} /* namespace detail */

template<typename _A>
using PropagateOnContainerMoveAssignment
    = typename octa::detail::PropagateOnContainerMoveAssignmentBase<_A>::Type;

/* allocator propagate on container swap */

namespace detail {
    template<typename _T>
    struct PropagateOnContainerSwapTest {
        template<typename _U> static char test(
            typename _U::PropagateOnContainerSwap * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _A, bool = PropagateOnContainerSwapTest<_A>::value>
    struct PropagateOnContainerSwapBase {
        typedef octa::False Type;
    };

    template<typename _A>
    struct PropagateOnContainerSwapBase<_A, true> {
        typedef typename _A::PropagateOnContainerSwap Type;
    };
} /* namespace detail */

template<typename _A>
using PropagateOnContainerSwap
    = typename octa::detail::PropagateOnContainerSwapBase<_A>::Type;

/* allocator is always equal */

namespace detail {
    template<typename _T>
    struct IsAlwaysEqualTest {
        template<typename _U> static char test(typename _U::IsAlwaysEqual * = 0);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _A, bool = IsAlwaysEqualTest<_A>::value>
    struct IsAlwaysEqualBase {
        typedef typename octa::IsEmpty<_A>::Type Type;
    };

    template<typename _A>
    struct IsAlwaysEqualBase<_A, true> {
        typedef typename _A::IsAlwaysEqual Type;
    };
} /* namespace detail */

template<typename _A>
using IsAlwaysEqual = typename octa::detail::IsAlwaysEqualBase<_A>::Type;

/* allocator allocate */

template<typename _A>
inline AllocatorPointer<_A>
allocator_allocate(_A &a, AllocatorSize<_A> n) {
    return a.allocate(n);
}

namespace detail {
    template<typename _A, typename _S, typename _CVP>
    auto allocate_hint_test(_A &&a, _S &&sz, _CVP &&p)
        -> decltype(a.allocate(sz, p), octa::True());

    template<typename _A, typename _S, typename _CVP>
    auto allocate_hint_test(const _A &, _S &&, _CVP &&)
        -> octa::False;

    template<typename _A, typename _S, typename _CVP>
    struct AllocateHintTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(allocate_hint_test(octa::declval<_A>(),
                                        octa::declval<_S>(),
                                        octa::declval<_CVP>())),
            octa::True
        >::value
    > {};

    template<typename _A>
    inline AllocatorPointer<_A> allocate(_A &a, AllocatorSize<_A> n,
                                         AllocatorConstVoidPointer<_A> h,
                                         octa::True) {
        return a.allocate(n, h);
    }

    template<typename _A>
    inline AllocatorPointer<_A> allocate(_A &a, AllocatorSize<_A> n,
                                         AllocatorConstVoidPointer<_A>,
                                         octa::False) {
        return a.allocate(n);
    }
} /* namespace detail */

template<typename _A>
inline AllocatorPointer<_A>
allocator_allocate(_A &a, AllocatorSize<_A> n,
                   AllocatorConstVoidPointer<_A> h) {
    return octa::detail::allocate(a, n, h,
        octa::detail::AllocateHintTest<
            _A, AllocatorSize<_A>, AllocatorConstVoidPointer<_A>
        >());
}

/* allocator deallocate */

template<typename _A>
inline void allocator_deallocate(_A &a, AllocatorPointer<_A> p,
                                 AllocatorSize<_A> n) {
    a.deallocate(p, n);
}

/* allocator construct */

namespace detail {
    template<typename _A, typename _T, typename ..._Args>
    auto construct_test(_A &&a, _T *p, _Args &&...args)
        -> decltype(a.construct(p, octa::forward<_Args>(args)...),
            octa::True());

    template<typename _A, typename _T, typename ..._Args>
    auto construct_test(const _A &, _T *, _Args &&...)
        -> octa::False;

    template<typename _A, typename _T, typename ..._Args>
    struct ConstructTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(construct_test(octa::declval<_A>(),
                                    octa::declval<_T>(),
                                    octa::declval<_Args>()...)),
            octa::True
        >::value
    > {};

    template<typename _A, typename _T, typename ..._Args>
    inline void construct(octa::True, _A &a, _T *p, _Args &&...args) {
        a.construct(p, octa::forward<_Args>(args)...);
    }

    template<typename _A, typename _T, typename ..._Args>
    inline void construct(octa::False, _A &, _T *p, _Args &&...args) {
        ::new ((void *)p) _T(octa::forward<_Args>(args)...);
    }
} /* namespace detail */

template<typename _A, typename _T, typename ..._Args>
inline void allocator_construct(_A &a, _T *p, _Args &&...args) {
    octa::detail::construct(octa::detail::ConstructTest<
        _A, _T *, _Args...
    >(), a, p, octa::forward<_Args>(args)...);
}

/* allocator destroy */

namespace detail {
    template<typename _A, typename _P>
    auto destroy_test(_A &&a, _P &&p) -> decltype(a.destroy(p), octa::True());

    template<typename _A, typename _P>
    auto destroy_test(const _A &, _P &&) -> octa::False;

    template<typename _A, typename _P>
    struct DestroyTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(destroy_test(octa::declval<_A>(), octa::declval<_P>())),
            octa::True
        >::value
    > {};

    template<typename _A, typename _T>
    inline void destroy(octa::True, _A &a, _T *p) {
        a.destroy(p);
    }

    template<typename _A, typename _T>
    inline void destroy(octa::False, _A &, _T *p) {
        p->~_T();
    }
} /* namespace detail */

template<typename _A, typename _T>
inline void allocator_destroy(_A &a, _T *p) {
    octa::detail::destroy(octa::detail::DestroyTest<_A, _T *>(), a, p);
}

/* allocator max size */

namespace detail {
    template<typename _A>
    auto alloc_max_size_test(_A &&a) -> decltype(a.max_size(), octa::True());

    template<typename _A>
    auto alloc_max_size_test(const _A &) -> octa::False;

    template<typename _A>
    struct AllocMaxSizeTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(alloc_max_size_test(octa::declval<_A &>())),
            octa::True
        >::value
    > {};

    template<typename _A>
    inline AllocatorSize<_A> alloc_max_size(octa::True, const _A &a) {
        return a.max_size();
    }

    template<typename _A>
    inline AllocatorSize<_A> alloc_max_size(octa::False, const _A &) {
        return AllocatorSize<_A>(~0);
    }
} /* namespace detail */

template<typename _A>
inline AllocatorSize<_A> allocator_max_size(const _A &a) {
    return octa::detail::alloc_max_size(octa::detail::AllocMaxSizeTest<
        const _A
    >(), a);
}

/* allocator container copy */

namespace detail {
    template<typename _A>
    auto alloc_copy_test(_A &&a) -> decltype(a.container_copy(), octa::True());

    template<typename _A>
    auto alloc_copy_test(const _A &) -> octa::False;

    template<typename _A>
    struct AllocCopyTest: octa::IntegralConstant<bool,
        octa::IsSame<
            decltype(alloc_copy_test(octa::declval<_A &>())), octa::True
        >::value
    > {};

    template<typename _A>
    inline AllocatorType<_A> alloc_container_copy(octa::True, const _A &a) {
        return a.container_copy();
    }

    template<typename _A>
    inline AllocatorType<_A> alloc_container_copy(octa::False, const _A &a) {
        return a;
    }
} /* namespace detail */

template<typename _A>
inline AllocatorType<_A> allocator_container_copy(const _A &a) {
    return octa::detail::alloc_container_copy(octa::detail::AllocCopyTest<
        const _A
    >(), a);
}
}

#endif