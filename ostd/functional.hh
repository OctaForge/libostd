/* Function objects for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_FUNCTIONAL_HH
#define OSTD_FUNCTIONAL_HH

#include <string.h>

#include <new>
#include <functional>
#include <tuple>

#include "ostd/platform.hh"
#include "ostd/memory.hh"
#include "ostd/utility.hh"
#include "ostd/type_traits.hh"

namespace ostd {

/* basic function objects */

#define OSTD_DEFINE_BINARY_OP(name, op, RT) \
template<typename T> \
struct name { \
    RT operator()(T const &x, T const &y) const { \
        return x op y; \
    } \
    using FirstArgument = T; \
    using SecondARgument = T; \
    using Result = RT; \
};

OSTD_DEFINE_BINARY_OP(Less, <, bool)
OSTD_DEFINE_BINARY_OP(LessEqual, <=, bool)
OSTD_DEFINE_BINARY_OP(Greater, >, bool)
OSTD_DEFINE_BINARY_OP(GreaterEqual, >=, bool)
OSTD_DEFINE_BINARY_OP(Equal, ==, bool)
OSTD_DEFINE_BINARY_OP(NotEqual, !=, bool)
OSTD_DEFINE_BINARY_OP(LogicalAnd, &&, bool)
OSTD_DEFINE_BINARY_OP(LogicalOr, ||, bool)
OSTD_DEFINE_BINARY_OP(Modulo, %, T)
OSTD_DEFINE_BINARY_OP(Multiply, *, T)
OSTD_DEFINE_BINARY_OP(Divide, /, T)
OSTD_DEFINE_BINARY_OP(Add, +, T)
OSTD_DEFINE_BINARY_OP(Subtract, -, T)
OSTD_DEFINE_BINARY_OP(BitAnd, &, T)
OSTD_DEFINE_BINARY_OP(BitOr, |, T)
OSTD_DEFINE_BINARY_OP(BitXor, ^, T)

#undef OSTD_DEFINE_BINARY_OP

namespace detail {
    template<typename T, bool = IsSame<RemoveConst<T>, char>>
    struct CharEqual {
        using FirstArgument = T *;
        using SecondArgument = T *;
        using Result = bool;
        bool operator()(T *x, T *y) const {
            return !strcmp(x, y);
        }
    };

    template<typename T>
    struct CharEqual<T, false> {
        using FirstArgument = T *;
        using SecondArgument = T *;
        using Result = bool;
        bool operator()(T *x, T *y) const {
            return x == y;
        }
    };
}

template<typename T>
struct EqualWithCstr {
    using FirstArgument = T;
    using SecondArgument = T;
    bool operator()(T const &x, T const &y) const {
        return x == y;
    }
};

template<typename T>
struct EqualWithCstr<T *>: detail::CharEqual<T> {};

template<typename T>
struct LogicalNot {
    bool operator()(T const &x) const { return !x; }
    using Argument = T;
    using Result = bool;
};

template<typename T>
struct Negate {
    bool operator()(T const &x) const { return -x; }
    using Argument = T;
    using Result = T;
};

template<typename T>
struct BinaryNegate {
    using FirstArgument = typename T::FirstArgument;
    using SecondArgument = typename T::SecondArgument;
    using Result = bool;

    explicit BinaryNegate(T const &f): p_fn(f) {}

    bool operator()(FirstArgument const &x, SecondArgument const &y) {
        return !p_fn(x, y);
    }
private:
    T p_fn;
};

template<typename T>
struct UnaryNegate {
    using Argument = typename T::Argument;
    using Result = bool;

    explicit UnaryNegate(T const &f): p_fn(f) {}
    bool operator()(Argument const &x) {
        return !p_fn(x);
    }
private:
    T p_fn;
};

template<typename T>
UnaryNegate<T> not1(T const &fn) {
    return UnaryNegate<T>(fn);
}

template<typename T>
BinaryNegate<T> not2(T const &fn) {
    return BinaryNegate<T>(fn);
}

/* endian swap */

template<typename T, size_t N = sizeof(T), bool IsNum = IsArithmetic<T>>
struct EndianSwap;

template<typename T>
struct EndianSwap<T, 2, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; std::uint16_t sv; } u;
        u.iv = v;
        u.sv = endian_swap16(u.sv);
        return u.iv;
    }
};

template<typename T>
struct EndianSwap<T, 4, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; std::uint32_t sv; } u;
        u.iv = v;
        u.sv = endian_swap32(u.sv);
        return u.iv;
    }
};

template<typename T>
struct EndianSwap<T, 8, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; std::uint64_t sv; } u;
        u.iv = v;
        u.sv = endian_swap64(u.sv);
        return u.iv;
    }
};

template<typename T>
T endian_swap(T x) { return EndianSwap<T>()(x); }

namespace detail {
    template<typename T, size_t N = sizeof(T), bool IsNum = IsArithmetic<T>>
    struct EndianSame;

    template<typename T>
    struct EndianSame<T, 2, true> {
        using Argument = T;
        using Result = T;
        T operator()(T v) const { return v; }
    };
    template<typename T>
    struct EndianSame<T, 4, true> {
        using Argument = T;
        using Result = T;
        T operator()(T v) const { return v; }
    };
    template<typename T>
    struct EndianSame<T, 8, true> {
        using Argument = T;
        using Result = T;
        T operator()(T v) const { return v; }
    };
}

#if OSTD_BYTE_ORDER == OSTD_ENDIAN_LIL
template<typename T>
struct FromLilEndian: detail::EndianSame<T> {};
template<typename T>
struct FromBigEndian: EndianSwap<T> {};
#else
template<typename T>
struct FromLilEndian: EndianSwap<T> {};
template<typename T>
struct FromBigEndian: detail::EndianSame<T> {};
#endif

template<typename T>
T from_lil_endian(T x) { return FromLilEndian<T>()(x); }
template<typename T>
T from_big_endian(T x) { return FromBigEndian<T>()(x); }

} /* namespace ostd */

#endif
