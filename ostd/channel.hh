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

namespace ostd {

struct channel_error: std::logic_error {
    using std::logic_error::logic_error;
};

template<typename T>
struct channel {
    void put(T const &val) {
        put_impl(val);
    }

    void put(T &&val) {
        put_impl(std::move(val));
    }

    bool get(T &val) {
        return get_impl(val, true);
    }

    bool try_get(T &val) {
        return get_impl(val, false);
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

private:
    template<typename U>
    void put_impl(U &&val) {
        std::unique_lock<std::mutex> l{p_lock};
        if (p_closed) {
            throw channel_error{"put in a closed channel"};
        }
        p_messages.push_back(std::forward<U>(val));
        p_cond.notify_one();
    }

    bool get_impl(T &val, bool w) {
        std::unique_lock<std::mutex> l{p_lock};
        if (w) {
            while (!p_closed && p_messages.empty()) {
                p_cond.wait(l);
            }
        }
        if (p_messages.empty()) {
            return false;
        }
        val = p_messages.front();
        p_messages.pop_front();
        return true;
    }

    std::list<T> p_messages;
    std::condition_variable p_cond;
    mutable std::mutex p_lock;
    bool p_closed = false;
};

} /* namespace ostd */

#endif
