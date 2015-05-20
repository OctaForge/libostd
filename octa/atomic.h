/* Atomics for OctaSTD. Supports GCC/Clang and possibly MSVC.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_ATOMIC_H
#define OCTA_ATOMIC_H

#include <stdint.h>
#include <stddef.h>

#include "octa/types.h"
#include "octa/type_traits.h"

namespace octa {
    enum class MemoryOrder {
        relaxed = 0,
        consume,
        acquire,
        release,
        acq_rel,
        seq_cst
    };

    template<typename T>
    struct __OctaAtomicBase {
        __OctaAtomicBase() {}
        explicit __OctaAtomicBase(T v): value(v) {}
        T value;
    };

    template<typename T> T __octa_atomic_create();

    template<typename T, typename U>
    EnableIf<sizeof(T()->value = __octa_atomic_create<U>()), char>
    __octa_test_atomic_assignable(int);

    template<typename T, typename U>
    int __octa_test_atomic_assignable(...);

    template<typename T, typename U>
    struct __OctaCanAtomicAssign {
        static constexpr bool value
            = (sizeof(__octa_test_atomic_assignable<T, U>(1)) == sizeof(char));
    };

    template<typename T>
    static inline EnableIf<
        __OctaCanAtomicAssign<volatile __OctaAtomicBase<T> *, T>::value
    > __octa_atomic_init(volatile __OctaAtomicBase<T> *a, T v) {
        a->value = v;
    }

    template<typename T>
    static inline EnableIf<
        !__OctaCanAtomicAssign<volatile __OctaAtomicBase<T> *, T>::value &&
         __OctaCanAtomicAssign<         __OctaAtomicBase<T> *, T>::value
    > __octa_atomic_init(volatile __OctaAtomicBase<T> *a, T v) {
        volatile char *to  = (volatile char *)(&a->value);
        volatile char *end = to + sizeof(T);
        char *from = (char *)(&v);
        while (to != end) *to++ =*from++;
    }

    template<typename T>
    static inline void __octa_atomic_init(__OctaAtomicBase<T> *a, T v) {
        a->value = v;
    }

    /* MSVC support
     *
     * untested, might not work
     *
     * reference: https://github.com/openvswitch/ovs/blob/master/lib/ovs-atomic-msvc.h
     */

#ifdef _MSC_VER

/* MSVC support is disabled for now */
#if 0
#define ATOMIC_BOOL_LOCK_FREE 2
#define ATOMIC_CHAR_LOCK_FREE 2
#define ATOMIC_CHAR16_T_LOCK_FREE 2
#define ATOMIC_CHAR32_T_LOCK_FREE 2
#define ATOMIC_WCHAR_T_LOCK_FREE 2
#define ATOMIC_SHORT_LOCK_FREE 2
#define ATOMIC_INT_LOCK_FREE 2
#define ATOMIC_LONG_LOCK_FREE 2
#define ATOMIC_LLONG_LOCK_FREE 2
#define ATOMIC_POINTER_LOCK_FREE 2

    static inline void __octa_atomic_thread_fence(MemoryOrder ord) {
        if (ord > MemoryOrder::consume) {
            _ReadWriteBarrier();
            if (ord == MemoryOrder::seq_cst) {
                MemoryBarrier();
                _ReadWriteBarrier();
            }
        }
    }

    static inline void __octa_atomic_signal_fence(MemoryOrder ord) {
        if (ord > MemoryOrder::consume) _ReadWriteBarrier();
    }

    static inline bool __octa_atomic_is_lock_free(size_t size) {
        return size <= sizeof(void *);
    }

    template<typename T, size_t N = sizeof(T)>
    struct __OctaMsvcAtomicStore;

    template<typename T>
    struct __OctaMsvcAtomicStore<T, 1> {
        static inline void store(volatile T *dst, T src, MemoryOrder ord) {
            if (ord == MemoryOrder::seq_cst)
                InterlockedExchange8((volatile int8_t *)dst, (int8_t)src);
            else *dst = src;
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicStore<T, 2> {
        static inline void store(volatile T *dst, T src, MemoryOrder ord) {
            if (ord == MemoryOrder::seq_cst)
                InterlockedExchange16((volatile int16_t *)dst, (int16_t)src);
            else *dst = src;
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicStore<T, 4> {
        static inline void store(volatile T *dst, T src, MemoryOrder ord) {
            if (ord == MemoryOrder::seq_cst)
                InterlockedExchange((volatile int32_t *)dst, (int32_t)src);
            else *dst = src;
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicStore<T, 8> {
        static inline void store(volatile T *dst, T src, MemoryOrder ord) {
            if (ord == MemoryOrder::relaxed)
                InterlockedExchangeNoFence64((volatile int64_t *)dst, (int64_t)src);
            else
                InterlockedExchange64((volatile int64_t *)dst, (int64_t)src);
        }
    };

    template<typename T>
    static inline void __octa_atomic_store(volatile __OctaAtomicBase<T> *a,
                                           T v, MemoryOrder ord) {
        __OctaMsvcAtomicStore<T>::store(&a->value, v, ord);
    }

    template<typename T>
    static inline void __octa_atomic_store(__OctaAtomicBase<T> *a,
                                           T v, MemoryOrder ord) {
        __OctaMsvcAtomicStore<T>::store(&a->value, v, ord);
    }

    template<typename T, size_t N = sizeof(T)>
    struct __OctaMsvcAtomicLoad;

    template<typename T>
    struct __OctaMsvcAtomicLoad<T, 1> {
        static inline void load(volatile T *src, T *dst) { *dst = *src; }
    };

    template<typename T>
    struct __OctaMsvcAtomicLoad<T, 2> {
        static inline void load(volatile T *src, T *dst) { *dst = *src; }
    };

    template<typename T>
    struct __OctaMsvcAtomicLoad<T, 4> {
        static inline void load(volatile T *src, T *dst) { *dst = *src; }
    };

    template<typename T>
    struct __OctaMsvcAtomicLoad<T, 8> {
        static inline void load(volatile T *src, T *dst) {
#pragma warning(push)
#pragma warning(disable:4047)
            *dst = InterlockedOr64((volatile int64_t *)(src), 0);
#pragma warning(pop)
        }
    };

    template<typename T>
    static inline T __octa_atomic_load(volatile __OctaAtomicBase<T> *a,
                                       MemoryOrder ord) {
        T r;
        __OctaMsvcAtomicLoad<T>::load(&a->value, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_load(__OctaAtomicBase<T> *a,
                                       MemoryOrder ord) {
        T r;
        __OctaMsvcAtomicLoad<T>::load(&a->value, &r);
        return r;
    }

    template<typename T, size_t N = sizeof(T)>
    struct __OctaMsvcAtomicExchange;

    template<typename T>
    struct __OctaMsvcAtomicExchange<T, 1> {
        static inline void exchange(volatile T *dst, T src, T *ret) {
            *ret = InterlockedExchange8((volatile int8_t *)dst, (int8_t)src);
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicExchange<T, 2> {
        static inline void exchange(volatile T *dst, T src, T *ret) {
            *ret = InterlockedExchange16((volatile int16_t *)dst, (int16_t)src);
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicExchange<T, 4> {
        static inline void exchange(volatile T *dst, T src, T *ret) {
            *ret = InterlockedExchange((volatile int32_t *)dst, (int32_t)src);
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicExchange<T, 8> {
        static inline void exchange(volatile T *dst, T src, T *ret) {
            *ret = InterlockedExchange64((volatile int64_t *)dst, (int64_t)src);
        }
    };

    template<typename T>
    static inline T __octa_atomic_exchange(volatile __OctaAtomicBase<T> *a,
                                           T v, memory_order) {
        T r;
        __OctaMsvcAtomicExchange<T>::exchange(&a->value, v, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_exchange(__OctaAtomicBase<T> *a,
                                           T v, memory_order) {
        T r;
        __OctaMsvcAtomicExchange<T>::exchange(&a->value, v, &r);
        return r;
    }

    template<typename T, size_t N = sizeof(T)>
    struct __OctaMsvcAtomicCompareExchange;

    template<typename T>
    struct __OctaMsvcAtomicCompareExchange<T, 1> {
        static inline bool exchange(volatile T *dst, T *exp, T src) {
            int8_t prev = _InterlockedCompareExchange8((volatile int8_t *)dst,
                (int8_t)src, (int8_t)*exp);
            if (prev == (int8_t)*exp) return true;
            *exp = (T)prev; return false;
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicCompareExchange<T, 2> {
        static inline bool exchange(volatile T *dst, T *exp, T src) {
            int16_t prev = _InterlockedCompareExchange16((volatile int16_t *)dst,
                (int16_t)src, (int16_t)*exp);
            if (prev == (int16_t)*exp) return true;
            *exp = (T)prev; return false;
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicCompareExchange<T, 4> {
        static inline bool exchange(volatile T *dst, T *exp, T src) {
            int32_t prev = _InterlockedCompareExchange((volatile int32_t *)dst,
                (int32_t)src, (int32_t)*exp);
            if (prev == (int32_t)*exp) return true;
            *exp = (T)prev; return false;
        }
    };

    template<typename T>
    struct __OctaMsvcAtomicCompareExchange<T, 8> {
        static inline bool exchange(volatile T *dst, T *exp, T src) {
            int64_t prev = _InterlockedCompareExchange64((volatile int64_t *)dst,
                (int64_t)src, (int64_t)*exp);
            if (prev == (int64_t)*exp) return true;
            *exp = (T)prev; return false;
        }
    };

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_strong(
        volatile __OctaAtomicBase<T> *a, T *expected, T v,
        memory_order, memory_order
    ) {
        return __OctaMsvcAtomicCompareExchange<T>::exchange(&a->value, expected, v);
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_strong(
        __OctaAtomicBase<T> *a, T *expected, T v,
        memory_order, memory_order
    ) {
        return __OctaMsvcAtomicCompareExchange<T>::exchange(&a->value, expected, v);
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_weak(
        volatile __OctaAtomicBase<T> *a, T *expected, T v,
        memory_order, memory_order
    ) {
        return __OctaMsvcAtomicCompareExchange<T>::exchange(&a->value, expected, v);
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_weak(
        __OctaAtomicBase<T> *a, T *expected, T v,
        memory_order, memory_order
    ) {
        return __OctaMsvcAtomicCompareExchange<T>::exchange(&a->value, expected, v);
    }

#define __OCTA_MSVC_ATOMIC_FETCH_OP(opn, dst, val, ret) \
    if (sizeof(*dst) == 1) { \
        *(ret) = _InterlockedExchange##opn##8((volatile int8_t *)dst, (int8_t)val); \
    } else if (sizeof(*dst) == 2) { \
        *(ret) = _InterlockedExchange##opn##16((volatile int16_t *)dst, (int16_t)val); \
    } else if (sizeof(*dst) == 4) { \
        *(ret) = InterlockedExchange##opn((volatile int32_t *)dst, (int32_t)val); \
    }  else if (sizeof(*dst) == 8) { \
        *(ret) = _InterlockedExchange##opn##64((volatile int64_t *)dst, (int64_t)val); \
    } else { \
        abort(); \
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_add(volatile __OctaAtomicBase<T> *a,
                                            U d, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Add, &a->value, d, &r);
        return r;
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_add(__OctaAtomicBase<T> *a,
                                            U d, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Add, &a->value, d, &r);
        return r;
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_sub(volatile __OctaAtomicBase<T> *a,
                                            U d, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Sub, &a->value, d, &r);
        return r;
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_sub(__OctaAtomicBase<T> *a,
                                            U d, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Sub, &a->value, d, &r);
        return r;
    }

#undef __OCTA_MSVC_ATOMIC_FETCH_OP

#define __OCTA_MSVC_ATOMIC_FETCH_OP(opn, dst, val, ret) \
    if (sizeof(*dst) == 1) { \
        *(ret) = Interlocked##opn##8((volatile int8_t *)dst, (int8_t)val); \
    } else if (sizeof(*dst) == 2) { \
        *(ret) = Interlocked##opn##16((volatile int16_t *)dst, (int16_t)val); \
    } else if (sizeof(*dst) == 4) { \
        *(ret) = Interlocked##opn((volatile int32_t *)dst, (int32_t)val); \
    }  else if (sizeof(*dst) == 8) { \
        *(ret) = Interlocked##opn##64((volatile int64_t *)dst, (int64_t)val); \
    } else { \
        abort(); \
    }

    template<typename T>
    static inline T __octa_atomic_fetch_and(volatile __OctaAtomicBase<T> *a,
                                            T pattern, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(And, &a->value, d, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_fetch_and(__OctaAtomicBase<T> *a,
                                            T pattern, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(And, &a->value, d, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_fetch_or(volatile __OctaAtomicBase<T> *a,
                                            T pattern, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Or, &a->value, d, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_fetch_or(__OctaAtomicBase<T> *a,
                                            T pattern, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Or, &a->value, d, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_fetch_xor(volatile __OctaAtomicBase<T> *a,
                                            T pattern, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Xor, &a->value, d, &r);
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_fetch_xor(__OctaAtomicBase<T> *a,
                                            T pattern, memory_order) {
        T r;
        __OCTA_MSVC_ATOMIC_FETCH_OP(Xor, &a->value, d, &r);
        return r;
    }

#undef __OCTA_MSVC_ATOMIC_FETCH_OP
#else
#error No MSVC support right now!
#endif

#else

    /* GCC, Clang support
     *
     * libc++ used for reference
     */

#ifdef __GNUC__

#define ATOMIC_BOOL_LOCK_FREE      __GCC_ATOMIC_BOOL_LOCK_FREE
#define ATOMIC_CHAR_LOCK_FREE      __GCC_ATOMIC_CHAR_LOCK_FREE
#define ATOMIC_CHAR16_T_LOCK_FREE  __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define ATOMIC_CHAR32_T_LOCK_FREE  __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define ATOMIC_WCHAR_T_LOCK_FREE   __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define ATOMIC_SHORT_LOCK_FREE     __GCC_ATOMIC_SHORT_LOCK_FREE
#define ATOMIC_INT_LOCK_FREE       __GCC_ATOMIC_INT_LOCK_FREE
#define ATOMIC_LONG_LOCK_FREE      __GCC_ATOMIC_LONG_LOCK_FREE
#define ATOMIC_LLONG_LOCK_FREE     __GCC_ATOMIC_LLONG_LOCK_FREE
#define ATOMIC_POINTER_LOCK_FREE   __GCC_ATOMIC_POINTER_LOCK_FREE

    static inline constexpr int __octa_to_gcc_order(MemoryOrder ord) {
        return ((ord == MemoryOrder::relaxed) ? __ATOMIC_RELAXED :
               ((ord == MemoryOrder::acquire) ? __ATOMIC_ACQUIRE :
               ((ord == MemoryOrder::release) ? __ATOMIC_RELEASE :
               ((ord == MemoryOrder::seq_cst) ? __ATOMIC_SEQ_CST :
               ((ord == MemoryOrder::acq_rel) ? __ATOMIC_ACQ_REL :
                                                __ATOMIC_CONSUME)))));
    }

    static inline constexpr int __octa_to_gcc_failure_order(MemoryOrder ord) {
        return ((ord == MemoryOrder::relaxed) ? __ATOMIC_RELAXED :
               ((ord == MemoryOrder::acquire) ? __ATOMIC_ACQUIRE :
               ((ord == MemoryOrder::release) ? __ATOMIC_RELAXED :
               ((ord == MemoryOrder::seq_cst) ? __ATOMIC_SEQ_CST :
               ((ord == MemoryOrder::acq_rel) ? __ATOMIC_ACQUIRE :
                                                __ATOMIC_CONSUME)))));
    }

    static inline void __octa_atomic_thread_fence(MemoryOrder ord) {
        __atomic_thread_fence(__octa_to_gcc_order(ord));
    }

    static inline void __octa_atomic_signal_fence(MemoryOrder ord) {
        __atomic_signal_fence(__octa_to_gcc_order(ord));
    }

    static inline bool __octa_atomic_is_lock_free(size_t size) {
        /* return __atomic_is_lock_free(size, 0); cannot be used on some platforms */
        return size <= sizeof(void *);
    }

    template<typename T>
    static inline void __octa_atomic_store(volatile __OctaAtomicBase<T> *a,
                                           T v, MemoryOrder ord) {
        __atomic_store(&a->value, &v, __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline void __octa_atomic_store(__OctaAtomicBase<T> *a,
                                           T v, MemoryOrder ord) {
        __atomic_store(&a->value, &v, __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_load(volatile __OctaAtomicBase<T> *a,
                                       MemoryOrder ord) {
        T r;
        __atomic_load(&a->value, &r, __octa_to_gcc_order(ord));
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_load(__OctaAtomicBase<T> *a,
                                       MemoryOrder ord) {
        T r;
        __atomic_load(&a->value, &r, __octa_to_gcc_order(ord));
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_exchange(volatile __OctaAtomicBase<T> *a,
                                           T v, MemoryOrder ord) {
        T r;
        __atomic_exchange(&a->value, &v, &r, __octa_to_gcc_order(ord));
        return r;
    }

    template<typename T>
    static inline T __octa_atomic_exchange(__OctaAtomicBase<T> *a,
                                           T v, MemoryOrder ord) {
        T r;
        __atomic_exchange(&a->value, &v, &r, __octa_to_gcc_order(ord));
        return r;
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_strong(
        volatile __OctaAtomicBase<T> *a, T *expected, T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->value, expected, &v, false,
            __octa_to_gcc_order(success), __octa_to_gcc_failure_order(failure));
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_strong(
        __OctaAtomicBase<T> *a, T *expected, T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->value, expected, &v, false,
            __octa_to_gcc_order(success), __octa_to_gcc_failure_order(failure));
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_weak(
        volatile __OctaAtomicBase<T> *a, T *expected, T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->value, expected, &v, true,
            __octa_to_gcc_order(success), __octa_to_gcc_failure_order(failure));
    }

    template<typename T>
    static inline bool __octa_atomic_compare_exchange_weak(
        __OctaAtomicBase<T> *a, T *expected, T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->value, expected, &v, true,
            __octa_to_gcc_order(success), __octa_to_gcc_failure_order(failure));
    }

    template<typename T>
    struct __OctaSkipAmt { static constexpr size_t value = 1; };

    template<typename T>
    struct __OctaSkipAmt<T *> { static constexpr size_t value = sizeof(T); };

    template<typename T> struct __OctaSkipAmt<T[]> {};
    template<typename T, size_t N> struct __OctaSkipAmt<T[N]> {};

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_add(volatile __OctaAtomicBase<T> *a,
                                            U d, MemoryOrder ord) {
        return __atomic_fetch_add(&a->value, d * __OctaSkipAmt<T>::value,
            __octa_to_gcc_order(ord));
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_add(__OctaAtomicBase<T> *a,
                                            U d, MemoryOrder ord) {
        return __atomic_fetch_add(&a->value, d * __OctaSkipAmt<T>::value,
            __octa_to_gcc_order(ord));
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_sub(volatile __OctaAtomicBase<T> *a,
                                            U d, MemoryOrder ord) {
        return __atomic_fetch_sub(&a->value, d * __OctaSkipAmt<T>::value,
            __octa_to_gcc_order(ord));
    }

    template<typename T, typename U>
    static inline T __octa_atomic_fetch_sub(__OctaAtomicBase<T> *a,
                                            U d, MemoryOrder ord) {
        return __atomic_fetch_sub(&a->value, d * __OctaSkipAmt<T>::value,
            __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_fetch_and(volatile __OctaAtomicBase<T> *a,
                                            T pattern, MemoryOrder ord) {
        return __atomic_fetch_and(&a->value, pattern,
            __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_fetch_and(__OctaAtomicBase<T> *a,
                                            T pattern, MemoryOrder ord) {
        return __atomic_fetch_and(&a->value, pattern,
            __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_fetch_or(volatile __OctaAtomicBase<T> *a,
                                            T pattern, MemoryOrder ord) {
        return __atomic_fetch_or(&a->value, pattern,
            __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_fetch_or(__OctaAtomicBase<T> *a,
                                            T pattern, MemoryOrder ord) {
        return __atomic_fetch_or(&a->value, pattern,
            __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_fetch_xor(volatile __OctaAtomicBase<T> *a,
                                            T pattern, MemoryOrder ord) {
        return __atomic_fetch_xor(&a->value, pattern,
            __octa_to_gcc_order(ord));
    }

    template<typename T>
    static inline T __octa_atomic_fetch_xor(__OctaAtomicBase<T> *a,
                                            T pattern, MemoryOrder ord) {
        return __atomic_fetch_xor(&a->value, pattern,
            __octa_to_gcc_order(ord));
    }
#else
# error Unsupported compiler
#endif
#endif

    template <typename T> inline T kill_dependency(T v) {
        return v;
    }

    template<typename T, bool = IsIntegral<T>::value && !IsSame<T, bool>::value>
    struct __OctaAtomic {
        mutable __OctaAtomicBase<T> p_a;

        __OctaAtomic() = default;

        constexpr __OctaAtomic(T v): p_a(v) {}

        __OctaAtomic(const __OctaAtomic &) = delete;

        __OctaAtomic &operator=(const __OctaAtomic &) = delete;
        __OctaAtomic &operator=(const __OctaAtomic &) volatile = delete;

        bool is_lock_free() const volatile {
            return __octa_atomic_is_lock_free(sizeof(T));
        }

        bool is_lock_free() const {
            return __octa_atomic_is_lock_free(sizeof(T));
        }

        void store(T v, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            __octa_atomic_store(&p_a, v, ord);
        }

        void store(T v, MemoryOrder ord = MemoryOrder::seq_cst) {
            __octa_atomic_store(&p_a, v, ord);
        }

        T load(MemoryOrder ord = MemoryOrder::seq_cst) const volatile {
            return __octa_atomic_load(&p_a, ord);
        }

        T load(MemoryOrder ord = MemoryOrder::seq_cst) const {
            return __octa_atomic_load(&p_a, ord);
        }

        operator T() const volatile { return load(); }
        operator T() const          { return load(); }

        T exchange(T v, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_exchange(&p_a, v, ord);
        }

        T exchange(T v, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_exchange(&p_a, v, ord);
        }

        bool compare_exchange_weak(T &e, T v, MemoryOrder s, MemoryOrder f)
        volatile {
            return __octa_atomic_compare_exchange_weak(&p_a, &e, v, s, f);
        }

        bool compare_exchange_weak(T& e, T v, MemoryOrder s, MemoryOrder f) {
            return __octa_atomic_compare_exchange_weak(&p_a, &e, v, s, f);
        }

        bool compare_exchange_strong(T& e, T v, MemoryOrder s, MemoryOrder f)
        volatile {
            return __octa_atomic_compare_exchange_strong(&p_a, &e, v, s, f);
        }

        bool compare_exchange_strong(T& e, T v, MemoryOrder s, MemoryOrder f) {
            return __octa_atomic_compare_exchange_strong(&p_a, &e, v, s, f);
        }

        bool compare_exchange_weak(T& e, T v, MemoryOrder ord
                                                  = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_compare_exchange_weak(&p_a, &e, v, ord, ord);
        }

        bool compare_exchange_weak(T& e, T v, MemoryOrder ord
                                                  = MemoryOrder::seq_cst) {
            return __octa_atomic_compare_exchange_weak(&p_a, &e, v, ord, ord);
        }

        bool compare_exchange_strong(T& e, T v, MemoryOrder ord
                                                    = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_compare_exchange_strong(&p_a, &e, v, ord, ord);
        }

        bool compare_exchange_strong(T& e, T v, MemoryOrder ord
                                                    = MemoryOrder::seq_cst) {
            return __octa_atomic_compare_exchange_strong(&p_a, &e, v, ord, ord);
        }
    };

    template<typename T>
    struct __OctaAtomic<T, true>: __OctaAtomic<T, false> {
        typedef __OctaAtomic<T, false> base_t;

        __OctaAtomic() = default;

        constexpr __OctaAtomic(T v): base_t(v) {}

        T fetch_add(T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_add(&this->p_a, op, ord);
        }

        T fetch_add(T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_add(&this->p_a, op, ord);
        }

        T fetch_sub(T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_sub(&this->p_a, op, ord);
        }

        T fetch_sub(T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_sub(&this->p_a, op, ord);
        }

        T fetch_and(T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_and(&this->p_a, op, ord);
        }

        T fetch_and(T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_and(&this->p_a, op, ord);
        }

        T fetch_or(T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_or(&this->p_a, op, ord);
        }

        T fetch_or(T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_or(&this->p_a, op, ord);
        }

        T fetch_xor(T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_xor(&this->p_a, op, ord);
        }

        T fetch_xor(T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_xor(&this->p_a, op, ord);
        }

        T operator++(int) volatile { return fetch_add(T(1));        }
        T operator++(int)          { return fetch_add(T(1));        }
        T operator--(int) volatile { return fetch_sub(T(1));        }
        T operator--(int)          { return fetch_sub(T(1));        }
        T operator++(   ) volatile { return fetch_add(T(1)) + T(1); }
        T operator++(   )          { return fetch_add(T(1)) + T(1); }
        T operator--(   ) volatile { return fetch_sub(T(1)) - T(1); }
        T operator--(   )          { return fetch_sub(T(1)) - T(1); }

        T operator+=(T op) volatile { return fetch_add(op) + op; }
        T operator+=(T op)          { return fetch_add(op) + op; }
        T operator-=(T op) volatile { return fetch_sub(op) - op; }
        T operator-=(T op)          { return fetch_sub(op) - op; }
        T operator&=(T op) volatile { return fetch_and(op) & op; }
        T operator&=(T op)          { return fetch_and(op) & op; }
        T operator|=(T op) volatile { return fetch_or (op) | op; }
        T operator|=(T op)          { return fetch_or (op) | op; }
        T operator^=(T op) volatile { return fetch_xor(op) ^ op; }
        T operator^=(T op)          { return fetch_xor(op) ^ op; }
    };

    template<typename T>
    struct Atomic: __OctaAtomic<T> {
        typedef __OctaAtomic<T> base_t;

        Atomic() = default;

        constexpr Atomic(T v): base_t(v) {}

        T operator=(T v) volatile {
            base_t::store(v); return v;
        }

        T operator=(T v) {
            base_t::store(v); return v;
        }
    };

    template<typename T>
    struct Atomic<T *>: __OctaAtomic<T *> {
        typedef __OctaAtomic<T *> base_t;

        Atomic() = default;

        constexpr Atomic(T *v): base_t(v) {}

        T *operator=(T *v) volatile {
            base_t::store(v); return v;
        }

        T *operator=(T *v) {
            base_t::store(v); return v;
        }

        T *fetch_add(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_fetch_add(&this->p_a, op, ord);
        }

        T *fetch_add(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_add(&this->p_a, op, ord);
        }

        T *fetch_sub(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_fetch_sub(&this->p_a, op, ord);
        }

        T *fetch_sub(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_sub(&this->p_a, op, ord);
        }


        T *operator++(int) volatile { return fetch_add(1);     }
        T *operator++(int)          { return fetch_add(1);     }
        T *operator--(int) volatile { return fetch_sub(1);     }
        T *operator--(int)          { return fetch_sub(1);     }
        T *operator++(   ) volatile { return fetch_add(1) + 1; }
        T *operator++(   )          { return fetch_add(1) + 1; }
        T *operator--(   ) volatile { return fetch_sub(1) - 1; }
        T *operator--(   )          { return fetch_sub(1) - 1; }

        T *operator+=(ptrdiff_t op) volatile { return fetch_add(op) + op; }
        T *operator+=(ptrdiff_t op)          { return fetch_add(op) + op; }
        T *operator-=(ptrdiff_t op) volatile { return fetch_sub(op) - op; }
        T *operator-=(ptrdiff_t op)          { return fetch_sub(op) - op; }
    };

    template<typename T>
    inline bool atomic_is_lock_free(const volatile Atomic<T> *a) {
        return a->is_lock_free();
    }

    template<typename T>
    inline bool atomic_is_lock_free(const Atomic<T> *a) {
        return a->is_lock_free();
    }

    template<typename T>
    inline void atomic_init(volatile Atomic<T> *a, T v) {
        __octa_atomic_init(&a->p_a, v);
    }

    template<typename T>
    inline void atomic_init(Atomic<T> *a, T v) {
        __octa_atomic_init(&a->p_a, v);
    }

    template <typename T>
    inline void atomic_store(volatile Atomic<T> *a, T v) {
        a->store(v);
    }

    template <typename T>
    inline void atomic_store(Atomic<T> *a, T v) {
        a->store(v);
    }

    template <typename T>
    inline void atomic_store_explicit(volatile Atomic<T> *a, T v,
                                      MemoryOrder ord) {
        a->store(v, ord);
    }

    template <typename T>
    inline void atomic_store_explicit(Atomic<T> *a, T v, MemoryOrder ord) {
        a->store(v, ord);
    }

    template <typename T>
    inline T atomic_load(const volatile Atomic<T> *a) {
        return a->load();
    }

    template <typename T>
    inline T atomic_load(const Atomic<T> *a) {
        return a->load();
    }

    template <typename T>
    inline  T atomic_load_explicit(const volatile Atomic<T> *a,
                                   MemoryOrder ord) {
        return a->load(ord);
    }

    template <typename T>
    inline T atomic_load_explicit(const Atomic<T> *a, MemoryOrder ord) {
        return a->load(ord);
    }

    template <typename T>
    inline T atomic_exchange(volatile Atomic<T> *a, T v) {
        return a->exchange(v);
    }

    template <typename T>
    inline T atomic_exchange(Atomic<T> *a, T v) {
        return a->exchange(v);
    }

    template <typename T>
    inline T atomic_exchange_explicit(volatile Atomic<T> *a, T v,
                                      MemoryOrder ord) {
        return a->exchange(v, ord);
    }

    template <typename T>
    inline T atomic_exchange_explicit(Atomic<T> *a, T v, MemoryOrder ord) {
        return a->exchange(v, ord);
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak(volatile Atomic<T> *a, T *e, T v) {
        return a->compare_exchange_weak(*e, v);
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak(Atomic<T> *a, T *e, T v) {
        return a->compare_exchange_weak(*e, v);
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong(volatile Atomic<T> *a, T *e, T v) {
        return a->compare_exchange_strong(*e, v);
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong(Atomic<T> *a, T *e, T v) {
        return a->compare_exchange_strong(*e, v);
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak_explicit(volatile Atomic<T> *a,
                                                      T *e, T v,
                                                      MemoryOrder s,
                                                      MemoryOrder f) {
        return a->compare_exchange_weak(*e, v, s, f);
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak_explicit(Atomic<T> *a, T *e, T v,
                                                      MemoryOrder s,
                                                      MemoryOrder f) {
        return a->compare_exchange_weak(*e, v, s, f);
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong_explicit(volatile Atomic<T> *a,
                                                        T *e, T v,
                                                        MemoryOrder s,
                                                        MemoryOrder f) {
        return a->compare_exchange_strong(*e, v, s, f);
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong_explicit(Atomic<T> *a, T *e, T v,
                                                        MemoryOrder s,
                                                        MemoryOrder f) {
        return a->compare_exchange_strong(*e, v, s, f);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_add(volatile Atomic<T> *a, T op) {
        return a->fetch_add(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_add(Atomic<T> *a, T op) {
        return a->fetch_add(op);
    }

    template <typename T>
    inline T *atomic_fetch_add(volatile Atomic<T *> *a, ptrdiff_t op) {
        return a->fetch_add(op);
    }

    template <typename T>
    inline T *atomic_fetch_add(Atomic<T *> *a, ptrdiff_t op) {
        return a->fetch_add(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_add_explicit(volatile Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_add_explicit(Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename T>
    inline T *atomic_fetch_add_explicit(volatile Atomic<T *> *a, ptrdiff_t op,
                                        MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename T>
    inline T *atomic_fetch_add_explicit(Atomic<T *> *a, ptrdiff_t op,
                                        MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_sub(volatile Atomic<T> *a, T op) {
        return a->fetch_sub(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_sub(Atomic<T> *a, T op) {
        return a->fetch_sub(op);
    }

    template <typename T>
    inline T *atomic_fetch_sub(volatile Atomic<T *> *a, ptrdiff_t op) {
        return a->fetch_sub(op);
    }

    template <typename T>
    inline T *atomic_fetch_sub(Atomic<T *> *a, ptrdiff_t op) {
        return a->fetch_sub(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_sub_explicit(volatile Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_sub_explicit(Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename T>
    inline T *atomic_fetch_sub_explicit(volatile Atomic<T *> *a, ptrdiff_t op,
                                        MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename T>
    inline T *atomic_fetch_sub_explicit(Atomic<T *> *a, ptrdiff_t op,
                                        MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_and(volatile Atomic<T> *a, T op) {
        return a->fetch_and(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_and(Atomic<T> *a, T op) {
        return a->fetch_and(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_and_explicit(volatile Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_and(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_and_explicit(Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_and(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_or(volatile Atomic<T> *a, T op) {
        return a->fetch_or(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_or(Atomic<T> *a, T op) {
        return a->fetch_or(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_or_explicit(volatile Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_or(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_or_explicit(Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_or(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_xor(volatile Atomic<T> *a, T op) {
        return a->fetch_xor(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_xor(Atomic<T> *a, T op) {
        return a->fetch_xor(op);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_xor_explicit(volatile Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_xor(op, ord);
    }

    template <typename T>
    inline EnableIf<IsIntegral<T>::value && !IsSame<T, bool>::value, T>
    atomic_fetch_xor_explicit(Atomic<T> *a, T op, MemoryOrder ord) {
        return a->fetch_xor(op, ord);
    }

    struct AtomicFlag {
        __OctaAtomicBase<bool> p_a;

        AtomicFlag() = default;

        AtomicFlag(bool b): p_a(b) {}

        AtomicFlag(const AtomicFlag &) = delete;

        AtomicFlag &operator=(const AtomicFlag &) = delete;
        AtomicFlag &operator=(const AtomicFlag &) volatile = delete;

        bool test_and_set(MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_exchange(&p_a, true, ord);
        }

        bool test_and_set(MemoryOrder ord = MemoryOrder::seq_cst) {
            return __octa_atomic_exchange(&p_a, true, ord);
        }

        void clear(MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            __octa_atomic_store(&p_a, false, ord);
        }

        void clear(MemoryOrder ord = MemoryOrder::seq_cst) {
            __octa_atomic_store(&p_a, false, ord);
        }
    };

    inline bool atomic_flag_test_and_set(volatile AtomicFlag *a) {
        return a->test_and_set();
    }

    inline bool atomic_flag_test_and_set(AtomicFlag *a) {
        return a->test_and_set();
    }

    inline bool atomic_flag_test_and_set_explicit(volatile AtomicFlag *a,
                                                  MemoryOrder ord) {
        return a->test_and_set(ord);
    }

    inline bool atomic_flag_test_and_set_explicit(AtomicFlag *a,
                                                  MemoryOrder ord) {
        return a->test_and_set(ord);
    }

    inline void atomic_flag_clear(volatile AtomicFlag *a) {
        a->clear();
    }

    inline void atomic_flag_clear(AtomicFlag *a) {
        a->clear();
    }

    inline void atomic_flag_clear_explicit(volatile AtomicFlag *a,
                                           MemoryOrder ord) {
        a->clear(ord);
    }

    inline void atomic_flag_clear_explicit(AtomicFlag *a, MemoryOrder ord) {
        a->clear(ord);
    }

    inline void atomic_thread_fence(MemoryOrder ord) {
        __octa_atomic_thread_fence(ord);
    }

    inline void atomic_signal_fence(MemoryOrder ord) {
        __octa_atomic_signal_fence(ord);
    }

    typedef Atomic<bool  > AtomicBool;
    typedef Atomic<char  > AtomicChar;
    typedef Atomic<schar > AtomicSchar;
    typedef Atomic<uchar > AtomicUchar;
    typedef Atomic<short > AtomicShort;
    typedef Atomic<ushort> AtomicUshort;
    typedef Atomic<int   > AtomicInt;
    typedef Atomic<uint  > AtomicUint;
    typedef Atomic<long  > AtomicLong;
    typedef Atomic<ulong > AtomicUlong;
    typedef Atomic<llong > AtomicLlong;
    typedef Atomic<ullong> AtomicUllong;

    typedef Atomic<char16_t> AtomicChar16;
    typedef Atomic<char32_t> AtomicChar32;
    typedef Atomic< wchar_t> AtomicWchar;

    typedef Atomic< intptr_t> AtomicIntptr;
    typedef Atomic<uintptr_t> AtomicUintptr;
    typedef Atomic<   size_t> AtomicSize;
    typedef Atomic<ptrdiff_t> AtomicPtrdiff;

#define ATOMIC_FLAG_INIT {false}
#define ATOMIC_VAR_INIT(__v) {__v}

}

#endif