/** @addtogroup Concurrency
 * @{
 */

/** @file generic_condvar.hh
 *
 * @brief A single type that can encapsulate different condvar types.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_GENERIC_CONDVAR_HH
#define OSTD_GENERIC_CONDVAR_HH

#include <type_traits>
#include <algorithm>
#include <condition_variable>

namespace ostd {

/** @addtogroup Concurrency
 * @{
 */

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

/** @brief A generic condition variable type.
 *
 * This is a type that implements a condition variable interface but can
 * encapsulate different real condition variable types while still having
 * just one static type. This is useful when you need to implement a data
 * structure that requires a condition variable but still want it to be
 * compatible with custom schedulers (which can use a custom condition
 * variable implementation for their logical threads) without having to
 * template it.
 *
 * The storage for the custom type is at least 6 pointers, depending on
 * the size of a standard std::condition_variable (if it's bigger, the
 * space is the size of that).
 */
struct generic_condvar {
    /** @brief Constructs the condvar using std::condition_variable. */
    generic_condvar() {
        new (reinterpret_cast<void *>(&p_condbuf))
            detail::cond_impl<std::condition_variable>();
    }

    /** @brief Constructs the condvar using a custom type.
     *
     * As condvars don't have to be move constructible and your own
     * condvar type can internally contain some custom state, the
     * condvar to store is constructed using a function, which is
     * required to return it.
     *
     * @param[in] func The function that is called to get the condvar.
     */
    template<typename F>
    generic_condvar(F &&func) {
        new (reinterpret_cast<void *>(&p_condbuf))
            detail::cond_impl<std::result_of_t<F()>>(func);
    }

    generic_condvar(generic_condvar const &) = delete;
    generic_condvar(generic_condvar &&) = delete;
    generic_condvar &operator=(generic_condvar const &) = delete;
    generic_condvar &operator=(generic_condvar &&) = delete;

    /** @brief Destroys the stored condvar. */
    ~generic_condvar() {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->~cond_iface();
    }

    /** @brief Notifies one waiting thread.
     *
     * If any threads are waiting on this condvar, this unblocks one of
     * them. The actual semantics and what the threads are are defined by
     * the condition variable type that is stored. This simply calls
     * `.notify_one()` on the stored condvar.
     *
     * @see notify_all(), wait(std::unique_lock<std::mutex>)
     */
    void notify_one() {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->notify_one();
    }

    /** @brief Notifies all waiting threads.
     *
     * If any threads are waiting on this condvar, this unblocks all of
     * them. The actual semantics and what the threads are are defined by
     * the condition variable type that is stored. This simply calls
     * `.notify_all()` on the stored condvar.
     *
     * @see notify_one(), wait(std::unique_lock<std::mutex>)
     */
    void notify_all() {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->notify_all();
    }

    /** @brief Blocks the current thread until the condvar is woken up.
     *
     * This atomically releases the given lock, blocks the current thread
     * and adds it to the waiting threads list, until unblocked by notify_one()
     * or notify_all(). It may also be unblocked spuriously, depending on the
     * implementation. The actual specific semantics and what the current
     * thread is depends on the implementation of the stored condvar. This
     * simply calls `.wait(l)` on it.
     *
     * @see notify_one(), notify_all()
     */
    void wait(std::unique_lock<std::mutex> &l) {
        reinterpret_cast<detail::cond_iface *>(&p_condbuf)->wait(l);
    }

private:
    static constexpr auto cvars = sizeof(std::condition_variable);
    static constexpr auto icvars =
        sizeof(detail::cond_impl<std::condition_variable>);
    std::aligned_storage_t<std::max(
        6 * sizeof(void *) + (icvars - cvars), icvars
    )> p_condbuf;
};

/** @} */

} /* namespace ostd */

#endif

/** @} */
