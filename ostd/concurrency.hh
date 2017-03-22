/* Concurrency module with custom scheduler support.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_CONCURRENCY_HH
#define OSTD_CONCURRENCY_HH

#include <vector>
#include <list>
#include <thread>
#include <utility>
#include <memory>

#include "ostd/platform.hh"
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
    void spawn(F func, A &&...args) {
        std::lock_guard<std::mutex> l{p_lock};
        p_threads.emplace_front();
        auto it = p_threads.begin();
        *it = std::thread{
            [this, it](auto func, auto ...args) {
                func(std::move(args)...);
                remove_thread(it);
            },
            std::move(func), std::forward<A>(args)...
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

namespace detail {
    struct csched_task;

    OSTD_EXPORT extern thread_local csched_task *current_csched_task;

    struct csched_task: coroutine_context {
        friend struct coroutine_context;

        csched_task() = delete;
        csched_task(csched_task const &) = delete;
        csched_task(csched_task &&) = delete;
        csched_task &operator=(csched_task const &) = delete;
        csched_task &operator=(csched_task &&) = delete;

        template<typename F, typename SA>
        csched_task(F &&f, SA &&sa): p_func(std::forward<F>(f)) {
            if (!p_func) {
                this->set_dead();
                return;
            }
            this->make_context<csched_task>(sa);
        }

        void operator()() {
            this->set_exec();
            csched_task *curr = std::exchange(current_csched_task, this);
            this->coro_jump();
            current_csched_task = curr;
            this->rethrow();
        }

        void yield() {
            /* we'll yield back to the thread we were scheduled to, which
             * will appropriately notify one or all other waiting threads
             * so we either get re-scheduled or the whole scheduler dies
             */
            this->yield_jump();
        }

        bool dead() const {
            return this->is_dead();
        }

        static csched_task *current() {
            return current_csched_task;
        }

    private:
        void resume_call() {
            p_func();
        }

        std::function<void()> p_func;
    };
}

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
        p_dispatcher([this]() {
            dispatch();
        }, basic_fixedsize_stack<TR, Protected>{ss}),
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
    void spawn(F func, A &&...args) {
        if constexpr(sizeof...(A) == 0) {
            p_coros.emplace_back([lfunc = std::move(func)]() {
                lfunc();
            }, p_stacks.get_allocator());
        } else {
            p_coros.emplace_back([lfunc = std::bind(
                std::move(func), std::forward<A>(args)...
            )]() mutable {
                lfunc();
            }, p_stacks.get_allocator());
        }
    }

    void yield() {
        auto ctx = detail::csched_task::current();
        if (!ctx) {
            /* yield from main means go to dispatcher and call first task */
            p_idx = p_coros.begin();
            p_dispatcher();
            return;
        }
        ctx->yield();
    }

    template<typename T>
    channel<T, coro_cond> make_channel() {
        return channel<T, coro_cond>{[this]() {
            return coro_cond{*this};
        }};
    }
private:
    void dispatch() {
        while (!p_coros.empty()) {
            if (p_idx == p_coros.end()) {
                /* we're at the end; it's time to return to main and
                 * continue there (potential yield from main results
                 * in continuing from this point with the first task)
                 */
                detail::csched_task::current()->yield();
                continue;
            }
            (*p_idx)();
            if (p_idx->dead()) {
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
    detail::csched_task p_dispatcher;
    std::list<detail::csched_task> p_coros;
    typename std::list<detail::csched_task>::iterator p_idx = p_coros.end();
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
    private:
        detail::csched_task p_func;

    public:
        task_cond *waiting_on = nullptr;
        task *next_waiting = nullptr;
        titer pos;

        template<typename F, typename SA>
        task(F &&f, SA &&sa):
            p_func(std::forward<F>(f), std::forward<SA>(sa))
        {}

        void operator()() {
            p_func();
        }

        void yield() {
            p_func.yield();
        }

        bool dead() const {
            return p_func.dead();
        }

        static task *current() {
            return reinterpret_cast<task *>(detail::csched_task::current());
        }
    };

    struct task_cond {
        friend struct basic_coroutine_scheduler;

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
            curr->waiting_on = this;
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
    auto start(F func, A &&...args) -> std::result_of_t<F(A...)> {
        /* start with one task in the queue, this way we can
         * say we've finished when the task queue becomes empty
         */
        using R = std::result_of_t<F(A...)>;

        /* the default 64 KiB stack won't cut it for main, allocate a stack
         * which matches the size of the process stack outside of the pool
         */
        basic_fixedsize_stack<TR, Protected> sa{detail::stack_main_size()};

        if constexpr(std::is_same_v<R, void>) {
            spawn_add(sa, std::move(func), std::forward<A>(args)...);
            /* actually start the thread pool */
            init();
        } else {
            R ret;
            spawn_add(sa, [&ret, func = std::move(func)](auto &&...args) {
                ret = func(std::forward<A>(args)...);
            }, std::forward<A>(args)...);
            init();
            return ret;
        }
    }

    template<typename F, typename ...A>
    void spawn(F func, A &&...args) {
        {
            std::lock_guard<std::mutex> l{p_lock};
            spawn_add(
                p_stacks.get_allocator(), std::move(func),
                std::forward<A>(args)...
            );
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
    template<typename SA, typename F, typename ...A>
    void spawn_add(SA &&sa, F &&func, A &&...args) {
        task *t = nullptr;
        if constexpr(sizeof...(A) == 0) {
            t = &p_available.emplace_back(
                std::forward<F>(func),
                std::forward<SA>(sa)
            );
        } else {
            t = &p_available.emplace_back(
                [lfunc = std::bind(
                    std::forward<F>(func), std::forward<A>(args)...
                )]() mutable {
                    lfunc();
                },
                std::forward<SA>(sa)
            );
        }
        t->pos = --p_available.end();
    }

    void init() {
        size_t size = std::thread::hardware_concurrency();
        std::vector<std::thread> thrs;
        thrs.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            thrs.emplace_back([this]() { thread_run(); });
        }
        for (size_t i = 0; i < size; ++i) {
            if (thrs[i].joinable()) {
                thrs[i].join();
            }
        }
    }

    void notify_one(task *&wl) {
        std::unique_lock<std::mutex> l{p_lock};
        if (wl == nullptr) {
            return;
        }
        wl->waiting_on = nullptr;
        p_available.splice(p_available.cbegin(), p_waiting, wl->pos);
        wl = std::exchange(wl->next_waiting, nullptr);
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
                wl = std::exchange(wl->next_waiting, nullptr);
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
        } else {
            p_waiting.splice(p_waiting.cbegin(), p_running, it);
            c.next_waiting = c.waiting_on->p_waiting;
            c.waiting_on->p_waiting = &c;
        }
    }

    std::condition_variable p_cond;
    std::mutex p_lock;
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
