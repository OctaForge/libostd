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
#include <optional>
#include <algorithm>
#include <list>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <memory>

#include <ostd/generic_condvar.hh>

namespace ostd {

/** @addtogroup Concurrency
 * @{
 */

/** @brief Thrown when manipulating a channel that has been closed. */
struct channel_error: std::logic_error {
    using std::logic_error::logic_error;
    /* empty, for vtable placement */
    virtual ~channel_error();
};

/** @brief A thread-safe message queue.
 *
 * A channel is a kind of message queue (FIFO) that is properly synchronized.
 * It can be used standalone or it can be used as a part of libostd's
 * concurrency system.
 *
 * It stores its internal state in a reference counted manner, so multiple
 * channel instances can reference the same internal state. The internal
 * state is freed as soon as the last channel instance referencing it is
 * destroyed.
 *
 * Channels are move constructible. Moving a channel transfers the state
 * into another channel and leaves the original state in an unusable
 * state, with no reference count change. Copying a channel increases
 * the reference count, so both instances point to the ssame state and
 * both are valid.
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

    channel(channel const &) = default;
    channel(channel &&) = default;
    channel &operator=(channel const &) = default;
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
     * @see put(T &&), get(), try_get(), close(), cloed()
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
     * @see put(T const &), emplace()
     */
    void put(T &&val) {
        p_state->put(std::move(val));
    }

    /** @brief Like put(), but constructs the element in-place.
     *
     * The arguments are to be passed to the element constructor.
     * No copy or move operations are performed.
     */
    template<typename ...A>
    void emplace(A &&...args) {
        p_state->emplace(std::forward<A>(args)...);
    }

    /** @brief Waits for a value and returns it.
     *
     * If the queue is empty at the time of the call, this will block the
     * calling task and wait until there is a value in the queue. Once there
     * is a value, it pops it out of the queue and returns it. The value is
     * moved out of the queue.
     *
     * If you don't want to wait and want to just check if there is something
     * in the queue already, use try_get().
     *
     * @returns The first inserted value in the queue.
     *
     * @throws ostd::channel_error when the channel is closed.
     *
     * @see try_get(), put(T const &), close(), closed()
     */
    T get() {
        T ret;
        /* guaranteed to return true if at all */
        p_state->get(ret, true);
        return ret;
    }

    /** @brief Gets a value from the queue if there is one.
     *
     * If a value is present in the queue, returns the value.
     * Otherwise returns std::nullopt. See get() for a waiting
     * version.
     *
     * @returns The value or std::nullopt if there isn't one.
     *
     * @throws ostd::channel_error when the channel is closed.
     *
     * @see get(), put(T const &), close(), closed()
     */
    std::optional<T> try_get() {
        T ret;
        if (!p_state->get(ret, false)) {
            return std::nullopt;
        }
        return std::move(ret);
    }

    /** @brief Checks if the channel is empty.
     *
     * A channel is empty if there are no values in the queue. It's also
     * considered empty if it's closed() (even if there are items left in
     * the queue).
     *
     * @returns `true` if empty, `false` otherwise.
     */
    bool empty() const noexcept {
        return p_state->empty();
    }

    /** @brief Checks if the channel is closed.
     *
     * A channel is only called after explicitly calling close().
     *
     * @returns `true` if closed, `false` otherwise.
     */
    bool closed() const noexcept {
        return p_state->closed();
    }

    /** @brief Closes the channel. No effect if already closed. */
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

        template<typename ...A>
        void emplace(A &&...args) {
            {
                std::lock_guard<std::mutex> l{p_lock};
                if (p_closed) {
                    throw channel_error{"emplace in a closed channel"};
                }
                p_messages.emplace(std::forward<A>(args)...);
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

        bool empty() const noexcept {
            std::lock_guard<std::mutex> l{p_lock};
            return p_closed || p_messages.empty();
        }

        bool closed() const noexcept {
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
