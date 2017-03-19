/* Channels provide the messaging system used for OctaSTD concurrency.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_CHANNEL_HH
#define OSTD_CHANNEL_HH

#include <list>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <memory>

namespace ostd {

struct channel_error: std::logic_error {
    using std::logic_error::logic_error;
};

template<typename T, typename C = std::condition_variable>
struct channel {
    using condition_variable_type = C;

    /* default ctor works for default C */
    channel(): p_state(new impl) {}

    /* constructing using a function object, keep in mind that condvars are
     * not copy or move constructible, so the func has to work in a way that
     * elides copying and moving (by directly returning the type ctor call)
     */
    template<typename F>
    channel(F func): p_state(new impl{func}) {}

    channel(channel const &) = default;
    channel(channel &&) = default;

    channel &operator=(channel const &) = default;
    channel &operator=(channel &&) = default;

    void put(T const &val) {
        p_state->put(val);
    }

    void put(T &&val) {
        p_state->put(std::move(val));
    }

    T get() {
        T ret;
        /* guaranteed to return true if at all */
        p_state->get(ret, true);
        return ret;
    }

    bool try_get(T &val) {
        return p_state->get(val, false);
    }

    bool is_closed() const {
        return p_state->is_closed();
    }

    void close() {
        p_state->close();
    }

private:
    struct impl {
        impl() {}

        template<typename F>
        impl(F &func): p_lock(), p_cond(func(p_lock)) {}

        template<typename U>
        void put(U &&val) {
            std::unique_lock<std::mutex> l{p_lock};
            if (p_closed) {
                throw channel_error{"put in a closed channel"};
            }
            p_messages.push_back(std::forward<U>(val));
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

        bool is_closed() const {
            std::unique_lock<std::mutex> l{p_lock};
            return p_closed;
        }

        void close() {
            std::unique_lock<std::mutex> l{p_lock};
            p_closed = true;
            p_cond.notify_all();
        }

        std::list<T> p_messages;
        mutable std::mutex p_lock;
        C p_cond;
        bool p_closed = false;
    };

    /* basic and inefficient, deal with it better later */
    std::shared_ptr<impl> p_state;
};

} /* namespace ostd */

#endif
