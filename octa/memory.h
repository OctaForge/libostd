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

namespace detail {
    template<typename _T>
    struct HasElement {
        template<typename _U> static int test(...);
        template<typename _U> static char test(typename _U::Element * = 0);

        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _T, bool = HasElement<_T>::value>
    struct PointerElementBase;

    template<typename _T> struct PointerElementBase<_T, true> {
        typedef typename _T::Element Type;
    };

    template<template<typename, typename...> class _T, typename _U, typename ..._A>
    struct PointerElementBase<_T<_U, _A...>, true> {
        typedef typename _T<_U, _A...>::Element Type;
    };

    template<template<typename, typename...> class _T, typename _U, typename ..._A>
    struct PointerElementBase<_T<_U, _A...>, false> {
        typedef _U Type;
    };

    template<typename _T>
    struct PointerElementType {
        typedef typename PointerElementBase<_T>::Type Type;
    };

    template<typename _T>
    struct PointerElementType<_T *> {
        typedef _T Type;
    };

    template<typename _T>
    struct HasDifference {
        template<typename _U> static int test(...);
        template<typename _U> static char test(typename _U::Difference * = 0);

        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _T, bool = HasDifference<_T>::value>
    struct PointerDifferenceBase {
        typedef ptrdiff_t Type;
    };

    template<typename _T> struct PointerDifferenceBase<_T, true> {
        typedef typename _T::Difference Type;
    };

    template<typename _T>
    struct PointerDifferenceType {
        typedef typename PointerDifferenceBase<_T>::Type Type;
    };

    template<typename _T>
    struct PointerDifferenceType<_T *> {
        typedef ptrdiff_t Type;
    };

    template<typename _T, typename _U>
    struct HasRebind {
        template<typename _V> static int test(...);
        template<typename _V> static char test(
            typename _V::template Rebind<_U> * = 0);

        static constexpr bool value = (sizeof(test<_T>(0)) == 1);
    };

    template<typename _T, typename _U, bool = HasRebind<_T, _U>::value>
    struct PointerRebindBase {
        typedef typename _T::template Rebind<_U> Type;
    };

    template<template<typename, typename...> class _T, typename _U,
        typename ..._A, typename _V
    >
    struct PointerRebindBase<_T<_U, _A...>, _V, true> {
        typedef typename _T<_U, _A...>::template Rebind<_V> Type;
    };

    template<template<typename, typename...> class _T, typename _U,
        typename ..._A, typename _V
    >
    struct PointerRebindBase<_T<_U, _A...>, _V, false> {
        typedef _T<_V, _A...> Type;
    };

    template<typename _T, typename _U>
    struct PointerRebindType {
        using type = typename PointerRebindBase<_T, _U>::Type;
    };

    template<typename _T, typename _U>
    struct PointerRebindType<_T *, _U> {
        using type = _U *;
    };

    template<typename _T>
    struct PointerPointer {
        typedef _T Type;
    };

    template<typename _T>
    struct PointerPointer<_T *> {
        typedef _T *Type;
    };
} /*namespace detail */

template<typename _T>
using Pointer = typename octa::detail::PointerPointer<_T>::Type;

template<typename _T>
using PointerElement = typename octa::detail::PointerElementType<_T>::Type;

template<typename _T>
using PointerDifference = typename octa::detail::PointerDifferenceType<_T>::Type;

template<typename _T, typename _U>
using PointerRebind = typename octa::detail::PointerRebindType<_T, _U>::Type;

/* pointer to */

namespace detail {
    struct PointerToNat {};

    template<typename _T>
    struct PointerTo {
        static _T pointer_to(octa::Conditional<
            octa::IsVoid<PointerElement<_T>>::value,
            PointerToNat, PointerElement<_T>
        > &r) {
            return _T::pointer_to(r);
        }
    };

    template<typename _T>
    struct PointerTo<_T *> {
        static _T pointer_to(octa::Conditional<
            octa::IsVoid<_T>::value, PointerToNat, _T
        > &r) {
            return octa::address_of(r);
        }
    };
}

template<typename _T>
static _T pointer_to(octa::Conditional<
    octa::IsVoid<PointerElement<_T>>::value,
    octa::detail::PointerToNat, PointerElement<_T>
> &r) {
    return octa::detail::PointerTo<_T>::pointer_to(r);
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

namespace detail {
    template<typename _T, typename _U, bool = octa::IsEmpty<_U>::value>
    struct BoxPair;

    template<typename _T, typename _U>
    struct BoxPair<_T, _U, false> { /* non-empty deleter */
        _T *p_ptr;
    private:
        _U  p_del;

    public:
        template<typename _D>
        BoxPair(_T *ptr, _D &&dltr): p_ptr(ptr), p_del(octa::forward<_D>(dltr)) {}

        _U &get_deleter() { return p_del; }
        const _U &get_deleter() const { return p_del; }

        void swap(BoxPair &v) {
            octa::swap(p_ptr, v.p_ptr);
            octa::swap(p_del, v.p_del);
        }
    };

    template<typename _T, typename _U>
    struct BoxPair<_T, _U, true>: _U { /* empty deleter */
        _T *p_ptr;

        template<typename _D>
        BoxPair(_T *ptr, _D &&dltr): _U(octa::forward<_D>(dltr)), p_ptr(ptr) {}

        _U &get_deleter() { return *this; }
        const _U &get_deleter() const { return *this; }

        void swap(BoxPair &v) {
            octa::swap(p_ptr, v.p_ptr);
        }
    };

    template<typename _T>
    static int ptr_test(...);
    template<typename _T>
    static char ptr_test(typename _T::Pointer * = 0);

    template<typename _T> struct HasPtr: octa::IntegralConstant<bool,
        (sizeof(ptr_test<_T>(0)) == 1)
    > {};

    template<typename _T, typename _D, bool = HasPtr<_D>::value>
    struct PointerBase {
        typedef typename _D::Pointer Type;
    };

    template<typename _T, typename _D> struct PointerBase<_T, _D, false> {
        typedef _T *Type;
    };

    template<typename _T, typename _D> struct PointerType {
        typedef typename PointerBase<_T, octa::RemoveReference<_D>>::Type Type;
    };
} /* namespace detail */

template<typename _T, typename _D = DefaultDelete<_T>>
struct Box {
    typedef _T Element;
    typedef _D Deleter;
    typedef typename octa::detail::PointerType<_T, _D>::Type Pointer;

private:
    struct Nat { int x; };

    typedef RemoveReference<_D> &_D_ref;
    typedef const RemoveReference<_D> &_D_cref;

public:
    constexpr Box(): p_stor(nullptr, _D()) {
        static_assert(!octa::IsPointer<_D>::value,
            "Box constructed with null fptr deleter");
    }
    constexpr Box(nullptr_t): p_stor(nullptr, _D()) {
        static_assert(!octa::IsPointer<_D>::value,
            "Box constructed with null fptr deleter");
    }

    explicit Box(Pointer p): p_stor(p, _D()) {
        static_assert(!octa::IsPointer<_D>::value,
            "Box constructed with null fptr deleter");
    }

    Box(Pointer p, octa::Conditional<octa::IsReference<_D>::value,
        _D, octa::AddLvalueReference<const _D>
    > d): p_stor(p, d) {}

    Box(Pointer p, octa::RemoveReference<_D> &&d):
    p_stor(p, octa::move(d)) {
        static_assert(!octa::IsReference<_D>::value,
            "rvalue deleter cannot be a ref");
    }

    Box(Box &&u): p_stor(u.release(), octa::forward<_D>(u.get_deleter())) {}

    template<typename _TT, typename _DD>
    Box(Box<_TT, _DD> &&u, octa::EnableIf<!octa::IsArray<_TT>::value
        && octa::IsConvertible<typename Box<_TT, _DD>::Pointer, Pointer>::value
        && octa::IsConvertible<_DD, _D>::value
        && (!octa::IsReference<_D>::value || octa::IsSame<_D, _DD>::value)
    > = Nat()): p_stor(u.release(), octa::forward<_DD>(u.get_deleter())) {}

    Box &operator=(Box &&u) {
        reset(u.release());
        p_stor.get_deleter() = octa::forward<_D>(u.get_deleter());
        return *this;
    }

    template<typename _TT, typename _DD>
    EnableIf<!octa::IsArray<_TT>::value
        && octa::IsConvertible<typename Box<_TT, _DD>::Pointer, Pointer>::value
        && octa::IsAssignable<_D &, _DD &&>::value,
        Box &
    > operator=(Box<_TT, _DD> &&u) {
        reset(u.release());
        p_stor.get_deleter() = octa::forward<_DD>(u.get_deleter());
        return *this;
    }

    Box &operator=(nullptr_t) {
        reset();
        return *this;
    }

    ~Box() { reset(); }

    octa::AddLvalueReference<_T> operator*() const { return *p_stor.p_ptr; }
    Pointer operator->() const { return p_stor.p_ptr; }

    explicit operator bool() const {
        return p_stor.p_ptr != nullptr;
    }

    Pointer get() const { return p_stor.p_ptr; }

    _D_ref  get_deleter()       { return p_stor.get_deleter(); }
    _D_cref get_deleter() const { return p_stor.get_deleter(); }

    Pointer release() {
        Pointer p = p_stor.p_ptr;
        p_stor.p_ptr = nullptr;
        return p;
    }

    void reset(Pointer p = nullptr) {
        Pointer tmp = p_stor.p_ptr;
        p_stor.p_ptr = p;
        if (tmp) p_stor.get_deleter()(tmp);
    }

    void swap(Box &u) {
        p_stor.swap(u.p_stor);
    }

private:
    octa::detail::BoxPair<_T, _D> p_stor;
};

namespace detail {
    template<typename _T, typename _U, bool = octa::IsSame<
        octa::RemoveCv<PointerElement<_T>>,
        octa::RemoveCv<PointerElement<_U>>
    >::value> struct SameOrLessCvQualifiedBase: octa::IsConvertible<_T, _U> {};

    template<typename _T, typename _U>
    struct SameOrLessCvQualifiedBase<_T, _U, false>: octa::False {};

    template<typename _T, typename _U, bool = octa::IsPointer<_T>::value
        || octa::IsSame<_T, _U>::value || octa::detail::HasElement<_T>::value
    > struct SameOrLessCvQualified: SameOrLessCvQualifiedBase<_T, _U> {};

    template<typename _T, typename _U>
    struct SameOrLessCvQualified<_T, _U, false>: octa::False {};
} /* namespace detail */

template<typename _T, typename _D>
struct Box<_T[], _D> {
    typedef _T Element;
    typedef _D Deleter;
    typedef typename octa::detail::PointerType<_T, _D>::Type Pointer;

private:
    struct Nat { int x; };

    typedef RemoveReference<_D> &_D_ref;
    typedef const RemoveReference<_D> &_D_cref;

public:
    constexpr Box(): p_stor(nullptr, _D()) {
        static_assert(!octa::IsPointer<_D>::value,
            "Box constructed with null fptr deleter");
    }
    constexpr Box(nullptr_t): p_stor(nullptr, _D()) {
        static_assert(!octa::IsPointer<_D>::value,
            "Box constructed with null fptr deleter");
    }

    template<typename _U> explicit Box(_U p, octa::EnableIf<
        octa::detail::SameOrLessCvQualified<_U, Pointer>::value, Nat
    > = Nat()): p_stor(p, _D()) {
        static_assert(!octa::IsPointer<_D>::value,
            "Box constructed with null fptr deleter");
    }

    template<typename _U> Box(_U p, octa::Conditional<
        octa::IsReference<_D>::value,
        _D, AddLvalueReference<const _D>
    > d, octa::EnableIf<octa::detail::SameOrLessCvQualified<_U, Pointer>::value,
    Nat> = Nat()): p_stor(p, d) {}

    Box(nullptr_t, octa::Conditional<octa::IsReference<_D>::value,
        _D, AddLvalueReference<const _D>
    > d): p_stor(nullptr, d) {}

    template<typename _U> Box(_U p, octa::RemoveReference<_D> &&d,
    octa::EnableIf<
        octa::detail::SameOrLessCvQualified<_U, Pointer>::value, Nat
    > = Nat()): p_stor(p, octa::move(d)) {
        static_assert(!octa::IsReference<_D>::value,
            "rvalue deleter cannot be a ref");
    }

    Box(nullptr_t, octa::RemoveReference<_D> &&d):
    p_stor(nullptr, octa::move(d)) {
        static_assert(!octa::IsReference<_D>::value,
            "rvalue deleter cannot be a ref");
    }

    Box(Box &&u): p_stor(u.release(), octa::forward<_D>(u.get_deleter())) {}

    template<typename _TT, typename _DD>
    Box(Box<_TT, _DD> &&u, EnableIf<IsArray<_TT>::value
        && octa::detail::SameOrLessCvQualified<typename Box<_TT, _DD>::Pointer,
                                               Pointer>::value
        && octa::IsConvertible<_DD, _D>::value
        && (!octa::IsReference<_D>::value ||
             octa::IsSame<_D, _DD>::value)> = Nat()
    ): p_stor(u.release(), octa::forward<_DD>(u.get_deleter())) {}

    Box &operator=(Box &&u) {
        reset(u.release());
        p_stor.get_deleter() = octa::forward<_D>(u.get_deleter());
        return *this;
    }

    template<typename _TT, typename _DD>
    EnableIf<octa::IsArray<_TT>::value
        && octa::detail::SameOrLessCvQualified<typename Box<_TT, _DD>::Pointer,
                                               Pointer>::value
        && IsAssignable<_D &, _DD &&>::value,
        Box &
    > operator=(Box<_TT, _DD> &&u) {
        reset(u.release());
        p_stor.get_deleter() = octa::forward<_DD>(u.get_deleter());
        return *this;
    }

    Box &operator=(nullptr_t) {
        reset();
        return *this;
    }

    ~Box() { reset(); }

    octa::AddLvalueReference<_T> operator[](size_t idx) const {
        return p_stor.p_ptr[idx];
    }

    explicit operator bool() const {
        return p_stor.p_ptr != nullptr;
    }

    Pointer get() const { return p_stor.p_ptr; }

    _D_ref  get_deleter()       { return p_stor.get_deleter(); }
    _D_cref get_deleter() const { return p_stor.get_deleter(); }

    Pointer release() {
        Pointer p = p_stor.p_ptr;
        p_stor.p_ptr = nullptr;
        return p;
    }

    template<typename _U> EnableIf<
        octa::detail::SameOrLessCvQualified<_U, Pointer>::value, void
    > reset(_U p) {
        Pointer tmp = p_stor.p_ptr;
        p_stor.p_ptr = p;
        if (tmp) p_stor.get_deleter()(tmp);
    }

    void reset(nullptr_t) {
        Pointer tmp = p_stor.p_ptr;
        p_stor.p_ptr = nullptr;
        if (tmp) p_stor.get_deleter()(tmp);
    }

    void reset() {
        reset(nullptr);
    }

    void swap(Box &u) {
        p_stor.swap(u.p_stor);
    }

private:
    octa::detail::BoxPair<_T, _D> p_stor;
};

namespace detail {
    template<typename _T> struct BoxIf {
        typedef octa::Box<_T> Box;
    };

    template<typename _T> struct BoxIf<_T[]> {
        typedef octa::Box<_T[]> BoxUnknownSize;
    };

    template<typename _T, size_t _N> struct BoxIf<_T[_N]> {
        typedef void BoxKnownSize;
    };
}

template<typename _T, typename ..._A>
typename octa::detail::BoxIf<_T>::Box make_box(_A &&...args) {
    return Box<_T>(new _T(octa::forward<_A>(args)...));
}

template<typename _T>
typename octa::detail::BoxIf<_T>::BoxUnknownSize make_box(size_t n) {
    return Box<_T>(new octa::RemoveExtent<_T>[n]());
}

template<typename _T, typename ..._A>
typename octa::detail::BoxIf<_T>::BoxKnownSize make_box(_A &&...args) = delete;

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
using AllocatorPointer = typename octa::detail::PointerType<
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
    template<typename _T, typename _U, bool = octa::detail::HasRebind<_T, _U>::value>
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