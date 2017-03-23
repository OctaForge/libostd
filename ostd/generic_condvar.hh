/* A generic condvar that can store any other condvar as a single type.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_GENERIC_CONDVAR_HH
#define OSTD_GENERIC_CONDVAR_HH

#include <type_traits>
#include <algorithm>
#include <condition_variable>

namespace ostd {

namespace detail {
    struct cond_iface {
        cond_iface() {}
        virtual ~cond_iface() {}
        virtual void notify_one() = 0;
        virtual void notify_all() = 0;
        virtual void wait(std::unique_lock<std::mutex> &) = 0;
    };

    template<typename C>
    struct cond_impl: cond_iface {
        cond_impl(): p_cond() {}
        template<typename F>
        cond_impl(F &func): p_cond(func()) {}
        void notify_one() {
            p_cond.notify_one();
        }
        void notify_all() {
            p_cond.notify_all();
        }
        void wait(std::unique_lock<std::mutex> &l) {
            p_cond.wait(l);
        }
    private:
        C p_cond;
    };
} /* namespace detail */

struct generic_condvar {
    generic_condvar() {
        new (reinterpret_cast<void *>(&p_condbuf))
            detail::cond_impl<std::condition_variable>();
    }
    template<typename F>
    generic_condvar(F &&func) {
        new (reinterpret_cast<void *>(&p_condbuf))
            detail::cond_impl<std::result_of_t<F()>>(func);
    }

    generic_condvar(generic_condvar const &) = delete;
    generic_condvar(generic_condvar &&) = delete;
    generic_condvar &operator=(generic_condvar const &) = delete;
    generic_condvar &operator=(generic_condvar &&) = delete;

    ~generic_condvar() {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->~cond_iface();
    }

    void notify_one() {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->notify_one();
    }
    void notify_all() {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->notify_all();
    }
    void wait(std::unique_lock<std::mutex> &l) {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->wait(l);
    }

private:
    std::aligned_storage_t<std::max(
        6 * sizeof(void *), sizeof(detail::cond_impl<std::condition_variable>)
    )> p_condbuf;
};

} /* namespace ostd */

#endif
