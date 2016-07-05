/* Function objects for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_FUNCTIONAL_HH
#define OSTD_FUNCTIONAL_HH

#include <string.h>

#include "ostd/platform.hh"
#include "ostd/new.hh"
#include "ostd/memory.hh"
#include "ostd/utility.hh"
#include "ostd/type_traits.hh"

namespace ostd {

/* basic function objects */

#define OSTD_DEFINE_BINARY_OP(name, op, RT) \
template<typename T> struct name { \
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

    template<typename T> struct CharEqual<T, false> {
        using FirstArgument = T *;
        using SecondArgument = T *;
        using Result = bool;
        bool operator()(T *x, T *y) const {
            return x == y;
        }
    };
}

template<typename T> struct EqualWithCstr {
    using FirstArgument = T;
    using SecondArgument = T;
    bool operator()(T const &x, T const &y) const {
        return x == y;
    }
};

template<typename T> struct EqualWithCstr<T *>: detail::CharEqual<T> {};

template<typename T> struct LogicalNot {
    bool operator()(T const &x) const { return !x; }
    using Argument = T;
    using Result = bool;
};

template<typename T> struct Negate {
    bool operator()(T const &x) const { return -x; }
    using Argument = T;
    using Result = T;
};

template<typename T> struct BinaryNegate {
    using FirstArgument = typename T::FirstArgument;
    using SecondArgument = typename T::SecondArgument;
    using Result = bool;

    explicit BinaryNegate(T const &f): p_fn(f) {}

    bool operator()(FirstArgument const &x,
                    SecondArgument const &y) {
        return !p_fn(x, y);
    }
private:
    T p_fn;
};

template<typename T> struct UnaryNegate {
    using Argument = typename T::Argument;
    using Result = bool;

    explicit UnaryNegate(T const &f): p_fn(f) {}
    bool operator()(Argument const &x) {
        return !p_fn(x);
    }
private:
    T p_fn;
};

template<typename T> UnaryNegate<T> not1(T const &fn) {
    return UnaryNegate<T>(fn);
}

template<typename T> BinaryNegate<T> not2(T const &fn) {
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
template<typename T> struct FromLilEndian: detail::EndianSame<T> {};
template<typename T> struct FromBigEndian: EndianSwap<T> {};
#else
template<typename T> struct FromLilEndian: EndianSwap<T> {};
template<typename T> struct FromBigEndian: detail::EndianSame<T> {};
#endif

template<typename T> T from_lil_endian(T x) { return FromLilEndian<T>()(x); }
template<typename T> T from_big_endian(T x) { return FromBigEndian<T>()(x); }

/* hash */

template<typename T> struct ToHash {
    using Argument = T;
    using Result = Size;

    Size operator()(T const &v) const {
        return v.to_hash();
    }
};

namespace detail {
    template<typename T> struct ToHashBase {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            return Size(v);
        }
    };
}

#define OSTD_HASH_BASIC(T) template<> struct ToHash<T>: detail::ToHashBase<T> {};

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

OSTD_HASH_BASIC(Char16)
OSTD_HASH_BASIC(Char32)
OSTD_HASH_BASIC(Wchar)

#undef OSTD_HASH_BASIC

namespace detail {
    template<Size E> struct FnvConstants {
        static constexpr Size prime = 16777619u;
        static constexpr Size offset = 2166136261u;
    };


    template<> struct FnvConstants<8> {
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

    template<typename T> struct ScalarHash<T, 0> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; Size h; } u;
            u.h = 0;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct ScalarHash<T, 1> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; Size h; } u;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct ScalarHash<T, 2> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; struct { Size h1, h2; }; } u;
            u.v = v;
            return mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };

    template<typename T> struct ScalarHash<T, 3> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; struct { Size h1, h2, h3; }; } u;
            u.v = v;
            return mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };

    template<typename T> struct ScalarHash<T, 4> {
        using Argument = T;
        using Result = Size;

        Size operator()(T v) const {
            union { T v; struct { Size h1, h2, h3, h4; }; } u;
            u.v = v;
            return mem_hash(static_cast<void const *>(&u), sizeof(u));
        }
    };
} /* namespace detail */

template<> struct ToHash<llong>: detail::ScalarHash<llong> {};
template<> struct ToHash<ullong>: detail::ScalarHash<ullong> {};

template<> struct ToHash<float>: detail::ScalarHash<float> {
    Size operator()(float v) const {
        if (v == 0) return 0;
        return detail::ScalarHash<float>::operator()(v);
    }
};

template<> struct ToHash<double>: detail::ScalarHash<double> {
    Size operator()(double v) const {
        if (v == 0) return 0;
        return detail::ScalarHash<double>::operator()(v);
    }
};

template<> struct ToHash<ldouble>: detail::ScalarHash<ldouble> {
    Size operator()(ldouble v) const {
        if (v == 0) return 0;
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

    template<typename T> struct ToHashPtr<T, true> {
        using Argument = T *;
        using Result = Size;
        Size operator()(T *v) const {
            return detail::mem_hash(v, strlen(v));
        }
    };
}

template<typename T> struct ToHash<T *>: detail::ToHashPtr<T> {};

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
template<typename T> void ref(T const &&) = delete;

template<typename T>
ReferenceWrapper<T const> cref(T const &v) {
    return ReferenceWrapper<T>(v);
}
template<typename T>
ReferenceWrapper<T const> cref(ReferenceWrapper<T> v) {
    return ReferenceWrapper<T>(v);
}
template<typename T> void cref(T const &&) = delete;

/* mem_fn */

namespace detail {
    template<typename, typename> struct MemTypes;
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
          decltype(((obj).*(p_ptr))(forward<A>(args)...)) {
            return ((obj).*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T const &obj, A &&...args) ->
          decltype(((obj).*(p_ptr))(forward<A>(args)...)) const {
            return ((obj).*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T *obj, A &&...args) ->
          decltype(((obj)->*(p_ptr))(forward<A>(args)...)) {
            return ((obj)->*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T const *obj, A &&...args) ->
          decltype(((obj)->*(p_ptr))(forward<A>(args)...)) const {
            return ((obj)->*(p_ptr))(forward<A>(args)...);
        }
    };
} /* namespace detail */

template<typename R, typename T>
detail::MemFn<R, T> mem_fn(R T:: *ptr) {
    return detail::MemFn<R, T>(ptr);
}

/* function impl
 * reference: http://probablydance.com/2013/01/13/a-faster-implementation-of-stdfunction
 */

template<typename> struct Function;

namespace detail {
    struct FunctorData {
        void *p1, *p2;
    };

    template<typename T>
    constexpr bool FunctorInPlace = sizeof(T) <= sizeof(FunctorData) &&
                                    (alignof(FunctorData) % alignof(T)) == 0 &&
                                    IsMoveConstructible<T>;

    struct FunctionManager;

    struct FmStorage {
        FunctorData data;
        FunctionManager const *manager;

        template<typename A>
        A &get_alloc() {
            union {
                FunctionManager const **m;
                A *alloc;
            } u;
            u.m = &manager;
            return *u.alloc;
        }
        template<typename A>
        A const &get_alloc() const {
            union {
                FunctionManager const * const *m;
                A const *alloc;
            } u;
            u.m = &manager;
            return *u.alloc;
        }
    };

    template<typename T, typename A, typename E = void>
    struct FunctorDataManager {
        template<typename R, typename ...Args>
        static R call(FunctorData const &s, Args ...args) {
            return (*reinterpret_cast<T *>(&const_cast<FunctorData &>(s)))(
                forward<Args>(args)...);
        }

        static void store_f(FmStorage &s, T v) {
            new (&get_ref(s)) T(forward<T>(v));
        }

        static void move_f(FmStorage &lhs, FmStorage &&rhs) {
            new (&get_ref(lhs)) T(move(get_ref(rhs)));
        }

        static void destroy_f(A &, FmStorage &s) {
            get_ref(s).~T();
        }

        static T &get_ref(FmStorage const &s) {
            union {
                FunctorData const *data;
                T *ret;
            } u;
            u.data = &s.data;
            return *u.ret;
        }
    };

    template<typename T, typename A>
    struct FunctorDataManager<T, A, EnableIf<!FunctorInPlace<T>>> {
        template<typename R, typename ...Args>
        static R call(FunctorData const &s, Args ...args) {
            return (*reinterpret_cast<AllocatorPointer<A> &>(
                const_cast<FunctorData &>(s)))(forward<Args>(args)...);
        }

        static void store_f(FmStorage &s, T v) {
            A &a = s.get_alloc<A>();
            AllocatorPointer<A> *ptr = new (&get_ptr_ref(s))
                AllocatorPointer<A>(allocator_allocate(a, 1));
            allocator_construct(a, *ptr, forward<T>(v));
        }

        static void move_f(FmStorage &lhs, FmStorage &&rhs) {
            new (&get_ptr_ref(lhs)) AllocatorPointer<A>(move(get_ptr_ref(rhs)));
            get_ptr_ref(rhs) = nullptr;
        }

        static void destroy_f(A &a, FmStorage &s) {
            AllocatorPointer<A> &ptr = get_ptr_ref(s);
            if (!ptr) return;
            allocator_destroy(a, ptr);
            allocator_deallocate(a, ptr, 1);
            ptr = nullptr;
        }

        static T &get_ref(FmStorage const &s) {
            return *get_ptr_ref(s);
        }

        static AllocatorPointer<A> &get_ptr_ref(FmStorage &s) {
            return reinterpret_cast<AllocatorPointer<A> &>(s.data);
        }

        static AllocatorPointer<A> &get_ptr_ref(FmStorage const &s) {
            return reinterpret_cast<AllocatorPointer<A> &>(
                const_cast<FunctorData &>(s.data));
        }
    };

    template<typename T, typename A>
    static FunctionManager const &get_default_fm();

    template<typename T, typename A>
    static void create_fm(FmStorage &s, A &&a) {
        new (&s.get_alloc<A>()) A(move(a));
        s.manager = &get_default_fm<T, A>();
    }

    struct FunctionManager {
        template<typename T, typename A>
        inline static constexpr FunctionManager create_default_manager() {
            return FunctionManager {
                &call_move_and_destroy<T, A>,
                &call_copy<T, A>,
                &call_copy_fo<T, A>,
                &call_destroy<T, A>
            };
        }

        void (* const call_move_and_destroyf)(FmStorage &lhs,
            FmStorage &&rhs);
        void (* const call_copyf)(FmStorage &lhs,
            FmStorage const &rhs);
        void (* const call_copyf_fo)(FmStorage &lhs,
            FmStorage const &rhs);
        void (* const call_destroyf)(FmStorage &s);

        template<typename T, typename A>
        static void call_move_and_destroy(FmStorage &lhs,
                                          FmStorage &&rhs) {
            using Spec = FunctorDataManager<T, A>;
            Spec::move_f(lhs, move(rhs));
            Spec::destroy_f(rhs.get_alloc<A>(), rhs);
            create_fm<T, A>(lhs, move(rhs.get_alloc<A>()));
            rhs.get_alloc<A>().~A();
        }

        template<typename T, typename A>
        static void call_copy(FmStorage &lhs,
                              FmStorage const &rhs) {
            using Spec = FunctorDataManager<T, A>;
            create_fm<T, A>(lhs, A(rhs.get_alloc<A>()));
            Spec::store_f(lhs, Spec::get_ref(rhs));
        }

        template<typename T, typename A>
        static void call_copy_fo(FmStorage &lhs,
                                 FmStorage const &rhs) {
            using Spec = FunctorDataManager<T, A>;
            Spec::store_f(lhs, Spec::get_ref(rhs));
        }

        template<typename T, typename A>
        static void call_destroy(FmStorage &s) {
            using Spec = FunctorDataManager<T, A>;
            Spec::destroy_f(s.get_alloc<A>(), s);
            s.get_alloc<A>().~A();
        }
    };

    template<typename T, typename A>
    inline static FunctionManager const &get_default_fm() {
        static FunctionManager const def_manager
            = FunctionManager::create_default_manager<T, A>();
        return def_manager;
    }

    template<typename R, typename...>
    struct FunctionBase {
        using Result = R;
    };

    template<typename R, typename T>
    struct FunctionBase<R, T> {
        using Result = R;
        using Argument = T;
    };

    template<typename R, typename T, typename U>
    struct FunctionBase<R, T, U> {
        using Result = R;
        using FirstArgument = T;
        using SecondArgument = U;
    };

    template<typename, typename>
    constexpr bool IsValidFunctor = false;

    template<typename R, typename ...A>
    constexpr bool IsValidFunctor<Function<R(A...)>, R(A...)> = false;

    template<typename T>
    T func_to_functor(T &&f) {
        return forward<T>(f);
    }

    template<typename RR, typename T, typename ...AA>
    auto func_to_functor(RR (T::*f)(AA...))
        -> decltype(mem_fn(f)) {
        return mem_fn(f);
    }

    template<typename RR, typename T, typename ...AA>
    auto func_to_functor(RR (T::*f)(AA...) const)
        -> decltype(mem_fn(f)) {
        return mem_fn(f);
    }

    struct ValidFunctorNat {};

    template<typename U, typename ...A>
    static decltype(func_to_functor(declval<U>()) (declval<A>()...))
        valid_functor_test(U *);
    template<typename, typename ...>
    static ValidFunctorNat valid_functor_test(...);

    template<typename T, typename R, typename ...A>
    constexpr bool IsValidFunctor<T, R(A...)>
        = IsConvertible<decltype(valid_functor_test<T, A...>(nullptr)), R>;

    template<typename T>
    using FunctorType = decltype(func_to_functor(declval<T>()));
} /* namespace detail */

template<typename R, typename ...Args>
struct Function<R(Args...)>: detail::FunctionBase<R, Args...> {
    Function(       ) { init_empty(); }
    Function(Nullptr) { init_empty(); }

    Function(Function &&f) {
        init_empty();
        swap(f);
    }

    Function(Function const &f): p_call(f.p_call) {
        f.p_stor.manager->call_copyf(p_stor, f.p_stor);
    }

    template<typename T, typename = EnableIf<
        detail::IsValidFunctor<T, R(Args...)>
    >> Function(T f) {
        if (func_is_null(f)) {
            init_empty();
            return;
        }
        initialize(detail::func_to_functor(forward<T>(f)),
            Allocator<detail::FunctorType<T>>());
    }

    template<typename A>
    Function(AllocatorArg, A const &) { init_empty(); }

    template<typename A>
    Function(AllocatorArg, A const &, Nullptr) { init_empty(); }

    template<typename A>
    Function(AllocatorArg, A const &, Function &&f) {
        init_empty();
        swap(f);
    }

    template<typename A>
    Function(AllocatorArg, A const &a, Function const &f):
    p_call(f.p_call) {
        detail::FunctionManager const *mfa
            = &detail::get_default_fm<AllocatorValue<A>, A>();
        if (f.p_stor.manager == mfa) {
            detail::create_fm<AllocatorValue<A>, A>(p_stor, A(a));
            mfa->call_copyf_fo(p_stor, f.p_stor);
            return;
        }

        using AA = AllocatorRebind<A, Function>;
        detail::FunctionManager const *mff
            = &detail::get_default_fm<Function, AA>();
        if (f.p_stor.manager == mff) {
            detail::create_fm<Function, AA>(p_stor, AA(a));
            mff->call_copyf_fo(p_stor, f.P_stor);
            return;
        }

        initialize(f, AA(a));
    }

    template<typename A, typename T, typename = EnableIf<
        detail::IsValidFunctor<T, R(Args...)>
    >> Function(AllocatorArg, A const &a, T f) {
        if (func_is_null(f)) {
            init_empty();
            return;
        }
        initialize(detail::func_to_functor(forward<T>(f)), A(a));
    }

    ~Function() {
        p_stor.manager->call_destroyf(p_stor);
    }

    Function &operator=(Function &&f) {
        p_stor.manager->call_destroyf(p_stor);
        swap(f);
        return *this;
    }

    Function &operator=(Function const &f) {
        p_stor.manager->call_destroyf(p_stor);
        swap(Function(f));
        return *this;
    };

    R operator()(Args ...args) const {
        return p_call(p_stor.data, forward<Args>(args)...);
    }

    template<typename F, typename A>
    void assign(F &&f, A const &a) {
        Function(allocator_arg, a, forward<F>(f)).swap(*this);
    }

    void swap(Function &f) {
        detail::FmStorage tmp;
        f.p_stor.manager->call_move_and_destroyf(tmp, move(f.p_stor));
        p_stor.manager->call_move_and_destroyf(f.p_stor, move(p_stor));
        tmp.manager->call_move_and_destroyf(p_stor, move(tmp));
        ostd::swap(p_call, f.p_call);
    }

    explicit operator bool() const { return p_call != nullptr; }

private:
    detail::FmStorage p_stor;
    R (*p_call)(detail::FunctorData const &, Args...);

    template<typename T, typename A>
    void initialize(T &&f, A &&a) {
        p_call = &detail::FunctorDataManager<T, A>::template call<R, Args...>;
        detail::create_fm<T, A>(p_stor, forward<A>(a));
        detail::FunctorDataManager<T, A>::store_f(p_stor, forward<T>(f));
    }

    void init_empty() {
        using emptyf = R(*)(Args...);
        using emptya = Allocator<emptyf>;
        p_call = nullptr;
        detail::create_fm<emptyf, emptya>(p_stor, emptya());
        detail::FunctorDataManager<emptyf, emptya>::store_f(p_stor,
            nullptr);
    }

    template<typename T>
    static bool func_is_null(T const &) { return false; }

    static bool func_is_null(R (* const &fptr)(Args...)) {
        return fptr == nullptr;
    }

    template<typename RR, typename T, typename ...AArgs>
    static bool func_is_null(RR (T::* const &fptr)(AArgs...)) {
        return fptr == nullptr;
    }

    template<typename RR, typename T, typename ...AArgs>
    static bool func_is_null(RR (T::* const &fptr)(AArgs...) const) {
        return fptr == nullptr;
    }
};

template<typename T>
bool operator==(Nullptr, Function<T> const &rhs) { return !rhs; }

template<typename T>
bool operator==(Function<T> const &lhs, Nullptr) { return !lhs; }

template<typename T>
bool operator!=(Nullptr, Function<T> const &rhs) { return rhs; }

template<typename T>
bool operator!=(Function<T> const &lhs, Nullptr) { return lhs; }

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

    template<typename F, bool = IsDefaultConstructible<F> &&
                                IsMoveConstructible<F>
    > struct DcFuncTypeObj {
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

template<typename F> using FunctionMakeDefaultConstructible
    = typename detail::DcFuncType<F>::Type;

} /* namespace ostd */

#endif
