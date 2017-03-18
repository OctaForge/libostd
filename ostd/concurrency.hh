/* Concurrency module with custom scheduler support.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_CONCURRENCY_HH
#define OSTD_CONCURRENCY_HH

#include <thread>
#include <utility>

#include "ostd/channel.hh"

namespace ostd {

struct thread_scheduler {
    ~thread_scheduler() {
        join_all();
    }

    template<typename F, typename ...A>
    auto start(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        return func(std::forward<A>(args)...);
    }

    template<typename F, typename ...A>
    void spawn(F &&func, A &&...args) {
        std::unique_lock<std::mutex> l{p_lock};
        p_threads.emplace_front();
        auto it = p_threads.begin();
        *it = std::thread{
            [this, it](auto func, auto ...args) {
                func(std::move(args)...);
                this->remove_thread(it);
            },
            std::forward<F>(func), std::forward<A>(args)...
        };
    }

    void yield() {
        std::this_thread::yield();
    }

    template<typename T>
    channel<T> make_channel() {
        return channel<T>{};
    }

private:
    void remove_thread(typename std::list<std::thread>::iterator it) {
        std::unique_lock<std::mutex> l{p_lock};
        std::thread t{std::exchange(p_dead, std::move(*it))};
        if (t.joinable()) {
            t.join();
        }
        p_threads.erase(it);
    }

    void join_all() {
        /* wait for all threads to finish */
        std::unique_lock<std::mutex> l{p_lock};
        if (p_dead.joinable()) {
            p_dead.join();
        }
        for (auto &t: p_threads) {
            t.join();
        }
    }

    std::list<std::thread> p_threads;
    std::thread p_dead;
    std::mutex p_lock;
};

} /* namespace ostd */

#endif
