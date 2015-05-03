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
        bool operator()(const T &x, const T &y) const noexcept( \
            noexcept(x op y) \
        ) { return x op y; } \
        typedef T first_argument_type; \
        typedef T second_argument_type; \
        typedef rettype result_type; \
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
        bool operator()(const T &x) const noexcept(noexcept(!x)) { return !x; }
        typedef T argument_type;
        typedef bool result_type;
    };

    template<typename T> struct Negate {
        bool operator()(const T &x) const noexcept(noexcept(-x)) { return -x; }
        typedef T argument_type;
        typedef T result_type;
    };

    template<typename T> struct BinaryNegate {
        typedef typename T::first_argument_type first_argument_type;
        typedef typename T::second_argument_type second_argument_type;
        typedef bool result_type;

        explicit BinaryNegate(const T &f) noexcept(
            IsNothrowCopyConstructible<T>::value
        ): p_fn(f) {}

        bool operator()(const first_argument_type &x,
                        const second_argument_type &y) noexcept(
            noexcept(p_fn(x, y))
        ) {
            return !p_fn(x, y);
        }
    private:
        T p_fn;
    };

    template<typename T> struct UnaryNegate {
        typedef typename T::argument_type argument_type;
        typedef bool result_type;

        explicit UnaryNegate(const T &f) noexcept(
            IsNothrowCopyConstructible<T>::value
        ): p_fn(f) {}
        bool operator()(const argument_type &x) noexcept(noexcept(p_fn(x))) {
            return !p_fn(x);
        }
    private:
        T p_fn;
    };

    template<typename T> UnaryNegate<T> not1(const T &fn) noexcept(
        IsNothrowCopyConstructible<UnaryNegate<T>>::value
    ) {
        return UnaryNegate<T>(fn);
    }

    template<typename T> BinaryNegate<T> not2(const T &fn) noexcept(
        IsNothrowCopyConstructible<BinaryNegate<T>>::value
    ) {
        return BinaryNegate<T>(fn);
    }

    /* reference wrapper */

    template<typename T>
    struct ReferenceWrapper {
        typedef T type;

        ReferenceWrapper(T &v) noexcept: p_ptr(address_of(v)) {}
        ReferenceWrapper(const ReferenceWrapper &) = default;
        ReferenceWrapper(T &&) = delete;

        ReferenceWrapper &operator=(const ReferenceWrapper &) = default;

        operator T &() const { return *p_ptr; }
        T &get() const { return *p_ptr; }

    private:
        T *p_ptr;
    };

    template<typename T>
    ReferenceWrapper<T> ref(T &v) noexcept {
        return ReferenceWrapper<T>(v);
    }
    template<typename T>
    ReferenceWrapper<T> ref(ReferenceWrapper<T> v) noexcept {
        return ReferenceWrapper<T>(v);
    }
    template<typename T> void ref(const T &&) = delete;

    template<typename T>
    ReferenceWrapper<const T> cref(const T &v) noexcept {
        return ReferenceWrapper<T>(v);
    }
    template<typename T>
    ReferenceWrapper<const T> cref(ReferenceWrapper<T> v) noexcept {
        return ReferenceWrapper<T>(v);
    }
    template<typename T> void cref(const T &&) = delete;

    /* mem_fn */

    template<typename, typename> struct __OctaMemTypes;
    template<typename T, typename R, typename ...A>
    struct __OctaMemTypes<T, R(A...)> {
        typedef R result_type;
        typedef T argument_type;
    };
    template<typename T, typename R, typename A>
    struct __OctaMemTypes<T, R(A)> {
        typedef R result_type;
        typedef T first_argument_type;
        typedef A second_argument_type;
    };
    template<typename T, typename R, typename ...A>
    struct __OctaMemTypes<T, R(A...) const> {
        typedef R result_type;
        typedef const T argument_type;
    };
    template<typename T, typename R, typename A>
    struct __OctaMemTypes<T, R(A) const> {
        typedef R result_type;
        typedef const T first_argument_type;
        typedef A second_argument_type;
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
    __OctaMemFn<R, T> mem_fn(R T:: *ptr) noexcept {
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
          && octa::IsNothrowMoveConstructible<T>();
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

        static void move_f(__OctaFunctorData &lhs, __OctaFunctorData &&rhs) noexcept {
            new (&get_ref(lhs)) T(move(get_ref(rhs)));
        }

        static void destroy_f(__OctaFunctorData &s) noexcept {
            get_ref(s).~T();
        }

        static T &get_ref(const __OctaFunctorData &s) noexcept {
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

        static void move_f(__OctaFunctorData &lhs, __OctaFunctorData &&rhs) noexcept {
            new (&get_ptr_ref(lhs)) T *(get_ptr_ref(rhs));
            get_ptr_ref(rhs) = nullptr;
        }

        static void destroy_f(__OctaFunctorData &s) noexcept {
            T *&ptr = get_ptr_ref(s);
            if (!ptr) return;
            delete ptr;
            ptr = nullptr;
        }

        static T &get_ref(const __OctaFunctorData &s) noexcept {
            return *get_ptr_ref(s);
        }

        static T *&get_ptr_ref(__OctaFunctorData &s) noexcept {
            return (T *&)s;
        }

        static T *&get_ptr_ref(const __OctaFunctorData &s) noexcept {
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
        typedef R result_type;
    };

    template<typename R, typename T>
    struct __OctaFunction<R, T> {
        typedef R result_type;
        typedef T argument_type;
    };

    template<typename R, typename T, typename U>
    struct __OctaFunction<R, T, U> {
        typedef R result_type;
        typedef T first_argument_type;
        typedef U second_argument_type;
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
        Function(         ) noexcept { initialize_empty(); }
        Function(nullptr_t) noexcept { initialize_empty(); }

        Function(Function &&f) noexcept {
            initialize_empty();
            swap(f);
        }

        Function(const Function &f) noexcept: p_call(f.p_call) {
            f.p_stor.manager->call_copy(p_stor, f.p_stor);
        }

        template<typename T>
        Function(T f, EnableIf<IsValidFunctor<T, R(A...)>::value, __OctaEmpty>
            = __OctaEmpty())
        noexcept(__OctaFunctorInPlace<T>::value) {
            if (func_is_null(f)) {
                initialize_empty();
                return;
            }
            initialize(func_to_functor(forward<T>(f)));
        }

        ~Function() {
            p_stor.manager->call_destroy(p_stor);
        }

        Function &operator=(Function &&f) noexcept {
            p_stor.manager->call_destroy(p_stor);
            swap(f);
            return *this;
        }

        Function &operator=(const Function &f) noexcept {
            p_stor.manager->call_destroy(p_stor);
            f.p_stor.manager->call_copy(p_stor, f.p_stor);
            return *this;
        };

        R operator()(A ...args) const {
            return p_call(p_stor.data, forward<A>(args)...);
        }

        template<typename F>
        void assign(F &&f) noexcept(__OctaFunctorInPlace<F>::value) {
            Function(forward<F>(f)).swap(*this);
        }

        void swap(Function &f) noexcept {
            __OctaFmStorage tmp;
            f.p_stor.manager->call_move_and_destroy(tmp, move(f.p_stor));
            p_stor.manager->call_move_and_destroy(f.p_stor, move(p_stor));
            tmp.manager->call_move_and_destroy(p_stor, move(tmp));
            octa::swap(p_call, f.p_call);
        }

        operator bool() const noexcept { return p_call != nullptr; }

    private:
        __OctaFmStorage p_stor;
        R (*p_call)(const __OctaFunctorData &, A...);

        template<typename T>
        void initialize(T f) {
            p_call = &__OctaFunctorDataManager<T>::template call<R, A...>;
            p_stor.manager = &__octa_get_default_fm<T>();
            __OctaFunctorDataManager<T>::store_f(p_stor.data, forward<T>(f));
        }

        void initialize_empty() noexcept {
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
    void swap(Function<T> &a, Function<T> &b) noexcept {
        a.swap(b);
    }

    template<typename T>
    bool operator==(nullptr_t, const Function<T> &rhs) noexcept { return !rhs; }

    template<typename T>
    bool operator==(const Function<T> &lhs, nullptr_t) noexcept { return !lhs; }

    template<typename T>
    bool operator!=(nullptr_t, const Function<T> &rhs) noexcept { return rhs; }

    template<typename T>
    bool operator!=(const Function<T> &lhs, nullptr_t) noexcept { return lhs; }
}

#endif