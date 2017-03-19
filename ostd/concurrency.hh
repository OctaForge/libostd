/* Concurrency module with custom scheduler support.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_CONCURRENCY_HH
#define OSTD_CONCURRENCY_HH

#include <thread>
#include <utility>
#include <memory>

#include "ostd/coroutine.hh"
#include "ostd/channel.hh"

namespace ostd {

struct thread_scheduler {
    template<typename T>
    using channel_type = channel<T>;

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

struct simple_coroutine_scheduler {
private:
    /* simple one just for channels */
    struct coro_cond {
        coro_cond() = delete;
        coro_cond(coro_cond const &) = delete;
        coro_cond(coro_cond &&) = delete;
        coro_cond &operator=(coro_cond const &) = delete;
        coro_cond &operator=(coro_cond &&) = delete;

        coro_cond(simple_coroutine_scheduler &s, std::mutex &mtx):
            p_sched(s), p_mtx(mtx)
        {}

        template<typename L>
        void wait(L &l) noexcept {
            l.unlock();
            while (!p_notified) {
                p_sched.yield();
            }
            p_notified = false;
            l.lock();
        }

        void notify_one() noexcept {
            p_notified = true;
            p_mtx.unlock();
            p_sched.yield();
            p_mtx.lock();
        }

        void notify_all() noexcept {
            p_notified = true;
            p_mtx.unlock();
            p_sched.yield();
            p_mtx.lock();
        }
    private:
        simple_coroutine_scheduler &p_sched;
        std::mutex &p_mtx;
        bool p_notified = false;
    };

public:
    template<typename T>
    using channel_type = channel<T, coro_cond>;

    template<typename F, typename ...A>
    auto start(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        using R = std::result_of_t<F(A...)>;
        if constexpr(std::is_same_v<R, void>) {
            spawn(std::forward<F>(func), std::forward<F>(args)...);
            dispatch();
        } else {
            R ret;
            spawn([lfunc = std::forward<F>(func), &ret](A &&...args) {
                ret = std::move(lfunc(std::forward<A>(args)...));
            }, std::forward<A>(args)...);
            dispatch();
            return ret;
        }
    }

    template<typename F, typename ...A>
    void spawn(F &&func, A &&...args) {
        if constexpr(sizeof...(A) == 0) {
            p_coros.emplace_back([lfunc = std::forward<F>(func)](auto) {
                lfunc();
            });
        } else {
            p_coros.emplace_back([lfunc = std::bind(
                std::forward<F>(func), std::forward<A>(args)...
            )](auto) mutable {
                lfunc();
            });
        }
    }

    void yield() {
        auto ctx = coroutine_context::current();
        coro *c = dynamic_cast<coro *>(ctx);
        if (c) {
            coro::yield_type{*c}();
            return;
        }
        throw std::runtime_error{"no task to yield"};
    }

    template<typename T>
    channel<T, coro_cond> make_channel() {
        return channel<T, coro_cond>{[this](std::mutex &mtx) {
            return coro_cond{*this, mtx};
        }};
    }
private:
    struct coro: coroutine<void()> {
        using coroutine<void()>::coroutine;
    };

    void dispatch() {
        while (!p_coros.empty()) {
            if (p_idx == p_coros.end()) {
                p_idx = p_coros.begin();
            }
            (*p_idx)();
            if (!*p_idx) {
                p_idx = p_coros.erase(p_idx);
            } else {
                ++p_idx;
            }
        }
    }

    std::list<coro> p_coros;
    typename std::list<coro>::iterator p_idx = p_coros.end();
};

template<typename S, typename F, typename ...A>
inline void spawn(S &sched, F &&func, A &&...args) {
    sched.spawn(std::forward<F>(func), std::forward<A>(args)...);
}

template<typename S>
inline void yield(S &sched) {
    sched.yield();
}

template<typename T, typename S>
inline auto make_channel(S &sched) -> typename S::template channel_type<T> {
    return sched.template make_channel<T>();
}

} /* namespace ostd */

#endif
