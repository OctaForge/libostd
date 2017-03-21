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
        std::lock_guard<std::mutex> l{p_lock};
        p_threads.emplace_front();
        auto it = p_threads.begin();
        *it = std::thread{
            [this, it](auto func, auto ...args) {
                func(std::move(args)...);
                remove_thread(it);
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
        std::lock_guard<std::mutex> l{p_lock};
        std::thread t{std::exchange(p_dead, std::move(*it))};
        if (t.joinable()) {
            t.join();
        }
        p_threads.erase(it);
    }

    void join_all() {
        /* wait for all threads to finish */
        std::lock_guard<std::mutex> l{p_lock};
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

template<typename TR, bool Protected>
struct basic_simple_coroutine_scheduler {
private:
    /* simple one just for channels */
    struct coro_cond {
        coro_cond() = delete;
        coro_cond(coro_cond const &) = delete;
        coro_cond(coro_cond &&) = delete;
        coro_cond &operator=(coro_cond const &) = delete;
        coro_cond &operator=(coro_cond &&) = delete;

        coro_cond(basic_simple_coroutine_scheduler &s): p_sched(s) {}

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
            p_sched.yield();
        }

        void notify_all() noexcept {
            p_notified = true;
            p_sched.yield();
        }
    private:
        basic_simple_coroutine_scheduler &p_sched;
        bool p_notified = false;
    };

public:
    template<typename T>
    using channel_type = channel<T, coro_cond>;

    basic_simple_coroutine_scheduler(
        size_t ss = TR::default_size(),
        size_t cs = basic_stack_pool<TR, Protected>::DEFAULT_CHUNK_SIZE
    ):
        p_stacks(ss, cs),
        p_dispatcher([this](auto yield_main) {
            this->dispatch(yield_main);
        }, p_stacks.get_allocator()),
        p_coros()
    {}

    template<typename F, typename ...A>
    auto start(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        using R = std::result_of_t<F(A...)>;
        if constexpr(std::is_same_v<R, void>) {
            func(std::forward<A>(args)...);
            finish();
        } else {
            auto ret = func(std::forward<A>(args)...);
            finish();
            return ret;
        }
    }

    template<typename F, typename ...A>
    void spawn(F &&func, A &&...args) {
        if constexpr(sizeof...(A) == 0) {
            p_coros.emplace_back([lfunc = std::forward<F>(func)](auto) {
                lfunc();
            }, p_stacks.get_allocator());
        } else {
            p_coros.emplace_back([lfunc = std::bind(
                std::forward<F>(func), std::forward<A>(args)...
            )](auto) mutable {
                lfunc();
            }, p_stacks.get_allocator());
        }
    }

    void yield() {
        auto ctx = coroutine_context::current();
        if (!ctx) {
            /* yield from main means go to dispatcher and call first task */
            p_idx = p_coros.begin();
            p_dispatcher();
            return;
        }
        coro *c = dynamic_cast<coro *>(ctx);
        if (c) {
            typename coro::yield_type{*c}();
            return;
        }
        throw std::runtime_error{"attempt to yield outside coroutine"};
    }

    template<typename T>
    channel<T, coro_cond> make_channel() {
        return channel<T, coro_cond>{[this]() {
            return coro_cond{*this};
        }};
    }
private:
    struct coro: coroutine<void()> {
        using coroutine<void()>::coroutine;
    };

    void dispatch(typename coro::yield_type &yield_main) {
        while (!p_coros.empty()) {
            if (p_idx == p_coros.end()) {
                /* we're at the end; it's time to return to main and
                 * continue there (potential yield from main results
                 * in continuing from this point with the first task)
                 */
                yield_main();
                continue;
            }
            (*p_idx)();
            if (!*p_idx) {
                p_idx = p_coros.erase(p_idx);
            } else {
                ++p_idx;
            }
        }
    }

    void finish() {
        /* main has finished, but there might be either suspended or never
         * started tasks in the queue; dispatch until there are none left
         */
        while (!p_coros.empty()) {
            p_idx = p_coros.begin();
            p_dispatcher();
        }
    }

    basic_stack_pool<TR, Protected> p_stacks;
    coro p_dispatcher;
    std::list<coro> p_coros;
    typename std::list<coro>::iterator p_idx = p_coros.end();
};

using simple_coroutine_scheduler =
    basic_simple_coroutine_scheduler<stack_traits, false>;

using protected_simple_coroutine_scheduler =
    basic_simple_coroutine_scheduler<stack_traits, true>;

template<typename TR, bool Protected>
struct basic_coroutine_scheduler {
private:
    struct task_cond;
    struct task;

    using tlist = std::list<task>;
    using titer = typename tlist::iterator;

    struct task {
        struct coro: coroutine<void()> {
            using coroutine<void()>::coroutine;
            task *tptr = nullptr;
        };

        coro func;
        task_cond *waiting_on = nullptr;
        task *next_waiting = nullptr;
        titer pos;

        task() = delete;
        template<typename F, typename SA>
        task(F &&f, SA &&alloc):
            func(std::forward<F>(f), std::forward<SA>(alloc))
        {
            func.tptr = this;
        }

        void operator()() {
            func();
        }

        void yield() {
            /* we'll yield back to the thread we were scheduled to, which
             * will appropriately notify one or all other waiting threads
             * so we either get re-scheduled or the whole scheduler dies
             */
            typename coro::yield_type{func}();
        }

        bool dead() const {
            return !func;
        }

        static task *current() {
            auto ctx = coroutine_context::current();
            coro *c = dynamic_cast<coro *>(ctx);
            if (!c) {
                std::terminate();
            }
            return c->tptr;
        }
    };

    struct task_cond {
        task_cond() = delete;
        task_cond(task_cond const &) = delete;
        task_cond(task_cond &&) = delete;
        task_cond &operator=(task_cond const &) = delete;
        task_cond &operator=(task_cond &&) = delete;

        task_cond(basic_coroutine_scheduler &s): p_sched(s) {}

        template<typename L>
        void wait(L &l) noexcept {
            l.unlock();
            task *curr = task::current();
            p_sched.wait(this, p_waiting, curr);
            curr->yield();
            l.lock();
        }

        void notify_one() noexcept {
            p_sched.notify_one(p_waiting);
        }

        void notify_all() noexcept {
            p_sched.notify_all(p_waiting);
        }
    private:
        basic_coroutine_scheduler &p_sched;
        task *p_waiting = nullptr;
    };

public:
    template<typename T>
    using channel_type = channel<T, task_cond>;

    basic_coroutine_scheduler(
        size_t ss = TR::default_size(),
        size_t cs = basic_stack_pool<TR, Protected>::DEFAULT_CHUNK_SIZE
    ):
        p_stacks(ss, cs)
    {}

    ~basic_coroutine_scheduler() {}

    template<typename F, typename ...A>
    auto start(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        /* start with one task in the queue, this way we can
         * say we've finished when the task queue becomes empty
         */
        using R = std::result_of_t<F(A...)>;
        if constexpr(std::is_same_v<R, void>) {
            spawn(std::forward<F>(func), std::forward<A>(args)...);
            /* actually start the thread pool */
            init();
            destroy();
        } else {
            R ret;
            spawn([&ret, func = std::forward<F>(func)](auto &&...args) {
                ret = func(std::forward<A>(args)...);
            }, std::forward<A>(args)...);
            init();
            destroy();
            return ret;
        }
    }

    template<typename F, typename ...A>
    void spawn(F &&func, A &&...args) {
        {
            std::lock_guard<std::mutex> l{p_lock};
            task *t = nullptr;
            if constexpr(sizeof...(A) == 0) {
                t = &p_available.emplace_back(
                    [lfunc = std::forward<F>(func)](auto) {
                        lfunc();
                    },
                    p_stacks.get_allocator()
                );
            } else {
                t = &p_available.emplace_back(
                    [lfunc = std::bind(
                        std::forward<F>(func), std::forward<A>(args)...
                    )](auto) mutable {
                        lfunc();
                    },
                    p_stacks.get_allocator()
                );
            }
            t->pos = --p_available.end();
        }
        p_cond.notify_one();
    }

    void yield() {
        task::current()->yield();
    }

    template<typename T>
    channel<T, task_cond> make_channel() {
        return channel<T, task_cond>{[this]() {
            return task_cond{*this};
        }};
    }
private:
    void init() {
        auto tf = [this]() {
            thread_run();
        };
        size_t size = std::thread::hardware_concurrency();
        for (size_t i = 0; i < size; ++i) {
            std::thread tid{tf};
            if (!tid.joinable()) {
                throw std::runtime_error{"coroutine_scheduler worker failed"};
            }
            p_thrs.push_back(std::move(tid));
        }
    }

    void destroy() {
        for (auto &tid: p_thrs) {
            tid.join();
        }
    }

    void wait(task_cond *cond, task *&wt, task *t) {
        std::lock_guard<std::mutex> l{p_lock};
        p_waiting.splice(p_waiting.cbegin(), p_running, t->pos);
        t->waiting_on = cond;
        t->next_waiting = wt;
        wt = t;
    }

    void notify_one(task *&wl) {
        std::unique_lock<std::mutex> l{p_lock};
        if (wl == nullptr) {
            return;
        }
        wl->waiting_on = nullptr;
        p_available.splice(p_available.cbegin(), p_waiting, wl->pos);
        wl = wl->next_waiting;
        l.unlock();
        p_cond.notify_one();
        task::current()->yield();
    }

    void notify_all(task *&wl) {
        {
            std::unique_lock<std::mutex> l{p_lock};
            while (wl != nullptr) {
                wl->waiting_on = nullptr;
                p_available.splice(p_available.cbegin(), p_waiting, wl->pos);
                wl = wl->next_waiting;
                l.unlock();
                p_cond.notify_one();
                l.lock();
            }
        }
        task::current()->yield();
    }

    void thread_run() {
        for (;;) {
            std::unique_lock<std::mutex> l{p_lock};
            /* wait for an item to become available */
            while (p_available.empty()) {
                /* if all lists have become empty, we're done */
                if (p_waiting.empty() && p_running.empty()) {
                    return;
                }
                p_cond.wait(l);
            }
            task_run(l);
        }
    }

    void task_run(std::unique_lock<std::mutex> &l) {
        auto it = p_available.begin();
        p_running.splice(p_running.cend(), p_available, it);
        task &c = *it;
        l.unlock();
        c();
        l.lock();
        if (c.dead()) {
            p_running.erase(it);
            /* we're dead, notify all threads so they can be joined
             * we check all three, saves the other threads some re-waiting
             * when a task or tasks are already running, and those that do
             * will do the final notify by themselves
             */
            if (p_available.empty() && p_waiting.empty() && p_running.empty()) {
                l.unlock();
                p_cond.notify_all();
            }
        } else if (!c.waiting_on) {
            /* reschedule to the end of the queue */
            p_available.splice(p_available.cend(), p_running, it);
            l.unlock();
            p_cond.notify_one();
        }
    }

    std::condition_variable p_cond;
    std::mutex p_lock;
    std::vector<std::thread> p_thrs;
    basic_stack_pool<TR, Protected> p_stacks;
    tlist p_available;
    tlist p_waiting;
    tlist p_running;
};

using coroutine_scheduler =
    basic_coroutine_scheduler<stack_traits, false>;

using protected_coroutine_scheduler =
    basic_coroutine_scheduler<stack_traits, true>;

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
