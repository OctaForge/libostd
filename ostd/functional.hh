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

template<typename T, Size N = sizeof(T), bool IsNum = IsArithmetic<T>>
struct EndianSwap;

template<typename T>
struct EndianSwap<T, 2, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; uint16_t sv; } u;
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
        union { T iv; uint32_t sv; } u;
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
        union { T iv; uint64_t sv; } u;
        u.iv = v;
        u.sv = endian_swap64(u.sv);
        return u.iv;
    }
};

template<typename T>
T endian_swap(T x) { return EndianSwap<T>()(x); }

namespace detail {
    template<typename T, Size N = sizeof(T), bool IsNum = IsArithmetic<T>>
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

/* hash */

template<typename T>
struct ToHash {
    using Argument = T;
    using Result = Size;

    Size operator()(T const &v) const {
        return v.to_hash();
    }
};

namespace detail {
    template<typename T>
    struct ToHashBase {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            return Size(v);
        }
    };
}

#define OSTD_HASH_BASIC(T) \
template<> \
struct ToHash<T>: detail::ToHashBase<T> {};

OSTD_HASH_BASIC(bool)
OSTD_HASH_BASIC(char)
OSTD_HASH_BASIC(short)
OSTD_HASH_BASIC(int)
OSTD_HASH_BASIC(long)

OSTD_HASH_BASIC(sbyte)
OSTD_HASH_BASIC(byte)
OSTD_HASH_BASIC(ushort)
OSTD_HASH_BASIC(uint)
OSTD_HASH_BASIC(ulong)

#ifndef OSTD_TYPES_CHAR_16_32_NO_BUILTINS
OSTD_HASH_BASIC(Char16)
OSTD_HASH_BASIC(Char32)
#endif
OSTD_HASH_BASIC(Wchar)

#undef OSTD_HASH_BASIC

namespace detail {
    template<Size E>
    struct FnvConstants {
        static constexpr Size prime = 16777619u;
        static constexpr Size offset = 2166136261u;
    };


    template<>
    struct FnvConstants<8> {
        /* conversion is necessary here because when compiling on
         * 32bit, compilers will complain, despite this template
         * not being instantiated...
         */
        static constexpr Size prime = Size(1099511628211u);
        static constexpr Size offset = Size(14695981039346656037u);
    };

    inline Size mem_hash(void const *p, Size l) {
        using Consts = FnvConstants<sizeof(Size)>;
        byte const *d = static_cast<byte const *>(p);
        Size h = Consts::offset;
        for (byte const *it = d, *end = d + l; it != end; ++it) {
            h ^= *it;
            h *= Consts::prime;
        }
        return h;
    }

    template<typename T, Size = sizeof(T) / sizeof(Size)>
    struct ScalarHash;

    template<typename T>
    struct ScalarHash<T, 0> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; Size h; } u;
            u.h = 0;
            u.v = v;
            return u.h;
        }
    };

    template<typename T>
    struct ScalarHash<T, 1> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; Size h; } u;
            u.v = v;
            return u.h;
        }
    };

    template<typename T>
    struct ScalarHash<T, 2> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; struct { Size h1, h2; }; } u;
            u.v = v;
            return mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };

    template<typename T>
    struct ScalarHash<T, 3> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; struct { Size h1, h2, h3; }; } u;
            u.v = v;
            return mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };

    template<typename T>
    struct ScalarHash<T, 4> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; struct { Size h1, h2, h3, h4; }; } u;
            u.v = v;
            return mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };
} /* namespace detail */

template<>
struct ToHash<llong>: detail::ScalarHash<llong> {};
template<>
struct ToHash<ullong>: detail::ScalarHash<ullong> {};

template<>
struct ToHash<float>: detail::ScalarHash<float> {
    Size operator()(float v) const {
        if (v == 0) return 0;
        return detail::ScalarHash<float>::operator()(v);
    }
};

template<>
struct ToHash<double>: detail::ScalarHash<double> {
    Size operator()(double v) const {
        if (v == 0) return 0;
        return detail::ScalarHash<double>::operator()(v);
    }
};

template<>
struct ToHash<ldouble>: detail::ScalarHash<ldouble> {
    Size operator()(ldouble v) const {
        if (v == 0) {
            return 0;
        }
#ifdef __i386__
        union { ldouble v; struct { Size h1, h2, h3, h4; }; } u;
        u.h1 = u.h2 = u.h3 = u.h4 = 0;
        u.v = v;
        return (u.h1 ^ u.h2 ^ u.h3 ^ u.h4);
#else
#ifdef __x86_64__
        union { ldouble v; struct { Size h1, h2; }; } u;
        u.h1 = u.h2 = 0;
        u.v = v;
        return (u.h1 ^ u.h2);
#else
        return detail::ScalarHash<ldouble>::operator()(v);
#endif
#endif
    }
};

namespace detail {
    template<typename T, bool = IsSame<RemoveConst<T>, char>>
    struct ToHashPtr {
        using Argument = T *;
        using Result = Size;
        Size operator()(T *v) const {
            union { T *v; Size h; } u;
            u.v = v;
            return detail::mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };

    template<typename T>
    struct ToHashPtr<T, true> {
        using Argument = T *;
        using Result = Size;
        Size operator()(T *v) const {
            return detail::mem_hash(v, strlen(v));
        }
    };
}

template<typename T>
struct ToHash<T *>: detail::ToHashPtr<T> {};

template<typename T>
typename ToHash<T>::Result to_hash(T const &v) {
    return ToHash<T>()(v);
}

} /* namespace ostd */

#endif
