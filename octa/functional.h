/* Function objects for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_FUNCTIONAL_H
#define OCTA_FUNCTIONAL_H

#include "octa/new.h"
#include "octa/memory.h"
#include "octa/utility.h"
#include "octa/traits.h"

namespace octa {
#define OCTA_DEFINE_BINARY_OP(name, op, rettype) \
    template<typename T> struct name { \
        bool operator()(const T &x, const T &y) const { return x op y; } \
        struct type { \
            typedef T first; \
            typedef T second; \
            typedef rettype result; \
        }; \
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
        struct type {
            typedef T argument;
            typedef bool result;
        };
    };

    template<typename T> struct Negate {
        bool operator()(const T &x) const { return -x; }
        struct type {
            typedef T argument;
            typedef T result;
        };
    };

    template<typename T> struct BinaryNegate {
        struct type {
            typedef typename T::type::first first;
            typedef typename T::type::second second;
            typedef bool result;
        };
        explicit BinaryNegate(const T &f): p_fn(f) {}
        bool operator()(const typename type::first &x,
                        const typename type::second &y) {
            return !p_fn(x, y);
        }
    private:
        T p_fn;
    };

    template<typename T> struct UnaryNegate {
        struct type {
            typedef typename T::type::argument argument;
            typedef bool result;
        };
        explicit UnaryNegate(const T &f): p_fn(f) {}
        bool operator()(const typename type::argument &x) {
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

    template<typename T> ReferenceWrapper<T> ref(T &v) {
        return ReferenceWrapper<T>(v);
    }
    template<typename T> ReferenceWrapper<T> ref(ReferenceWrapper<T> v) {
        return ReferenceWrapper<T>(v);
    }
    template<typename T> void ref(const T &&) = delete;

    template<typename T> ReferenceWrapper<const T> cref(const T &v) {
        return ReferenceWrapper<T>(v);
    }
    template<typename T> ReferenceWrapper<const T> cref(ReferenceWrapper<T> v) {
        return ReferenceWrapper<T>(v);
    }
    template<typename T> void cref(const T &&) = delete;

    namespace internal {
        template<typename, typename> struct MemTypes;
        template<typename T, typename R, typename ...A>
        struct MemTypes<T, R(A...)> {
            typedef R result;
            typedef T argument;
        };
        template<typename T, typename R, typename A>
        struct MemTypes<T, R(A)> {
            typedef R result;
            typedef T first;
            typedef A second;
        };
        template<typename T, typename R, typename ...A>
        struct MemTypes<T, R(A...) const> {
            typedef R result;
            typedef const T argument;
        };
        template<typename T, typename R, typename A>
        struct MemTypes<T, R(A) const> {
            typedef R result;
            typedef const T first;
            typedef A second;
        };

        template<typename R, typename T>
        class MemFn {
            R T::*p_ptr;
        public:
            struct type: MemTypes<T, R> {};

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
    }

    template<typename R, typename T>
    internal::MemFn<R, T> mem_fn(R T:: *ptr) {
        return internal::MemFn<R, T>(ptr);
    }

    /* function impl
     * reference: http://probablydance.com/2013/01/13/a-faster-implementation-of-stdfunction
     */

    template<typename> struct Function;

    namespace internal {
        struct FunctorData {
            void *p1, *p2;
        };

        template<typename T>
        struct FunctorInPlace {
            static constexpr bool value = sizeof(T)  <= sizeof(FunctorData)
              && (alignof(FunctorData) % alignof(T)) == 0;
        };

        template<typename T, typename E = void>
        struct FunctorDataManager {
            template<typename R, typename ...A>
            static R call(const FunctorData &s, A ...args) {
                return ((T &)s)(forward<A>(args)...);
            }

            static void store_f(FunctorData &s, T v) {
                new (&get_ref(s)) T(forward<T>(v));
            }

            static void move_f(FunctorData &lhs, FunctorData &&rhs) {
                new (&get_ref(lhs)) T(move(get_ref(rhs)));
            }

            static void destroy_f(FunctorData &s) {
                get_ref(s).~T();
            }

            static T &get_ref(const FunctorData &s) {
                return (T &)s;
            }
        };

        template<typename T>
        struct FunctorDataManager<T, typename EnableIf<!FunctorInPlace<T>
        ::value>::type> {
            template<typename R, typename ...A>
            static R call(const FunctorData &s, A ...args) {
                return (*(T *&)s)(forward<A>(args)...);
            }

            static void store_f(FunctorData &s, T v) {
                new (&get_ptr_ref(s)) T *(new T(forward<T>(v)));
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

        struct ManagerStorage {
            FunctorData data;
            const FunctionManager *manager;
        };

        template<typename T>
        static const FunctionManager &get_default_manager();

        struct FunctionManager {
            template<typename T>
            inline static const FunctionManager create_default_manager() {
                return FunctionManager {
                    &t_call_move_and_destroy<T>,
                    &t_call_copy<T>,
                    &t_call_destroy<T>
                };
            }

            void (* const call_move_and_destroy)(ManagerStorage &lhs, ManagerStorage &&rhs);
            void (* const call_copy)(ManagerStorage &lhs, const ManagerStorage &rhs);
            void (* const call_destroy)(ManagerStorage &s);

            template<typename T>
            static void t_call_move_and_destroy(ManagerStorage &lhs, ManagerStorage &&rhs) {
                typedef FunctorDataManager<T> spec;
                spec::move_f(lhs.data, move(rhs.data));
                spec::destroy_f(rhs.data);
                lhs.manager = &get_default_manager<T>();
            }

            template<typename T>
            static void t_call_copy(ManagerStorage &lhs, const ManagerStorage &rhs) {
                typedef FunctorDataManager<T> spec;
                lhs.manager = &get_default_manager<T>();
                spec::store_f(lhs.data, spec::get_ref(rhs.data));
            }

            template<typename T>
            static void t_call_destroy(ManagerStorage &s) {
                typedef FunctorDataManager<T> spec;
                spec::destroy_f(s.data);
            }
        };

        template<typename T>
        inline static const FunctionManager &get_default_manager() {
            static const FunctionManager def_manager = FunctionManager::create_default_manager<T>();
            return def_manager;
        }

        template<typename R, typename...>
        struct FunctionBase {
            struct type {
                typedef R result;
            };
        };

        template<typename R, typename T>
        struct FunctionBase<R, T> {
            struct type {
                typedef R result;
                typedef T argument;
            };
        };

        template<typename R, typename T, typename U>
        struct FunctionBase<R, T, U> {
            struct type {
                typedef R result;
                typedef T first;
                typedef U second;
            };
        };
    }

    template<typename R, typename ...A>
    struct Function<R(A...)>: internal::FunctionBase<R, A...> {
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
        Function(T f) {
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

        template<typename F>
        void assign(F &&f) {
            Function(forward<F>(f)).swap(*this);
        }

        void swap(Function &f) {
            internal::ManagerStorage tmp;
            f.p_stor.manager->call_move_and_destroy(tmp, move(f.p_stor));
            p_stor.manager->call_move_and_destroy(f.p_stor, move(p_stor));
            tmp.manager->call_move_and_destroy(p_stor, move(tmp));
            octa::swap(p_call, f.p_call);
        }

        operator bool() const { return p_call != nullptr; }

    private:
        internal::ManagerStorage p_stor;
        R (*p_call)(const internal::FunctorData &, A...);

        template<typename T>
        void initialize(T f) {
            p_call = &internal::FunctorDataManager<T>::template call<R, A...>;
            p_stor.manager = &internal::get_default_manager<T>();
            internal::FunctorDataManager<T>::store_f(p_stor.data, forward<T>(f));
        }

        void initialize_empty() {
            typedef R(*emptyf)(A...);
            p_call = nullptr;
            p_stor.manager = &internal::get_default_manager<emptyf>();
            internal::FunctorDataManager<emptyf>::store_f(p_stor.data, nullptr);
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
    };

    template<typename T>
    void swap(Function<T> &a, Function<T> &b) {
        a.swap(b);
    }

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