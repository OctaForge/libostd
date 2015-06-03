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
template<typename _T> struct _name { \
    _rettype operator()(const _T &x, const _T &y) const { \
        return x _op y; \
    } \
    typedef _T FirstArgument; \
    typedef _T SecondArgument; \
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
OCTA_DEFINE_BINARY_OP(Modulus, %, _T)
OCTA_DEFINE_BINARY_OP(Multiplies, *, _T)
OCTA_DEFINE_BINARY_OP(Divides, /, _T)
OCTA_DEFINE_BINARY_OP(Plus, +, _T)
OCTA_DEFINE_BINARY_OP(Minus, -, _T)
OCTA_DEFINE_BINARY_OP(BitAnd, &, _T)
OCTA_DEFINE_BINARY_OP(BitOr, |, _T)
OCTA_DEFINE_BINARY_OP(BitXor, ^, _T)

#undef OCTA_DEFINE_BINARY_OP

template<typename _T> struct LogicalNot {
    bool operator()(const _T &x) const { return !x; }
    typedef _T Argument;
    typedef bool Result;
};

template<typename _T> struct Negate {
    bool operator()(const _T &x) const { return -x; }
    typedef _T Argument;
    typedef _T Result;
};

template<typename _T> struct BinaryNegate {
    typedef typename _T::FirstArgument FirstArgument;
    typedef typename _T::SecondArgument SecondArgument;
    typedef bool Result;

    explicit BinaryNegate(const _T &f): p_fn(f) {}

    bool operator()(const FirstArgument &x,
                    const SecondArgument &y) {
        return !p_fn(x, y);
    }
private:
    _T p_fn;
};

template<typename _T> struct UnaryNegate {
    typedef typename _T::Argument Argument;
    typedef bool Result;

    explicit UnaryNegate(const _T &f): p_fn(f) {}
    bool operator()(const Argument &x) {
        return !p_fn(x);
    }
private:
    _T p_fn;
};

template<typename _T> UnaryNegate<_T> not1(const _T &fn) {
    return UnaryNegate<_T>(fn);
}

template<typename _T> BinaryNegate<_T> not2(const _T &fn) {
    return BinaryNegate<_T>(fn);
}

/* hash */

template<typename _T> struct Hash;

namespace detail {
    template<typename _T> struct HashBase {
        typedef _T Argument;
        typedef size_t Result;

        size_t operator()(_T v) const {
            return size_t(v);
        }
    };
}

#define OCTA_HASH_BASIC(_T) template<> struct Hash<_T>: octa::detail::HashBase<_T> {};

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

    template<typename _T, size_t = sizeof(_T) / sizeof(size_t)>
    struct ScalarHash;

    template<typename _T> struct ScalarHash<_T, 0> {
        typedef _T Argument;
        typedef size_t Result;

        size_t operator()(_T v) const {
            union { _T v; size_t h; } u;
            u.h = 0;
            u.v = v;
            return u.h;
        }
    };

    template<typename _T> struct ScalarHash<_T, 1> {
        typedef _T Argument;
        typedef size_t Result;

        size_t operator()(_T v) const {
            union { _T v; size_t h; } u;
            u.v = v;
            return u.h;
        }
    };

    template<typename _T> struct ScalarHash<_T, 2> {
        typedef _T Argument;
        typedef size_t Result;

        size_t operator()(_T v) const {
            union { _T v; struct { size_t h1, h2; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename _T> struct ScalarHash<_T, 3> {
        typedef _T Argument;
        typedef size_t Result;

        size_t operator()(_T v) const {
            union { _T v; struct { size_t h1, h2, h3; }; } u;
            u.v = v;
            return mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename _T> struct ScalarHash<_T, 4> {
        typedef _T Argument;
        typedef size_t Result;

        size_t operator()(_T v) const {
            union { _T v; struct { size_t h1, h2, h3, h4; }; } u;
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

template<typename _T> struct Hash<_T *> {
    typedef _T *Argument;
    typedef size_t Result;

    size_t operator()(_T *v) const {
        union { _T *v; size_t h; } u;
        u.v = v;
        return octa::detail::mem_hash((const void *)&u, sizeof(u));
    }
};

/* reference wrapper */

template<typename _T>
struct ReferenceWrapper {
    typedef _T type;

    ReferenceWrapper(_T &v): p_ptr(address_of(v)) {}
    ReferenceWrapper(const ReferenceWrapper &) = default;
    ReferenceWrapper(_T &&) = delete;

    ReferenceWrapper &operator=(const ReferenceWrapper &) = default;

    operator _T &() const { return *p_ptr; }
    _T &get() const { return *p_ptr; }

private:
    _T *p_ptr;
};

template<typename _T>
ReferenceWrapper<_T> ref(_T &v) {
    return ReferenceWrapper<_T>(v);
}
template<typename _T>
ReferenceWrapper<_T> ref(ReferenceWrapper<_T> v) {
    return ReferenceWrapper<_T>(v);
}
template<typename _T> void ref(const _T &&) = delete;

template<typename _T>
ReferenceWrapper<const _T> cref(const _T &v) {
    return ReferenceWrapper<_T>(v);
}
template<typename _T>
ReferenceWrapper<const _T> cref(ReferenceWrapper<_T> v) {
    return ReferenceWrapper<_T>(v);
}
template<typename _T> void cref(const _T &&) = delete;

/* mem_fn */

namespace detail {
    template<typename, typename> struct MemTypes;
    template<typename _T, typename _R, typename ..._A>
    struct MemTypes<_T, _R(_A...)> {
        typedef _R Result;
        typedef _T Argument;
    };
    template<typename _T, typename _R, typename _A>
    struct MemTypes<_T, _R(_A)> {
        typedef _R Result;
        typedef _T FirstArgument;
        typedef _A SecondArgument;
    };
    template<typename _T, typename _R, typename ..._A>
    struct MemTypes<_T, _R(_A...) const> {
        typedef _R Result;
        typedef const _T Argument;
    };
    template<typename _T, typename _R, typename _A>
    struct MemTypes<_T, _R(_A) const> {
        typedef _R Result;
        typedef const _T FirstArgument;
        typedef _A SecondArgument;
    };

    template<typename _R, typename _T>
    class MemFn: MemTypes<_T, _R> {
        _R _T::*p_ptr;
    public:
        MemFn(_R _T::*ptr): p_ptr(ptr) {}
        template<typename... _A>
        auto operator()(_T &obj, _A &&...args) ->
          decltype(((obj).*(p_ptr))(forward<_A>(args)...)) {
            return ((obj).*(p_ptr))(forward<_A>(args)...);
        }
        template<typename... _A>
        auto operator()(const _T &obj, _A &&...args) ->
          decltype(((obj).*(p_ptr))(forward<_A>(args)...)) const {
            return ((obj).*(p_ptr))(forward<_A>(args)...);
        }
        template<typename... _A>
        auto operator()(_T *obj, _A &&...args) ->
          decltype(((obj)->*(p_ptr))(forward<_A>(args)...)) {
            return ((obj)->*(p_ptr))(forward<_A>(args)...);
        }
        template<typename... _A>
        auto operator()(const _T *obj, _A &&...args) ->
          decltype(((obj)->*(p_ptr))(forward<_A>(args)...)) const {
            return ((obj)->*(p_ptr))(forward<_A>(args)...);
        }
    };
} /* namespace detail */

template<typename _R, typename _T>
octa::detail::MemFn<_R, _T> mem_fn(_R _T:: *ptr) {
    return octa::detail::MemFn<_R, _T>(ptr);
}

/* function impl
 * reference: http://probablydance.com/2013/01/13/a-faster-implementation-of-stdfunction
 */

template<typename> struct Function;

namespace detail {
    struct FunctorData {
        void *p1, *p2;
    };

    template<typename _T>
    struct FunctorInPlace {
        static constexpr bool value = sizeof(_T)  <= sizeof(FunctorData)
          && (alignof(FunctorData) % alignof(_T)) == 0
          && octa::IsMoveConstructible<_T>::value;
    };

    template<typename _T, typename _E = void>
    struct FunctorDataManager {
        template<typename _R, typename ..._A>
        static _R call(const FunctorData &s, _A ...args) {
            return ((_T &)s)(octa::forward<_A>(args)...);
        }

        static void store_f(FunctorData &s, _T v) {
            new (&get_ref(s)) _T(octa::forward<_T>(v));
        }

        static void move_f(FunctorData &lhs, FunctorData &&rhs) {
            new (&get_ref(lhs)) _T(octa::move(get_ref(rhs)));
        }

        static void destroy_f(FunctorData &s) {
            get_ref(s).~_T();
        }

        static _T &get_ref(const FunctorData &s) {
            return (_T &)s;
        }
    };

    template<typename _T>
    struct FunctorDataManager<_T,
        EnableIf<!FunctorInPlace<_T>::value>
    > {
        template<typename _R, typename ..._A>
        static _R call(const FunctorData &s, _A ...args) {
            return (*(_T *&)s)(octa::forward<_A>(args)...);
        }

        static void store_f(FunctorData &s, _T v) {
            new (&get_ptr_ref(s)) _T *(new _T(octa::forward<_T>(v)));
        }

        static void move_f(FunctorData &lhs, FunctorData &&rhs) {
            new (&get_ptr_ref(lhs)) _T *(get_ptr_ref(rhs));
            get_ptr_ref(rhs) = nullptr;
        }

        static void destroy_f(FunctorData &s) {
            _T *&ptr = get_ptr_ref(s);
            if (!ptr) return;
            delete ptr;
            ptr = nullptr;
        }

        static _T &get_ref(const FunctorData &s) {
            return *get_ptr_ref(s);
        }

        static _T *&get_ptr_ref(FunctorData &s) {
            return (_T *&)s;
        }

        static _T *&get_ptr_ref(const FunctorData &s) {
            return (_T *&)s;
        }
    };

    struct FunctionManager;

    struct FmStorage {
        FunctorData data;
        const FunctionManager *manager;
    };

    template<typename _T>
    static const FunctionManager &get_default_fm();

    struct FunctionManager {
        template<typename _T>
        inline static const FunctionManager create_default_manager() {
            return FunctionManager {
                &call_move_and_destroy<_T>,
                &call_copy<_T>,
                &call_destroy<_T>
            };
        }

        void (* const call_move_and_destroyf)(FmStorage &lhs,
            FmStorage &&rhs);
        void (* const call_copyf)(FmStorage &lhs,
            const FmStorage &rhs);
        void (* const call_destroyf)(FmStorage &s);

        template<typename _T>
        static void call_move_and_destroy(FmStorage &lhs,
        FmStorage &&rhs) {
            typedef FunctorDataManager<_T> _spec;
            _spec::move_f(lhs.data, octa::move(rhs.data));
            _spec::destroy_f(rhs.data);
            lhs.manager = &get_default_fm<_T>();
        }

        template<typename _T>
        static void call_copy(FmStorage &lhs,
        const FmStorage &rhs) {
            typedef FunctorDataManager<_T> _spec;
            lhs.manager = &get_default_fm<_T>();
            _spec::store_f(lhs.data, _spec::get_ref(rhs.data));
        }

        template<typename _T>
        static void call_destroy(FmStorage &s) {
            typedef FunctorDataManager<_T> _spec;
            _spec::destroy_f(s.data);
        }
    };

    template<typename _T>
    inline static const FunctionManager &get_default_fm() {
        static const FunctionManager def_manager
            = FunctionManager::create_default_manager<_T>();
        return def_manager;
    }

    template<typename _R, typename...>
    struct FunctionBase {
        typedef _R Result;
    };

    template<typename _R, typename _T>
    struct FunctionBase<_R, _T> {
        typedef _R Result;
        typedef _T Argument;
    };

    template<typename _R, typename _T, typename _U>
    struct FunctionBase<_R, _T, _U> {
        typedef _R Result;
        typedef _T FirstArgument;
        typedef _U SecondArgument;
    };

    template<typename, typename>
    struct IsValidFunctor {
        static constexpr bool value = false;
    };

    template<typename _R, typename ..._A>
    struct IsValidFunctor<Function<_R(_A...)>, _R(_A...)> {
        static constexpr bool value = false;
    };

    struct Empty {
    };

    template<typename _T>
    _T func_to_functor(_T &&f) {
        return octa::forward<_T>(f);
    }

    template<typename _RR, typename _T, typename ..._AA>
    auto func_to_functor(_RR (_T::*f)(_AA...))
        -> decltype(mem_fn(f)) {
        return mem_fn(f);
    }

    template<typename _RR, typename _T, typename ..._AA>
    auto func_to_functor(_RR (_T::*f)(_AA...) const)
        -> decltype(mem_fn(f)) {
        return mem_fn(f);
    }

    template<typename _T, typename _R, typename ..._A>
    struct IsValidFunctor<_T, _R(_A...)> {
        template<typename _U>
        static decltype(func_to_functor(octa::declval<_U>())
            (octa::declval<_A>()...)) test(_U *);
        template<typename>
        static Empty test(...);

        static constexpr bool value = octa::IsConvertible<
            decltype(test<_T>(nullptr)), _R
        >::value;
    };
} /* namespace detail */

template<typename _R, typename ..._A>
struct Function<_R(_A...)>: octa::detail::FunctionBase<_R, _A...> {
    Function(         ) { init_empty(); }
    Function(nullptr_t) { init_empty(); }

    Function(Function &&f) {
        init_empty();
        swap(f);
    }

    Function(const Function &f): p_call(f.p_call) {
        f.p_stor.manager->call_copyf(p_stor, f.p_stor);
    }

    template<typename _T>
    Function(_T f, EnableIf<
        octa::detail::IsValidFunctor<_T, _R(_A...)>::value,
        octa::detail::Empty
    > = octa::detail::Empty()) {
        if (func_is_null(f)) {
            init_empty();
            return;
        }
        initialize(octa::detail::func_to_functor(octa::forward<_T>(f)));
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

    _R operator()(_A ...args) const {
        return p_call(p_stor.data, octa::forward<_A>(args)...);
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
    _R (*p_call)(const octa::detail::FunctorData &, _A...);

    template<typename _T>
    void initialize(_T f) {
        p_call = &octa::detail::FunctorDataManager<_T>::template call<_R, _A...>;
        p_stor.manager = &octa::detail::get_default_fm<_T>();
        octa::detail::FunctorDataManager<_T>::store_f(p_stor.data,
            octa::forward<_T>(f));
    }

    void init_empty() {
        typedef _R(*emptyf)(_A...);
        p_call = nullptr;
        p_stor.manager = &octa::detail::get_default_fm<emptyf>();
        octa::detail::FunctorDataManager<emptyf>::store_f(p_stor.data, nullptr);
    }

    template<typename _T>
    static bool func_is_null(const _T &) { return false; }

    static bool func_is_null(_R (* const &fptr)(_A...)) {
        return fptr == nullptr;
    }

    template<typename _RR, typename _T, typename ..._AA>
    static bool func_is_null(_RR (_T::* const &fptr)(_AA...)) {
        return fptr == nullptr;
    }

    template<typename _RR, typename _T, typename ..._AA>
    static bool func_is_null(_RR (_T::* const &fptr)(_AA...) const) {
        return fptr == nullptr;
    }
};

template<typename _T>
bool operator==(nullptr_t, const Function<_T> &rhs) { return !rhs; }

template<typename _T>
bool operator==(const Function<_T> &lhs, nullptr_t) { return !lhs; }

template<typename _T>
bool operator!=(nullptr_t, const Function<_T> &rhs) { return rhs; }

template<typename _T>
bool operator!=(const Function<_T> &lhs, nullptr_t) { return lhs; }

} /* namespace octa */

#endif