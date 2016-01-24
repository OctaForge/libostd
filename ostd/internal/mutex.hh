/* Locking related core internals.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_MUTEX_HH
#define OSTD_INTERNAL_MUTEX_HH

#include <stdlib.h>
#include <pthread.h>

#include "ostd/utility.hh"

namespace ostd {

struct Mutex {
    using NativeHandle = pthread_mutex_t *;

    constexpr Mutex(): p_mtx(PTHREAD_MUTEX_INITIALIZER) {}

    ~Mutex() {
        pthread_mutex_destroy(&p_mtx);
    }

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    bool lock() {
        return !pthread_mutex_lock(&p_mtx);
    }

    int try_lock() {
        /* TODO handle return value correctly */
        return pthread_mutex_trylock(&p_mtx);
    }

    bool unlock() {
        return !pthread_mutex_unlock(&p_mtx);
    }

    NativeHandle native_handle() { return &p_mtx; }

private:
    pthread_mutex_t p_mtx;
};

struct DeferLock {};
struct TryToLock {};
struct AdoptLock {};

constexpr DeferLock defer_lock {};
constexpr TryToLock try_to_lock {};
constexpr AdoptLock adopt_lock {};

template<typename T>
struct LockGuard {
    using MutexType = T;

    explicit LockGuard(MutexType &m): p_mtx(m) { m.lock(); }
    LockGuard(MutexType &m, AdoptLock): p_mtx(m) {}
    ~LockGuard() { p_mtx.unlock(); }

    LockGuard(const LockGuard &) = delete;
    LockGuard &operator=(const LockGuard &) = delete;

private:
    MutexType &p_mtx;
};

template<typename T>
struct UniqueLock {
    using MutexType = T;

    UniqueLock(): p_mtx(nullptr), p_owns(false) {}

    explicit UniqueLock(MutexType &m): p_mtx(&m), p_owns(true) {
        m.lock();
    }

    UniqueLock(MutexType &m, DeferLock): p_mtx(&m), p_owns(false) {}

    UniqueLock(MutexType &m, TryToLock): p_mtx(&m) {
        int ret = m.try_lock();
        if (ret) {
            p_mtx = nullptr;
            p_owns = false;
            return;
        }
        p_owns = (ret == 0);
    }

    UniqueLock(MutexType &m, AdoptLock): p_mtx(&m), p_owns(true) {}

    ~UniqueLock() {
        if (p_owns) p_mtx->unlock();
    }

    UniqueLock(const UniqueLock &) = delete;
    UniqueLock &operator=(const UniqueLock &) = delete;

    UniqueLock(UniqueLock &&u): p_mtx(u.p_mtx), p_owns(u.p_owns) {
        u.p_mtx = nullptr;
        u.p_owns = false;
    }

    UniqueLock &operator=(UniqueLock &&u) {
        if (p_owns) p_mtx->unlock();
        p_mtx = u.p_mtx;
        p_owns = u.p_owns;
        u.p_mtx = nullptr;
        u.p_owns = false;
        return *this;
    }

    bool lock() {
        if (!p_mtx || p_owns) return false;
        bool ret = p_mtx->lock();
        if (ret) p_owns = true;
        return ret;
    }

    int try_lock() {
        if (!p_mtx || p_owns) return 1;
        int ret = p_mtx->try_lock();
        if (ret) return ret;
        p_owns = (ret == 0);
        return ret;
    }

    bool unlock() {
        if (!p_mtx || p_owns) return false;
        bool ret = p_mtx->unlock();
        if (ret) p_owns = false;
        return ret;
    }

    void swap(UniqueLock &u) {
        detail::swap_adl(p_mtx, u.p_mtx);
        detail::swap_adl(p_owns, u.p_owns);
    }

    MutexType *release() {
        MutexType *ret = p_mtx;
        p_mtx = nullptr;
        p_owns = false;
        return ret;
    }

    bool owns_lock() const { return p_owns; }
    explicit operator bool() const { return p_owns; }
    MutexType *mutex() const { return p_mtx; }

private:
    MutexType *p_mtx;
    bool p_owns;
};

struct Condition {
    using NativeHandle = pthread_cond_t *;

    constexpr Condition(): p_cnd(PTHREAD_COND_INITIALIZER) {}

    Condition(const Condition &) = delete;
    Condition &operator=(const Condition &) = delete;

    ~Condition() {
        pthread_cond_destroy(&p_cnd);
    }

    bool signal() {
        return !pthread_cond_signal(&p_cnd);
    }

    bool broadcast() {
        return !pthread_cond_broadcast(&p_cnd);
    }

    bool wait(UniqueLock<Mutex> &l) {
        if (!l.owns_lock())
            return false;
        return !pthread_cond_wait(&p_cnd, l.mutex()->native_handle());
    }

    NativeHandle native_handle() { return &p_cnd; }

private:
    pthread_cond_t p_cnd;
};

} /* namespace ostd */

#endif