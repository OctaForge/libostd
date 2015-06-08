/* Function objects for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_FUNCTIONAL_H
#define OCTA_FUNCTIONAL_H

#include "octa/new.h"
#include "octa/memory.h"
#include "octa/utility.h"
#include "octa/type_traits.h"

namespace octa {

/* basic function objects */

#define OCTA_DEFINE_BINARY_OP(name, op, RT) \
template<typename T> struct name { \
    RT operator()(const T &x, const T &y) const { \
        return x op y; \
    } \
    using FirstArgument = T; \
    using SecondARgument = T; \
    using Result = RT; \
};

OCTA_DEFINE_BINARY_OP(Less, <, bool)
OCTA_DEFINE_BINARY_OP(LessEqual, <=, bool)
OCTA_DEFINE_BINARY_OP(Greater, >, bool)
OCTA_DEFINE_BINARY_OP(GreaterEqual, >=, bool)
OCTA_DEFINE_BINARY_OP(Equal, ==, bool)
OCTA_DEFINE_BINARY_OP(NotEqual, !=, bool)
OCTA_DEFINE_BINARY_OP(LogicalAnd, &&, bool)
OCTA_DEFINE_BINARY_OP(LogicalOr, ||, bool)
OCTA_DEFINE_BINARY_OP(Modulus, %, T)
OCTA_DEFINE_BINARY_OP(Multiplies, *, T)
OCTA_DEFINE_BINARY_OP(Divides, /, T)
OCTA_DEFINE_BINARY_OP(Plus, +, T)
OCTA_DEFINE_BINARY_OP(Minus, -, T)
OCTA_DEFINE_BINARY_OP(BitAnd, &, T)
OCTA_DEFINE_BINARY_OP(BitOr, |, T)
OCTA_DEFINE_BINARY_OP(BitXor, ^, T)

#undef OCTA_DEFINE_BINARY_OP

template<typename T> struct LogicalNot {
    bool operator()(const T &x) const { return !x; }
    using Argument = T;
    using Result = bool;
};

template<typename T> struct Negate {
    bool operator()(const T &x) const { return -x; }
    using Argument = T;
    using Result = T;
};

template<typename T> struct BinaryNegate {
    using FirstArgument = typename T::FirstArgument;
    using SecondArgument = typename T::SecondArgument;
    using Result = bool;

    explicit BinaryNegate(const T &f): p_fn(f) {}

    bool operator()(const FirstArgument &x,
                    const SecondArgument &y) {
        return !p_fn(x, y);
    }
private:
    T p_fn;
};

template<typename T> struct UnaryNegate {
    using Argument = typename T::Argument;
    using Result = bool;

    explicit UnaryNegate(const T &f): p_fn(f) {}
    bool operator()(const Argument &x) {
        return !p_fn(x);
    }
private:
    T p_fn;
};

template<typename T> UnaryNegate<T> not1(const T &fn) {
    return UnaryNegate<T>(fn);
}

template<typename T> BinaryNegate<T> not2(const T &fn) {
    return BinaryNegate<T>(fn);
}

/* hash */

template<typename T> struct Hash;

namespace detail {
    template<typename T> struct HashBase {
        using Argument = T;
        using Result = size_t;

        size_t operator()(T v) const {
            return size_t(v);
        }
    };
}

#define OCTA_HASH_BASIC(T) template<> struct Hash<T>: octa::detail::HashBase<T> {};

OCTA_HASH_BASIC(bool)
OCTA_HASH_BASIC(char)
OCTA_HASH_BASIC(schar)
OCTA_HASH_BASIC(uchar)
OCTA_HASH_BASIC(char16_t)
OCTA_HASH_BASIC(char32_t)
OCTA_HASH_BASIC(wchar_t)
OCTA_HASH_BASIC(short)
OCTA_HASH_BASIC(ushort)
OCTA_HASH_BASIC(int)
OCTA_HASH_BASIC(uint)
OCTA_HASH_BASIC(long)
OCTA_HASH_BASIC(ulong)

#undef OCTA_HASH_BASIC

namespace detail {
    static inline size_t mem_hash(const void *p, size_t l) {
        const uchar *d = (const uchar *)p;
        size_t h = 5381;
        for (size_t i = 0; i < l; ++i) h = ((h << 5) + h) ^ d[i];
        return h;
    }

    template<typename T, size_t = sizeof(T) / sizeof(size_t)>
    struct ScalarHash;

    template<typename T> struct ScalarHash<T, 0> {
        using Argument = T;
        using Result = size_t;

        size_t operator()(T v) const {
            union { T v; size_t h; } u;
            u.h = 0;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct ScalarHash<T, 1> {
        using Argument = T;
        using Result = size_t;

        size_t operator()(T v) const {
            union { T v; size_t h; } u;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct ScalarHash<T, 2> {
        using Argument = T;
        using Result = size_t;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename T> struct ScalarHash<T, 3> {
        using Argument = T;
        using Result = size_t;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2, h3; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename T> struct ScalarHash<T, 4> {
        using Argument = T;
        using Result = size_t;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2, h3, h4; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };
} /* namespace detail */

template<> struct Hash<llong>: octa::detail::ScalarHash<llong> {};
template<> struct Hash<ullong>: octa::detail::ScalarHash<ullong> {};

template<> struct Hash<float>: octa::detail::ScalarHash<float> {
    size_t operator()(float v) const {
        if (v == 0) return 0;
        return octa::detail::ScalarHash<float>::operator()(v);
    }
};

template<> struct Hash<double>: octa::detail::ScalarHash<double> {
    size_t operator()(double v) const {
        if (v == 0) return 0;
        return octa::detail::ScalarHash<double>::operator()(v);
    }
};

template<> struct Hash<ldouble>: octa::detail::ScalarHash<ldouble> {
    size_t operator()(ldouble v) const {
        if (v == 0) return 0;
#ifdef __i386__
        union { ldouble v; struct { size_t h1, h2, h3, h4; }; } u;
        u.h1 = u.h2 = u.h3 = u.h4 = 0;
        u.v = v;
        return (u.h1 ^ u.h2 ^ u.h3 ^ u.h4);
#else
#ifdef __x86_64__
        union { ldouble v; struct { size_t h1, h2; }; } u;
        u.h1 = u.h2 = 0;
        u.v = v;
        return (u.h1 ^ u.h2);
#else
        return octa::detail::ScalarHash<ldouble>::operator()(v);
#endif
#endif
    }
};

template<typename T> struct Hash<T *> {
    using Argument = T *;
    using Result = size_t;

    size_t operator()(T *v) const {
        union { T *v; size_t h; } u;
        u.v = v;
        return octa::detail::mem_hash((const void *)&u, sizeof(u));
    }
};

/* reference wrapper */

template<typename T>
struct ReferenceWrapper {
    using Type = T;

    ReferenceWrapper(T &v): p_ptr(address_of(v)) {}
    ReferenceWrapper(const ReferenceWrapper &) = default;
    ReferenceWrapper(T &&) = delete;

    ReferenceWrapper &operator=(const ReferenceWrapper &) = default;

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
template<typename T> void ref(const T &&) = delete;

template<typename T>
ReferenceWrapper<const T> cref(const T &v) {
    return ReferenceWrapper<T>(v);
}
template<typename T>
ReferenceWrapper<const T> cref(ReferenceWrapper<T> v) {
    return ReferenceWrapper<T>(v);
}
template<typename T> void cref(const T &&) = delete;

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
        using Argument = const T;
    };
    template<typename T, typename R, typename A>
    struct MemTypes<T, R(A) const> {
        using Result = R;
        using FirstArgument = const T;
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
        auto operator()(const T &obj, A &&...args) ->
          decltype(((obj).*(p_ptr))(forward<A>(args)...)) const {
            return ((obj).*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(T *obj, A &&...args) ->
          decltype(((obj)->*(p_ptr))(forward<A>(args)...)) {
            return ((obj)->*(p_ptr))(forward<A>(args)...);
        }
        template<typename... A>
        auto operator()(const T *obj, A &&...args) ->
          decltype(((obj)->*(p_ptr))(forward<A>(args)...)) const {
            return ((obj)->*(p_ptr))(forward<A>(args)...);
        }
    };
} /* namespace detail */

template<typename R, typename T>
octa::detail::MemFn<R, T> mem_fn(R T:: *ptr) {
    return octa::detail::MemFn<R, T>(ptr);
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
    struct FunctorInPlace {
        static constexpr bool value = sizeof(T)  <= sizeof(FunctorData)
          && (alignof(FunctorData) % alignof(T)) == 0
          && octa::IsMoveConstructible<T>::value;
    };

    struct FunctionManager;

    struct FmStorage {
        FunctorData data;
        const FunctionManager *manager;

        template<typename A>
        A &get_alloc() {
            union {
                const FunctionManager **m;
                A *alloc;
            } u;
            u.m = &manager;
            return *u.alloc;
        }
        template<typename A>
        const A &get_alloc() const {
            union {
                const FunctionManager * const *m;
                const A *alloc;
            } u;
            u.m = &manager;
            return *u.alloc;
        }
    };

    template<typename T, typename A, typename E = void>
    struct FunctorDataManager {
        template<typename R, typename ...Args>
        static R call(const FunctorData &s, Args ...args) {
            return ((T &)s)(octa::forward<Args>(args)...);
        }

        static void store_f(FmStorage &s, T v) {
            new (&get_ref(s)) T(octa::forward<T>(v));
        }

        static void move_f(FmStorage &lhs, FmStorage &&rhs) {
            new (&get_ref(lhs)) T(octa::move(get_ref(rhs)));
        }

        static void destroy_f(A &, FmStorage &s) {
            get_ref(s).~T();
        }

        static T &get_ref(const FmStorage &s) {
            union {
                const FunctorData *data;
                T *ret;
            } u;
            u.data = &s.data;
            return *u.ret;
        }
    };

    template<typename T, typename A>
    struct FunctorDataManager<T, A,
        EnableIf<!FunctorInPlace<T>::value>
    > {
        template<typename R, typename ...Args>
        static R call(const FunctorData &s, Args ...args) {
            return (*(octa::AllocatorPointer<A> &)s)
                (octa::forward<Args>(args)...);
        }

        static void store_f(FmStorage &s, T v) {
            A &a = s.get_alloc<A>();
            AllocatorPointer<A> *ptr = new (&get_ptr_ref(s))
                AllocatorPointer<A>(allocator_allocate(a, 1));
            allocator_construct(a, *ptr, octa::forward<T>(v));
        }

        static void move_f(FmStorage &lhs, FmStorage &&rhs) {
            new (&get_ptr_ref(lhs)) AllocatorPointer<A>(octa::move(
                get_ptr_ref(rhs)));
            get_ptr_ref(rhs) = nullptr;
        }

        static void destroy_f(A &a, FmStorage &s) {
            AllocatorPointer<A> &ptr = get_ptr_ref(s);
            if (!ptr) return;
            allocator_destroy(a, ptr);
            allocator_deallocate(a, ptr, 1);
            ptr = nullptr;
        }

        static T &get_ref(const FmStorage &s) {
            return *get_ptr_ref(s);
        }

        static AllocatorPointer<A> &get_ptr_ref(FmStorage &s) {
            return (AllocatorPointer<A> &)(s.data);
        }

        static AllocatorPointer<A> &get_ptr_ref(const FmStorage &s) {
            return (AllocatorPointer<A> &)(s.data);
        }
    };

    template<typename T, typename A>
    static const FunctionManager &get_default_fm();

    template<typename T, typename A>
    static void create_fm(FmStorage &s, A &&a) {
        new (&s.get_alloc<A>()) A(octa::move(a));
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
            const FmStorage &rhs);
        void (* const call_copyf_fo)(FmStorage &lhs,
            const FmStorage &rhs);
        void (* const call_destroyf)(FmStorage &s);

        template<typename T, typename A>
        static void call_move_and_destroy(FmStorage &lhs,
        FmStorage &&rhs) {
            using Spec = FunctorDataManager<T, A>;
            Spec::move_f(lhs, octa::move(rhs));
            Spec::destroy_f(rhs.get_alloc<A>(), rhs);
            create_fm<T, A>(lhs, octa::move(rhs.get_alloc<A>()));
            rhs.get_alloc<A>().~A();
        }

        template<typename T, typename A>
        static void call_copy(FmStorage &lhs,
        const FmStorage &rhs) {
            using Spec = FunctorDataManager<T, A>;
            create_fm<T, A>(lhs, A(rhs.get_alloc<A>()));
            Spec::store_f(lhs, Spec::get_ref(rhs));
        }

        template<typename T, typename A>
        static void call_copy_fo(FmStorage &lhs,
        const FmStorage &rhs) {
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
    inline static const FunctionManager &get_default_fm() {
        static const FunctionManager def_manager
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
    struct IsValidFunctor {
        static constexpr bool value = false;
    };

    template<typename R, typename ...A>
    struct IsValidFunctor<Function<R(A...)>, R(A...)> {
        static constexpr bool value = false;
    };

    template<typename T>
    T func_to_functor(T &&f) {
        return octa::forward<T>(f);
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

    template<typename T, typename R, typename ...A>
    struct IsValidFunctor<T, R(A...)> {
        struct Nat {};

        template<typename U>
        static decltype(func_to_functor(octa::declval<U>())
            (octa::declval<A>()...)) test(U *);
        template<typename>
        static Nat test(...);

        static constexpr bool value = octa::IsConvertible<
            decltype(test<T>(nullptr)), R
        >::value;
    };

    template<typename T>
    using FunctorType = decltype(func_to_functor(octa::declval<T>()));
} /* namespace detail */

template<typename R, typename ...Args>
struct Function<R(Args...)>: octa::detail::FunctionBase<R, Args...> {
    Function(       ) { init_empty(); }
    Function(Nullptr) { init_empty(); }

    Function(Function &&f) {
        init_empty();
        swap(f);
    }

    Function(const Function &f): p_call(f.p_call) {
        f.p_stor.manager->call_copyf(p_stor, f.p_stor);
    }

    template<typename T>
    Function(T f, EnableIf<
        octa::detail::IsValidFunctor<T, R(Args...)>::value, bool
    > = true) {
        if (func_is_null(f)) {
            init_empty();
            return;
        }
        initialize(octa::detail::func_to_functor(octa::forward<T>(f)),
            octa::Allocator<octa::detail::FunctorType<T>>());
    }

    template<typename A>
    Function(octa::AllocatorArg, const A &) { init_empty(); }

    template<typename A>
    Function(octa::AllocatorArg, const A &, Nullptr) { init_empty(); }

    template<typename A>
    Function(octa::AllocatorArg, const A &, Function &&f) {
        init_empty();
        swap(f);
    }

    template<typename A>
    Function(octa::AllocatorArg, const A &a, const Function &f):
    p_call(f.p_call) {
        const octa::detail::FunctionManager *mfa
            = &octa::detail::get_default_fm<octa::AllocatorValue<A>, A>();
        if (f.p_stor.manager == mfa) {
            octa::detail::create_fm<octa::AllocatorValue<A>, A>(p_stor, A(a));
            mfa->call_copyf_fo(p_stor, f.p_stor);
            return;
        }

        using AA = AllocatorRebind<A, Function>;
        const octa::detail::FunctionManager *mff
            = &octa::detail::get_default_fm<Function, AA>();
        if (f.p_stor.manager == mff) {
            octa::detail::create_fm<Function, AA>(p_stor, AA(a));
            mff->call_copyf_fo(p_stor, f.P_stor);
            return;
        }

        initialize(f, AA(a));
    }

    template<typename A, typename T>
    Function(octa::AllocatorArg, const A &a, T f, EnableIf<
        octa::detail::IsValidFunctor<T, R(Args...)>::value, bool
    > = true) {
        if (func_is_null(f)) {
            init_empty();
            return;
        }
        initialize(octa::detail::func_to_functor(octa::forward<T>(f)), A(a));
    }

    ~Function() {
        p_stor.manager->call_destroyf(p_stor);
    }

    Function &operator=(Function &&f) {
        p_stor.manager->call_destroyf(p_stor);
        swap(f);
        return *this;
    }

    Function &operator=(const Function &f) {
        p_stor.manager->call_destroyf(p_stor);
        swap(Function(f));
        return *this;
    };

    R operator()(Args ...args) const {
        return p_call(p_stor.data, octa::forward<Args>(args)...);
    }

    template<typename F, typename A>
    void assign(F &&f, const A &a) {
        Function(octa::allocator_arg, a, octa::forward<F>(f)).swap(*this);
    }

    void swap(Function &f) {
        octa::detail::FmStorage tmp;
        f.p_stor.manager->call_move_and_destroyf(tmp,
            octa::move(f.p_stor));
        p_stor.manager->call_move_and_destroyf(f.p_stor,
            octa::move(p_stor));
        tmp.manager->call_move_and_destroyf(p_stor,
            octa::move(tmp));
        octa::swap(p_call, f.p_call);
    }

    operator bool() const { return p_call != nullptr; }

private:
    octa::detail::FmStorage p_stor;
    R (*p_call)(const octa::detail::FunctorData &, Args...);

    template<typename T, typename A>
    void initialize(T &&f, A &&a) {
        p_call = &octa::detail::FunctorDataManager<T, A>::template call<R, Args...>;
        octa::detail::create_fm<T, A>(p_stor, octa::forward<A>(a));
        octa::detail::FunctorDataManager<T, A>::store_f(p_stor,
            octa::forward<T>(f));
    }

    void init_empty() {
        using emptyf = R(*)(Args...);
        using emptya = octa::Allocator<emptyf>;
        p_call = nullptr;
        octa::detail::create_fm<emptyf, emptya>(p_stor, emptya());
        octa::detail::FunctorDataManager<emptyf, emptya>::store_f(p_stor,
            nullptr);
    }

    template<typename T>
    static bool func_is_null(const T &) { return false; }

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
bool operator==(Nullptr, const Function<T> &rhs) { return !rhs; }

template<typename T>
bool operator==(const Function<T> &lhs, Nullptr) { return !lhs; }

template<typename T>
bool operator!=(Nullptr, const Function<T> &rhs) { return rhs; }

template<typename T>
bool operator!=(const Function<T> &lhs, Nullptr) { return lhs; }

namespace detail {
    template<typename F>
    struct DcLambdaTypes: DcLambdaTypes<decltype(&F::operator())> {};

    template<typename C, typename R, typename ...A>
    struct DcLambdaTypes<R (C::*)(A...) const> {
        using Ptr = R (*)(A...);
        using Obj = octa::Function<R(A...)>;
    };

    template<typename F>
    struct DcFuncTest {
        template<typename FF>
        static char test(typename DcLambdaTypes<FF>::Ptr);
        template<typename FF>
        static int test(...);
        static constexpr bool value = (sizeof(test<F>(octa::declval<F>())) == 1);
    };

    template<typename F, bool = DcFuncTest<F>::value>
    struct DcFuncTypeObjBase {
        using Type = typename DcLambdaTypes<F>::Obj;
    };

    template<typename F>
    struct DcFuncTypeObjBase<F, true> {
        using Type = typename DcLambdaTypes<F>::Ptr;
    };

    template<typename F, bool = octa::IsDefaultConstructible<F>::value &&
                                octa::IsMoveConstructible<F>::value
    > struct DcFuncTypeObj {
        using Type = typename DcFuncTypeObjBase<F>::Type;
    };

    template<typename F>
    struct DcFuncTypeObj<F, true> {
        using Type = F;
    };

    template<typename F, bool = octa::IsClass<F>::value>
    struct DcFuncType {
        using Type = F;
    };

    template<typename F>
    struct DcFuncType<F, true> {
        using Type = typename DcFuncTypeObj<F>::Type;
    };
}

template<typename F> using FunctionMakeDefaultConstructible
    = typename octa::detail::DcFuncType<F>::Type;

} /* namespace octa */

#endif