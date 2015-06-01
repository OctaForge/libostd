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

#define __OCTA_DEFINE_BINARY_OP(_name, _op, _rettype) \
    template<typename _T> struct _name { \
        _rettype operator()(const _T &__x, const _T &__y) const { \
            return __x _op __y; \
        } \
        typedef _T FirstArgType; \
        typedef _T SecondArgType; \
        typedef _rettype ResultType; \
    };

    __OCTA_DEFINE_BINARY_OP(Less, <, bool)
    __OCTA_DEFINE_BINARY_OP(LessEqual, <=, bool)
    __OCTA_DEFINE_BINARY_OP(Greater, >, bool)
    __OCTA_DEFINE_BINARY_OP(GreaterEqual, >=, bool)
    __OCTA_DEFINE_BINARY_OP(Equal, ==, bool)
    __OCTA_DEFINE_BINARY_OP(NotEqual, !=, bool)
    __OCTA_DEFINE_BINARY_OP(LogicalAnd, &&, bool)
    __OCTA_DEFINE_BINARY_OP(LogicalOr, ||, bool)
    __OCTA_DEFINE_BINARY_OP(Modulus, %, _T)
    __OCTA_DEFINE_BINARY_OP(Multiplies, *, _T)
    __OCTA_DEFINE_BINARY_OP(Divides, /, _T)
    __OCTA_DEFINE_BINARY_OP(Plus, +, _T)
    __OCTA_DEFINE_BINARY_OP(Minus, -, _T)
    __OCTA_DEFINE_BINARY_OP(BitAnd, &, _T)
    __OCTA_DEFINE_BINARY_OP(BitOr, |, _T)
    __OCTA_DEFINE_BINARY_OP(BitXor, ^, _T)

#undef __OCTA_DEFINE_BINARY_OP

    template<typename _T> struct LogicalNot {
        bool operator()(const _T &__x) const { return !__x; }
        typedef _T ArgType;
        typedef bool ResultType;
    };

    template<typename _T> struct Negate {
        bool operator()(const _T &__x) const { return -__x; }
        typedef _T ArgType;
        typedef _T ResultType;
    };

    template<typename _T> struct BinaryNegate {
        typedef typename _T::FirstArgType FirstArgType;
        typedef typename _T::SecondArgType SecondArgType;
        typedef bool ResultType;

        explicit BinaryNegate(const _T &__f): __fn(__f) {}

        bool operator()(const FirstArgType &__x,
                        const SecondArgType &__y) {
            return !__fn(__x, __y);
        }
    private:
        _T __fn;
    };

    template<typename _T> struct UnaryNegate {
        typedef typename _T::ArgType ArgType;
        typedef bool ResultType;

        explicit UnaryNegate(const _T &__f): __fn(__f) {}
        bool operator()(const ArgType &__x) {
            return !__fn(__x);
        }
    private:
        _T __fn;
    };

    template<typename _T> UnaryNegate<_T> not1(const _T &__fn) {
        return UnaryNegate<_T>(__fn);
    }

    template<typename _T> BinaryNegate<_T> not2(const _T &__fn) {
        return BinaryNegate<_T>(__fn);
    }

    /* hash */

    template<typename _T> struct Hash;

    template<typename _T> struct __OctaHashBase {
        typedef _T ArgType;
        typedef size_t ResultType;

        size_t operator()(_T __v) const {
            return size_t(__v);
        }
    };

#define __OCTA_HASH_BASIC(_T) template<> struct Hash<_T>: __OctaHashBase<_T> {};

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

    static inline size_t __octa_mem_hash(const void *__p, size_t __l) {
        const uchar *__d = (const uchar *)__p;
        size_t __h = 5381;
        for (size_t __i = 0; __i < __l; ++__i) __h = ((__h << 5) + __h) ^ __d[__i];
        return __h;
    }

    template<typename _T, size_t = sizeof(_T) / sizeof(size_t)>
    struct __OctaScalarHash;

    template<typename _T> struct __OctaScalarHash<_T, 0> {
        typedef _T ArgType;
        typedef size_t ResultType;

        size_t operator()(_T __v) const {
            union { _T __v; size_t __h; } __u;
            __u.__h = 0;
            __u.__v = __v;
            return __u.__h;
        }
    };

    template<typename _T> struct __OctaScalarHash<_T, 1> {
        typedef _T ArgType;
        typedef size_t ResultType;

        size_t operator()(_T __v) const {
            union { _T __v; size_t __h; } __u;
            __u.__v = __v;
            return __u.__h;
        }
    };

    template<typename _T> struct __OctaScalarHash<_T, 2> {
        typedef _T ArgType;
        typedef size_t ResultType;

        size_t operator()(_T __v) const {
            union { _T __v; struct { size_t __h1, __h2; }; } __u;
            __u.__v = __v;
            return __octa_mem_hash((const void *)&__u, sizeof(__u));
        }
    };

    template<typename _T> struct __OctaScalarHash<_T, 3> {
        typedef _T ArgType;
        typedef size_t ResultType;

        size_t operator()(_T __v) const {
            union { _T __v; struct { size_t __h1, __h2, __h3; }; } __u;
            __u.__v = __v;
            return __octa_mem_hash((const void *)&__u, sizeof(__u));
        }
    };

    template<typename _T> struct __OctaScalarHash<_T, 4> {
        typedef _T ArgType;
        typedef size_t ResultType;

        size_t operator()(_T __v) const {
            union { _T __v; struct { size_t __h1, __h2, __h3, __h4; }; } __u;
            __u.__v = __v;
            return __octa_mem_hash((const void *)&__u, sizeof(__u));
        }
    };

    template<> struct Hash<llong>: __OctaScalarHash<llong> {};
    template<> struct Hash<ullong>: __OctaScalarHash<ullong> {};

    template<> struct Hash<float>: __OctaScalarHash<float> {
        size_t operator()(float __v) const {
            if (__v == 0) return 0;
            return __OctaScalarHash<float>::operator()(__v);
        }
    };

    template<> struct Hash<double>: __OctaScalarHash<double> {
        size_t operator()(double __v) const {
            if (__v == 0) return 0;
            return __OctaScalarHash<double>::operator()(__v);
        }
    };

    template<> struct Hash<ldouble>: __OctaScalarHash<ldouble> {
        size_t operator()(ldouble __v) const {
            if (__v == 0) return 0;
#ifdef __i386__
            union { ldouble __v; struct { size_t __h1, __h2, __h3, __h4; }; } __u;
            __u.__h1 = __u.__h2 = __u.__h3 = __u.__h4 = 0;
            __u.__v = __v;
            return (__u.__h1 ^ __u.__h2 ^ __u.__h3 ^ __u.__h4);
#else
#ifdef __x86_64__
            union { ldouble __v; struct { size_t __h1, __h2; }; } __u;
            __u.__h1 = __u.__h2 = 0;
            __u.__v = __v;
            return (__u.__h1 ^ __u.__h2);
#else
            return __OctaScalarHash<ldouble>::operator()(__v);
#endif
#endif
        }
    };

    template<typename _T> struct Hash<_T *> {
        typedef _T *ArgType;
        typedef size_t ResultType;

        size_t operator()(_T *__v) const {
            union { _T *__v; size_t __h; } __u;
            __u.__v = __v;
            return __octa_mem_hash((const void *)&__u, sizeof(__u));
        }
    };

    /* reference wrapper */

    template<typename _T>
    struct ReferenceWrapper {
        typedef _T type;

        ReferenceWrapper(_T &__v): __ptr(address_of(__v)) {}
        ReferenceWrapper(const ReferenceWrapper &) = default;
        ReferenceWrapper(_T &&) = delete;

        ReferenceWrapper &operator=(const ReferenceWrapper &) = default;

        operator _T &() const { return *__ptr; }
        _T &get() const { return *__ptr; }

    private:
        _T *__ptr;
    };

    template<typename _T>
    ReferenceWrapper<_T> ref(_T &__v) {
        return ReferenceWrapper<_T>(__v);
    }
    template<typename _T>
    ReferenceWrapper<_T> ref(ReferenceWrapper<_T> __v) {
        return ReferenceWrapper<_T>(__v);
    }
    template<typename _T> void ref(const _T &&) = delete;

    template<typename _T>
    ReferenceWrapper<const _T> cref(const _T &__v) {
        return ReferenceWrapper<_T>(__v);
    }
    template<typename _T>
    ReferenceWrapper<const _T> cref(ReferenceWrapper<_T> __v) {
        return ReferenceWrapper<_T>(__v);
    }
    template<typename _T> void cref(const _T &&) = delete;

    /* mem_fn */

    template<typename, typename> struct __OctaMemTypes;
    template<typename _T, typename _R, typename ..._A>
    struct __OctaMemTypes<_T, _R(_A...)> {
        typedef _R ResultType;
        typedef _T ArgType;
    };
    template<typename _T, typename _R, typename _A>
    struct __OctaMemTypes<_T, _R(_A)> {
        typedef _R ResultType;
        typedef _T FirstArgType;
        typedef _A SecondArgType;
    };
    template<typename _T, typename _R, typename ..._A>
    struct __OctaMemTypes<_T, _R(_A...) const> {
        typedef _R ResultType;
        typedef const _T ArgType;
    };
    template<typename _T, typename _R, typename _A>
    struct __OctaMemTypes<_T, _R(_A) const> {
        typedef _R ResultType;
        typedef const _T FirstArgType;
        typedef _A SecondArgType;
    };

    template<typename _R, typename _T>
    class __OctaMemFn: __OctaMemTypes<_T, _R> {
        _R _T::*__ptr;
    public:
        __OctaMemFn(_R _T::*__ptr): __ptr(__ptr) {}
        template<typename... _A>
        auto operator()(_T &__obj, _A &&...__args) ->
          decltype(((__obj).*(__ptr))(forward<_A>(__args)...)) {
            return ((__obj).*(__ptr))(forward<_A>(__args)...);
        }
        template<typename... _A>
        auto operator()(const _T &__obj, _A &&...__args) ->
          decltype(((__obj).*(__ptr))(forward<_A>(__args)...)) const {
            return ((__obj).*(__ptr))(forward<_A>(__args)...);
        }
        template<typename... _A>
        auto operator()(_T *__obj, _A &&...__args) ->
          decltype(((__obj)->*(__ptr))(forward<_A>(__args)...)) {
            return ((__obj)->*(__ptr))(forward<_A>(__args)...);
        }
        template<typename... _A>
        auto operator()(const _T *__obj, _A &&...__args) ->
          decltype(((__obj)->*(__ptr))(forward<_A>(__args)...)) const {
            return ((__obj)->*(__ptr))(forward<_A>(__args)...);
        }
    };

    template<typename _R, typename _T>
    __OctaMemFn<_R, _T> mem_fn(_R _T:: *__ptr) {
        return __OctaMemFn<_R, _T>(__ptr);
    }

    /* function impl
     * reference: http://probablydance.com/2013/01/13/a-faster-implementation-of-stdfunction
     */

    template<typename> struct Function;

    struct __OctaFunctorData {
        void *__p1, *__p2;
    };

    template<typename _T>
    struct __OctaFunctorInPlace {
        static constexpr bool value = sizeof(_T)  <= sizeof(__OctaFunctorData)
          && (alignof(__OctaFunctorData) % alignof(_T)) == 0
          && octa::IsMoveConstructible<_T>::value;
    };

    template<typename _T, typename _E = void>
    struct __OctaFunctorDataManager {
        template<typename _R, typename ..._A>
        static _R __call(const __OctaFunctorData &__s, _A ...__args) {
            return ((_T &)__s)(octa::forward<_A>(__args)...);
        }

        static void __store_f(__OctaFunctorData &__s, _T __v) {
            new (&__get_ref(__s)) _T(octa::forward<_T>(__v));
        }

        static void __move_f(__OctaFunctorData &__lhs, __OctaFunctorData &&__rhs) {
            new (&__get_ref(__lhs)) _T(octa::move(__get_ref(__rhs)));
        }

        static void __destroy_f(__OctaFunctorData &__s) {
            __get_ref(__s).~_T();
        }

        static _T &__get_ref(const __OctaFunctorData &__s) {
            return (_T &)__s;
        }
    };

    template<typename _T>
    struct __OctaFunctorDataManager<_T, EnableIf<!__OctaFunctorInPlace<_T>::value>> {
        template<typename _R, typename ..._A>
        static _R __call(const __OctaFunctorData &__s, _A ...__args) {
            return (*(_T *&)__s)(octa::forward<_A>(__args)...);
        }

        static void __store_f(__OctaFunctorData &__s, _T __v) {
            new (&__get_ptr_ref(__s)) _T *(new _T(octa::forward<_T>(__v)));
        }

        static void __move_f(__OctaFunctorData &__lhs, __OctaFunctorData &&__rhs) {
            new (&__get_ptr_ref(__lhs)) _T *(__get_ptr_ref(__rhs));
            __get_ptr_ref(__rhs) = nullptr;
        }

        static void __destroy_f(__OctaFunctorData &__s) {
            _T *&__ptr = __get_ptr_ref(__s);
            if (!__ptr) return;
            delete __ptr;
            __ptr = nullptr;
        }

        static _T &__get_ref(const __OctaFunctorData &__s) {
            return *__get_ptr_ref(__s);
        }

        static _T *&__get_ptr_ref(__OctaFunctorData &__s) {
            return (_T *&)__s;
        }

        static _T *&__get_ptr_ref(const __OctaFunctorData &__s) {
            return (_T *&)__s;
        }
    };

    struct __OctaFunctionManager;

    struct __OctaFmStorage {
        __OctaFunctorData __data;
        const __OctaFunctionManager *__manager;
    };

    template<typename _T>
    static const __OctaFunctionManager &__octa_get_default_fm();

    struct __OctaFunctionManager {
        template<typename _T>
        inline static const __OctaFunctionManager __create_default_manager() {
            return __OctaFunctionManager {
                &__call_move_and_destroy<_T>,
                &__call_copy<_T>,
                &__call_destroy<_T>
            };
        }

        void (* const __call_move_and_destroyf)(__OctaFmStorage &__lhs,
            __OctaFmStorage &&__rhs);
        void (* const __call_copyf)(__OctaFmStorage &__lhs,
            const __OctaFmStorage &__rhs);
        void (* const __call_destroyf)(__OctaFmStorage &__s);

        template<typename _T>
        static void __call_move_and_destroy(__OctaFmStorage &__lhs,
        __OctaFmStorage &&__rhs) {
            typedef __OctaFunctorDataManager<_T> _spec;
            _spec::__move_f(__lhs.__data, octa::move(__rhs.__data));
            _spec::__destroy_f(__rhs.__data);
            __lhs.__manager = &__octa_get_default_fm<_T>();
        }

        template<typename _T>
        static void __call_copy(__OctaFmStorage &__lhs,
        const __OctaFmStorage &__rhs) {
            typedef __OctaFunctorDataManager<_T> _spec;
            __lhs.__manager = &__octa_get_default_fm<_T>();
            _spec::__store_f(__lhs.__data, _spec::__get_ref(__rhs.__data));
        }

        template<typename _T>
        static void __call_destroy(__OctaFmStorage &__s) {
            typedef __OctaFunctorDataManager<_T> _spec;
            _spec::__destroy_f(__s.__data);
        }
    };

    template<typename _T>
    inline static const __OctaFunctionManager &__octa_get_default_fm() {
        static const __OctaFunctionManager __def_manager
            = __OctaFunctionManager::__create_default_manager<_T>();
        return __def_manager;
    }

    template<typename _R, typename...>
    struct __OctaFunction {
        typedef _R ResultType;
    };

    template<typename _R, typename _T>
    struct __OctaFunction<_R, _T> {
        typedef _R ResultType;
        typedef _T ArgType;
    };

    template<typename _R, typename _T, typename _U>
    struct __OctaFunction<_R, _T, _U> {
        typedef _R ResultType;
        typedef _T FirstArgType;
        typedef _U SecondArgType;
    };

    template<typename, typename>
    struct __OctaIsValidFunctor {
        static constexpr bool value = false;
    };

    template<typename _R, typename ..._A>
    struct __OctaIsValidFunctor<Function<_R(_A...)>, _R(_A...)> {
        static constexpr bool value = false;
    };

    struct __OctaEmpty {
    };

    template<typename _T>
    _T __octa_func_to_functor(_T &&__f) {
        return octa::forward<_T>(__f);
    }

    template<typename _RR, typename _T, typename ..._AA>
    auto __octa_func_to_functor(_RR (_T::*__f)(_AA...))
        -> decltype(mem_fn(__f)) {
        return mem_fn(__f);
    }

    template<typename _RR, typename _T, typename ..._AA>
    auto __octa_func_to_functor(_RR (_T::*__f)(_AA...) const)
        -> decltype(mem_fn(__f)) {
        return mem_fn(__f);
    }

    template<typename _T, typename _R, typename ..._A>
    struct __OctaIsValidFunctor<_T, _R(_A...)> {
        template<typename _U>
        static decltype(__octa_func_to_functor(octa::declval<_U>())
            (octa::declval<_A>()...)) __test(_U *);
        template<typename>
        static __OctaEmpty __test(...);

        static constexpr bool value = octa::IsConvertible<
            decltype(__test<_T>(nullptr)), _R
        >::value;
    };

    template<typename _R, typename ..._A>
    struct Function<_R(_A...)>: __OctaFunction<_R, _A...> {
        Function(         ) { __init_empty(); }
        Function(nullptr_t) { __init_empty(); }

        Function(Function &&__f) {
            __init_empty();
            swap(__f);
        }

        Function(const Function &__f): __call(__f.__call) {
            __f.__stor.__manager->__call_copyf(__stor, __f.__stor);
        }

        template<typename _T>
        Function(_T __f, EnableIf<__OctaIsValidFunctor<_T, _R(_A...)>::value, __OctaEmpty>
                          = __OctaEmpty()) {
            if (__func_is_null(__f)) {
                __init_empty();
                return;
            }
            __initialize(__octa_func_to_functor(octa::forward<_T>(__f)));
        }

        ~Function() {
            __stor.__manager->__call_destroyf(__stor);
        }

        Function &operator=(Function &&__f) {
            __stor.__manager->__call_destroyf(__stor);
            swap(__f);
            return *this;
        }

        Function &operator=(const Function &__f) {
            __stor.__manager->__call_destroyf(__stor);
            __f.__stor.__manager->__call_copyf(__stor, __f.__stor);
            return *this;
        };

        _R operator()(_A ...__args) const {
            return __call(__stor.__data, octa::forward<_A>(__args)...);
        }

        template<typename _F> void assign(_F &&__f) {
            Function(octa::forward<_F>(__f)).swap(*this);
        }

        void swap(Function &__f) {
            __OctaFmStorage __tmp;
            __f.__stor.__manager->__call_move_and_destroyf(__tmp,
                octa::move(__f.__stor));
            __stor.__manager->__call_move_and_destroyf(__f.__stor,
                octa::move(__stor));
            __tmp.__manager->__call_move_and_destroyf(__stor,
                octa::move(__tmp));
            octa::swap(__call, __f.__call);
        }

        operator bool() const { return __call != nullptr; }

    private:
        __OctaFmStorage __stor;
        _R (*__call)(const __OctaFunctorData &, _A...);

        template<typename _T>
        void __initialize(_T __f) {
            __call = &__OctaFunctorDataManager<_T>::template __call<_R, _A...>;
            __stor.__manager = &__octa_get_default_fm<_T>();
            __OctaFunctorDataManager<_T>::__store_f(__stor.__data,
                octa::forward<_T>(__f));
        }

        void __init_empty() {
            typedef _R(*__emptyf)(_A...);
            __call = nullptr;
            __stor.__manager = &__octa_get_default_fm<__emptyf>();
            __OctaFunctorDataManager<__emptyf>::__store_f(__stor.__data, nullptr);
        }

        template<typename _T>
        static bool __func_is_null(const _T &) { return false; }

        static bool __func_is_null(_R (* const &__fptr)(_A...)) {
            return __fptr == nullptr;
        }

        template<typename _RR, typename _T, typename ..._AA>
        static bool __func_is_null(_RR (_T::* const &__fptr)(_AA...)) {
            return __fptr == nullptr;
        }

        template<typename _RR, typename _T, typename ..._AA>
        static bool __func_is_null(_RR (_T::* const &__fptr)(_AA...) const) {
            return __fptr == nullptr;
        }
    };

    template<typename _T>
    bool operator==(nullptr_t, const Function<_T> &__rhs) { return !__rhs; }

    template<typename _T>
    bool operator==(const Function<_T> &__lhs, nullptr_t) { return !__lhs; }

    template<typename _T>
    bool operator!=(nullptr_t, const Function<_T> &__rhs) { return __rhs; }

    template<typename _T>
    bool operator!=(const Function<_T> &__lhs, nullptr_t) { return __lhs; }
}

#endif