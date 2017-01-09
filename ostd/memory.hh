/* Memory utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_MEMORY_HH
#define OSTD_MEMORY_HH

#include <stddef.h>

#include "ostd/new.hh"
#include "ostd/utility.hh"
#include "ostd/type_traits.hh"

namespace ostd {
/* address of */

template<typename T> constexpr T *address_of(T &v) noexcept {
    return reinterpret_cast<T *>(
        &const_cast<char &>(reinterpret_cast<char const volatile &>(v))
    );
}

/* pointer traits */

namespace detail {
    template<typename T>
    struct HasElement {
        template<typename U>
        static int test(...);
        template<typename U>
        static char test(typename U::Element * = 0);

        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename T, bool = HasElement<T>::value>
    struct PointerElementBase;

    template<typename T>
    struct PointerElementBase<T, true> {
        using Type = typename T::Element;
    };

    template<template<typename, typename...> class T, typename U, typename ...A>
    struct PointerElementBase<T<U, A...>, true> {
        using Type = typename T<U, A...>::Element;
    };

    template<template<typename, typename...> class T, typename U, typename ...A>
    struct PointerElementBase<T<U, A...>, false> {
        using Type = U;
    };

    template<typename T>
    struct PointerElementType {
        using Type = typename PointerElementBase<T>::Type;
    };

    template<typename T>
    struct PointerElementType<T *> {
        using Type = T;
    };

    template<typename T>
    struct HasDifference {
        template<typename U>
        static int test(...);
        template<typename U>
        static char test(typename U::Difference * = 0);

        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename T, bool = HasDifference<T>::value>
    struct PointerDifferenceBase {
        using Type = Ptrdiff;
    };

    template<typename T>
    struct PointerDifferenceBase<T, true> {
        using Type = typename T::Difference;
    };

    template<typename T>
    struct PointerDifferenceType {
        using Type = typename PointerDifferenceBase<T>::Type;
    };

    template<typename T>
    struct PointerDifferenceType<T *> {
        using Type = Ptrdiff;
    };

    template<typename T, typename U>
    struct HasRebind {
        template<typename V>
        static int test(...);
        template<typename V>
        static char test(typename V::template Rebind<U> * = 0);

        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename T, typename U, bool = HasRebind<T, U>::value>
    struct PointerRebindBase {
        using Type = typename T::template Rebind<U>;
    };

    template<
        template<typename, typename...> class T, typename U,
        typename ...A, typename V
    >
    struct PointerRebindBase<T<U, A...>, V, true> {
        using Type = typename T<U, A...>::template Rebind<V>;
    };

    template<
        template<typename, typename...> class T, typename U,
        typename ...A, typename V
    >
    struct PointerRebindBase<T<U, A...>, V, false> {
        using Type = T<V, A...>;
    };

    template<typename T, typename U>
    struct PointerRebindType {
        using Type = typename PointerRebindBase<T, U>::Type;
    };

    template<typename T, typename U>
    struct PointerRebindType<T *, U> {
        using Type = U *;
    };

    template<typename T>
    struct PointerPointer {
        using Type = T;
    };

    template<typename T>
    struct PointerPointer<T *> {
        using Type = T *;
    };
} /*namespace detail */

template<typename T>
using Pointer = typename detail::PointerPointer<T>::Type;

template<typename T>
using PointerElement = typename detail::PointerElementType<T>::Type;

template<typename T>
using PointerDifference = typename detail::PointerDifferenceType<T>::Type;

template<typename T, typename U>
using PointerRebind = typename detail::PointerRebindType<T, U>::Type;

/* pointer to */

namespace detail {
    struct PointerToNat {};

    template<typename T>
    struct PointerTo {
        static T pointer_to(
            Conditional<
                IsVoid<PointerElement<T>>, PointerToNat, PointerElement<T>
            > &r
        ) {
            return T::pointer_to(r);
        }
    };

    template<typename T>
    struct PointerTo<T *> {
        static T pointer_to(Conditional<IsVoid<T>, PointerToNat, T> &r) {
            return address_of(r);
        }
    };
}

template<typename T>
static T pointer_to(
    Conditional<
        IsVoid<PointerElement<T>>, detail::PointerToNat, PointerElement<T>
    > &r
) {
    return detail::PointerTo<T>::pointer_to(r);
}

/* default deleter */

template<typename T>
struct DefaultDelete {
    constexpr DefaultDelete() = default;

    template<typename U>
    DefaultDelete(DefaultDelete<U> const &) noexcept {};

    void operator()(T *p) const {
        delete p;
    }
};

template<typename T>
struct DefaultDelete<T[]> {
    constexpr DefaultDelete() = default;

    template<typename U>
    DefaultDelete(DefaultDelete<U[]> const &) noexcept {};

    void operator()(T *p) const {
        delete[] p;
    }
    template<typename U>
    void operator()(U *) const = delete;
};

/* box */

namespace detail {
    template<typename T>
    static int ptr_test(...);
    template<typename T>
    static char ptr_test(typename T::Pointer * = 0);

    template<typename T>
    constexpr bool HasPtr = sizeof(ptr_test<T>(0)) == 1;

    template<typename T, typename D, bool = HasPtr<D>>
    struct PointerBase {
        using Type = typename D::Pointer;
    };

    template<typename T, typename D>
    struct PointerBase<T, D, false> {
        using Type = T *;
    };

    template<typename T, typename D>
    struct PointerType {
        using Type = typename PointerBase<T, RemoveReference<D>>::Type;
    };
} /* namespace detail */

template<typename T, typename D = DefaultDelete<T>>
struct Box {
    using Element = T;
    using Deleter = D;
    using Pointer = typename detail::PointerType<T, D>::Type;

private:
    struct Nat { int x; };

    using Dref = RemoveReference<D> &;
    using Dcref = RemoveReference<D> const &;

public:
    constexpr Box() noexcept: p_stor(nullptr, D()) {
        static_assert(!IsPointer<D>, "Box constructed with null fptr deleter");
    }
    constexpr Box(Nullptr) noexcept: p_stor(nullptr, D()) {
        static_assert(!IsPointer<D>, "Box constructed with null fptr deleter");
    }

    explicit Box(Pointer p) noexcept: p_stor(p, D()) {
        static_assert(!IsPointer<D>, "Box constructed with null fptr deleter");
    }

    Box(
        Pointer p, Conditional<IsReference<D>, D, AddLvalueReference<D const>> d
    ) noexcept:
        p_stor(p, d)
    {}

    Box(Pointer p, RemoveReference<D> &&d) noexcept:
        p_stor(p, move(d))
    {
        static_assert(!IsReference<D>, "rvalue deleter cannot be a ref");
    }

    Box(Box &&u) noexcept: p_stor(u.release(), forward<D>(u.get_deleter())) {}

    template<typename TT, typename DD>
    Box(
        Box<TT, DD> &&u, EnableIf<
            !IsArray<TT> &&
            IsConvertible<typename Box<TT, DD>::Pointer, Pointer> &&
            IsConvertible<DD, D> && (!IsReference<D> || IsSame<D, DD>),
            Nat
        > = Nat()
    ) noexcept: p_stor(u.release(), forward<DD>(u.get_deleter())) {}

    Box &operator=(Box &&u) noexcept {
        reset(u.release());
        p_stor.second() = forward<D>(u.get_deleter());
        return *this;
    }

    template<typename TT, typename DD>
    EnableIf<
        !IsArray<TT> &&
        IsConvertible<typename Box<TT, DD>::Pointer, Pointer> &&
        IsAssignable<D &, DD &&>,
        Box &
    > operator=(Box<TT, DD> &&u) noexcept {
        reset(u.release());
        p_stor.second() = forward<DD>(u.get_deleter());
        return *this;
    }

    Box &operator=(Nullptr) noexcept {
        reset();
        return *this;
    }

    ~Box() { reset(); }

    AddLvalueReference<T> operator*() const { return *p_stor.first(); }
    Pointer operator->() const noexcept { return p_stor.first(); }

    explicit operator bool() const noexcept {
        return p_stor.first() != nullptr;
    }

    Pointer get() const noexcept { return p_stor.first(); }

    Dref  get_deleter()       noexcept { return p_stor.second(); }
    Dcref get_deleter() const noexcept { return p_stor.second(); }

    Pointer release() noexcept {
        Pointer p = p_stor.first();
        p_stor.first() = nullptr;
        return p;
    }

    void reset(Pointer p = nullptr) noexcept {
        Pointer tmp = p_stor.first();
        p_stor.first() = p;
        if (tmp) {
            p_stor.second()(tmp);
        }
    }

    void swap(Box &u) noexcept {
        p_stor.swap(u.p_stor);
    }

private:
    detail::CompressedPair<T *, D> p_stor;
};

namespace detail {
    template<typename T, typename U, bool = IsSame<
        RemoveCv<PointerElement<T>>,
        RemoveCv<PointerElement<U>>
    >>
    constexpr bool SameOrLessCvQualifiedBase = IsConvertible<T, U>;

    template<typename T, typename U>
    constexpr bool SameOrLessCvQualifiedBase<T, U, false> = false;

    template<typename T, typename U, bool =
        IsPointer<T> || IsSame<T, U> || detail::HasElement<T>::value
    >
    constexpr bool SameOrLessCvQualified = SameOrLessCvQualifiedBase<T, U>;

    template<typename T, typename U>
    constexpr bool SameOrLessCvQualified<T, U, false> = false;
} /* namespace detail */

template<typename T, typename D>
struct Box<T[], D> {
    using Element = T;
    using Deleter = D;
    using Pointer = typename detail::PointerType<T, D>::Type;

private:
    struct Nat { int x; };

    using Dref = RemoveReference<D> &;
    using Dcref = RemoveReference<D> const &;

public:
    constexpr Box() noexcept: p_stor(nullptr, D()) {
        static_assert(!IsPointer<D>, "Box constructed with null fptr deleter");
    }
    constexpr Box(Nullptr) noexcept: p_stor(nullptr, D()) {
        static_assert(!IsPointer<D>, "Box constructed with null fptr deleter");
    }

    template<typename U> explicit Box(U p, EnableIf<
        detail::SameOrLessCvQualified<U, Pointer>, Nat
    > = Nat()) noexcept: p_stor(p, D()) {
        static_assert(!IsPointer<D>, "Box constructed with null fptr deleter");
    }

    template<typename U> Box(U p, Conditional<
        IsReference<D>, D, AddLvalueReference<D const>
    > d, EnableIf<
        detail::SameOrLessCvQualified<U, Pointer>, Nat
    > = Nat()) noexcept:
        p_stor(p, d)
    {}

    Box(
        Nullptr, Conditional<IsReference<D>, D, AddLvalueReference<D const>> d
    ) noexcept:
        p_stor(nullptr, d)
    {}

    template<typename U> Box(U p, RemoveReference<D> &&d,
    EnableIf<detail::SameOrLessCvQualified<U, Pointer>, Nat> = Nat()) noexcept:
        p_stor(p, move(d))
    {
        static_assert(!IsReference<D>, "rvalue deleter cannot be a ref");
    }

    Box(Nullptr, RemoveReference<D> &&d) noexcept:
        p_stor(nullptr, move(d))
    {
        static_assert(!IsReference<D>, "rvalue deleter cannot be a ref");
    }

    Box(Box &&u) noexcept: p_stor(u.release(), forward<D>(u.get_deleter())) {}

    template<typename TT, typename DD>
    Box(
        Box<TT, DD> &&u, EnableIf<
            IsArray<TT> &&
            detail::SameOrLessCvQualified<typename Box<TT, DD>::Pointer,Pointer> &&
            IsConvertible<DD, D> && (!IsReference<D> || IsSame<D, DD>),
            Nat
        > = Nat()
    ) noexcept:
        p_stor(u.release(), forward<DD>(u.get_deleter()))
    {}

    Box &operator=(Box &&u) noexcept {
        reset(u.release());
        p_stor.second() = forward<D>(u.get_deleter());
        return *this;
    }

    template<typename TT, typename DD>
    EnableIf<
        IsArray<TT> &&
        detail::SameOrLessCvQualified<typename Box<TT, DD>::Pointer, Pointer> &&
        IsAssignable<D &, DD &&>,
        Box &
    > operator=(Box<TT, DD> &&u) noexcept {
        reset(u.release());
        p_stor.second() = forward<DD>(u.get_deleter());
        return *this;
    }

    Box &operator=(Nullptr) noexcept {
        reset();
        return *this;
    }

    ~Box() { reset(); }

    AddLvalueReference<T> operator[](Size idx) const {
        return p_stor.first()[idx];
    }

    explicit operator bool() const noexcept {
        return p_stor.first() != nullptr;
    }

    Pointer get() const noexcept { return p_stor.first(); }

    Dref  get_deleter()       noexcept { return p_stor.second(); }
    Dcref get_deleter() const noexcept { return p_stor.second(); }

    Pointer release() noexcept {
        Pointer p = p_stor.first();
        p_stor.first() = nullptr;
        return p;
    }

    template<typename U>
    EnableIf<
        detail::SameOrLessCvQualified<U, Pointer>, void
    > reset(U p) noexcept {
        Pointer tmp = p_stor.first();
        p_stor.first() = p;
        if (tmp) {
            p_stor.second()(tmp);
        }
    }

    void reset(Nullptr) noexcept {
        Pointer tmp = p_stor.first();
        p_stor.first() = nullptr;
        if (tmp) {
            p_stor.second()(tmp);
        }
    }

    void reset() noexcept {
        reset(nullptr);
    }

    void swap(Box &u) noexcept {
        p_stor.swap(u.p_stor);
    }

private:
    detail::CompressedPair<T *, D> p_stor;
};

namespace detail {
    template<typename T>
    struct BoxIf {
        using BoxType = Box<T>;
    };

    template<typename T>
    struct BoxIf<T[]> {
        using BoxUnknownSize = Box<T[]>;
    };

    template<typename T, Size N>
    struct BoxIf<T[N]> {
        using BoxKnownSize = void;
    };
}

template<typename T, typename ...A>
typename detail::BoxIf<T>::BoxType make_box(A &&...args) {
    return Box<T>(new T(forward<A>(args)...));
}

template<typename T>
typename detail::BoxIf<T>::BoxUnknownSize make_box(Size n) {
    return Box<T>(new RemoveExtent<T>[n]());
}

template<typename T, typename ...A>
typename detail::BoxIf<T>::BoxKnownSize make_box(A &&...args) = delete;

/* allocator */

template<typename T>
struct Allocator {
    using Value = T;

    Allocator() noexcept {}
    template<typename U>
    Allocator(Allocator<U> const &) noexcept {}

    Value *allocate(Size n) {
        return reinterpret_cast<Value *>(::new byte[n * sizeof(T)]);
    }

    void deallocate(Value *p, Size) noexcept {
        ::delete[] reinterpret_cast<byte *>(p);
    }
};

template<typename T, typename U>
bool operator==(Allocator<T> const &, Allocator<U> const &) noexcept {
    return true;
}

template<typename T, typename U>
bool operator!=(Allocator<T> const &, Allocator<U> const &) noexcept {
    return false;
}

/* allocator traits - modeled after libc++ */

namespace detail {
    template<typename T>
    struct ConstPtrTest {
        template<typename U>
        static char test(typename U::ConstPointer * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename T, typename P, typename A, bool = ConstPtrTest<A>::value>
    struct ConstPointer {
        using Type = typename A::ConstPointer;
    };

    template<typename T, typename P, typename A>
    struct ConstPointer<T, P, A, false> {
        using Type = PointerRebind<P, T const>;
    };

    template<typename T>
    struct VoidPtrTest {
        template<typename U>
        static char test(typename U::VoidPointer * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename P, typename A, bool = VoidPtrTest<A>::value>
    struct VoidPointer {
        using Type = typename A::VoidPointer;
    };

    template<typename P, typename A>
    struct VoidPointer<P, A, false> {
        using Type = PointerRebind<P, void>;
    };

    template<typename T>
    struct ConstVoidPtrTest {
        template<typename U> static char test(
            typename U::ConstVoidPointer * = 0);
        template<typename U> static  int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename P, typename A, bool = ConstVoidPtrTest<A>::value>
    struct ConstVoidPointer {
        using Type = typename A::ConstVoidPointer;
    };

    template<typename P, typename A>
    struct ConstVoidPointer<P, A, false> {
        using Type = PointerRebind<P, void const>;
    };

    template<typename T>
    struct SizeTest {
        template<typename U>
        static char test(typename U::Size * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename A, typename D, bool = SizeTest<A>::value>
    struct SizeBase {
        using Type = MakeUnsigned<D>;
    };

    template<typename A, typename D>
    struct SizeBase<A, D, true> {
        using Type = typename A::Size;
    };
} /* namespace detail */

/* allocator type traits */

template<typename A>
using AllocatorType = A;

template<typename A>
using AllocatorValue = typename AllocatorType<A>::Value;

template<typename A>
using AllocatorPointer = typename detail::PointerType<
    AllocatorValue<A>, AllocatorType<A>
>::Type;

template<typename A>
using AllocatorConstPointer = typename detail::ConstPointer<
    AllocatorValue<A>, AllocatorPointer<A>, AllocatorType<A>
>::Type;

template<typename A>
using AllocatorVoidPointer = typename detail::VoidPointer<
    AllocatorPointer<A>, AllocatorType<A>
>::Type;

template<typename A>
using AllocatorConstVoidPointer = typename detail::ConstVoidPointer<
    AllocatorPointer<A>, AllocatorType<A>
>::Type;

/* allocator difference */

namespace detail {
    template<typename T>
    struct DiffTest {
        template<typename U>
        static char test(typename U::Difference * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename A, typename P, bool = DiffTest<A>::value>
    struct AllocDifference {
        using Type = PointerDifference<P>;
    };

    template<typename A, typename P>
    struct AllocDifference<A, P, true> {
        using Type = typename A::Difference;
    };
}

template<typename A>
using AllocatorDifference = typename detail::AllocDifference<
    A, AllocatorPointer<A>
>::Type;

/* allocator size */

template<typename A>
using AllocatorSize = typename detail::SizeBase<
    A, AllocatorDifference<A>
>::Type;

/* allocator rebind */

namespace detail {
    template<typename T, typename U, bool = detail::HasRebind<T, U>::value>
    struct AllocTraitsRebindType {
        using Type = typename T::template Rebind<U>;
    };

    template<
        template<typename, typename...> class A, typename T,
        typename ...Args, typename U
    >
    struct AllocTraitsRebindType<A<T, Args...>, U, true> {
        using Type = typename A<T, Args...>::template Rebind<U>;
    };

    template<
        template<typename, typename...> class A, typename T,
        typename ...Args, typename U
    >
    struct AllocTraitsRebindType<A<T, Args...>, U, false> {
        using Type = A<U, Args...>;
    };
} /* namespace detail */

template<typename A, typename T>
using AllocatorRebind = typename detail::AllocTraitsRebindType<
    AllocatorType<A>, T
>::Type;

/* allocator propagate on container copy assignment */

namespace detail {
    template<typename T>
    struct PropagateOnContainerCopyAssignmentTest {
        template<typename U>
        static char test(decltype(U::PropagateOnContainerCopyAssignment) * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename A, bool =
        PropagateOnContainerCopyAssignmentTest<A>::value
    >
    constexpr bool PropagateOnContainerCopyAssignmentBase = false;

    template<typename A>
    constexpr bool PropagateOnContainerCopyAssignmentBase<A, true> =
        A::PropagateOnContainerCopyAssignment;
} /* namespace detail */

template<typename A>
constexpr bool AllocatorPropagateOnContainerCopyAssignment =
    detail::PropagateOnContainerCopyAssignmentBase<A>;

/* allocator propagate on container move assignment */

namespace detail {
    template<typename T>
    struct PropagateOnContainerMoveAssignmentTest {
        template<typename U>
        static char test(decltype(U::PropagateOnContainerMoveAssignment) * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename A, bool =
        PropagateOnContainerMoveAssignmentTest<A>::value
    >
    constexpr bool PropagateOnContainerMoveAssignmentBase = false;

    template<typename A>
    constexpr bool PropagateOnContainerMoveAssignmentBase<A, true> =
        A::PropagateOnContainerMoveAssignment;
} /* namespace detail */

template<typename A>
constexpr bool AllocatorPropagateOnContainerMoveAssignment =
    detail::PropagateOnContainerMoveAssignmentBase<A>;

/* allocator propagate on container swap */

namespace detail {
    template<typename T>
    struct PropagateOnContainerSwapTest {
        template<typename U>
        static char test(decltype(U::PropagateOnContainerSwap) * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename A, bool = PropagateOnContainerSwapTest<A>::value>
    constexpr bool PropagateOnContainerSwapBase = false;

    template<typename A>
    constexpr bool PropagateOnContainerSwapBase<A, true> =
        A::PropagateOnContainerSwap;
} /* namespace detail */

template<typename A>
constexpr bool AllocatorPropagateOnContainerSwap =
    detail::PropagateOnContainerSwapBase<A>;

/* allocator is always equal */

namespace detail {
    template<typename T>
    struct IsAlwaysEqualTest {
        template<typename U>
        static char test(decltype(U::IsAlwaysEqual) * = 0);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename A, bool = IsAlwaysEqualTest<A>::value>
    constexpr bool IsAlwaysEqualBase = IsEmpty<A>;

    template<typename A>
    constexpr bool IsAlwaysEqualBase<A, true> = A::IsAlwaysEqual;
} /* namespace detail */

template<typename A>
constexpr bool AllocatorIsAlwaysEqual = detail::IsAlwaysEqualBase<A>;

/* allocator allocate */

template<typename A>
inline AllocatorPointer<A> allocator_allocate(A &a, AllocatorSize<A> n) {
    return a.allocate(n);
}

namespace detail {
    template<typename A, typename S, typename CVP>
    auto allocate_hint_test(A &&a, S &&sz, CVP &&p) ->
        decltype(a.allocate(sz, p), True());

    template<typename A, typename S, typename CVP>
    auto allocate_hint_test(A const &, S &&, CVP &&) -> False;

    template<typename A, typename S, typename CVP>
    constexpr bool AllocateHintTest =
        IsSame<
            decltype(allocate_hint_test(
                declval<A>(), declval<S>(), declval<CVP>()
            )), True
        >;

    template<typename A>
    inline AllocatorPointer<A> allocate(
        A &a, AllocatorSize<A> n, AllocatorConstVoidPointer<A> h, True
    ) {
        return a.allocate(n, h);
    }

    template<typename A>
    inline AllocatorPointer<A> allocate(
        A &a, AllocatorSize<A> n, AllocatorConstVoidPointer<A>, False
    ) {
        return a.allocate(n);
    }
} /* namespace detail */

template<typename A>
inline AllocatorPointer<A> allocator_allocate(
    A &a, AllocatorSize<A> n, AllocatorConstVoidPointer<A> h
) {
    return detail::allocate(
        a, n, h, BoolConstant<detail::AllocateHintTest<
            A, AllocatorSize<A>, AllocatorConstVoidPointer<A>
        >>()
    );
}

/* allocator deallocate */

template<typename A>
inline void allocator_deallocate(
    A &a, AllocatorPointer<A> p, AllocatorSize<A> n
) noexcept {
    a.deallocate(p, n);
}

/* allocator construct */

namespace detail {
    template<typename A, typename T, typename ...Args>
    auto construct_test(A &&a, T *p, Args &&...args) ->
        decltype(a.construct(p, forward<Args>(args)...), True());

    template<typename A, typename T, typename ...Args>
    auto construct_test(A const &, T *, Args &&...) -> False;

    template<typename A, typename T, typename ...Args>
    constexpr bool ConstructTest =
        IsSame<
            decltype(construct_test(
                declval<A>(), declval<T>(), declval<Args>()...
            )), True
        >;

    template<typename A, typename T, typename ...Args>
    inline void construct(True, A &a, T *p, Args &&...args) {
        a.construct(p, forward<Args>(args)...);
    }

    template<typename A, typename T, typename ...Args>
    inline void construct(False, A &, T *p, Args &&...args) {
        ::new (p) T(forward<Args>(args)...);
    }
} /* namespace detail */

template<typename A, typename T, typename ...Args>
inline void allocator_construct(A &a, T *p, Args &&...args) {
    detail::construct(
        BoolConstant<detail::ConstructTest<A, T *, Args...>>(),
        a, p, forward<Args>(args)...
    );
}

/* allocator destroy */

namespace detail {
    template<typename A, typename P>
    auto destroy_test(A &&a, P &&p) -> decltype(a.destroy(p), True());

    template<typename A, typename P>
    auto destroy_test(A const &, P &&) -> False;

    template<typename A, typename P>
    constexpr bool DestroyTest =
        IsSame<decltype(destroy_test(declval<A>(), declval<P>())), True>;

    template<typename A, typename T>
    inline void destroy(True, A &a, T *p) {
        a.destroy(p);
    }

    template<typename A, typename T>
    inline void destroy(False, A &, T *p) {
        p->~T();
    }
} /* namespace detail */

template<typename A, typename T>
inline void allocator_destroy(A &a, T *p) noexcept {
    detail::destroy(BoolConstant<detail::DestroyTest<A, T *>>(), a, p);
}

/* allocator max size */

namespace detail {
    template<typename A>
    auto alloc_max_size_test(A &&a) -> decltype(a.max_size(), True());

    template<typename A>
    auto alloc_max_size_test(A const &) -> False;

    template<typename A>
    constexpr bool AllocMaxSizeTest =
        IsSame<decltype(alloc_max_size_test(declval<A &>())), True>;

    template<typename A>
    inline AllocatorSize<A> alloc_max_size(True, A const &a) {
        return a.max_size();
    }

    template<typename A>
    inline AllocatorSize<A> alloc_max_size(False, A const &) {
        return AllocatorSize<A>(~0);
    }
} /* namespace detail */

template<typename A>
inline AllocatorSize<A> allocator_max_size(A const &a) noexcept {
    return detail::alloc_max_size(
        BoolConstant<detail::AllocMaxSizeTest<A const>>(), a
    );
}

/* allocator container copy */

namespace detail {
    template<typename A>
    auto alloc_copy_test(A &&a) -> decltype(a.container_copy(), True());

    template<typename A>
    auto alloc_copy_test(A const &) -> False;

    template<typename A>
    constexpr bool AllocCopyTest =
        IsSame<decltype(alloc_copy_test(declval<A &>())), True>;

    template<typename A>
    inline AllocatorType<A> alloc_container_copy(True, A const &a) {
        return a.container_copy();
    }

    template<typename A>
    inline AllocatorType<A> alloc_container_copy(False, A const &a) {
        return a;
    }
} /* namespace detail */

template<typename A>
inline AllocatorType<A> allocator_container_copy(A const &a) {
    return detail::alloc_container_copy(
        BoolConstant<detail::AllocCopyTest<A const>>(), a
    );
}

/* allocator arg */

struct AllocatorArg {};

constexpr AllocatorArg allocator_arg = AllocatorArg();

/* uses allocator */

namespace detail {
    template<typename T>
    struct HasAllocatorType {
        template<typename U>
        static char test(typename U::Allocator *);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == 1);
    };

    template<typename T, typename A, bool = HasAllocatorType<T>::value>
    constexpr bool UsesAllocatorBase = IsConvertible<A, typename T::Allocator>;

    template<typename T, typename A>
    constexpr bool UsesAllocatorBase<T, A, false> = false;
}

template<typename T, typename A>
constexpr bool UsesAllocator = detail::UsesAllocatorBase<T, A>;

/* uses allocator ctor */

namespace detail {
    template<typename T, typename A, typename ...Args>
    struct UsesAllocCtor {
        static constexpr bool ua = UsesAllocator<T, A>;
        static constexpr bool ic = IsConstructible<
            T, AllocatorArg, A, Args...
        >;
        static constexpr int value = ua ? (2 - ic) : 0;
    };
}

template<typename T, typename A, typename ...Args>
constexpr int UsesAllocatorConstructor =
    detail::UsesAllocCtor<T, A, Args...>::value;

/* util for other classes */

namespace detail {
    template<typename A>
    struct AllocatorDestructor {
        using Pointer = AllocatorPointer<A>;
        using Size = ostd::Size;
        AllocatorDestructor(A &a, Size s) noexcept: p_alloc(a), p_size(s) {}
        void operator()(Pointer p) noexcept {
            allocator_deallocate(p_alloc, p, p_size);
        }
    private:
        A &p_alloc;
        Size p_size;
    };
}

} /* namespace ostd */

#endif
