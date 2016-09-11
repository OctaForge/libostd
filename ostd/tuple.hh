/* Tuples or OctaSTD. Partially taken from the libc++ project.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_TUPLE_HH
#define OSTD_TUPLE_HH

#include "ostd/internal/tuple.hh"

#include "ostd/types.hh"
#include "ostd/type_traits.hh"
#include "ostd/memory.hh"
#include "ostd/utility.hh"

namespace ostd {

/* tuple size */

template<typename ...T>
constexpr Size TupleSize<Tuple<T...>> = sizeof...(T);

/* tuple element */

template<Size I, typename ...T>
struct TupleElementBase<I, Tuple<T...>> {
    using Type = typename TupleElementBase<I, detail::TupleTypes<T...>>::Type;
};

/* tuple leaf */

namespace detail {
    template<Size I, typename H, bool = IsEmpty<H>>
    struct TupleLeaf {
        constexpr TupleLeaf(): p_value() {
            static_assert(
                !IsReference<H>,
                "attempt to default construct a reference element in a tuple"
            );
        }

        template<typename A>
        TupleLeaf(Constant<int, 0>, A const &): p_value() {
            static_assert(
                !IsReference<H>,
                "attempt to default construct a reference element in a tuple"
            );
        }
        template<typename A>
        TupleLeaf(Constant<int, 1>, A const &a): p_value(allocator_arg, a) {
            static_assert(
                !IsReference<H>,
                "attempt to default construct a reference element in a tuple"
            );
        }
        template<typename A>
        TupleLeaf(Constant<int, 2>, A const &a): p_value(a) {
            static_assert(
                !IsReference<H>,
                "attempt to default construct a reference element in a tuple"
            );
        }

        template<typename T, typename = EnableIf<
            !IsSame<Decay<T>, TupleLeaf> && IsConstructible<H, T>
        >>
        explicit TupleLeaf(T &&t): p_value(forward<T>(t)) {
            static_assert(
                !IsReference<H> || (
                    IsLvalueReference<H> && (
                        IsLvalueReference<T> || IsSame<
                            RemoveReference<T>,
                            ReferenceWrapper<RemoveReference<H>>
                        >
                    )
                ) || (IsRvalueReference<H> && !IsLvalueReference<T>),
                "attempt to construct a reference element in a tuple with an rvalue"
            );
        }

        template<typename T, typename A>
        explicit TupleLeaf(Constant<int, 0>, A const &, T &&t):
            p_value(forward<T>(t))
        {
            static_assert(
                !IsLvalueReference<H> || (
                    IsLvalueReference<H> && (
                        IsLvalueReference<T> || IsSame<
                            RemoveReference<T>,
                            ReferenceWrapper<RemoveReference<H>>
                        >
                    )
                ),
                "attempt to construct a reference element in a tuple with an rvalue"
            );
        }

        template<typename T, typename A>
        explicit TupleLeaf(Constant<int, 1>, A const &a, T &&t):
            p_value(allocator_arg, a, forward<T>(t))
        {
            static_assert(
                !IsLvalueReference<H> || (
                    IsLvalueReference<H> && (
                        IsLvalueReference<T> || IsSame<
                            RemoveReference<T>,
                            ReferenceWrapper<RemoveReference<H>>
                        >
                    )
                ),
                "attempt to construct a reference element in a tuple with an rvalue"
            );
        }

        template<typename T, typename A>
        explicit TupleLeaf(Constant<int, 2>, A const &a, T &&t):
            p_value(forward<T>(t), a)
        {
            static_assert(
                !IsLvalueReference<H> || (
                    IsLvalueReference<H> && (
                        IsLvalueReference<T> || IsSame<
                            RemoveReference<T>,
                            ReferenceWrapper<RemoveReference<H>>
                        >
                    )
                ),
                "attempt to construct a reference element in a tuple with an rvalue"
            );
        }

        TupleLeaf(TupleLeaf const &) = default;
        TupleLeaf(TupleLeaf &&) = default;

        template<typename T>
        TupleLeaf &operator=(T &&t) {
            p_value = forward<T>(t);
            return *this;
        }

        void swap(TupleLeaf &t) {
            swap_adl(get(), t.get());
        }

        H &get() { return p_value; }
        H const &get() const { return p_value; }

    private:
        TupleLeaf &operator=(TupleLeaf const &);
        H p_value;
    };

    template<Size I, typename H>
    struct TupleLeaf<I, H, true>: private H {
        constexpr TupleLeaf() {}

        template<typename A>
        TupleLeaf(Constant<int, 0>, A const &) {}

        template<typename A>
        TupleLeaf(Constant<int, 1>, A const &a):
            H(allocator_arg, a)
        {}

        template<typename A>
        TupleLeaf(Constant<int, 2>, A const &a): H(a) {}

        template<typename T, typename = EnableIf<
            !IsSame<Decay<T>, TupleLeaf> && IsConstructible<H, T>
        >>
        explicit TupleLeaf(T &&t): H(forward<T>(t)) {}

        template<typename T, typename A>
        explicit TupleLeaf(Constant<int, 0>, A const &, T &&t):
            H(forward<T>(t))
        {}

        template<typename T, typename A>
        explicit TupleLeaf(Constant<int, 1>, A const &a, T &&t):
            H(allocator_arg, a, forward<T>(t))
        {}

        template<typename T, typename A>
        explicit TupleLeaf(Constant<int, 2>, A const &a, T &&t):
            H(forward<T>(t), a)
        {}

        TupleLeaf(TupleLeaf const &) = default;
        TupleLeaf(TupleLeaf &&) = default;

        template<typename T>
        TupleLeaf &operator=(T &&t) {
            H::operator=(forward<T>(t));
            return *this;
        }

        void swap(TupleLeaf &t) {
            swap_adl(get(), t.get());
        }

        H &get() { return *static_cast<H *>(this); }
        H const &get() const { return *static_cast<H const *>(this); }

    private:
        TupleLeaf &operator=(TupleLeaf const &);
    };
} /* namespace detail */

/* internal utils */

namespace detail {
    template<typename ...A>
    inline void tuple_swallow(A &&...) {}

    template<bool ...A>
    constexpr bool TupleAll = true;

    template<bool B, bool ...A>
    constexpr bool TupleAll<B, A...> = B && TupleAll<A...>;

    template<typename T>
    constexpr bool TupleAllDefaultConstructible = detail::Undefined<T>();

    template<typename ...A>
    constexpr bool TupleAllDefaultConstructible<TupleTypes<A...>> =
        TupleAll<IsDefaultConstructible<A>...>;
}

/* tuple implementation */

namespace detail {
    template<typename, typename ...>
    struct TupleBase;

    template<Size ...I, typename ...A>
    struct TupleBase<TupleIndices<I...>, A...>: TupleLeaf<I, A>... {
        constexpr TupleBase() {}

        template<
            Size ...Ia, typename ...Aa, Size ...Ib,
            typename ...Ab, typename ...T
        >
        explicit TupleBase(
            TupleIndices<Ia...>, TupleTypes<Aa...>,
            TupleIndices<Ib...>, TupleTypes<Ab...>, T &&...t
        ):
            TupleLeaf<Ia, Aa>(forward<T>(t))...,
            TupleLeaf<Ib, Ab>()...
        {}

        template<
            typename Alloc, Size ...Ia, typename ...Aa,
            Size ...Ib, typename ...Ab, typename ...T
        >
        explicit TupleBase(
            AllocatorArg, Alloc const &a,
            TupleIndices<Ia...>, TupleTypes<Aa...>,
            TupleIndices<Ib...>, TupleTypes<Ab...>, T &&...t
        ):
            TupleLeaf<Ia, Aa>(
                UsesAllocatorConstructor<Aa, Alloc, T>, a,
                forward<T>(t)
            )...,
            TupleLeaf<Ib, Ab>(UsesAllocatorConstructor<Ab, Alloc>, a)...
        {}

        template<typename T, typename = EnableIf<
            TupleConstructible<T, Tuple<A...>>
        >>
        TupleBase(T &&t):
            TupleLeaf<I, A>(
                forward<TupleElement<I, MakeTupleTypes<T>>>(get<I>(t))
            )...
        {}

        template<typename Alloc, typename T, typename = EnableIf<
            TupleConvertible<T, Tuple<A...>>
        >>
        TupleBase(AllocatorArg, Alloc const &a, T &&t):
            TupleLeaf<I, A>(
                UsesAllocatorConstructor<
                    A, Alloc, TupleElement<I, MakeTupleTypes<T>>
                >, a, forward<TupleElement<I, MakeTupleTypes<T>>>(get<I>(t))
            )...
        {}

        template<typename T>
        EnableIf<TupleAssignable<T, Tuple<A...>>, TupleBase &> operator=(T &&t) {
            tuple_swallow(TupleLeaf<I, A>::operator=(
                forward<TupleElement<I, MakeTupleTypes<T>>>(get<I>(t))
            )...);
            return *this;
        }

        TupleBase(TupleBase const &) = default;
        TupleBase(TupleBase &&) = default;

        TupleBase &operator=(TupleBase const &t) {
            tuple_swallow(TupleLeaf<I, A>::operator=(
                (static_cast<TupleLeaf<I, A> const &>(t)).get()
            )...);
            return *this;
        }

        TupleBase &operator=(TupleBase &&t) {
            tuple_swallow(TupleLeaf<I, A>::operator=(
                forward<A>((static_cast<TupleLeaf<I, A> &>(t)).get())
            )...);
            return *this;
        }

        void swap(TupleBase &t) {
            tuple_swallow(
                TupleLeaf<I, A>::swap(static_cast<TupleLeaf<I, A> &>(t))...
            );
        }
    };
} /* namespace detail */

template<typename ...A>
class Tuple {
    using Base = detail::TupleBase<detail::MakeTupleIndices<sizeof...(A)>, A...>;
    Base p_base;

    template<Size I, typename ...T>
    friend TupleElement<I, Tuple<T...>> &get(Tuple<T...> &);

    template<Size I, typename ...T>
    friend TupleElement<I, Tuple<T...>> const &get(Tuple<T...> const &);

    template<Size I, typename ...T>
    friend TupleElement<I, Tuple<T...>> &&get(Tuple<T...> &&);

    template<Size I, typename ...T>
    friend TupleElement<I, Tuple<T...>> const &&get(Tuple<T...> const &&);

public:
    template<bool D = true, typename = EnableIf<
        detail::TupleAll<(D && IsDefaultConstructible<A>)...>
    >>
    Tuple() {}

    explicit Tuple(A const &...t):
        p_base(
            detail::MakeTupleIndices<sizeof...(A)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(A)>(),
            detail::MakeTupleIndices<0>(),
            detail::MakeTupleTypes<Tuple, 0>(), t...
        )
    {}

    template<typename Alloc>
    Tuple(AllocatorArg, Alloc const &a, A const &...t):
        p_base(
            allocator_arg, a,
            detail::MakeTupleIndices<sizeof...(A)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(A)>(),
            detail::MakeTupleIndices<0>(),
            detail::MakeTupleTypes<Tuple, 0>(), t...
        )
    {}

    template<typename ...T, EnableIf<
        (sizeof...(T) <= sizeof...(A)) &&
        detail::TupleConvertible<
            Tuple<T...>,
            detail::MakeTupleTypes<
                Tuple,
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        > &&
        detail::TupleAllDefaultConstructible<
            detail::MakeTupleTypes<
                Tuple, sizeof...(A),
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        >, bool
    > = true>
    Tuple(T &&...t):
        p_base(
            detail::MakeTupleIndices<sizeof...(T)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(T)>(),
            detail::MakeTupleIndices<sizeof...(A), sizeof...(T)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(A), sizeof...(T)>(),
            forward<T>(t)...
        )
    {}

    template<typename ...T, EnableIf<
        (sizeof...(T) <= sizeof...(A)) &&
        detail::TupleConstructible<
            Tuple<T...>,
            detail::MakeTupleTypes<
                Tuple,
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        > &&
        !detail::TupleConvertible<
            Tuple<T...>,
            detail::MakeTupleTypes<
                Tuple,
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        > &&
        detail::TupleAllDefaultConstructible<
            detail::MakeTupleTypes<
                Tuple, sizeof...(A),
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        >, bool
    > = true>
    Tuple(T &&...t):
        p_base(
            detail::MakeTupleIndices<sizeof...(T)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(T)>(),
            detail::MakeTupleIndices<sizeof...(A), sizeof...(T)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(A), sizeof...(T)>(),
            forward<T>(t)...
        )
    {}

    template<typename Alloc, typename ...T, typename = EnableIf<
        (sizeof...(T) <= sizeof...(A)) &&
        detail::TupleConvertible<
            Tuple<T...>,
            detail::MakeTupleTypes<
                Tuple,
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        > &&
        detail::TupleAllDefaultConstructible<
            detail::MakeTupleTypes<
                Tuple, sizeof...(A),
                (sizeof...(T) < sizeof...(A)) ? sizeof...(T) : sizeof...(A)
            >
        >
    >> Tuple(AllocatorArg, Alloc const &a, T &&...t):
        p_base(
            allocator_arg, a, detail::MakeTupleIndices<sizeof...(T)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(T)>(),
            detail::MakeTupleIndices<sizeof...(A), sizeof...(T)>(),
            detail::MakeTupleTypes<Tuple, sizeof...(A), sizeof...(T)>(),
            forward<T>(t)...
        )
    {}

    template<typename T, EnableIf<
        detail::TupleConvertible<T, Tuple>, bool
    > = true>
    Tuple(T &&t): p_base(forward<T>(t)) {}

    template<typename T, EnableIf<
        detail::TupleConstructible<T, Tuple> &&
        !detail::TupleConvertible<T, Tuple>, bool
    > = true>
    Tuple(T &&t): p_base(forward<T>(t)) {}

    template<typename Alloc, typename T, typename = EnableIf<
        detail::TupleConvertible<T, Tuple>
    >>
    Tuple(AllocatorArg, Alloc const &a, T &&t):
        p_base(allocator_arg, a, forward<T>(t))
    {}

    template<typename T, typename = EnableIf<detail::TupleAssignable<T, Tuple>>>
    Tuple &operator=(T &&t) {
        p_base.operator=(forward<T>(t));
        return *this;
    }

    void swap(Tuple &t) {
        p_base.swap(t.p_base);
    }
};

template<> class Tuple<> {
public:
    constexpr Tuple() {}
    template<typename A> Tuple(AllocatorArg, A const &) {}
    template<typename A> Tuple(AllocatorArg, A const &, Tuple const &) {}
    void swap(Tuple &) {}
};

/* get */

template<Size I, typename ...A>
inline TupleElement<I, Tuple<A...>> &get(Tuple<A...> &t) {
    using Type = TupleElement<I, Tuple<A...>>;
    return static_cast<detail::TupleLeaf<I, Type> &>(t.p_base).get();
}

template<Size I, typename ...A>
inline TupleElement<I, Tuple<A...>> const &get(Tuple<A...> const &t) {
    using Type = TupleElement<I, Tuple<A...>>;
    return static_cast<detail::TupleLeaf<I, Type> const &>(t.p_base).get();
}

template<Size I, typename ...A>
inline TupleElement<I, Tuple<A...>> &&get(Tuple<A...> &&t) {
    using Type = TupleElement<I, Tuple<A...>>;
    return static_cast<Type &&>(
        static_cast<detail::TupleLeaf<I, Type> &&>(t.p_base).get());
}

template<Size I, typename ...A>
inline TupleElement<I, Tuple<A...>> const &&get(Tuple<A...> const &&t) {
    using Type = TupleElement<I, Tuple<A...>>;
    return static_cast<Type const &&>(
        static_cast<detail::TupleLeaf<I, Type> const &&>(t.p_base).get());
}

/* tie */

template<typename ...T>
inline Tuple<T &...> tie(T &...t) {
    return Tuple<T &...>(t...);
}

/* ignore */

namespace detail {
    struct Ignore {
        template<typename T>
        Ignore const &operator=(T &&) const { return *this; }
    };
}

static detail::Ignore const ignore = detail::Ignore();

/* make tuple */

namespace detail {
    template<typename T>
    struct MakeTupleReturnType {
        using Type = T;
    };

    template<typename T>
    struct MakeTupleReturnType<ReferenceWrapper<T>> {
        using Type = T &;
    };

    template<typename T>
    struct MakeTupleReturn {
        using Type = typename MakeTupleReturnType<Decay<T>>::Type;
    };
}

template<typename ...T>
inline Tuple<typename detail::MakeTupleReturn<T>::Type...> make_tuple(T &&...t) {
    return Tuple<typename detail::MakeTupleReturn<T>::Type...>(forward<T>(t)...);
}

/* forward as tuple */

template<typename ...T>
inline Tuple<T &&...> forward_as_tuple(T &&...t) {
    return Tuple<T &&...>(forward<T>(t)...);
}

/* tuple relops */

namespace detail {
    template<Size I>
    struct TupleEqual {
        template<typename T, typename U>
        bool operator()(T const &x, U const &y) {
            return TupleEqual<I - 1>()(x, y) && (get<I>(x) == get<I>(y));
        }
    };

    template<>
    struct TupleEqual<0> {
        template<typename T, typename U>
        bool operator()(T const &, U const &) {
            return true;
        }
    };
}

template<typename ...T, typename ...U>
inline bool operator==(Tuple<T...> const &x, Tuple<U...> const &y) {
    return detail::TupleEqual<sizeof...(T)>(x, y);
}

template<typename ...T, typename ...U>
inline bool operator!=(Tuple<T...> const &x, Tuple<U...> const &y) {
    return !(x == y);
}

namespace detail {
    template<Size I>
    struct TupleLess {
        template<typename T, typename U>
        bool operator()(T const &x, U const &y) {
            constexpr Size J = TupleSize<T> - I;
            if (get<J>(x) < get<J>(y)) return true;
            if (get<J>(y) < get<J>(x)) return false;
            return TupleLess<I - 1>()(x, y);
        }
    };

    template<>
    struct TupleLess<0> {
        template<typename T, typename U>
        bool operator()(T const &, U const &) {
            return true;
        }
    };
}

template<typename ...T, typename ...U>
inline bool operator<(Tuple<T...> const &x, Tuple<U...> const &y) {
    return detail::TupleLess<sizeof...(T)>(x, y);
}

template<typename ...T, typename ...U>
inline bool operator>(Tuple<T...> const &x, Tuple<U...> const &y) {
    return y < x;
}

template<typename ...T, typename ...U>
inline bool operator<=(Tuple<T...> const &x, Tuple<U...> const &y) {
    return !(y < x);
}

template<typename ...T, typename ...U>
inline bool operator>=(Tuple<T...> const &x, Tuple<U...> const &y) {
    return !(x < y);
}

/* uses alloc */

template<typename ...T, typename A>
constexpr bool UsesAllocator<Tuple<T...>, A> = true;

/* piecewise pair stuff */

template<typename T, typename U>
template<typename ...A1, typename ...A2, Size ...I1, Size ...I2>
Pair<T, U>::Pair(
    PiecewiseConstruct, Tuple<A1...> &fa, Tuple<A2...> &sa,
    detail::TupleIndices<I1...>, detail::TupleIndices<I2...>
):
    first(forward<A1>(get<I1>(fa))...), second(forward<A2>(get<I2>(sa))...)
{}

namespace detail {
    template<typename T, typename U>
    template<typename ...A1, typename ...A2, Size ...I1, Size ...I2>
    CompressedPairBase<T, U, 0>::CompressedPairBase(
        PiecewiseConstruct, Tuple<A1...> &fa, Tuple<A2...> &sa,
        detail::TupleIndices<I1...>, detail::TupleIndices<I2...>
    ):
        p_first(forward<A1>(get<I1>(fa))...),
        p_second(forward<A2>(get<I2>(sa))...)
    {}

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, Size ...I1, Size ...I2>
    CompressedPairBase<T, U, 1>::CompressedPairBase(
        PiecewiseConstruct, Tuple<A1...> &fa, Tuple<A2...> &sa,
        detail::TupleIndices<I1...>, detail::TupleIndices<I2...>
    ):
        T(forward<A1>(get<I1>(fa))...),
        p_second(forward<A2>(get<I2>(sa))...)
    {}

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, Size ...I1, Size ...I2>
    CompressedPairBase<T, U, 2>::CompressedPairBase(
        PiecewiseConstruct, Tuple<A1...> &fa, Tuple<A2...> &sa,
        detail::TupleIndices<I1...>, detail::TupleIndices<I2...>
    ):
        U(forward<A2>(get<I2>(sa))...),
        p_first(forward<A1>(get<I1>(fa))...)
    {}

    template<typename T, typename U>
    template<typename ...A1, typename ...A2, Size ...I1, Size ...I2>
    CompressedPairBase<T, U, 3>::CompressedPairBase(
        PiecewiseConstruct, Tuple<A1...> &fa, Tuple<A2...> &sa,
        detail::TupleIndices<I1...>, detail::TupleIndices<I2...>
    ):
        T(forward<A1>(get<I1>(fa))...),
        U(forward<A2>(get<I2>(sa))...)
    {}
} /* namespace detail */

} /* namespace ostd */

#endif
