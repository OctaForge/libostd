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

namespace detail {
    template<typename _T>
    struct AtomicBase {
        AtomicBase() {}
        explicit AtomicBase(_T v): p_value(v) {}
        _T p_value;
    };

    template<typename _T> _T atomic_create();

    template<typename _T, typename _U>
    EnableIf<sizeof(_T()->value = atomic_create<_U>()), char>
    test_atomic_assignable(int);

    template<typename _T, typename _U>
    int test_atomic_assignable(...);

    template<typename _T, typename _U>
    struct CanAtomicAssign {
        static constexpr bool value
            = (sizeof(test_atomic_assignable<_T, _U>(1)) == sizeof(char));
    };

    template<typename _T>
    static inline EnableIf<
        CanAtomicAssign<volatile AtomicBase<_T> *, _T>::value
    > atomic_init(volatile AtomicBase<_T> *a, _T v) {
        a->p_value = v;
    }

    template<typename _T>
    static inline EnableIf<
        !CanAtomicAssign<volatile AtomicBase<_T> *, _T>::value &&
         CanAtomicAssign<         AtomicBase<_T> *, _T>::value
    > atomic_init(volatile AtomicBase<_T> *a, _T v) {
        volatile char *to  = (volatile char *)(&a->p_value);
        volatile char *end = to + sizeof(_T);
        char *from = (char *)(&v);
        while (to != end) *to++ =*from++;
    }

    template<typename _T>
    static inline void atomic_init(AtomicBase<_T> *a, _T v) {
        a->p_value = v;
    }
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

namespace detail {
    static inline constexpr int to_gcc_order(MemoryOrder ord) {
        return ((ord == MemoryOrder::relaxed) ? __ATOMIC_RELAXED :
               ((ord == MemoryOrder::acquire) ? __ATOMIC_ACQUIRE :
               ((ord == MemoryOrder::release) ? __ATOMIC_RELEASE :
               ((ord == MemoryOrder::seq_cst) ? __ATOMIC_SEQ_CST :
               ((ord == MemoryOrder::acq_rel) ? __ATOMIC_ACQ_REL :
                                                __ATOMIC_CONSUME)))));
    }

    static inline constexpr int to_gcc_failure_order(MemoryOrder ord) {
        return ((ord == MemoryOrder::relaxed) ? __ATOMIC_RELAXED :
               ((ord == MemoryOrder::acquire) ? __ATOMIC_ACQUIRE :
               ((ord == MemoryOrder::release) ? __ATOMIC_RELAXED :
               ((ord == MemoryOrder::seq_cst) ? __ATOMIC_SEQ_CST :
               ((ord == MemoryOrder::acq_rel) ? __ATOMIC_ACQUIRE :
                                                __ATOMIC_CONSUME)))));
    }

    static inline void atomic_thread_fence(MemoryOrder ord) {
        __atomic_thread_fence(to_gcc_order(ord));
    }

    static inline void atomic_signal_fence(MemoryOrder ord) {
        __atomic_signal_fence(to_gcc_order(ord));
    }

    static inline bool atomic_is_lock_free(size_t size) {
        /* return __atomic_is_lock_free(size, 0); cannot be used on some platforms */
        return size <= sizeof(void *);
    }

    template<typename _T>
    static inline void atomic_store(volatile AtomicBase<_T> *a,
                                           _T v, MemoryOrder ord) {
        __atomic_store(&a->p_value, &v, to_gcc_order(ord));
    }

    template<typename _T>
    static inline void atomic_store(AtomicBase<_T> *a,
                                           _T v, MemoryOrder ord) {
        __atomic_store(&a->p_value, &v, to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_load(volatile AtomicBase<_T> *a,
                                       MemoryOrder ord) {
        _T r;
        __atomic_load(&a->p_value, &r, to_gcc_order(ord));
        return r;
    }

    template<typename _T>
    static inline _T atomic_load(AtomicBase<_T> *a,
                                       MemoryOrder ord) {
        _T r;
        __atomic_load(&a->p_value, &r, to_gcc_order(ord));
        return r;
    }

    template<typename _T>
    static inline _T atomic_exchange(volatile AtomicBase<_T> *a,
                                           _T v, MemoryOrder ord) {
        _T r;
        __atomic_exchange(&a->p_value, &v, &r, to_gcc_order(ord));
        return r;
    }

    template<typename _T>
    static inline _T atomic_exchange(AtomicBase<_T> *a,
                                           _T v, MemoryOrder ord) {
        _T r;
        __atomic_exchange(&a->p_value, &v, &r, to_gcc_order(ord));
        return r;
    }

    template<typename _T>
    static inline bool atomic_compare_exchange_strong(
        volatile AtomicBase<_T> *a, _T *expected, _T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->p_value, expected, &v, false,
            to_gcc_order(success), to_gcc_failure_order(failure));
    }

    template<typename _T>
    static inline bool atomic_compare_exchange_strong(
        AtomicBase<_T> *a, _T *expected, _T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->p_value, expected, &v, false,
            to_gcc_order(success), to_gcc_failure_order(failure));
    }

    template<typename _T>
    static inline bool atomic_compare_exchange_weak(
        volatile AtomicBase<_T> *a, _T *expected, _T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->p_value, expected, &v, true,
            to_gcc_order(success), to_gcc_failure_order(failure));
    }

    template<typename _T>
    static inline bool atomic_compare_exchange_weak(
        AtomicBase<_T> *a, _T *expected, _T v,
        MemoryOrder success, MemoryOrder failure
    ) {
        return __atomic_compare_exchange(&a->p_value, expected, &v, true,
            to_gcc_order(success), to_gcc_failure_order(failure));
    }

    template<typename _T>
    struct SkipAmt { static constexpr size_t value = 1; };

    template<typename _T>
    struct SkipAmt<_T *> { static constexpr size_t value = sizeof(_T); };

    template<typename _T> struct SkipAmt<_T[]> {};
    template<typename _T, size_t _N> struct SkipAmt<_T[_N]> {};

    template<typename _T, typename _U>
    static inline _T atomic_fetch_add(volatile AtomicBase<_T> *a,
                                            _U d, MemoryOrder ord) {
        return __atomic_fetch_add(&a->p_value, d * SkipAmt<_T>::value,
            to_gcc_order(ord));
    }

    template<typename _T, typename _U>
    static inline _T atomic_fetch_add(AtomicBase<_T> *a,
                                            _U d, MemoryOrder ord) {
        return __atomic_fetch_add(&a->p_value, d * SkipAmt<_T>::value,
            to_gcc_order(ord));
    }

    template<typename _T, typename _U>
    static inline _T atomic_fetch_sub(volatile AtomicBase<_T> *a,
                                            _U d, MemoryOrder ord) {
        return __atomic_fetch_sub(&a->p_value, d * SkipAmt<_T>::value,
            to_gcc_order(ord));
    }

    template<typename _T, typename _U>
    static inline _T atomic_fetch_sub(AtomicBase<_T> *a,
                                            _U d, MemoryOrder ord) {
        return __atomic_fetch_sub(&a->p_value, d * SkipAmt<_T>::value,
            to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_fetch_and(volatile AtomicBase<_T> *a,
                                            _T pattern, MemoryOrder ord) {
        return __atomic_fetch_and(&a->p_value, pattern,
            to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_fetch_and(AtomicBase<_T> *a,
                                            _T pattern, MemoryOrder ord) {
        return __atomic_fetch_and(&a->p_value, pattern,
            to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_fetch_or(volatile AtomicBase<_T> *a,
                                            _T pattern, MemoryOrder ord) {
        return __atomic_fetch_or(&a->p_value, pattern,
            to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_fetch_or(AtomicBase<_T> *a,
                                            _T pattern, MemoryOrder ord) {
        return __atomic_fetch_or(&a->p_value, pattern,
            to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_fetch_xor(volatile AtomicBase<_T> *a,
                                            _T pattern, MemoryOrder ord) {
        return __atomic_fetch_xor(&a->p_value, pattern,
            to_gcc_order(ord));
    }

    template<typename _T>
    static inline _T atomic_fetch_xor(AtomicBase<_T> *a,
                                            _T pattern, MemoryOrder ord) {
        return __atomic_fetch_xor(&a->p_value, pattern,
            to_gcc_order(ord));
    }
} /* namespace detail */
#else
# error Unsupported compiler
#endif

template <typename _T> inline _T kill_dependency(_T v) {
    return v;
}

namespace detail {
    template<typename _T, bool = octa::IsIntegral<_T>::value &&
                                !octa::IsSame<_T, bool>::value>
    struct Atomic {
        mutable AtomicBase<_T> p_a;

        Atomic() = default;

        constexpr Atomic(_T v): p_a(v) {}

        Atomic(const Atomic &) = delete;

        Atomic &operator=(const Atomic &) = delete;
        Atomic &operator=(const Atomic &) volatile = delete;

        bool is_lock_free() const volatile {
            return atomic_is_lock_free(sizeof(_T));
        }

        bool is_lock_free() const {
            return atomic_is_lock_free(sizeof(_T));
        }

        void store(_T v, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            atomic_store(&p_a, v, ord);
        }

        void store(_T v, MemoryOrder ord = MemoryOrder::seq_cst) {
            atomic_store(&p_a, v, ord);
        }

        _T load(MemoryOrder ord = MemoryOrder::seq_cst) const volatile {
            return atomic_load(&p_a, ord);
        }

        _T load(MemoryOrder ord = MemoryOrder::seq_cst) const {
            return atomic_load(&p_a, ord);
        }

        operator _T() const volatile { return load(); }
        operator _T() const          { return load(); }

        _T exchange(_T v, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return atomic_exchange(&p_a, v, ord);
        }

        _T exchange(_T v, MemoryOrder ord = MemoryOrder::seq_cst) {
            return atomic_exchange(&p_a, v, ord);
        }

        bool compare_exchange_weak(_T &e, _T v, MemoryOrder s,
                                   MemoryOrder f) volatile {
            return atomic_compare_exchange_weak(&p_a, &e, v, s, f);
        }

        bool compare_exchange_weak(_T &e, _T v, MemoryOrder s,
                                   MemoryOrder f) {
            return atomic_compare_exchange_weak(&p_a, &e, v, s, f);
        }

        bool compare_exchange_strong(_T &e, _T v, MemoryOrder s,
                                     MemoryOrder f) volatile {
            return atomic_compare_exchange_strong(&p_a, &e, v, s, f);
        }

        bool compare_exchange_strong(_T &e, _T v, MemoryOrder s,
                                     MemoryOrder f) {
            return atomic_compare_exchange_strong(&p_a, &e, v, s, f);
        }

        bool compare_exchange_weak(_T &e, _T v, MemoryOrder ord
                                                  = MemoryOrder::seq_cst)
        volatile {
            return atomic_compare_exchange_weak(&p_a, &e, v, ord, ord);
        }

        bool compare_exchange_weak(_T &e, _T v, MemoryOrder ord
                                                  = MemoryOrder::seq_cst) {
            return atomic_compare_exchange_weak(&p_a, &e, v, ord, ord);
        }

        bool compare_exchange_strong(_T &e, _T v, MemoryOrder ord
                                                    = MemoryOrder::seq_cst)
        volatile {
            return atomic_compare_exchange_strong(&p_a, &e, v, ord, ord);
        }

        bool compare_exchange_strong(_T &e, _T v, MemoryOrder ord
                                                    = MemoryOrder::seq_cst) {
            return atomic_compare_exchange_strong(&p_a, &e, v, ord, ord);
        }
    };

    template<typename _T>
    struct Atomic<_T, true>: Atomic<_T, false> {
        typedef Atomic<_T, false> _base_t;

        Atomic() = default;

        constexpr Atomic(_T v): _base_t(v) {}

        _T fetch_add(_T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return atomic_fetch_add(&this->p_a, op, ord);
        }

        _T fetch_add(_T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return atomic_fetch_add(&this->p_a, op, ord);
        }

        _T fetch_sub(_T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return atomic_fetch_sub(&this->p_a, op, ord);
        }

        _T fetch_sub(_T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return atomic_fetch_sub(&this->p_a, op, ord);
        }

        _T fetch_and(_T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return atomic_fetch_and(&this->p_a, op, ord);
        }

        _T fetch_and(_T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return atomic_fetch_and(&this->p_a, op, ord);
        }

        _T fetch_or(_T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return atomic_fetch_or(&this->p_a, op, ord);
        }

        _T fetch_or(_T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return atomic_fetch_or(&this->p_a, op, ord);
        }

        _T fetch_xor(_T op, MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return atomic_fetch_xor(&this->p_a, op, ord);
        }

        _T fetch_xor(_T op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return atomic_fetch_xor(&this->p_a, op, ord);
        }

        _T operator++(int) volatile { return fetch_add(_T(1));         }
        _T operator++(int)          { return fetch_add(_T(1));         }
        _T operator--(int) volatile { return fetch_sub(_T(1));         }
        _T operator--(int)          { return fetch_sub(_T(1));         }
        _T operator++(   ) volatile { return fetch_add(_T(1)) + _T(1); }
        _T operator++(   )          { return fetch_add(_T(1)) + _T(1); }
        _T operator--(   ) volatile { return fetch_sub(_T(1)) - _T(1); }
        _T operator--(   )          { return fetch_sub(_T(1)) - _T(1); }

        _T operator+=(_T op) volatile { return fetch_add(op) + op; }
        _T operator+=(_T op)          { return fetch_add(op) + op; }
        _T operator-=(_T op) volatile { return fetch_sub(op) - op; }
        _T operator-=(_T op)          { return fetch_sub(op) - op; }
        _T operator&=(_T op) volatile { return fetch_and(op) & op; }
        _T operator&=(_T op)          { return fetch_and(op) & op; }
        _T operator|=(_T op) volatile { return fetch_or (op) | op; }
        _T operator|=(_T op)          { return fetch_or (op) | op; }
        _T operator^=(_T op) volatile { return fetch_xor(op) ^ op; }
        _T operator^=(_T op)          { return fetch_xor(op) ^ op; }
    };
}

    template<typename _T>
    struct Atomic: octa::detail::Atomic<_T> {
        typedef octa::detail::Atomic<_T> _base_t;

        Atomic() = default;

        constexpr Atomic(_T v): _base_t(v) {}

        _T operator=(_T v) volatile {
            _base_t::store(v); return v;
        }

        _T operator=(_T v) {
            _base_t::store(v); return v;
        }
    };

    template<typename _T>
    struct Atomic<_T *>: octa::detail::Atomic<_T *> {
        typedef octa::detail::Atomic<_T *> _base_t;

        Atomic() = default;

        constexpr Atomic(_T *v): _base_t(v) {}

        _T *operator=(_T *v) volatile {
            _base_t::store(v); return v;
        }

        _T *operator=(_T *v) {
            _base_t::store(v); return v;
        }

        _T *fetch_add(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst)
        volatile {
            return octa::detail::atomic_fetch_add(&this->p_a, op, ord);
        }

        _T *fetch_add(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return octa::detail::atomic_fetch_add(&this->p_a, op, ord);
        }

        _T *fetch_sub(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst)
        volatile {
            return octa::detail::atomic_fetch_sub(&this->p_a, op, ord);
        }

        _T *fetch_sub(ptrdiff_t op, MemoryOrder ord = MemoryOrder::seq_cst) {
            return octa::detail::atomic_fetch_sub(&this->p_a, op, ord);
        }


        _T *operator++(int) volatile { return fetch_add(1);     }
        _T *operator++(int)          { return fetch_add(1);     }
        _T *operator--(int) volatile { return fetch_sub(1);     }
        _T *operator--(int)          { return fetch_sub(1);     }
        _T *operator++(   ) volatile { return fetch_add(1) + 1; }
        _T *operator++(   )          { return fetch_add(1) + 1; }
        _T *operator--(   ) volatile { return fetch_sub(1) - 1; }
        _T *operator--(   )          { return fetch_sub(1) - 1; }

        _T *operator+=(ptrdiff_t op) volatile { return fetch_add(op) + op; }
        _T *operator+=(ptrdiff_t op)          { return fetch_add(op) + op; }
        _T *operator-=(ptrdiff_t op) volatile { return fetch_sub(op) - op; }
        _T *operator-=(ptrdiff_t op)          { return fetch_sub(op) - op; }
    };

    template<typename _T>
    inline bool atomic_is_lock_free(const volatile Atomic<_T> *a) {
        return a->is_lock_free();
    }

    template<typename _T>
    inline bool atomic_is_lock_free(const Atomic<_T> *a) {
        return a->is_lock_free();
    }

    template<typename _T>
    inline void atomic_init(volatile Atomic<_T> *a, _T v) {
        octa::detail::atomic_init(&a->p_a, v);
    }

    template<typename _T>
    inline void atomic_init(Atomic<_T> *a, _T v) {
        octa::detail::atomic_init(&a->p_a, v);
    }

    template <typename _T>
    inline void atomic_store(volatile Atomic<_T> *a, _T v) {
        a->store(v);
    }

    template <typename _T>
    inline void atomic_store(Atomic<_T> *a, _T v) {
        a->store(v);
    }

    template <typename _T>
    inline void atomic_store_explicit(volatile Atomic<_T> *a, _T v,
                                      MemoryOrder ord) {
        a->store(v, ord);
    }

    template <typename _T>
    inline void atomic_store_explicit(Atomic<_T> *a, _T v,
                                      MemoryOrder ord) {
        a->store(v, ord);
    }

    template <typename _T>
    inline _T atomic_load(const volatile Atomic<_T> *a) {
        return a->load();
    }

    template <typename _T>
    inline _T atomic_load(const Atomic<_T> *a) {
        return a->load();
    }

    template <typename _T>
    inline _T atomic_load_explicit(const volatile Atomic<_T> *a,
                                   MemoryOrder ord) {
        return a->load(ord);
    }

    template <typename _T>
    inline _T atomic_load_explicit(const Atomic<_T> *a, MemoryOrder ord) {
        return a->load(ord);
    }

    template <typename _T>
    inline _T atomic_exchange(volatile Atomic<_T> *a, _T v) {
        return a->exchange(v);
    }

    template <typename _T>
    inline _T atomic_exchange(Atomic<_T> *a, _T v) {
        return a->exchange(v);
    }

    template <typename _T>
    inline _T atomic_exchange_explicit(volatile Atomic<_T> *a, _T v,
                                      MemoryOrder ord) {
        return a->exchange(v, ord);
    }

    template <typename _T>
    inline _T atomic_exchange_explicit(Atomic<_T> *a, _T v,
                                       MemoryOrder ord) {
        return a->exchange(v, ord);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak(volatile Atomic<_T> *a,
                                             _T *e, _T v) {
        return a->compare_exchange_weak(*e, v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak(Atomic<_T> *a, _T *e, _T v) {
        return a->compare_exchange_weak(*e, v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong(volatile Atomic<_T> *a,
                                               _T *e, _T v) {
        return a->compare_exchange_strong(*e, v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong(Atomic<_T> *a, _T *e, _T v) {
        return a->compare_exchange_strong(*e, v);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak_explicit(volatile Atomic<_T> *a,
                                                      _T *e, _T v,
                                                      MemoryOrder s,
                                                      MemoryOrder f) {
        return a->compare_exchange_weak(*e, v, s, f);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_weak_explicit(Atomic<_T> *a, _T *e,
                                                      _T v,
                                                      MemoryOrder s,
                                                      MemoryOrder f) {
        return a->compare_exchange_weak(*e, v, s, f);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong_explicit(volatile Atomic<_T> *a,
                                                        _T *e, _T v,
                                                        MemoryOrder s,
                                                        MemoryOrder f) {
        return a->compare_exchange_strong(*e, v, s, f);
    }

    template <typename _T>
    inline bool atomic_compare_exchange_strong_explicit(Atomic<_T> *a, _T *e,
                                                        _T v,
                                                        MemoryOrder s,
                                                        MemoryOrder f) {
        return a->compare_exchange_strong(*e, v, s, f);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add(volatile Atomic<_T> *a, _T op) {
        return a->fetch_add(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add(Atomic<_T> *a, _T op) {
        return a->fetch_add(op);
    }

    template <typename _T>
    inline _T *atomic_fetch_add(volatile Atomic<_T *> *a, ptrdiff_t op) {
        return a->fetch_add(op);
    }

    template <typename _T>
    inline _T *atomic_fetch_add(Atomic<_T *> *a, ptrdiff_t op) {
        return a->fetch_add(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add_explicit(volatile Atomic<_T> *a, _T op,
                              MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_add_explicit(Atomic<_T> *a, _T op, MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_add_explicit(volatile Atomic<_T *> *a,
                                        ptrdiff_t op, MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_add_explicit(Atomic<_T *> *a, ptrdiff_t op,
                                        MemoryOrder ord) {
        return a->fetch_add(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub(volatile Atomic<_T> *a, _T op) {
        return a->fetch_sub(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub(Atomic<_T> *a, _T op) {
        return a->fetch_sub(op);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub(volatile Atomic<_T *> *a, ptrdiff_t op) {
        return a->fetch_sub(op);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub(Atomic<_T *> *a, ptrdiff_t op) {
        return a->fetch_sub(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub_explicit(volatile Atomic<_T> *a, _T op,
                              MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_sub_explicit(Atomic<_T> *a, _T op, MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub_explicit(volatile Atomic<_T *> *a,
                                        ptrdiff_t op, MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename _T>
    inline _T *atomic_fetch_sub_explicit(Atomic<_T *> *a, ptrdiff_t op,
                                        MemoryOrder ord) {
        return a->fetch_sub(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and(volatile Atomic<_T> *a, _T op) {
        return a->fetch_and(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and(Atomic<_T> *a, _T op) {
        return a->fetch_and(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and_explicit(volatile Atomic<_T> *a, _T op,
                              MemoryOrder ord) {
        return a->fetch_and(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_and_explicit(Atomic<_T> *a, _T op, MemoryOrder ord) {
        return a->fetch_and(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or(volatile Atomic<_T> *a, _T op) {
        return a->fetch_or(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or(Atomic<_T> *a, _T op) {
        return a->fetch_or(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or_explicit(volatile Atomic<_T> *a, _T op,
                             MemoryOrder ord) {
        return a->fetch_or(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_or_explicit(Atomic<_T> *a, _T op, MemoryOrder ord) {
        return a->fetch_or(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor(volatile Atomic<_T> *a, _T op) {
        return a->fetch_xor(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor(Atomic<_T> *a, _T op) {
        return a->fetch_xor(op);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor_explicit(volatile Atomic<_T> *a, _T op,
                              MemoryOrder ord) {
        return a->fetch_xor(op, ord);
    }

    template <typename _T>
    inline octa::EnableIf<octa::IsIntegral<_T>::value &&
                         !octa::IsSame<_T, bool>::value, _T>
    atomic_fetch_xor_explicit(Atomic<_T> *a, _T op, MemoryOrder ord) {
        return a->fetch_xor(op, ord);
    }

    struct AtomicFlag {
        octa::detail::AtomicBase<bool> p_a;

        AtomicFlag() = default;

        AtomicFlag(bool b): p_a(b) {}

        AtomicFlag(const AtomicFlag &) = delete;

        AtomicFlag &operator=(const AtomicFlag &) = delete;
        AtomicFlag &operator=(const AtomicFlag &) volatile = delete;

        bool test_and_set(MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            return octa::detail::atomic_exchange(&p_a, true, ord);
        }

        bool test_and_set(MemoryOrder ord = MemoryOrder::seq_cst) {
            return octa::detail::atomic_exchange(&p_a, true, ord);
        }

        void clear(MemoryOrder ord = MemoryOrder::seq_cst) volatile {
            octa::detail::atomic_store(&p_a, false, ord);
        }

        void clear(MemoryOrder ord = MemoryOrder::seq_cst) {
            octa::detail::atomic_store(&p_a, false, ord);
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
        octa::detail::atomic_thread_fence(ord);
    }

    inline void atomic_signal_fence(MemoryOrder ord) {
        octa::detail::atomic_signal_fence(ord);
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
#define ATOMIC_VAR_INIT(v) {v}

}

#endif