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
#include <stdexcept>

#include "ostd/platform.hh"
#include "ostd/coroutine.hh"
#include "ostd/channel.hh"
#include "ostd/generic_condvar.hh"

namespace ostd {

struct scheduler {
private:
    struct stack_allocator {
        stack_allocator() = delete;
        stack_allocator(scheduler &s) noexcept: p_sched(&s) {}

        stack_context allocate() {
            return p_sched->allocate_stack();
        }

        void deaallocate(stack_context &st) noexcept {
            p_sched->deallocate_stack(st);
        }

    private:
        scheduler *p_sched;
    };

public:
    scheduler() {}

    scheduler(scheduler const &) = delete;
    scheduler(scheduler &&) = delete;
    scheduler &operator=(scheduler const &) = delete;
    scheduler &operator=(scheduler &&) = delete;

    virtual void spawn(std::function<void()>) = 0;
    virtual void yield() noexcept = 0;
    virtual generic_condvar make_condition() = 0;

    virtual stack_context allocate_stack() = 0;
    virtual void deallocate_stack(stack_context &st) noexcept = 0;
    virtual void reserve_stacks(size_t n) = 0;

    stack_allocator get_stack_allocator() noexcept {
        return stack_allocator{*this};
    }

    template<typename F, typename ...A>
    void spawn(F &&func, A &&...args) {
        if constexpr(sizeof...(A) == 0) {
            spawn(std::function<void()>{func});
        } else {
            spawn(std::function<void()>{
                std::bind(std::forward<F>(func), std::forward<A>(args)...)
            });
        }
    }

    template<typename T>
    channel<T> make_channel() {
        return channel<T>{[this]() {
            return make_condition();
        }};
    }

    template<typename T, typename F>
    coroutine<T> make_coroutine(F &&func) {
        return coroutine<T>{std::forward<F>(func), get_stack_allocator()};
    }

    template<typename T, typename F>
    generator<T> make_generator(F &&func) {
        return generator<T>{std::forward<F>(func), get_stack_allocator()};
    }
};

namespace detail {
    OSTD_EXPORT extern scheduler *current_scheduler;

    struct current_scheduler_owner {
        current_scheduler_owner() = delete;

        template<typename S>
        current_scheduler_owner(S &sched) {
            if (current_scheduler) {
                throw std::logic_error{"another scheduler already running"};
            }
            current_scheduler = &sched;
        }

        current_scheduler_owner(current_scheduler_owner const &) = delete;
        current_scheduler_owner(current_scheduler_owner &&) = delete;
        current_scheduler_owner &operator=(current_scheduler_owner const &) = delete;
        current_scheduler_owner &operator=(current_scheduler_owner &&) = delete;

        ~current_scheduler_owner() {
            current_scheduler = nullptr;
        }
    };
}

template<typename SA>
struct basic_thread_scheduler: scheduler {
    basic_thread_scheduler(SA &&sa = SA{}): p_stacks(std::move(sa)) {}

    template<typename F, typename ...A>
    auto start(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        detail::current_scheduler_owner iface{*this};
        if constexpr(std::is_same_v<std::result_of_t<F(A...)>, void>) {
            func(std::forward<A>(args)...);
            join_all();
        } else {
            auto ret = func(std::forward<A>(args)...);
            join_all();
            return ret;
        }
    }

    void spawn(std::function<void()> func) {
        std::lock_guard<std::mutex> l{p_lock};
        p_threads.emplace_front();
        auto it = p_threads.begin();
        *it = std::thread{[this, it, lfunc = std::move(func)]() {
            lfunc();
            remove_thread(it);
        }};
    }

    void yield() noexcept {
        std::this_thread::yield();
    }

    generic_condvar make_condition() {
        return generic_condvar{};
    }

    stack_context allocate_stack() {
        std::lock_guard<std::mutex> l{p_lock};
        return p_stacks.allocate();
    }

    void deallocate_stack(stack_context &st) noexcept {
        std::lock_guard<std::mutex> l{p_lock};
        p_stacks.deallocate(st);
    }

    void reserve_stacks(size_t n) {
        std::lock_guard<std::mutex> l{p_lock};
        p_stacks.reserve(n);
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
            p_threads.pop_front();
        }
    }

    SA p_stacks;
    std::list<std::thread> p_threads;
    std::thread p_dead;
    std::mutex p_lock;
};

using thread_scheduler = basic_thread_scheduler<stack_pool>;

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

        void yield() noexcept {
            /* we'll yield back to the thread we were scheduled to, which
             * will appropriately notify one or all other waiting threads
             * so we either get re-scheduled or the whole scheduler dies
             */
            this->yield_jump();
        }

        bool dead() const noexcept {
            return this->is_dead();
        }

        static csched_task *current() noexcept {
            return current_csched_task;
        }

    private:
        void resume_call() {
            p_func();
        }

        std::function<void()> p_func;
    };
}

template<typename SA>
struct basic_simple_coroutine_scheduler: scheduler {
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
    basic_simple_coroutine_scheduler(SA &&sa = SA{}):
        p_stacks(std::move(sa))
    {}

    template<typename F, typename ...A>
    auto start(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        detail::current_scheduler_owner iface{*this};

        using R = std::result_of_t<F(A...)>;

        basic_fixedsize_stack<typename SA::traits_type, false> sa{
            detail::stack_main_size()
        };

        if constexpr(std::is_same_v<R, void>) {
            if constexpr(sizeof...(A) == 0) {
                p_coros.emplace_back(std::forward<F>(func), sa);
            } else {
                p_coros.emplace_back(std::bind(
                    std::forward<F>(func), std::forward<A>(args)...
                ), sa);
            }
            dispatch();
        } else {
            R ret;
            if constexpr(sizeof...(A) == 0) {
                p_coros.emplace_back([&ret, lfunc = std::forward<F>(func)] {
                    ret = lfunc();
                }, sa);
            } else {
                p_coros.emplace_back([&ret, lfunc = std::bind(
                    std::forward<F>(func), std::forward<A>(args)...
                )]() {
                    ret = lfunc();
                }, sa);
            }
            dispatch();
            return ret;
        }
    }

    void spawn(std::function<void()> func) {
        p_coros.emplace_back(std::move(func), p_stacks.get_allocator());
    }

    void yield() noexcept {
        detail::csched_task::current()->yield();
    }

    generic_condvar make_condition() {
        return generic_condvar{[this]() {
            return coro_cond{*this};
        }};
    }

    stack_context allocate_stack() {
        return p_stacks.allocate();
    }

    void deallocate_stack(stack_context &st) noexcept {
        p_stacks.deallocate(st);
    }

    void reserve_stacks(size_t n) {
        p_stacks.reserve(n);
    }

private:
    void dispatch() {
        while (!p_coros.empty()) {
            if (p_idx == p_coros.end()) {
                p_idx = p_coros.begin();
            }
            (*p_idx)();
            if (p_idx->dead()) {
                p_idx = p_coros.erase(p_idx);
            } else {
                ++p_idx;
            }
        }
    }

    SA p_stacks;
    std::list<detail::csched_task> p_coros;
    typename std::list<detail::csched_task>::iterator p_idx = p_coros.end();
};

using simple_coroutine_scheduler = basic_simple_coroutine_scheduler<stack_pool>;

template<typename SA>
struct basic_coroutine_scheduler: scheduler {
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

        template<typename F, typename TSA>
        task(F &&f, TSA &&sa):
            p_func(std::forward<F>(f), std::forward<TSA>(sa))
        {}

        void operator()() {
            p_func();
        }

        void yield() noexcept {
            p_func.yield();
        }

        bool dead() const noexcept {
            return p_func.dead();
        }

        static task *current() noexcept {
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
            /* lock until the task has been added to the wait queue,
             * that ensures that any notify/notify_any has to wait
             * until after the task has fully blocked... we can't
             * use unique_lock or lock_guard because they're scoped
             */
            p_sched.p_lock.lock();
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
    basic_coroutine_scheduler(SA &&sa = SA{}):
        p_stacks(std::move(sa))
    {}

    ~basic_coroutine_scheduler() {}

    template<typename F, typename ...A>
    auto start(F func, A &&...args) -> std::result_of_t<F(A...)> {
        detail::current_scheduler_owner iface{*this};

        /* start with one task in the queue, this way we can
         * say we've finished when the task queue becomes empty
         */
        using R = std::result_of_t<F(A...)>;

        /* the default 64 KiB stack won't cut it for main, allocate a stack
         * which matches the size of the process stack outside of the pool
         */
        basic_fixedsize_stack<typename SA::traits_type, false> sa{
            detail::stack_main_size()
        };

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

    void spawn(std::function<void()> func) {
        {
            std::lock_guard<std::mutex> l{p_lock};
            spawn_add(p_stacks.get_allocator(), std::move(func));
        }
        p_cond.notify_one();
    }

    void yield() noexcept {
        task::current()->yield();
    }

    generic_condvar make_condition() {
        return generic_condvar{[this]() {
            return task_cond{*this};
        }};
    }

    stack_context allocate_stack() {
        std::lock_guard<std::mutex> l{p_lock};
        return p_stacks.allocate();
    }

    void deallocate_stack(stack_context &st) noexcept {
        std::lock_guard<std::mutex> l{p_lock};
        p_stacks.deallocate(st);
    }

    void reserve_stacks(size_t n) {
        std::lock_guard<std::mutex> l{p_lock};
        p_stacks.reserve(n);
    }

private:
    template<typename TSA, typename F, typename ...A>
    void spawn_add(TSA &&sa, F &&func, A &&...args) {
        task *t = nullptr;
        if constexpr(sizeof...(A) == 0) {
            t = &p_available.emplace_back(
                std::forward<F>(func),
                std::forward<TSA>(sa)
            );
        } else {
            t = &p_available.emplace_back(
                [lfunc = std::bind(
                    std::forward<F>(func), std::forward<A>(args)...
                )]() mutable {
                    lfunc();
                },
                std::forward<TSA>(sa)
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
        if (c.dead()) {
            l.lock();
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
            l.lock();
            p_available.splice(p_available.cend(), p_running, it);
            l.unlock();
            p_cond.notify_one();
        } else {
            p_waiting.splice(p_waiting.cbegin(), p_running, it);
            c.next_waiting = c.waiting_on->p_waiting;
            c.waiting_on->p_waiting = &c;
            /* wait locks the mutex, so manually unlock it here */
            p_lock.unlock();
        }
    }

    std::condition_variable p_cond;
    std::mutex p_lock;
    SA p_stacks;
    tlist p_available;
    tlist p_waiting;
    tlist p_running;
};

using coroutine_scheduler = basic_coroutine_scheduler<stack_pool>;

template<typename F, typename ...A>
inline void spawn(F &&func, A &&...args) {
    detail::current_scheduler->spawn(
        std::forward<F>(func), std::forward<A>(args)...
    );
}

inline void yield() noexcept {
    detail::current_scheduler->yield();
}

template<typename T>
inline channel<T> make_channel() {
    return detail::current_scheduler->make_channel<T>();
}

template<typename T, typename F>
inline coroutine<T> make_coroutine(F &&func) {
    return detail::current_scheduler->make_coroutine<T>(std::forward<F>(func));
}

template<typename T, typename F>
inline generator<T> make_generator(F &&func) {
    return detail::current_scheduler->make_generator<T>(std::forward<F>(func));
}

inline void reserve_stacks(size_t n) {
    detail::current_scheduler->reserve_stacks(n);
}

} /* namespace ostd */

#endif
