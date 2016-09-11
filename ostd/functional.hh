/* Function objects for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 * Portions of this file are originally adapted from the libc++ project.
 */

#ifndef OSTD_FUNCTIONAL_HH
#define OSTD_FUNCTIONAL_HH

#include <string.h>

#include "ostd/platform.hh"
#include "ostd/new.hh"
#include "ostd/memory.hh"
#include "ostd/utility.hh"
#include "ostd/type_traits.hh"
#include <ostd/tuple.hh>

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

/* reference wrapper */

template<typename T>
struct ReferenceWrapper {
    using Type = T;

    ReferenceWrapper(T &v): p_ptr(address_of(v)) {}
    ReferenceWrapper(ReferenceWrapper const &) = default;
    ReferenceWrapper(T &&) = delete;

    ReferenceWrapper &operator=(ReferenceWrapper const &) = default;

    operator T &() const { return *p_ptr; }
    T &get() const { return *p_ptr; }

private:
    T *p_ptr;
};

template<typename T>
ReferenceWrapper<T> ref(T &v) {
    return ReferenceWrapper<T>(v);
}
template<typename T>
ReferenceWrapper<T> ref(ReferenceWrapper<T> v) {
    return ReferenceWrapper<T>(v);
}
template<typename T>
void ref(T const &&) = delete;

template<typename T>
ReferenceWrapper<T const> cref(T const &v) {
    return ReferenceWrapper<T>(v);
}
template<typename T>
ReferenceWrapper<T const> cref(ReferenceWrapper<T> v) {
    return ReferenceWrapper<T>(v);
}
template<typename T>
void cref(T const &&) = delete;

/* mem_fn */

namespace detail {
    template<typename, typename>
    struct MemTypes;
    template<typename T, typename R, typename ...A>
    struct MemTypes<T, R(A...)> {
        using Result = R;
        using Argument = T;
    };
    template<typename T, typename R, typename A>
    struct MemTypes<T, R(A)> {
        using Result = R;
        using FirstArgument = T;
        using SecondArgument = A;
    };
    template<typename T, typename R, typename ...A>
    struct MemTypes<T, R(A...) const> {
        using Result = R;
        using Argument = T const;
    };
    template<typename T, typename R, typename A>
    struct MemTypes<T, R(A) const> {
        using Result = R;
        using FirstArgument = T const;
        using SecondArgument = A;
    };

    template<typename R, typename T>
    class MemFn: MemTypes<T, R> {
        R T::*p_ptr;
    public:
        MemFn(R T::*ptr): p_ptr(ptr) {}
        template<typename... A>
        auto operator()(T &obj, A &&...args) ->
            decltype(((obj).*(p_ptr))(forward<A>(args)...))
        {
            return ((obj).*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T const &obj, A &&...args) ->
            decltype(((obj).*(p_ptr))(forward<A>(args)...))
        const {
            return ((obj).*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T *obj, A &&...args) ->
            decltype(((obj)->*(p_ptr))(forward<A>(args)...))
        {
            return ((obj)->*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T const *obj, A &&...args) ->
            decltype(((obj)->*(p_ptr))(forward<A>(args)...))
        const {
            return ((obj)->*(p_ptr))(forward<A>(args)...);
        }
    };
} /* namespace detail */

template<typename R, typename T>
detail::MemFn<R, T> mem_fn(R T:: *ptr) {
    return detail::MemFn<R, T>(ptr);
}

/* function impl adapted from libc++
 */

template<typename>
class Function;

namespace detail {
    template<typename T>
    inline bool func_is_not_null(T const &) {
        return true;
    }

    template<typename T>
    inline bool func_is_not_null(T *ptr) {
        return ptr;
    }

    template<typename R, typename C>
    inline bool func_is_not_null(R C::*ptr) {
        return ptr;
    }

    template<typename T>
    inline bool func_is_not_null(Function<T> const &f) {
        return !!f;
    }

    template<typename R>
    struct FuncInvokeVoidReturnWrapper {
        template<typename ...A>
        static R call(A &&...args) {
            return func_invoke(forward<A>(args)...);
        }
    };

    template<>
    struct FuncInvokeVoidReturnWrapper<void> {
        template<typename ...A>
        static void call(A &&...args) {
            func_invoke(forward<A>(args)...);
        }
    };

    template<typename T>
    class FuncBase;

    template<typename R, typename ...A>
    class FuncBase<R(A...)> {
        FuncBase(FuncBase const &);
        FuncBase &operator=(FuncBase const &);
    public:
        FuncBase() {}
        virtual ~FuncBase() {}
        virtual FuncBase *clone() const = 0;
        virtual void clone(FuncBase *) const = 0;
        virtual void destroy() noexcept = 0;
        virtual void destroy_deallocate() noexcept = 0;
        virtual R operator()(A &&...args) = 0;
    };

    template<typename F, typename A, typename AT>
    class FuncCore;

    template<typename F, typename A, typename R, typename ...AT>
    class FuncCore<F, A, R(AT...)>: public FuncBase<R(AT...)> {
        CompressedPair<F, A> f_stor;
public:
        explicit FuncCore(F &&f):
            f_stor(
                piecewise_construct,
                forward_as_tuple(ostd::move(f)),
                forward_as_tuple()
            )
        {}

        explicit FuncCore(F const &f, A const &a):
            f_stor(
                piecewise_construct,
                forward_as_tuple(f),
                forward_as_tuple(a)
            )
        {}

        explicit FuncCore(F const &f, A &&a):
            f_stor(
                piecewise_construct,
                forward_as_tuple(f),
                forward_as_tuple(ostd::move(a))
            )
        {}

        explicit FuncCore(F &&f, A &&a):
            f_stor(
                piecewise_construct,
                forward_as_tuple(ostd::move(f)),
                forward_as_tuple(ostd::move(a))
            )
        {}

        virtual FuncBase<R(AT...)> *clone() const;
        virtual void clone(FuncBase<R(AT...)> *) const;
        virtual void destroy() noexcept;
        virtual void destroy_deallocate() noexcept;
        virtual R operator()(AT &&...args);
    };

    template<typename F, typename A, typename R, typename ...AT>
    FuncBase<R(AT...)> *FuncCore<F, A, R(AT...)>::clone() const {
        using AA = AllocatorRebind<A, FuncCore>;
        AA a(f_stor.second());
        using D = AllocatorDestructor<AA>;
        Box<FuncCore, D> hold(a.allocate(1), D(a, 1));
        ::new(hold.get()) FuncCore(f_stor.first(), A(a));
        return hold.release();
    }

    template<typename F, typename A, typename R, typename ...AT>
    void FuncCore<F, A, R(AT...)>::clone(FuncBase<R(AT...)> *p) const {
        ::new (p) FuncCore(f_stor.first(), f_stor.second());
    }

    template<typename F, typename A, typename R, typename ...AT>
    void FuncCore<F, A, R(AT...)>::destroy() noexcept {
        f_stor.~CompressedPair<F, A>();
    }

    template<typename F, typename A, typename R, typename ...AT>
    void FuncCore<F, A, R(AT...)>::destroy_deallocate() noexcept {
        using AA = AllocatorRebind<A, FuncCore>;
        AA a(f_stor.second());
        f_stor.~CompressedPair<F, A>();
        a.deallocate(this, 1);
    }

    template<typename F, typename A, typename R, typename ...AT>
    R FuncCore<F, A, R(AT...)>::operator()(AT &&...args) {
        using Invoker = FuncInvokeVoidReturnWrapper<R>;
        return Invoker::call(f_stor.first(), forward<AT>(args)...);
    }
} /* namespace detail */

template<typename R, typename ...Args>
class Function<R(Args...)> {
    using Base = detail::FuncBase<R(Args...)>;
    AlignedStorage<3 * sizeof(void *)> p_buf;
    Base *p_f;

    static inline Base *as_base(void *p) {
        return reinterpret_cast<Base *>(p);
    }

    template<
        typename F,
        bool = !IsSame<F, Function> && detail::IsInvokable<F &, Args...>
    >
    struct CallableBase;

    template<typename F>
    struct CallableBase<F, true> {
        static constexpr bool value =
            IsSame<R, void> || IsConvertible<detail::InvokeOf<F &, Args...>, R>;
    };
    template<typename F>
    struct CallableBase<F, false> {
        static constexpr bool value = false;
    };

    template<typename F>
    static constexpr bool Callable = CallableBase<F>::value;

public:
    using Result = R;

    Function() noexcept: p_f(nullptr) {}
    Function(Nullptr) noexcept: p_f(nullptr) {}

    Function(Function const &);
    Function(Function &&) noexcept;

    template<
        typename F, typename = EnableIf<Callable<F> && !IsSame<F, Function>>
    >
    Function(F);

    template<typename A>
    Function(AllocatorArg, A const &) noexcept: p_f(nullptr) {}
    template<typename A>
    Function(AllocatorArg, A const &, Nullptr) noexcept: p_f(nullptr) {}
    template<typename A>
    Function(AllocatorArg, A const &, Function const &);
    template<typename A>
    Function(AllocatorArg, A const &, Function &&);
    template<typename F, typename A, typename = EnableIf<Callable<F>>>
    Function(AllocatorArg, A const &, F);

    Function &operator=(Function const &f) {
        Function(f).swap(*this);
        return *this;
    }

    Function &operator=(Function &&) noexcept;
    Function &operator=(Nullptr) noexcept;

    template<typename F>
    EnableIf<
        Callable<Decay<F>> && !IsSame<RemoveReference<F>, Function>,
        Function &
    > operator=(F &&f) {
        Function(forward<F>(f)).swap(*this);
        return *this;
    }

    ~Function();

    void swap(Function &) noexcept;

    explicit operator bool() const noexcept { return p_f; }

    /* deleted overloads close possible hole in the type system */
    template<typename RR, typename ...AA>
    bool operator==(Function<RR(AA...)> &) const = delete;
    template<typename RR, typename ...AA>
    bool operator!=(Function<RR(AA...)> &) const = delete;

    R operator()(Args ...a) const {
        return (*p_f)(forward<Args>(a)...);
    }
};

template<typename R, typename ...Args>
Function<R(Args...)>::Function(Function const &f) {
    if (!f.p_f) {
        p_f = nullptr;
    } else if (static_cast<void *>(f.p_f) == &f.p_buf) {
        p_f = as_base(&p_buf);
        f.p_f->clone(p_f);
    } else {
        p_f = f.p_f->clone();
    }
}

template<typename R, typename ...Args>
template<typename A>
Function<R(Args...)>::Function(
    AllocatorArg, A const &, Function const &f
) {
    if (!f.p_f) {
        p_f = nullptr;
    } else if (static_cast<void *>(f.p_f) == &f.p_buf) {
        p_f = as_base(&p_buf);
        f.p_f->clone(p_f);
    } else {
        p_f = f.p_f->clone();
    }
}

template<typename R, typename ...Args>
Function<R(Args...)>::Function(Function &&f) noexcept {
    if (!f.p_f) {
        p_f = nullptr;
    } else if (static_cast<void *>(f.p_f) == &f.p_buf) {
        p_f = as_base(&p_buf);
        f.p_f->clone(p_f);
    } else {
        p_f = f.p_f;
        f.p_f = nullptr;
    }
}

template<typename R, typename ...Args>
template<typename A>
Function<R(Args...)>::Function(
    AllocatorArg, A const &, Function &&f
) {
    if (!f.p_f) {
        p_f = nullptr;
    } else if (static_cast<void *>(f.p_f) == &f.p_buf) {
        p_f = as_base(&p_buf);
        f.p_f->clone(p_f);
    } else {
        p_f = f.p_f;
        f.p_f = nullptr;
    }
}

template<typename R, typename ...Args>
template<typename F, typename>
Function<R(Args...)>::Function(F f): p_f(nullptr) {
    if (!detail::func_is_not_null(f)) {
        return;
    }
    using FF = detail::FuncCore<F, Allocator<F>, R(Args...)>;
    if ((sizeof(FF) <= sizeof(p_buf)) && IsNothrowCopyConstructible<F>) {
        p_f = ::new(static_cast<void *>(&p_buf)) FF(move(f));
        return;
    }
    using AA = Allocator<FF>;
    AA a;
    using D = detail::AllocatorDestructor<AA>;
    Box<FF, D> hold(a.allocate(1), D(a, 1));
    ::new(hold.get()) FF(move(f), Allocator<F>(a));
    p_f = hold.release();
}

template<typename R, typename ...Args>
template<typename F, typename A, typename>
Function<R(Args...)>::Function(AllocatorArg, A const &a, F f):
    p_f(nullptr)
{
    if (!detail::func_is_not_null(f)) {
        return;
    }
    using FF = detail::FuncCore<F, A, R(Args...)>;
    using AA = AllocatorRebind<A, FF>;
    AA aa(a);
    if (
        (sizeof(FF) <= sizeof(p_buf)) && IsNothrowCopyConstructible<F> &&
        IsNothrowCopyConstructible<AA>
    ) {
        p_f = ::new(static_cast<void *>(&p_buf)) FF(move(f), A(aa));
        return;
    }
    using D = detail::AllocatorDestructor<AA>;
    Box<FF, D> hold(aa.allocate(1), D(aa, 1));
    ::new(hold.get()) FF(move(f), A(aa));
    p_f = hold.release();
}

template<typename R, typename ...Args>
Function<R(Args...)> &Function<R(Args...)>::operator=(Function &&f)
    noexcept
{
    if (static_cast<void *>(p_f) == &p_buf) {
        p_f->destroy();
    } else if (p_f) {
        p_f->destroy_deallocate();
    }
    if (f.p_f == nullptr) {
        p_f = nullptr;
    } else if (static_cast<void *>(f.p_f) == &f.p_buf) {
        p_f = as_base(&p_buf);
        f.p_f->clone(p_f);
    } else {
        p_f = f.p_f;
        f.p_f = nullptr;
    }
    return *this;
}

template<typename R, typename ...Args>
Function<R(Args...)> &Function<R(Args...)>::operator=(Nullptr) noexcept {
    if (static_cast<void *>(p_f) == &p_buf) {
        p_f->destroy();
    } else if (p_f) {
        p_f->destroy_deallocate();
    }
    p_f = nullptr;
    return *this;
}

template<typename R, typename ...Args>
Function<R(Args...)>::~Function() {
    if (static_cast<void *>(p_f) == &p_buf) {
        p_f->destroy();
    } else if (p_f) {
        p_f->destroy_deallocate();
    }
}

template<typename R, typename ...Args>
void Function<R(Args...)>::swap(Function &f) noexcept {
    if (
        (static_cast<void *>(p_f) == &p_buf) &&
        (static_cast<void *>(f.p_f) == &f.p_buf)
    ) {
        /* both in small storage */
        AlignedStorage<sizeof(p_buf)> tmpbuf;
        Base *t = as_base(&tmpbuf);
        p_f->clone(t);
        p_f->destroy();
        p_f = nullptr;
        f.p_f->clone(as_base(&p_buf));
        f.p_f->destroy();
        f.p_f = nullptr;
        p_f = as_base(&p_buf);
        t->clone(as_base(&f.p_buf));
        t->destroy();
        f.p_f = as_base(&f.p_buf);
    } else if (static_cast<void *>(p_f) == &p_buf) {
        /* ours in small storage */
        p_f->clone(as_base(&f.p_buf));
        p_f->destroy();
        p_f = f.p_f;
        f.p_f = as_base(&f.p_buf);
    } else if (static_cast<void *>(f.p_f) == &f.p_buf) {
        /* the other in small storage */
        f.p_f->clone(as_base(&p_buf));
        f.p_f->destroy();
        f.p_f = p_f;
        p_f = as_base(&p_buf);
    } else {
        detail::swap_adl(p_f, f.p_f);
    }
}

template<typename R, typename ...Args>
inline bool operator==(Function<R(Args...)> const &f, Nullptr) noexcept {
    return !f;
}

template<typename R, typename ...Args>
inline bool operator==(Nullptr, Function<R(Args...)> const &f) noexcept {
    return !f;
}

template<typename R, typename ...Args>
inline bool operator!=(Function<R(Args...)> const &f, Nullptr) noexcept {
    return bool(f);
}

template<typename R, typename ...Args>
inline bool operator!=(Nullptr, Function<R(Args...)> const &f) noexcept {
    return bool(f);
}

namespace detail {
    template<typename F>
    struct DcLambdaTypes: DcLambdaTypes<decltype(&F::operator())> {};

    template<typename C, typename R, typename ...A>
    struct DcLambdaTypes<R (C::*)(A...) const> {
        using Ptr = R (*)(A...);
        using Obj = Function<R(A...)>;
    };

    template<typename FF>
    static char dc_func_test(typename DcLambdaTypes<FF>::Ptr);
    template<typename FF>
    static int dc_func_test(...);

    template<typename F>
    constexpr bool DcFuncTest = (sizeof(dc_func_test<F>(declval<F>())) == 1);

    template<typename F, bool = DcFuncTest<F>>
    struct DcFuncTypeObjBase {
        using Type = typename DcLambdaTypes<F>::Obj;
    };

    template<typename F>
    struct DcFuncTypeObjBase<F, true> {
        using Type = typename DcLambdaTypes<F>::Ptr;
    };

    template<typename F, bool =
        IsDefaultConstructible<F> && IsMoveConstructible<F>
    >
    struct DcFuncTypeObj {
        using Type = typename DcFuncTypeObjBase<F>::Type;
    };

    template<typename F>
    struct DcFuncTypeObj<F, true> {
        using Type = F;
    };

    template<typename F, bool = IsClass<F>>
    struct DcFuncType {
        using Type = F;
    };

    template<typename F>
    struct DcFuncType<F, true> {
        using Type = typename DcFuncTypeObj<F>::Type;
    };
}

template<typename F>
using FunctionMakeDefaultConstructible = typename detail::DcFuncType<F>::Type;

} /* namespace ostd */

#endif
