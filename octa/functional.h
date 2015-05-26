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

#define __OCTA_DEFINE_BINARY_OP(name, op, rettype) \
    template<typename T> struct name { \
        rettype operator()(const T &x, const T &y) const { return x op y; } \
        typedef T FirstArgType; \
        typedef T SecondArgType; \
        typedef rettype ResultType; \
    };

    __OCTA_DEFINE_BINARY_OP(Less, <, bool)
    __OCTA_DEFINE_BINARY_OP(LessEqual, <=, bool)
    __OCTA_DEFINE_BINARY_OP(Greater, >, bool)
    __OCTA_DEFINE_BINARY_OP(GreaterEqual, >=, bool)
    __OCTA_DEFINE_BINARY_OP(Equal, ==, bool)
    __OCTA_DEFINE_BINARY_OP(NotEqual, !=, bool)
    __OCTA_DEFINE_BINARY_OP(LogicalAnd, &&, bool)
    __OCTA_DEFINE_BINARY_OP(LogicalOr, ||, bool)
    __OCTA_DEFINE_BINARY_OP(Modulus, %, T)
    __OCTA_DEFINE_BINARY_OP(Multiplies, *, T)
    __OCTA_DEFINE_BINARY_OP(Divides, /, T)
    __OCTA_DEFINE_BINARY_OP(Plus, +, T)
    __OCTA_DEFINE_BINARY_OP(Minus, -, T)
    __OCTA_DEFINE_BINARY_OP(BitAnd, &, T)
    __OCTA_DEFINE_BINARY_OP(BitOr, |, T)
    __OCTA_DEFINE_BINARY_OP(BitXor, ^, T)

#undef __OCTA_DEFINE_BINARY_OP

    template<typename T> struct LogicalNot {
        bool operator()(const T &x) const { return !x; }
        typedef T ArgType;
        typedef bool ResultType;
    };

    template<typename T> struct Negate {
        bool operator()(const T &x) const { return -x; }
        typedef T ArgType;
        typedef T ResultType;
    };

    template<typename T> struct BinaryNegate {
        typedef typename T::FirstArgType FirstArgType;
        typedef typename T::SecondArgType SecondArgType;
        typedef bool ResultType;

        explicit BinaryNegate(const T &f): p_fn(f) {}

        bool operator()(const FirstArgType &x,
                        const SecondArgType &y) {
            return !p_fn(x, y);
        }
    private:
        T p_fn;
    };

    template<typename T> struct UnaryNegate {
        typedef typename T::ArgType ArgType;
        typedef bool ResultType;

        explicit UnaryNegate(const T &f): p_fn(f) {}
        bool operator()(const ArgType &x) {
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

    template<typename T> struct __OctaHashBase {
        typedef T ArgType;
        typedef size_t ResultType;

        size_t operator()(T v) const {
            return (size_t)v;
        }
    };

#define __OCTA_HASH_BASIC(T) template<> struct Hash<T>: __OctaHashBase<T> {};

    __OCTA_HASH_BASIC(bool)
    __OCTA_HASH_BASIC(char)
    __OCTA_HASH_BASIC(schar)
    __OCTA_HASH_BASIC(uchar)
    __OCTA_HASH_BASIC(char16_t)
    __OCTA_HASH_BASIC(char32_t)
    __OCTA_HASH_BASIC(wchar_t)
    __OCTA_HASH_BASIC(short)
    __OCTA_HASH_BASIC(ushort)
    __OCTA_HASH_BASIC(int)
    __OCTA_HASH_BASIC(uint)
    __OCTA_HASH_BASIC(long)
    __OCTA_HASH_BASIC(ulong)

#undef __OCTA_HASH_BASIC

    static inline size_t __octa_mem_hash(const void *p, size_t l) {
        const uchar *d = (const uchar *)p;
        size_t h = 5381;
        for (size_t i = 0; i < l; ++i) h = ((h << 5) + h) ^ d[i];
        return h;
    }

    template<typename T, size_t = sizeof(T) / sizeof(size_t)>
    struct __OctaScalarHash;

    template<typename T> struct __OctaScalarHash<T, 0> {
        typedef T ArgType;
        typedef size_t ResultType;

        size_t operator()(T v) const {
            union { T v; size_t h; } u;
            u.h = 0;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct __OctaScalarHash<T, 1> {
        typedef T ArgType;
        typedef size_t ResultType;

        size_t operator()(T v) const {
            union { T v; size_t h; } u;
            u.v = v;
            return u.h;
        }
    };

    template<typename T> struct __OctaScalarHash<T, 2> {
        typedef T ArgType;
        typedef size_t ResultType;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2; }; } u;
            u.v = v;
            return __octa_mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename T> struct __OctaScalarHash<T, 3> {
        typedef T ArgType;
        typedef size_t ResultType;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2, h3; }; } u;
            u.v = v;
            return __octa_mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<typename T> struct __OctaScalarHash<T, 4> {
        typedef T ArgType;
        typedef size_t ResultType;

        size_t operator()(T v) const {
            union { T v; struct { size_t h1, h2, h3, h4; }; } u;
            u.v = v;
            return __octa_mem_hash((const void *)&u, sizeof(u));
        }
    };

    template<> struct Hash<llong>: __OctaScalarHash<llong> {};
    template<> struct Hash<ullong>: __OctaScalarHash<ullong> {};

    template<> struct Hash<float>: __OctaScalarHash<float> {
        size_t operator()(float v) const {
            if (v == 0) return 0;
            return __OctaScalarHash<float>::operator()(v);
        }
    };

    template<> struct Hash<double>: __OctaScalarHash<double> {
        size_t operator()(double v) const {
            if (v == 0) return 0;
            return __OctaScalarHash<double>::operator()(v);
        }
    };

    template<> struct Hash<ldouble>: __OctaScalarHash<ldouble> {
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
            return __OctaScalarHash<ldouble>::operator()(v);
#endif
#endif
        }
    };

    template<typename T> struct Hash<T *> {
        typedef T *ArgType;
        typedef size_t ResultType;

        size_t operator()(T *v) const {
            union { T *v; size_t h; } u;
            u.v = v;
            return __octa_mem_hash((const void *)&u, sizeof(u));
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

    template<typename, typename> struct __OctaMemTypes;
    template<typename T, typename R, typename ...A>
    struct __OctaMemTypes<T, R(A...)> {
        typedef R ResultType;
        typedef T ArgType;
    };
    template<typename T, typename R, typename A>
    struct __OctaMemTypes<T, R(A)> {
        typedef R ResultType;
        typedef T FirstArgType;
        typedef A SecondArgType;
    };
    template<typename T, typename R, typename ...A>
    struct __OctaMemTypes<T, R(A...) const> {
        typedef R ResultType;
        typedef const T ArgType;
    };
    template<typename T, typename R, typename A>
    struct __OctaMemTypes<T, R(A) const> {
        typedef R ResultType;
        typedef const T FirstArgType;
        typedef A SecondArgType;
    };

    template<typename R, typename T>
    class __OctaMemFn: __OctaMemTypes<T, R> {
        R T::*p_ptr;
    public:
        __OctaMemFn(R T::*ptr): p_ptr(ptr) {}
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

    template<typename R, typename T>
    __OctaMemFn<R, T> mem_fn(R T:: *ptr) {
        return __OctaMemFn<R, T>(ptr);
    }

    /* function impl
     * reference: http://probablydance.com/2013/01/13/a-faster-implementation-of-stdfunction
     */

    template<typename> struct Function;

    struct __OctaFunctorData {
        void *p1, *p2;
    };

    template<typename T>
    struct __OctaFunctorInPlace {
        static constexpr bool value = sizeof(T)  <= sizeof(__OctaFunctorData)
          && (alignof(__OctaFunctorData) % alignof(T)) == 0
          && octa::IsMoveConstructible<T>::value;
    };

    template<typename T, typename E = void>
    struct __OctaFunctorDataManager {
        template<typename R, typename ...A>
        static R call(const __OctaFunctorData &s, A ...args) {
            return ((T &)s)(forward<A>(args)...);
        }

        static void store_f(__OctaFunctorData &s, T v) {
            new (&get_ref(s)) T(forward<T>(v));
        }

        static void move_f(__OctaFunctorData &lhs, __OctaFunctorData &&rhs) {
            new (&get_ref(lhs)) T(move(get_ref(rhs)));
        }

        static void destroy_f(__OctaFunctorData &s) {
            get_ref(s).~T();
        }

        static T &get_ref(const __OctaFunctorData &s) {
            return (T &)s;
        }
    };

    template<typename T>
    struct __OctaFunctorDataManager<T, EnableIf<!__OctaFunctorInPlace<T>::value>> {
        template<typename R, typename ...A>
        static R call(const __OctaFunctorData &s, A ...args) {
            return (*(T *&)s)(forward<A>(args)...);
        }

        static void store_f(__OctaFunctorData &s, T v) {
            new (&get_ptr_ref(s)) T *(new T(forward<T>(v)));
        }

        static void move_f(__OctaFunctorData &lhs, __OctaFunctorData &&rhs) {
            new (&get_ptr_ref(lhs)) T *(get_ptr_ref(rhs));
            get_ptr_ref(rhs) = nullptr;
        }

        static void destroy_f(__OctaFunctorData &s) {
            T *&ptr = get_ptr_ref(s);
            if (!ptr) return;
            delete ptr;
            ptr = nullptr;
        }

        static T &get_ref(const __OctaFunctorData &s) {
            return *get_ptr_ref(s);
        }

        static T *&get_ptr_ref(__OctaFunctorData &s) {
            return (T *&)s;
        }

        static T *&get_ptr_ref(const __OctaFunctorData &s) {
            return (T *&)s;
        }
    };

    struct __OctaFunctionManager;

    struct __OctaFmStorage {
        __OctaFunctorData data;
        const __OctaFunctionManager *manager;
    };

    template<typename T>
    static const __OctaFunctionManager &__octa_get_default_fm();

    struct __OctaFunctionManager {
        template<typename T>
        inline static const __OctaFunctionManager create_default_manager() {
            return __OctaFunctionManager {
                &t_call_move_and_destroy<T>,
                &t_call_copy<T>,
                &t_call_destroy<T>
            };
        }

        void (* const call_move_and_destroy)(__OctaFmStorage &lhs,
            __OctaFmStorage &&rhs);
        void (* const call_copy)(__OctaFmStorage &lhs,
            const __OctaFmStorage &rhs);
        void (* const call_destroy)(__OctaFmStorage &s);

        template<typename T>
        static void t_call_move_and_destroy(__OctaFmStorage &lhs,
        __OctaFmStorage &&rhs) {
            typedef __OctaFunctorDataManager<T> spec;
            spec::move_f(lhs.data, move(rhs.data));
            spec::destroy_f(rhs.data);
            lhs.manager = &__octa_get_default_fm<T>();
        }

        template<typename T>
        static void t_call_copy(__OctaFmStorage &lhs,
        const __OctaFmStorage &rhs) {
            typedef __OctaFunctorDataManager<T> spec;
            lhs.manager = &__octa_get_default_fm<T>();
            spec::store_f(lhs.data, spec::get_ref(rhs.data));
        }

        template<typename T>
        static void t_call_destroy(__OctaFmStorage &s) {
            typedef __OctaFunctorDataManager<T> spec;
            spec::destroy_f(s.data);
        }
    };

    template<typename T>
    inline static const __OctaFunctionManager &__octa_get_default_fm() {
        static const __OctaFunctionManager def_manager
            = __OctaFunctionManager::create_default_manager<T>();
        return def_manager;
    }

    template<typename R, typename...>
    struct __OctaFunction {
        typedef R ResultType;
    };

    template<typename R, typename T>
    struct __OctaFunction<R, T> {
        typedef R ResultType;
        typedef T ArgType;
    };

    template<typename R, typename T, typename U>
    struct __OctaFunction<R, T, U> {
        typedef R ResultType;
        typedef T FirstArgType;
        typedef U SecondArgType;
    };

    template<typename, typename>
    struct IsValidFunctor {
        static constexpr bool value = false;
    };

    template<typename R, typename ...A>
    struct IsValidFunctor<Function<R(A...)>, R(A...)> {
        static constexpr bool value = false;
    };

    struct __OctaEmpty {
    };

    template<typename T>
    T func_to_functor(T &&f) {
        return forward<T>(f);
    }

    template<typename RR, typename T, typename ...AA>
    auto func_to_functor(RR (T::*f)(AA...)) -> decltype(mem_fn(f)) {
        return mem_fn(f);
    }

    template<typename RR, typename T, typename ...AA>
    auto func_to_functor(RR (T::*f)(AA...) const) -> decltype(mem_fn(f)) {
        return mem_fn(f);
    }

    template<typename T, typename R, typename ...A>
    struct IsValidFunctor<T, R(A...)> {
        template<typename U>
        static decltype(func_to_functor(declval<U>())(declval<A>()...)) __octa_test(U *);
        template<typename>
        static __OctaEmpty __octa_test(...);

        static constexpr bool value = IsConvertible<
            decltype(__octa_test<T>(nullptr)), R
        >::value;
    };

    template<typename R, typename ...A>
    struct Function<R(A...)>: __OctaFunction<R, A...> {
        Function(         ) { initialize_empty(); }
        Function(nullptr_t) { initialize_empty(); }

        Function(Function &&f) {
            initialize_empty();
            swap(f);
        }

        Function(const Function &f): p_call(f.p_call) {
            f.p_stor.manager->call_copy(p_stor, f.p_stor);
        }

        template<typename T>
        Function(T f, EnableIf<IsValidFunctor<T, R(A...)>::value, __OctaEmpty>
                          = __OctaEmpty()) {
            if (func_is_null(f)) {
                initialize_empty();
                return;
            }
            initialize(func_to_functor(forward<T>(f)));
        }

        ~Function() {
            p_stor.manager->call_destroy(p_stor);
        }

        Function &operator=(Function &&f) {
            p_stor.manager->call_destroy(p_stor);
            swap(f);
            return *this;
        }

        Function &operator=(const Function &f) {
            p_stor.manager->call_destroy(p_stor);
            f.p_stor.manager->call_copy(p_stor, f.p_stor);
            return *this;
        };

        R operator()(A ...args) const {
            return p_call(p_stor.data, forward<A>(args)...);
        }

        template<typename F> void assign(F &&f) {
            Function(forward<F>(f)).swap(*this);
        }

        void swap(Function &f) {
            __OctaFmStorage tmp;
            f.p_stor.manager->call_move_and_destroy(tmp, move(f.p_stor));
            p_stor.manager->call_move_and_destroy(f.p_stor, move(p_stor));
            tmp.manager->call_move_and_destroy(p_stor, move(tmp));
            octa::swap(p_call, f.p_call);
        }

        operator bool() const { return p_call != nullptr; }

    private:
        __OctaFmStorage p_stor;
        R (*p_call)(const __OctaFunctorData &, A...);

        template<typename T>
        void initialize(T f) {
            p_call = &__OctaFunctorDataManager<T>::template call<R, A...>;
            p_stor.manager = &__octa_get_default_fm<T>();
            __OctaFunctorDataManager<T>::store_f(p_stor.data, forward<T>(f));
        }

        void initialize_empty() {
            typedef R(*emptyf)(A...);
            p_call = nullptr;
            p_stor.manager = &__octa_get_default_fm<emptyf>();
            __OctaFunctorDataManager<emptyf>::store_f(p_stor.data, nullptr);
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
}

#endif