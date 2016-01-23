/* Locking related core internals.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INTERNAL_MUTEX_HH
#define OSTD_INTERNAL_MUTEX_HH

#include <stdlib.h>
#include <threads.h>

#include "ostd/utility.hh"

namespace ostd {

struct Mutex {
    using NativeHandle = mtx_t *;

    Mutex() {
        if (mtx_init(&p_mtx, mtx_plain) != thrd_success)
            p_mtx = 0;
    }

    ~Mutex() {
        if (p_mtx)
            mtx_destroy(&p_mtx);
    }

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    bool lock() {
        if (!p_mtx)
            return false;
        return mtx_lock(&p_mtx) == thrd_success;
    }

    int try_lock() {
        if (!p_mtx)
            return 0;
        int ret = mtx_trylock(&p_mtx);
        if (ret == thrd_busy)
            return -1;
        return 1;
    }

    bool unlock() {
        if (!p_mtx)
            return false;
        return mtx_unlock(&p_mtx) == thrd_success;
    }

    NativeHandle native_handle() { return &p_mtx; }

private:
    mtx_t p_mtx;
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
        m->lock();
    }

    UniqueLock(MutexType &m, DeferLock): p_mtx(&m), p_owns(false) {}

    UniqueLock(MutexType &m, TryToLock): p_mtx(&m) {
        int ret = m.try_lock();
        if (!ret) {
            p_mtx = nullptr;
            p_owns = false;
            return;
        }
        p_owns = (ret == 1);
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
        if (!p_mtx || p_owns) return 0;
        int ret = p_mtx->try_lock();
        if (!ret) return 0;
        p_owns = (ret == 1);
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
    using NativeHandle = cnd_t *;

    Condition() {
        if (cnd_init(&p_cnd) != thrd_success)
            p_cnd = 0;
    }

    Condition(const Condition &) = delete;
    Condition &operator=(const Condition &) = delete;

    ~Condition() {
        if (p_cnd) cnd_destroy(&p_cnd);
    }

    bool signal() {
        if (!p_cnd) return false;
        return cnd_signal(&p_cnd) == thrd_success;
    }

    bool broadcast() {
        if (!p_cnd) return false;
        return cnd_broadcast(&p_cnd) == thrd_success;
    }

    bool wait(UniqueLock<Mutex> &l) {
        if (!p_cnd) return false;
        return cnd_wait(&p_cnd, l.mutex()->native_handle()) == thrd_success;
    }

    NativeHandle native_handle() { return &p_cnd; }

private:
    cnd_t p_cnd;
};

} /* namespace ostd */

#endif