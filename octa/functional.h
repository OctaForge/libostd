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

#define OCTA_DEFINE_BINARY_OP(_name, _op, _rettype) \
template<typename T> struct _name { \
    _rettype operator()(const T &x, const T &y) const { \
        return x _op y; \
    } \
    typedef T FirstArgument; \
    typedef T SecondArgument; \
    typedef _rettype Result; \
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
    typedef T Argument;
    typedef bool Result;
};

template<typename T> struct Negate {
    bool operator()(const T &x) const { return -x; }
    typedef T Argument;
    typedef T Result;
};

template<typename T> struct BinaryNegate {
    typedef typename T::FirstArgument FirstArgument;
    typedef typename T::SecondArgument SecondArgument;
    typedef bool Result;

    explicit BinaryNegate(const T &f): p_fn(f) {}

    bool operator()(const FirstArgument &x,
                    const SecondArgument &y) {
        return !p_fn(x, y);
    }
private:
    T p_fn;
};

template<typename T> struct UnaryNegate {
    typedef typename T::Argument Argument;
    typedef bool Result;

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
        typedef T Argument;
        typedef size_t Result;

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
        typedef T Argument;
        typedef size_t Result;

        size_t operator()(T v) const {
            union { T v; size_t h; } u;
            u.h = 0;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct ScalarHash<T, 1> {
        typedef T Argument;
        typedef size_t Result;

        size_t operator()(T v) const {
            union { T v; size_t h; } u;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct ScalarHash<T, 2> {
        typedef T Argument;
        typedef size_t Result;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename T> struct ScalarHash<T, 3> {
        typedef T Argument;
        typedef size_t Result;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2, h3; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename T> struct ScalarHash<T, 4> {
        typedef T Argument;
        typedef size_t Result;

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
    typedef T *Argument;
    typedef size_t Result;

    size_t operator()(T *v) const {
        union { T *v; size_t h; } u;
        u.v = v;
        return octa::detail::mem_hash((const void *)&u, sizeof(u));
    }
};

/* reference wrapper */

template<typename T>
struct ReferenceWrapper {
    typedef T type;

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
        typedef R Result;
        typedef T Argument;
    };
    template<typename T, typename R, typename A>
    struct MemTypes<T, R(A)> {
        typedef R Result;
        typedef T FirstArgument;
        typedef A SecondArgument;
    };
    template<typename T, typename R, typename ...A>
    struct MemTypes<T, R(A...) const> {
        typedef R Result;
        typedef const T Argument;
    };
    template<typename T, typename R, typename A>
    struct MemTypes<T, R(A) const> {
        typedef R Result;
        typedef const T FirstArgument;
        typedef A SecondArgument;
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

    template<typename T, typename E = void>
    struct FunctorDataManager {
        template<typename R, typename ...A>
        static R call(const FunctorData &s, A ...args) {
            return ((T &)s)(octa::forward<A>(args)...);
        }

        static void store_f(FunctorData &s, T v) {
            new (&get_ref(s)) T(octa::forward<T>(v));
        }

        static void move_f(FunctorData &lhs, FunctorData &&rhs) {
            new (&get_ref(lhs)) T(octa::move(get_ref(rhs)));
        }

        static void destroy_f(FunctorData &s) {
            get_ref(s).~T();
        }

        static T &get_ref(const FunctorData &s) {
            return (T &)s;
        }
    };

    template<typename T>
    struct FunctorDataManager<T,
        EnableIf<!FunctorInPlace<T>::value>
    > {
        template<typename R, typename ...A>
        static R call(const FunctorData &s, A ...args) {
            return (*(T *&)s)(octa::forward<A>(args)...);
        }

        static void store_f(FunctorData &s, T v) {
            new (&get_ptr_ref(s)) T *(new T(octa::forward<T>(v)));
        }

        static void move_f(FunctorData &lhs, FunctorData &&rhs) {
            new (&get_ptr_ref(lhs)) T *(get_ptr_ref(rhs));
            get_ptr_ref(rhs) = nullptr;
        }

        static void destroy_f(FunctorData &s) {
            T *&ptr = get_ptr_ref(s);
            if (!ptr) return;
            delete ptr;
            ptr = nullptr;
        }

        static T &get_ref(const FunctorData &s) {
            return *get_ptr_ref(s);
        }

        static T *&get_ptr_ref(FunctorData &s) {
            return (T *&)s;
        }

        static T *&get_ptr_ref(const FunctorData &s) {
            return (T *&)s;
        }
    };

    struct FunctionManager;

    struct FmStorage {
        FunctorData data;
        const FunctionManager *manager;
    };

    template<typename T>
    static const FunctionManager &get_default_fm();

    struct FunctionManager {
        template<typename T>
        inline static const FunctionManager create_default_manager() {
            return FunctionManager {
                &call_move_and_destroy<T>,
                &call_copy<T>,
                &call_destroy<T>
            };
        }

        void (* const call_move_and_destroyf)(FmStorage &lhs,
            FmStorage &&rhs);
        void (* const call_copyf)(FmStorage &lhs,
            const FmStorage &rhs);
        void (* const call_destroyf)(FmStorage &s);

        template<typename T>
        static void call_move_and_destroy(FmStorage &lhs,
        FmStorage &&rhs) {
            typedef FunctorDataManager<T> _spec;
            _spec::move_f(lhs.data, octa::move(rhs.data));
            _spec::destroy_f(rhs.data);
            lhs.manager = &get_default_fm<T>();
        }

        template<typename T>
        static void call_copy(FmStorage &lhs,
        const FmStorage &rhs) {
            typedef FunctorDataManager<T> _spec;
            lhs.manager = &get_default_fm<T>();
            _spec::store_f(lhs.data, _spec::get_ref(rhs.data));
        }

        template<typename T>
        static void call_destroy(FmStorage &s) {
            typedef FunctorDataManager<T> _spec;
            _spec::destroy_f(s.data);
        }
    };

    template<typename T>
    inline static const FunctionManager &get_default_fm() {
        static const FunctionManager def_manager
            = FunctionManager::create_default_manager<T>();
        return def_manager;
    }

    template<typename R, typename...>
    struct FunctionBase {
        typedef R Result;
    };

    template<typename R, typename T>
    struct FunctionBase<R, T> {
        typedef R Result;
        typedef T Argument;
    };

    template<typename R, typename T, typename U>
    struct FunctionBase<R, T, U> {
        typedef R Result;
        typedef T FirstArgument;
        typedef U SecondArgument;
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
} /* namespace detail */

template<typename R, typename ...A>
struct Function<R(A...)>: octa::detail::FunctionBase<R, A...> {
    Function(         ) { init_empty(); }
    Function(nullptr_t) { init_empty(); }

    Function(Function &&f) {
        init_empty();
        swap(f);
    }

    Function(const Function &f): p_call(f.p_call) {
        f.p_stor.manager->call_copyf(p_stor, f.p_stor);
    }

    template<typename T>
    Function(T f, EnableIf<
        octa::detail::IsValidFunctor<T, R(A...)>::value, bool
    > = true) {
        if (func_is_null(f)) {
            init_empty();
            return;
        }
        initialize(octa::detail::func_to_functor(octa::forward<T>(f)));
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
        f.p_stor.manager->call_copyf(p_stor, f.p_stor);
        return *this;
    };

    R operator()(A ...args) const {
        return p_call(p_stor.data, octa::forward<A>(args)...);
    }

    template<typename _F> void assign(_F &&f) {
        Function(octa::forward<_F>(f)).swap(*this);
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
    R (*p_call)(const octa::detail::FunctorData &, A...);

    template<typename T>
    void initialize(T f) {
        p_call = &octa::detail::FunctorDataManager<T>::template call<R, A...>;
        p_stor.manager = &octa::detail::get_default_fm<T>();
        octa::detail::FunctorDataManager<T>::store_f(p_stor.data,
            octa::forward<T>(f));
    }

    void init_empty() {
        typedef R(*emptyf)(A...);
        p_call = nullptr;
        p_stor.manager = &octa::detail::get_default_fm<emptyf>();
        octa::detail::FunctorDataManager<emptyf>::store_f(p_stor.data, nullptr);
    }

    template<typename T>
    static bool func_is_null(const T &) { return false; }

    static bool func_is_null(R (* const &fptr)(A...)) {
        return fptr == nullptr;
    }

    template<typename RR, typename T, typename ...AA>
    static bool func_is_null(RR (T::* const &fptr)(AA...)) {
        return fptr == nullptr;
    }

    template<typename RR, typename T, typename ...AA>
    static bool func_is_null(RR (T::* const &fptr)(AA...) const) {
        return fptr == nullptr;
    }
};

template<typename T>
bool operator==(nullptr_t, const Function<T> &rhs) { return !rhs; }

template<typename T>
bool operator==(const Function<T> &lhs, nullptr_t) { return !lhs; }

template<typename T>
bool operator!=(nullptr_t, const Function<T> &rhs) { return rhs; }

template<typename T>
bool operator!=(const Function<T> &lhs, nullptr_t) { return lhs; }

} /* namespace octa */

#endif