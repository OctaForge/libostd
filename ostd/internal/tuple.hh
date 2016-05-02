/* Some tuple internals for inclusion from various headers. Partially
 * taken from the libc++ project.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_TUPLE_HH
#define OSTD_INTERNAL_TUPLE_HH

#include "ostd/types.hh"
#include "ostd/type_traits.hh"

namespace ostd {

template<typename ...A> class Tuple;
template<typename T, typename U> struct Pair;
template<typename T, Size I> struct Array;

/* tuple size */

template<typename T> constexpr Size TupleSize = detail::Undefined<T>();

template<typename T> constexpr Size TupleSize<const T> = TupleSize<T>;
template<typename T> constexpr Size TupleSize<volatile T> = TupleSize<T>;
template<typename T> constexpr Size TupleSize<const volatile T> = TupleSize<T>;

/* tuple element */

template<Size I, typename T> struct TupleElementBase;
template<Size I, typename T>
struct TupleElementBase<I, const T> {
    using Type = AddConst<typename TupleElementBase<I, T>::Type>;
};
template<Size I, typename T>
struct TupleElementBase<I, volatile T> {
    using Type = AddVolatile<typename TupleElementBase<I, T>::Type>;
};
template<Size I, typename T>
struct TupleElementBase<I, const volatile T> {
    using Type = AddCv<typename TupleElementBase<I, T>::Type>;
};

template<Size I, typename T>
using TupleElement = typename TupleElementBase<I, T>::Type;

/* is tuple-like */

template<typename T> constexpr bool IsTupleLike = false;

template<typename T> constexpr bool IsTupleLike<const T> = IsTupleLike<T>;
template<typename T> constexpr bool IsTupleLike<volatile T> = IsTupleLike<T>;
template<typename T> constexpr bool IsTupleLike<const volatile T> = IsTupleLike<T>;

/* tuple specializations */

template<typename ...A> constexpr bool IsTupleLike<Tuple<A...>> = true;

template<Size I, typename ...A>
TupleElement<I, Tuple<A...>> &get(Tuple<A...> &);

template<Size I, typename ...A>
const TupleElement<I, Tuple<A...>> &get(const Tuple<A...> &);

template<Size I, typename ...A>
TupleElement<I, Tuple<A...>> &&get(Tuple<A...> &&);

template<Size I, typename ...A>
const TupleElement<I, Tuple<A...>> &&get(const Tuple<A...> &&);

/* pair specializations */

template<typename T, typename U> constexpr bool IsTupleLike<Pair<T, U>> = true;

template<Size I, typename T, typename U>
TupleElement<I, Pair<T, U>> &get(Pair<T, U> &);

template<Size I, typename T, typename U>
const TupleElement<I, Pair<T, U>> &get(const Pair<T, U> &);

template<Size I, typename T, typename U>
TupleElement<I, Pair<T, U>> &&get(Pair<T, U> &&);

template<Size I, typename T, typename U>
const TupleElement<I, Pair<T, U>> &&get(const Pair<T, U> &&);

/* array specializations */

template<typename T, Size I> constexpr bool IsTupleLike<Array<T, I>> = true;

template<Size I, typename T, Size S>
T &get(Array<T, S> &);

template<Size I, typename T, Size S>
const T &get(const Array<T, S> &);

template<Size I, typename T, Size S>
T &&get(Array<T, S> &&);

template<Size I, typename T, Size S>
const T &&get(const Array<T, S> &&);

/* make tuple indices */

namespace detail {
    template<Size ...> struct TupleIndices {};

    template<Size S, typename T, Size E> struct MakeTupleIndicesBase;

    template<Size S, Size ...I, Size E>
    struct MakeTupleIndicesBase<S, TupleIndices<I...>, E> {
        using Type = typename MakeTupleIndicesBase<S + 1,
            TupleIndices<I..., S>, E>::Type;
    };

    template<Size E, Size ...I>
    struct MakeTupleIndicesBase<E, TupleIndices<I...>, E> {
        using Type = TupleIndices<I...>;
    };

    template<Size E, Size S>
    struct MakeTupleIndicesImpl {
        static_assert(S <= E, "MakeTupleIndices input error");
        using Type = typename MakeTupleIndicesBase<S, TupleIndices<>, E>::Type;
    };

    template<Size E, Size S = 0>
    using MakeTupleIndices = typename MakeTupleIndicesImpl<E, S>::Type;
}

/* tuple types */

namespace detail {
    template<typename ...T> struct TupleTypes {};
}

template<Size I> struct TupleElementBase<I, detail::TupleTypes<>> {
public:
    static_assert(I == 0, "TupleElement index out of range");
    static_assert(I != 0, "TupleElement index out of range");
};

template<typename H, typename ...T>
struct TupleElementBase<0, detail::TupleTypes<H, T...>> {
public:
    using Type = H;
};

template<Size I, typename H, typename ...T>
struct TupleElementBase<I, detail::TupleTypes<H, T...>> {
public:
    using Type = typename TupleElementBase<I - 1,
        detail::TupleTypes<T...>>::Type;
};

template<typename ...T> constexpr Size TupleSize<detail::TupleTypes<T...>>
    = sizeof...(T);

template<typename ...T> constexpr bool IsTupleLike<detail::TupleTypes<T...>> = true;

/* make tuple types */

namespace detail {
    template<typename TT, typename T, Size S, Size E>
    struct MakeTupleTypesBase;

    template<typename ...TS, typename T, Size S, Size E>
    struct MakeTupleTypesBase<TupleTypes<TS...>, T, S, E> {
        using TR = RemoveReference<T>;
        using Type = typename MakeTupleTypesBase<TupleTypes<TS...,
            Conditional<IsLvalueReference<T>,
                TupleElement<S, TR> &,
                TupleElement<S, TR>>>, T, S + 1, E>::Type;
    };

    template<typename ...TS, typename T, Size E>
    struct MakeTupleTypesBase<TupleTypes<TS...>, T, E, E> {
        using Type = TupleTypes<TS...>;
    };

    template<typename T, Size E, Size S>
    struct MakeTupleTypesImpl {
        static_assert(S <= E, "MakeTupleTypes input error");
        using Type = typename MakeTupleTypesBase<TupleTypes<>, T, S, E>::Type;
    };

    template<typename T, Size E = TupleSize<RemoveReference<T>>, Size S = 0>
    using MakeTupleTypes = typename MakeTupleTypesImpl<T, E, S>::Type;
}

/* tuple convertible */

namespace detail {
    template<typename, typename>
    constexpr bool TupleConvertibleBase = false;

    template<typename T, typename ...TT, typename U, typename ...UU>
    constexpr bool TupleConvertibleBase<TupleTypes<T, TT...>, TupleTypes<U, UU...>>
        = IsConvertible<T, U> && TupleConvertibleBase<TupleTypes<TT...>,
                                                      TupleTypes<UU...>>;

    template<>
    constexpr bool TupleConvertibleBase<TupleTypes<>, TupleTypes<>> = true;

    template<bool, typename, typename>
    constexpr bool TupleConvertibleApply = false;

    template<typename T, typename U>
    constexpr bool TupleConvertibleApply<true, T, U>
        = TupleConvertibleBase<MakeTupleTypes<T>, MakeTupleTypes<U>>;

    template<typename T, typename U, bool = IsTupleLike<RemoveReference<T>>,
                                     bool = IsTupleLike<U>>
    constexpr bool TupleConvertible = false;

    template<typename T, typename U>
    constexpr bool TupleConvertible<T, U, true, true> = TupleConvertibleApply<
        TupleSize<RemoveReference<T>> == TupleSize<U>, T, U
    >;
}

/* tuple constructible */

namespace detail {
    template<typename, typename>
    constexpr bool TupleConstructibleBase = false;

    template<typename T, typename ...TT, typename U, typename ...UU>
    constexpr bool TupleConstructibleBase<TupleTypes<T, TT...>, TupleTypes<U, UU...>>
        = IsConstructible<U, T> && TupleConstructibleBase<TupleTypes<TT...>,
                                                          TupleTypes<UU...>>;

    template<>
    constexpr bool TupleConstructibleBase<TupleTypes<>, TupleTypes<>> = true;

    template<bool, typename, typename>
    constexpr bool TupleConstructibleApply = false;

    template<typename T, typename U>
    constexpr bool TupleConstructibleApply<true, T, U>
        = TupleConstructibleBase<MakeTupleTypes<T>, MakeTupleTypes<U>>;

    template<typename T, typename U, bool = IsTupleLike<RemoveReference<T>>,
                                     bool = IsTupleLike<U>>
    constexpr bool TupleConstructible = false;

    template<typename T, typename U>
    constexpr bool TupleConstructible<T, U, true, true> = TupleConstructibleApply<
        TupleSize<RemoveReference<T>> == TupleSize<U>, T, U
    >;
}

/* tuple assignable */

namespace detail {
    template<typename, typename>
    constexpr bool TupleAssignableBase = false;

    template<typename T, typename ...TT, typename U, typename ...UU>
    constexpr bool TupleAssignableBase<TupleTypes<T, TT...>, TupleTypes<U, UU...>>
        = IsAssignable<U &, T> && TupleAssignableBase<TupleTypes<TT...>,
                                                      TupleTypes<UU...>>;

    template<>
    constexpr bool TupleAssignableBase<TupleTypes<>, TupleTypes<>> = true;

    template<bool, typename, typename>
    constexpr bool TupleAssignableApply = false;

    template<typename T, typename U>
    constexpr bool TupleAssignableApply<true, T, U>
        = TupleAssignableBase<MakeTupleTypes<T>, MakeTupleTypes<U>>;

    template<typename T, typename U, bool = IsTupleLike<RemoveReference<T>>,
                                     bool = IsTupleLike<U>>
    constexpr bool TupleAssignable = false;

    template<typename T, typename U>
    constexpr bool TupleAssignable<T, U, true, true> = TupleAssignableApply<
        TupleSize<RemoveReference<T>> == TupleSize<U>, T, U
    >;
}

} /* namespace ostd */

#endif
