/** @addtogroup Concurrency
 * @{
 */

/** @file channel.hh
 *
 * @brief Thread-safe queue for cross-task data transfer.
 *
 * This file implements channels, a kind of thread-safe queue that can be
 * used to send and receive values across tasks safely.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_CHANNEL_HH
#define OSTD_CHANNEL_HH

#include <type_traits>
#include <algorithm>
#include <list>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <memory>

#include "ostd/generic_condvar.hh"

namespace ostd {

/** @addtogroup Concurrency
 * @{
 */

/** Thrown when manipulating a channel that has been closed. */
struct channel_error: std::logic_error {
    using std::logic_error::logic_error;
};

/** @brief A thread-safe message queue.
 *
 * A channel is a kind of message queue (FIFO) that is properly synchronized.
 * It can be used standalone or it can be used as a part of OctaSTD's
 * concurrency system.
 *
 * It stores its internal state in a reference counted manner, so multiple
 * channel instances can reference the same internal state. The internal
 * state is freed as soon as the last channel instance referencing it is
 * destroyed.
 *
 * @tparam T The type of the values in the queue.
 */
template<typename T>
struct channel {
    /** @brief Constructs a default channel.
     *
     * This uses std::condition_variable as its internal condition type,
     * so it will work with standard threads (raw or when used with C++'s
     * async APIs). You can also use channels with ostd's concurrency system
     * though - see ostd::make_channel() and channel(F).
     */
    channel(): p_state(new impl) {}

    /** @brief Constructs a channel with a custom condition variable type.
     *
     * Channels use #ostd::generic_condvar to store their condvars internally,
     * so you can provide a custom type as well. This comes in handy for
     * example when doing custom scheduling.
     *
     * The condvar cannot be passed directly though, as it's not required to
     * be copy or move constructible, so it's passed in through a function
     * instead - the function is meant to return the condvar.
     *
     * However, typically you won't be using this directly, as you're meant
     * to use the higher level concurrency system, which lready provides the
     * ostd::make_channel() function.
     *
     * @param[in] func A function that returns the desired condvar.
     */
    template<typename F>
    channel(F func): p_state(new impl{func}) {}

    /** @brief Creates a new reference to the channel.
     *
     * This does not copy per se, as channels store a refcounted internal
     * state. It increments the reference count on the state and creates
     * a new channel instance that references this state.
     *
     * If you don't want to increment and instead you want to transfer the
     * reference to a new instance, use channel(channel &&).
     *
     * @see channel(channel &&), operator=(channel const &)
     */
    channel(channel const &) = default;

    /** @brief Moves the internal state reference to a new channel instance.
     *
     * This is like channel(channel const &) except it does not increment
     * the reference; it moves the reference to a new container instead,
     * leaving the other one uninitialized. You cannot use the channel
     * the reference was moved from anymore afterwards.
     *
     * @see channel(channel const &), operator=(channel &&)
     */
    channel(channel &&) = default;

    /** @see channel(channel const &) */
    channel &operator=(channel const &) = default;

    /** @see channel(channel &&) */
    channel &operator=(channel &&) = default;

    /** @brief Inserts a copy of a value into the queue.
     *
     * This will insert a copy of @p val into the queue and notify any
     * single task waiting on the queue (if present, see get()) that the
     * queue is ready.
     *
     * @param[in] val The value to insert.
     *
     * @throws ostd::channel_error when the channel is closed.
     *
     * @see put(T &&), get(), try_get(), close(), is_closed()
     */
    void put(T const &val) {
        p_state->put(val);
    }

    /** @brief Moves a value into the queue.
     *
     * Same as put(T const &), except it moves the value.
     *
     * @param[in] val The value to insert.
     *
     * @throws ostd::channel_error when the channel is closed.
     *
     * @see put(T const &)
     */
    void put(T &&val) {
        p_state->put(std::move(val));
    }

    /** @brief Waits for a value and returns it.
     *
     * If the queue is empty at the time of the call, this will block the
     * calling task and wait until there is a value in the queue. Once there
     * is a value, it pops it out of the queue and returns it. The value is
     * moved out of the queue.
     *
     * If you don't want to wait and want to just check if there is something
     * in the queue already, use try_get(T &).
     *
     * @returns The first inserted value in the queue.
     *
     * @throws ostd::channel_error when the channel is closed.
     *
     * @see try_get(T &), put(T const &), close(), is_closed()
     */
    T get() {
        T ret;
        /* guaranteed to return true if at all */
        p_state->get(ret, true);
        return ret;
    }

    /** @brief Gets a value from the queue if there is one.
     *
     * If a value is present in the queue, writes it into @p val and returns
     * `true`. Otherwise returns `false`. See get() for a waiting version.
     *
     * @returns `true` if a value was retrieved and `false` otherwise.
     *
     * @throws ostd::channel_error when the channel is closed.
     *
     * @see try_get(T &), put(T const &), close(), is_closed()
     */
    bool try_get(T &val) {
        return p_state->get(val, false);
    }

    /** @brief Checks if the channel is closed.
     *
     * A channel is only called after explicitly calling close().
     *
     * @returns `true` if closed, `false` otherwise.
     */
    bool is_closed() const noexcept {
        return p_state->is_closed();
    }

    /** Closes the channel. No effect if already closed. */
    void close() noexcept {
        p_state->close();
    }

private:
    struct impl {
        impl() {
        }

        template<typename F>
        impl(F &func): p_lock(), p_cond(func()) {}

        template<typename U>
        void put(U &&val) {
            {
                std::lock_guard<std::mutex> l{p_lock};
                if (p_closed) {
                    throw channel_error{"put in a closed channel"};
                }
                p_messages.push_back(std::forward<U>(val));
            }
            p_cond.notify_one();
        }

        bool get(T &val, bool w) {
            std::unique_lock<std::mutex> l{p_lock};
            if (w) {
                while (!p_closed && p_messages.empty()) {
                    p_cond.wait(l);
                }
            }
            if (p_messages.empty()) {
                if (p_closed) {
                    throw channel_error{"get from a closed channel"};
                }
                return false;
            }
            val = std::move(p_messages.front());
            p_messages.pop_front();
            return true;
        }

        bool is_closed() const noexcept {
            std::lock_guard<std::mutex> l{p_lock};
            return p_closed;
        }

        void close() noexcept {
            {
                std::lock_guard<std::mutex> l{p_lock};
                p_closed = true;
            }
            p_cond.notify_all();
        }

        std::list<T> p_messages;
        mutable std::mutex p_lock;
        generic_condvar p_cond;
        bool p_closed = false;
    };

    /* basic and inefficient, deal with it better later */
    std::shared_ptr<impl> p_state;
};

/** @} */

} /* namespace ostd */

#endif

/** @} */
