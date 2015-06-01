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

    template<typename _T>
    struct __OctaAtomicBase {
        __OctaAtomicBase() {}
        explicit __OctaAtomicBase(_T __v): __value(__v) {}
        _T __value;
    };

    template<typename _T> _T __octa_atomic_create();

    template<typename _T, typename _U>
    EnableIf<sizeof(_T()->value = __octa_atomic_create<_U>()), char>
    __octa_test_atomic_assignable(int);

    template<typename _T, typename _U>
    int __octa_test_atomic_assignable(...);

    template<typename _T, typename _U>
    struct __OctaCanAtomicAssign {
        static constexpr bool value
            = (sizeof(__octa_test_atomic_assignable<_T, _U>(1)) == sizeof(char));
    };

    template<typename _T>
    static inline EnableIf<
        __OctaCanAtomicAssign<volatile __OctaAtomicBase<_T> *, _T>::value
    > __octa_atomic_init(volatile __OctaAtomicBase<_T> *__a, _T __v) {
        __a->__value = __v;
    }

    template<typename _T>
    static inline EnableIf<
        !__OctaCanAtomicAssign<volatile __OctaAtomicBase<_T> *, _T>::value &&
         __OctaCanAtomicAssign<         __OctaAtomicBase<_T> *, _T>::value
    > __octa_atomic_init(volatile __OctaAtomicBase<_T> *__a, _T __v) {
        volatile char *__to  = (volatile char *)(&__a->__value);
        volatile char *__end = __to + sizeof(_T);
        char *__from = (char *)(&__v);
        while (__to != __end) *__to++ =*__from++;
    }

    template<typename _T>
    static inline void __octa_atomic_init(__OctaAtomicBase<_T> *__a, _T __v) {
        __a->__value = __v;
    }

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

    static inline constexpr int __octa_to_gcc_order(MemoryOrder __ord) {
        return ((__ord == MemoryOrder::relaxed) ? __ATOMIC_RELAXED :
               ((__ord == MemoryOrder::acquire) ? __ATOMIC_ACQUIRE :
               ((__ord == MemoryOrder::release) ? __ATOMIC_RELEASE :
               ((__ord == MemoryOrder::seq_cst) ? __ATOMIC_SEQ_CST :
               ((__ord == MemoryOrder::acq_rel) ? __ATOMIC_ACQ_REL :
                                                __ATOMIC_CONSUME)))));
    }

    static inline constexpr int __octa_to_gcc_failure_order(MemoryOrder __ord) {
        return ((__ord == MemoryOrder::relaxed) ? __ATOMIC_RELAXED :
               ((__ord == MemoryOrder::acquire) ? __ATOMIC_ACQUIRE :
               ((__ord == MemoryOrder::release) ? __ATOMIC_RELAXED :
               ((__ord == MemoryOrder::seq_cst) ? __ATOMIC_SEQ_CST :
               ((__ord == MemoryOrder::acq_rel) ? __ATOMIC_ACQUIRE :
                                                __ATOMIC_CONSUME)))));
    }

    static inline void __octa_atomic_thread_fence(MemoryOrder __ord) {
        __atomic_thread_fence(__octa_to_gcc_order(__ord));
    }

    static inline void __octa_atomic_signal_fence(MemoryOrder __ord) {
        __atomic_signal_fence(__octa_to_gcc_order(__ord));
    }

    static inline bool __octa_atomic_is_lock_free(size_t __size) {
        /* return __atomic_is_lock_free(size, 0); cannot be used on some platforms */
        return __size <= sizeof(void *);
    }

    template<typename _T>
    static inline void __octa_atomic_store(volatile __OctaAtomicBase<_T> *__a,
                                           _T __v, MemoryOrder __ord) {
        __atomic_store(&__a->__value, &__v, __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline void __octa_atomic_store(__OctaAtomicBase<_T> *__a,
                                           _T __v, MemoryOrder __ord) {
        __atomic_store(&__a->__value, &__v, __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_load(volatile __OctaAtomicBase<_T> *__a,
                                       MemoryOrder __ord) {
        _T __r;
        __atomic_load(&__a->__value, &__r, __octa_to_gcc_order(__ord));
        return __r;
    }

    template<typename _T>
    static inline _T __octa_atomic_load(__OctaAtomicBase<_T> *__a,
                                       MemoryOrder __ord) {
        _T __r;
        __atomic_load(&__a->__value, &__r, __octa_to_gcc_order(__ord));
        return __r;
    }

    template<typename _T>
    static inline _T __octa_atomic_exchange(volatile __OctaAtomicBase<_T> *__a,
                                           _T __v, MemoryOrder __ord) {
        _T __r;
        __atomic_exchange(&__a->__value, &__v, &__r, __octa_to_gcc_order(__ord));
        return __r;
    }

    template<typename _T>
    static inline _T __octa_atomic_exchange(__OctaAtomicBase<_T> *__a,
                                           _T __v, MemoryOrder __ord) {
        _T __r;
        __atomic_exchange(&__a->__value, &__v, &__r, __octa_to_gcc_order(__ord));
        return __r;
    }

    template<typename _T>
    static inline bool __octa_atomic_compare_exchange_strong(
        volatile __OctaAtomicBase<_T> *__a, _T *__expected, _T __v,
        MemoryOrder __success, MemoryOrder __failure
    ) {
        return __atomic_compare_exchange(&__a->__value, __expected, &__v, false,
            __octa_to_gcc_order(__success), __octa_to_gcc_failure_order(__failure));
    }

    template<typename _T>
    static inline bool __octa_atomic_compare_exchange_strong(
        __OctaAtomicBase<_T> *__a, _T *__expected, _T __v,
        MemoryOrder __success, MemoryOrder __failure
    ) {
        return __atomic_compare_exchange(&__a->__value, __expected, &__v, false,
            __octa_to_gcc_order(__success), __octa_to_gcc_failure_order(__failure));
    }

    template<typename _T>
    static inline bool __octa_atomic_compare_exchange_weak(
        volatile __OctaAtomicBase<_T> *__a, _T *__expected, _T __v,
        MemoryOrder __success, MemoryOrder __failure
    ) {
        return __atomic_compare_exchange(&__a->__value, __expected, &__v, true,
            __octa_to_gcc_order(__success), __octa_to_gcc_failure_order(__failure));
    }

    template<typename _T>
    static inline bool __octa_atomic_compare_exchange_weak(
        __OctaAtomicBase<_T> *__a, _T *__expected, _T __v,
        MemoryOrder __success, MemoryOrder __failure
    ) {
        return __atomic_compare_exchange(&__a->__value, __expected, &__v, true,
            __octa_to_gcc_order(__success), __octa_to_gcc_failure_order(__failure));
    }

    template<typename _T>
    struct __OctaSkipAmt { static constexpr size_t value = 1; };

    template<typename _T>
    struct __OctaSkipAmt<_T *> { static constexpr size_t value = sizeof(_T); };

    template<typename _T> struct __OctaSkipAmt<_T[]> {};
    template<typename _T, size_t _N> struct __OctaSkipAmt<_T[_N]> {};

    template<typename _T, typename _U>
    static inline _T __octa_atomic_fetch_add(volatile __OctaAtomicBase<_T> *__a,
                                            _U __d, MemoryOrder __ord) {
        return __atomic_fetch_add(&__a->__value, __d * __OctaSkipAmt<_T>::value,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T, typename _U>
    static inline _T __octa_atomic_fetch_add(__OctaAtomicBase<_T> *__a,
                                            _U __d, MemoryOrder __ord) {
        return __atomic_fetch_add(&__a->__value, __d * __OctaSkipAmt<_T>::value,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T, typename _U>
    static inline _T __octa_atomic_fetch_sub(volatile __OctaAtomicBase<_T> *__a,
                                            _U __d, MemoryOrder __ord) {
        return __atomic_fetch_sub(&__a->__value, __d * __OctaSkipAmt<_T>::value,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T, typename _U>
    static inline _T __octa_atomic_fetch_sub(__OctaAtomicBase<_T> *__a,
                                            _U __d, MemoryOrder __ord) {
        return __atomic_fetch_sub(&__a->__value, __d * __OctaSkipAmt<_T>::value,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_fetch_and(volatile __OctaAtomicBase<_T> *__a,
                                            _T __pattern, MemoryOrder __ord) {
        return __atomic_fetch_and(&__a->__value, __pattern,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_fetch_and(__OctaAtomicBase<_T> *__a,
                                            _T __pattern, MemoryOrder __ord) {
        return __atomic_fetch_and(&__a->__value, __pattern,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_fetch_or(volatile __OctaAtomicBase<_T> *__a,
                                            _T __pattern, MemoryOrder __ord) {
        return __atomic_fetch_or(&__a->__value, __pattern,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_fetch_or(__OctaAtomicBase<_T> *__a,
                                            _T __pattern, MemoryOrder __ord) {
        return __atomic_fetch_or(&__a->__value, __pattern,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_fetch_xor(volatile __OctaAtomicBase<_T> *__a,
                                            _T __pattern, MemoryOrder __ord) {
        return __atomic_fetch_xor(&__a->__value, __pattern,
            __octa_to_gcc_order(__ord));
    }

    template<typename _T>
    static inline _T __octa_atomic_fetch_xor(__OctaAtomicBase<_T> *__a,
                                            _T __pattern, MemoryOrder __ord) {
        return __atomic_fetch_xor(&__a->__value, __pattern,
            __octa_to_gcc_order(__ord));
    }
#else
# error Unsupported compiler
#endif

    template <typename _T> inline _T kill_dependency(_T __v) {
        return __v;
    }

    template<typename _T, bool = octa::IsIntegral<_T>::value &&
                                !octa::IsSame<_T, bool>::value>
    struct __OctaAtomic {
        mutable __OctaAtomicBase<_T> __a;

        __OctaAtomic() = default;

        constexpr __OctaAtomic(_T __v): __a(__v) {}

        __OctaAtomic(const __OctaAtomic &) = delete;

        __OctaAtomic &operator=(const __OctaAtomic &) = delete;
        __OctaAtomic &operator=(const __OctaAtomic &) volatile = delete;

        bool is_lock_free() const volatile {
            return __octa_atomic_is_lock_free(sizeof(_T));
        }

        bool is_lock_free() const {
            return __octa_atomic_is_lock_free(sizeof(_T));
        }

        void store(_T __v, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            __octa_atomic_store(&__a, __v, __ord);
        }

        void store(_T __v, MemoryOrder __ord = MemoryOrder::seq_cst) {
            __octa_atomic_store(&__a, __v, __ord);
        }

        _T load(MemoryOrder __ord = MemoryOrder::seq_cst) const volatile {
            return __octa_atomic_load(&__a, __ord);
        }

        _T load(MemoryOrder __ord = MemoryOrder::seq_cst) const {
            return __octa_atomic_load(&__a, __ord);
        }

        operator _T() const volatile { return load(); }
        operator _T() const          { return load(); }

        _T exchange(_T __v, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_exchange(&__a, __v, __ord);
        }

        _T exchange(_T __v, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_exchange(&__a, __v, __ord);
        }

        bool compare_exchange_weak(_T &__e, _T __v, MemoryOrder __s,
                                   MemoryOrder __f) volatile {
            return __octa_atomic_compare_exchange_weak(&__a, &__e, __v,
                __s, __f);
        }

        bool compare_exchange_weak(_T &__e, _T __v, MemoryOrder __s,
                                   MemoryOrder __f) {
            return __octa_atomic_compare_exchange_weak(&__a, &__e, __v,
                __s, __f);
        }

        bool compare_exchange_strong(_T &__e, _T __v, MemoryOrder __s,
                                     MemoryOrder __f) volatile {
            return __octa_atomic_compare_exchange_strong(&__a, &__e, __v,
                __s, __f);
        }

        bool compare_exchange_strong(_T &__e, _T __v, MemoryOrder __s,
                                     MemoryOrder __f) {
            return __octa_atomic_compare_exchange_strong(&__a, &__e, __v,
                __s, __f);
        }

        bool compare_exchange_weak(_T &__e, _T __v, MemoryOrder __ord
                                                  = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_compare_exchange_weak(&__a, &__e, __v,
                __ord, __ord);
        }

        bool compare_exchange_weak(_T &__e, _T __v, MemoryOrder __ord
                                                  = MemoryOrder::seq_cst) {
            return __octa_atomic_compare_exchange_weak(&__a, &__e, __v,
                __ord, __ord);
        }

        bool compare_exchange_strong(_T &__e, _T __v, MemoryOrder __ord
                                                    = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_compare_exchange_strong(&__a, &__e,
                __v, __ord, __ord);
        }

        bool compare_exchange_strong(_T &__e, _T __v, MemoryOrder __ord
                                                    = MemoryOrder::seq_cst) {
            return __octa_atomic_compare_exchange_strong(&__a, &__e, __v,
                __ord, __ord);
        }
    };

    template<typename _T>
    struct __OctaAtomic<_T, true>: __OctaAtomic<_T, false> {
        typedef __OctaAtomic<_T, false> _base_t;

        __OctaAtomic() = default;

        constexpr __OctaAtomic(_T __v): _base_t(__v) {}

        _T fetch_add(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_add(&this->__a, __op, __ord);
        }

        _T fetch_add(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_add(&this->__a, __op, __ord);
        }

        _T fetch_sub(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_sub(&this->__a, __op, __ord);
        }

        _T fetch_sub(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_sub(&this->__a, __op, __ord);
        }

        _T fetch_and(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_and(&this->__a, __op, __ord);
        }

        _T fetch_and(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_and(&this->__a, __op, __ord);
        }

        _T fetch_or(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_or(&this->__a, __op, __ord);
        }

        _T fetch_or(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_or(&this->__a, __op, __ord);
        }

        _T fetch_xor(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_fetch_xor(&this->__a, __op, __ord);
        }

        _T fetch_xor(_T __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_xor(&this->__a, __op, __ord);
        }

        _T operator++(int) volatile { return fetch_add(_T(1));         }
        _T operator++(int)          { return fetch_add(_T(1));         }
        _T operator--(int) volatile { return fetch_sub(_T(1));         }
        _T operator--(int)          { return fetch_sub(_T(1));         }
        _T operator++(   ) volatile { return fetch_add(_T(1)) + _T(1); }
        _T operator++(   )          { return fetch_add(_T(1)) + _T(1); }
        _T operator--(   ) volatile { return fetch_sub(_T(1)) - _T(1); }
        _T operator--(   )          { return fetch_sub(_T(1)) - _T(1); }

        _T operator+=(_T __op) volatile { return fetch_add(__op) + __op; }
        _T operator+=(_T __op)          { return fetch_add(__op) + __op; }
        _T operator-=(_T __op) volatile { return fetch_sub(__op) - __op; }
        _T operator-=(_T __op)          { return fetch_sub(__op) - __op; }
        _T operator&=(_T __op) volatile { return fetch_and(__op) & __op; }
        _T operator&=(_T __op)          { return fetch_and(__op) & __op; }
        _T operator|=(_T __op) volatile { return fetch_or (__op) | __op; }
        _T operator|=(_T __op)          { return fetch_or (__op) | __op; }
        _T operator^=(_T __op) volatile { return fetch_xor(__op) ^ __op; }
        _T operator^=(_T __op)          { return fetch_xor(__op) ^ __op; }
    };

    template<typename _T>
    struct Atomic: __OctaAtomic<_T> {
        typedef __OctaAtomic<_T> _base_t;

        Atomic() = default;

        constexpr Atomic(_T __v): _base_t(__v) {}

        _T operator=(_T __v) volatile {
            _base_t::store(__v); return __v;
        }

        _T operator=(_T __v) {
            _base_t::store(__v); return __v;
        }
    };

    template<typename _T>
    struct Atomic<_T *>: __OctaAtomic<_T *> {
        typedef __OctaAtomic<_T *> _base_t;

        Atomic() = default;

        constexpr Atomic(_T *__v): _base_t(__v) {}

        _T *operator=(_T *__v) volatile {
            _base_t::store(__v); return __v;
        }

        _T *operator=(_T *__v) {
            _base_t::store(__v); return __v;
        }

        _T *fetch_add(ptrdiff_t __op, MemoryOrder __ord = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_fetch_add(&this->__a, __op, __ord);
        }

        _T *fetch_add(ptrdiff_t __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_add(&this->__a, __op, __ord);
        }

        _T *fetch_sub(ptrdiff_t __op, MemoryOrder __ord = MemoryOrder::seq_cst)
        volatile {
            return __octa_atomic_fetch_sub(&this->__a, __op, __ord);
        }

        _T *fetch_sub(ptrdiff_t __op, MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_fetch_sub(&this->__a, __op, __ord);
        }


        _T *operator++(int) volatile { return fetch_add(1);     }
        _T *operator++(int)          { return fetch_add(1);     }
        _T *operator--(int) volatile { return fetch_sub(1);     }
        _T *operator--(int)          { return fetch_sub(1);     }
        _T *operator++(   ) volatile { return fetch_add(1) + 1; }
        _T *operator++(   )          { return fetch_add(1) + 1; }
        _T *operator--(   ) volatile { return fetch_sub(1) - 1; }
        _T *operator--(   )          { return fetch_sub(1) - 1; }

        _T *operator+=(ptrdiff_t __op) volatile { return fetch_add(__op) + __op; }
        _T *operator+=(ptrdiff_t __op)          { return fetch_add(__op) + __op; }
        _T *operator-=(ptrdiff_t __op) volatile { return fetch_sub(__op) - __op; }
        _T *operator-=(ptrdiff_t __op)          { return fetch_sub(__op) - __op; }
    };

    template<typename _T>
    inline bool atomic_is_lock_free(const volatile Atomic<_T> *__a) {
        return __a->is_lock_free();
    }

    template<typename _T>
    inline bool atomic_is_lock_free(const Atomic<_T> *__a) {
        return __a->is_lock_free();
    }

    template<typename _T>
    inline void atomic_init(volatile Atomic<_T> *__a, _T __v) {
        __octa_atomic_init(&__a->__a, __v);
    }

    template<typename _T>
    inline void atomic_init(Atomic<_T> *__a, _T __v) {
        __octa_atomic_init(&__a->__a, __v);
    }

    template <typename _T>
    inline void atomic_store(volatile Atomic<_T> *__a, _T __v) {
        __a->store(__v);
    }

    template <typename _T>
    inline void atomic_store(Atomic<_T> *__a, _T __v) {
        __a->store(__v);
    }

    template <typename _T>
    inline void atomic_store_explicit(volatile Atomic<_T> *__a, _T __v,
                                      MemoryOrder __ord) {
        __a->store(__v, __ord);
    }

    template <typename _T>
    inline void atomic_store_explicit(Atomic<_T> *__a, _T __v,
                                      MemoryOrder __ord) {
        __a->store(__v, __ord);
    }

    template <typename _T>
    inline _T atomic_load(const volatile Atomic<_T> *__a) {
        return __a->load();
    }

    template <typename _T>
    inline _T atomic_load(const Atomic<_T> *__a) {
        return __a->load();
    }

    template <typename _T>
    inline _T atomic_load_explicit(const volatile Atomic<_T> *__a,
                                   MemoryOrder __ord) {
        return __a->load(__ord);
    }

    template <typename _T>
    inline _T atomic_load_explicit(const Atomic<_T> *__a, MemoryOrder __ord) {
        return __a->load(__ord);
    }

    template <typename _T>
    inline _T atomic_exchange(volatile Atomic<_T> *__a, _T __v) {
        return __a->exchange(__v);
    }

    template <typename _T>
    inline _T atomic_exchange(Atomic<_T> *__a, _T __v) {
        return __a->exchange(__v);
    }

    template <typename _T>
    inline _T atomic_exchange_explicit(volatile Atomic<_T> *__a, _T __v,
                                      MemoryOrder __ord) {
        return __a->exchange(__v, __ord);
    }

    template <typename _T>
    inline _T atomic_exchange_explicit(Atomic<_T> *__a, _T __v,
                                       MemoryOrder __ord) {
        return __a->exchange(__v, __ord);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak(volatile Atomic<_T> *__a,
                                             _T *__e, _T __v) {
        return __a->compare_exchange_weak(*__e, __v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak(Atomic<_T> *__a, _T *__e, _T __v) {
        return __a->compare_exchange_weak(*__e, __v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong(volatile Atomic<_T> *__a,
                                               _T *__e, _T __v) {
        return __a->compare_exchange_strong(*__e, __v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong(Atomic<_T> *__a, _T *__e, _T __v) {
        return __a->compare_exchange_strong(*__e, __v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak_explicit(volatile Atomic<_T> *__a,
                                                      _T *__e, _T __v,
                                                      MemoryOrder __s,
                                                      MemoryOrder __f) {
        return __a->compare_exchange_weak(*__e, __v, __s, __f);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak_explicit(Atomic<_T> *__a, _T *__e,
                                                      _T __v,
                                                      MemoryOrder __s,
                                                      MemoryOrder __f) {
        return __a->compare_exchange_weak(*__e, __v, __s, __f);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong_explicit(volatile Atomic<_T> *__a,
                                                        _T *__e, _T __v,
                                                        MemoryOrder __s,
                                                        MemoryOrder __f) {
        return __a->compare_exchange_strong(*__e, __v, __s, __f);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong_explicit(Atomic<_T> *__a, _T *__e,
                                                        _T __v,
                                                        MemoryOrder __s,
                                                        MemoryOrder __f) {
        return __a->compare_exchange_strong(*__e, __v, __s, __f);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add(volatile Atomic<_T> *__a, _T __op) {
        return __a->fetch_add(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add(Atomic<_T> *__a, _T __op) {
        return __a->fetch_add(__op);
    }

    template <typename _T>
    inline _T *atomic_fetch_add(volatile Atomic<_T *> *__a, ptrdiff_t __op) {
        return __a->fetch_add(__op);
    }

    template <typename _T>
    inline _T *atomic_fetch_add(Atomic<_T *> *__a, ptrdiff_t __op) {
        return __a->fetch_add(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add_explicit(volatile Atomic<_T> *__a, _T __op,
                              MemoryOrder __ord) {
        return __a->fetch_add(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add_explicit(Atomic<_T> *__a, _T __op, MemoryOrder __ord) {
        return __a->fetch_add(__op, __ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_add_explicit(volatile Atomic<_T *> *__a,
                                        ptrdiff_t __op,
                                        MemoryOrder __ord) {
        return __a->fetch_add(__op, __ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_add_explicit(Atomic<_T *> *__a, ptrdiff_t __op,
                                        MemoryOrder __ord) {
        return __a->fetch_add(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub(volatile Atomic<_T> *__a, _T __op) {
        return __a->fetch_sub(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub(Atomic<_T> *__a, _T __op) {
        return __a->fetch_sub(__op);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub(volatile Atomic<_T *> *__a, ptrdiff_t __op) {
        return __a->fetch_sub(__op);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub(Atomic<_T *> *__a, ptrdiff_t __op) {
        return __a->fetch_sub(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub_explicit(volatile Atomic<_T> *__a, _T __op,
                              MemoryOrder __ord) {
        return __a->fetch_sub(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub_explicit(Atomic<_T> *__a, _T __op, MemoryOrder __ord) {
        return __a->fetch_sub(__op, __ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub_explicit(volatile Atomic<_T *> *__a,
                                        ptrdiff_t __op,
                                        MemoryOrder __ord) {
        return __a->fetch_sub(__op, __ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub_explicit(Atomic<_T *> *__a, ptrdiff_t __op,
                                        MemoryOrder __ord) {
        return __a->fetch_sub(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and(volatile Atomic<_T> *__a, _T __op) {
        return __a->fetch_and(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and(Atomic<_T> *__a, _T __op) {
        return __a->fetch_and(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and_explicit(volatile Atomic<_T> *__a, _T __op,
                              MemoryOrder __ord) {
        return __a->fetch_and(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and_explicit(Atomic<_T> *__a, _T __op, MemoryOrder __ord) {
        return __a->fetch_and(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or(volatile Atomic<_T> *__a, _T __op) {
        return __a->fetch_or(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or(Atomic<_T> *__a, _T __op) {
        return __a->fetch_or(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or_explicit(volatile Atomic<_T> *__a, _T __op,
                             MemoryOrder __ord) {
        return __a->fetch_or(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or_explicit(Atomic<_T> *__a, _T __op, MemoryOrder __ord) {
        return __a->fetch_or(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor(volatile Atomic<_T> *__a, _T __op) {
        return __a->fetch_xor(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor(Atomic<_T> *__a, _T __op) {
        return __a->fetch_xor(__op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor_explicit(volatile Atomic<_T> *__a, _T __op,
                              MemoryOrder __ord) {
        return __a->fetch_xor(__op, __ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor_explicit(Atomic<_T> *__a, _T __op, MemoryOrder __ord) {
        return __a->fetch_xor(__op, __ord);
    }

    struct AtomicFlag {
        __OctaAtomicBase<bool> __a;

        AtomicFlag() = default;

        AtomicFlag(bool __b): __a(__b) {}

        AtomicFlag(const AtomicFlag &) = delete;

        AtomicFlag &operator=(const AtomicFlag &) = delete;
        AtomicFlag &operator=(const AtomicFlag &) volatile = delete;

        bool test_and_set(MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            return __octa_atomic_exchange(&__a, true, __ord);
        }

        bool test_and_set(MemoryOrder __ord = MemoryOrder::seq_cst) {
            return __octa_atomic_exchange(&__a, true, __ord);
        }

        void clear(MemoryOrder __ord = MemoryOrder::seq_cst) volatile {
            __octa_atomic_store(&__a, false, __ord);
        }

        void clear(MemoryOrder __ord = MemoryOrder::seq_cst) {
            __octa_atomic_store(&__a, false, __ord);
        }
    };

    inline bool atomic_flag_test_and_set(volatile AtomicFlag *__a) {
        return __a->test_and_set();
    }

    inline bool atomic_flag_test_and_set(AtomicFlag *__a) {
        return __a->test_and_set();
    }

    inline bool atomic_flag_test_and_set_explicit(volatile AtomicFlag *__a,
                                                  MemoryOrder __ord) {
        return __a->test_and_set(__ord);
    }

    inline bool atomic_flag_test_and_set_explicit(AtomicFlag *__a,
                                                  MemoryOrder __ord) {
        return __a->test_and_set(__ord);
    }

    inline void atomic_flag_clear(volatile AtomicFlag *__a) {
        __a->clear();
    }

    inline void atomic_flag_clear(AtomicFlag *__a) {
        __a->clear();
    }

    inline void atomic_flag_clear_explicit(volatile AtomicFlag *__a,
                                           MemoryOrder __ord) {
        __a->clear(__ord);
    }

    inline void atomic_flag_clear_explicit(AtomicFlag *__a, MemoryOrder __ord) {
        __a->clear(__ord);
    }

    inline void atomic_thread_fence(MemoryOrder __ord) {
        __octa_atomic_thread_fence(__ord);
    }

    inline void atomic_signal_fence(MemoryOrder __ord) {
        __octa_atomic_signal_fence(__ord);
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