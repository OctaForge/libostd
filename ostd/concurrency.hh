/** @defgroup Concurrency
 *
 * @brief Concurrent/parallel task execution support and related APIs.
 *
 * libostd provides an elaborate concurrency system that covers multiple
 * schedulers with different characteristics as well as different ways to
 * pass data between tasks.
 *
 * It implements 1:1 (tasks are OS threads), N:1 (tasks are lightweight
 * threads running on a single thread) and M:N (tasks are lightweight
 * threads running on a fixed number of OS threads) scheduling approaches.
 *
 * The system is flexible and extensible; it can be used as a base for higher
 * level systems, and its individual components are independently usable.
 *
 * Typical usage of the concurrency system is as follows:
 *
 * ~~~{.cc}
 * #include <ostd/concurrency.hh>
 *
 * int main() {
 *     ostd::coroutine_scheduler{}.start([]() {
 *         // the rest of main follows
 *     });
 * }
 * ~~~
 *
 * See the examples provided with the library for further information.
 *
 * It also implements all sorts of utilities for dealing with parallel
 * programs and synchronization, including a thread pool and other facilities.
 *
 * @{
 */

/** @file concurrency.hh
 *
 * @brief Different concurrent schedulers and related APIs.
 *
 * This file implements several schedulers as well as APIs to spawn
 * tasks, coroutines and channels that utilize the current scheduler's API.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_CONCURRENCY_HH
#define OSTD_CONCURRENCY_HH

#include <cstddef>
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

/** @addtogroup Concurrency
 * @{
 */

/** @brief A base interface for any scheduler.
 *
 * All schedulers derive from this. Its core interface is defined using
 * pure virtual functions, which the schedulers are supposed to implement.
 *
 * Every scheduler its supposed to implement its own method `start`, which
 * will take a function, arguments and will return any value returned from
 * the given function. The given function will be used as the very first
 * task (the main task) which typically replaces your `main` function.
 *
 * The `start` method will also set the internal current scheduler pointer
 * so that APIs such as ostd::spawn() can work.
 */
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

protected:
    /** Does nothing, this base class is empty. */
    scheduler() {}

public:
    /** Does nothing, this base class is empty. */
    virtual ~scheduler() {}

    /** A scheduler is not copy constructible. */
    scheduler(scheduler const &) = delete;

    /** A scheduler is not move constructible. */
    scheduler(scheduler &&) = delete;

    /** A scheduler is not copy assignable. */
    scheduler &operator=(scheduler const &) = delete;

    /** A scheduler is not move assignable. */
    scheduler &operator=(scheduler &&) = delete;

    /** @brief Spawns a task.
     *
     * Spawns a task and schedules it for execution. This is a low level
     * interface function. Typically you will want ostd::spawn().
     * The detailed behavior of the function is completely scheduler dependent.
     *
     * @see ostd::spawn(), spawn()
     */
    virtual void do_spawn(std::function<void()>) = 0;

    /** @brief Tells the scheduler to re-schedule the current task.
     *
     * In #ostd::thread_scheduler, this is just a hint, as it uses OS threading
     * facilities. In coroutine based schedulers, this will typically suspend
     * the currently running task and re-schedule it for later.
     *
     * @see ostd::yield()
     */
    virtual void yield() noexcept = 0;

    /** @brief Creates a condition variable using #ostd::generic_condvar.
     *
     * A scheduler might be using a custom condition variable type depending
     * on how its tasks are implemented. Other data structures might want to
     * use these condition variables to synchronize (see make_channel() for
     * an example).
     *
     * @see make_channel(), make_coroutine(), make_generator()
     */
    virtual generic_condvar make_condition() = 0;

    /** @brief Allocates a stack suitable for a coroutine.
     *
     * If the scheduler uses coroutine based tasks, this allows us to
     * create coroutines and generators that use the same stacks as tasks.
     * This has benefits particularly when a pooled stack allocator is in
     * use for the tasks.
     *
     * Using get_stack_allocator() you can create an actual stack allocator
     * usable with coroutines that uses this set of methods.
     *
     * @see deallocate_stack(), reserve_stacks(), get_stack_allocator()
     */
    virtual stack_context allocate_stack() = 0;

    /** @brief Deallocates a stack allocated with allocate_stack().
     *
     * @see allocate_stack(), reserve_stacks(), get_stack_allocator()
     */
    virtual void deallocate_stack(stack_context &st) noexcept = 0;

    /** @brief Reserves at least `n` stacks.
     *
     * If the stack allocator used in the scheduler is pooled, this will
     * reserve the given number of stacks (or more). It can, however, be
     * a no-op if the allocator is not pooled.
     *
     * @see allocate_stack(), deallocate_stack(), get_stack_allocator()
     */
    virtual void reserve_stacks(std::size_t n) = 0;

    /** @brief Gets a stack allocator using the scheduler's stack allocation.
     *
     * The stack allocator will use allocate_stack() and deallocate_stack()
     * to perform the alloaction and deallocation.
     */
    stack_allocator get_stack_allocator() noexcept {
        return stack_allocator{*this};
    }

    /** @brief Spawns a task using any callable and a set of arguments.
     *
     * Just like do_spawn(), but works for any callable and accepts arguments.
     * If any arguments are provided, they're bound to the callable first.
     * Then the result is converted to the right type for do_spawn() and passed.
     *
     * You can use this to spawn a task directly on a scheduler. However,
     * typically you will not want to pass the scheduler around and instead
     * use the generic ostd::spawn(), which works on any scheduler.
     *
     * @see do_spawn(), ostd::spawn()
     */
    template<typename F, typename ...A>
    void spawn(F &&func, A &&...args) {
        if constexpr(sizeof...(A) == 0) {
            do_spawn(func);
        } else {
            do_spawn(
                std::bind(std::forward<F>(func), std::forward<A>(args)...)
            );
        }
    }

    /** @brief Creates a channel suitable for the scheduler.
     *
     * Returns a channel that uses a condition variable type returned by
     * make_condition(). You can use this to create a channel directly
     * with the scheduler. However, typically you will not want to pass
     * it around, so ostd::make_channel() is a more convenient choice.
     *
     * @tparam T The type of the channel value.
     *
     * @see ostd::make_channel()
     */
    template<typename T>
    channel<T> make_channel() {
        return channel<T>{[this]() {
            return make_condition();
        }};
    }

    /** @brief Creates a coroutine using the scheduler's stack allocator.
     *
     * Using ostd::make_coroutine() will do the same thing, but without
     * the need to explicitly pass the scheduler around.
     *
     * @tparam T The type passed to the coroutine, `Result(Args...)`.
     *
     * @see make_generator(), ostd::make_coroutine()
     */
    template<typename T, typename F>
    coroutine<T> make_coroutine(F &&func) {
        return coroutine<T>{std::forward<F>(func), get_stack_allocator()};
    }

    /** @brief Creates a generator using the scheduler's stack allocator.
     *
     * Using ostd::make_generator() will do the same thing, but without
     * the need to explicitly pass the scheduler around.
     *
     * @tparam T The value type of the generator.
     *
     * @see make_coroutine(), ostd::make_generator()
     */
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

/** @brief A scheduler that uses an `std::thread` per each task.
 *
 * This one doesn't actually do any scheduling, it leaves it to the OS.
 * Effectively this implements a 1:1 model.
 *
 * @tparam SA The stack allocator to use when requesting stacks. It's not
 *         actually used anywhere else, as thread stacks are managed by the OS.
 *         Can be a stack pool and only has to be move constructible.
 */
template<typename SA>
struct basic_thread_scheduler: scheduler {
    /* @brief Creates the scheduler.
     *
     * @param[in] sa The provided stack allocator.
     */
    basic_thread_scheduler(SA &&sa = SA{}): p_stacks(std::move(sa)) {}

    /** @brief Starts the scheduler given a set of arguments.
     *
     * Sets the internal current scheduler pointer to this scheduler and
     * calls the given function. As it doesn't do any scheduling, it really
     * just calls. Then it waits for all threads (tasks) it spawned to finish
     * and returns the value returned from the given function, if any.
     *
     * @returns The result of `func`.
     */
    template<typename F, typename ...A>
    auto start(F func, A &&...args) -> std::result_of_t<F(A...)> {
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

    void do_spawn(std::function<void()> func) {
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
        if constexpr(!SA::is_thread_safe) {
            std::lock_guard<std::mutex> l{p_lock};
            return p_stacks.allocate();
        } else {
            return p_stacks.allocate();
        }
    }

    void deallocate_stack(stack_context &st) noexcept {
        if constexpr(!SA::is_thread_safe) {
            std::lock_guard<std::mutex> l{p_lock};
            p_stacks.deallocate(st);
        } else {
            p_stacks.deallocate();
        }
    }

    void reserve_stacks(std::size_t n) {
        if constexpr(!SA::is_thread_safe) {
            std::lock_guard<std::mutex> l{p_lock};
            p_stacks.reserve(n);
        } else {
            p_stacks.reserve(n);
        }
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

/** An #ostd::basic_thread_scheduler using #ostd::stack_pool. */
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

/** @brief A scheduler that uses a coroutine type for tasks on a single thread.
 *
 * Effectively implements the N:1 model. Runs on a single thread, so it doesn't
 * make any use of multicore systems. The tasks bypass the
 * coroutine_context::current() method, so they're completely hidden from the
 * outside code. This also has several advantages for code using coroutines.
 *
 * @tparam SA The stack allocator to use when requesting stacks. Used for
 *            the tasks as well as for the stack request methods.
 */
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
    /* @brief Creates the scheduler.
     *
     * @param[in] sa The provided stack allocator.
     */
    basic_simple_coroutine_scheduler(SA &&sa = SA{}):
        p_stacks(std::move(sa))
    {}

    /** @brief Starts the scheduler given a set of arguments.
     *
     * Sets the internal current scheduler pointer to this scheduler creates
     * the main task using the separate provided stack allocator. This is
     * useful because the task stacks tend to be rather small and we need
     * a much bigger stack for the main task.
     *
     * After creating the task, starts the dispatcher on the thread. Returns
     * the return value of the provided main task function once it finishes.
     *
     * @returns The result of `func`.
     */
    template<typename TSA, typename F, typename ...A>
    auto start(std::allocator_arg_t, TSA &&sa, F func, A &&...args)
        -> std::result_of_t<F(A...)>
    {
        detail::current_scheduler_owner iface{*this};

        using R = std::result_of_t<F(A...)>;

        if constexpr(std::is_same_v<R, void>) {
            if constexpr(sizeof...(A) == 0) {
                p_coros.emplace_back(std::move(func), std::forward<TSA>(sa));
            } else {
                p_coros.emplace_back(std::bind(
                    std::move(func), std::forward<A>(args)...
                ), std::forward<TSA>(sa));
            }
            dispatch();
        } else {
            R ret;
            if constexpr(sizeof...(A) == 0) {
                p_coros.emplace_back([&ret, lfunc = std::move(func)] {
                    ret = lfunc();
                }, std::forward<TSA>(sa));
            } else {
                p_coros.emplace_back([&ret, lfunc = std::bind(
                    std::move(func), std::forward<A>(args)...
                )]() {
                    ret = lfunc();
                }, std::forward<TSA>(sa));
            }
            dispatch();
            return ret;
        }
    }

    /** @brief Starts the scheduler given a set of arguments.
     *
     * Like start() but uses a fixed size stack that has the same size as
     * the main thread stack.
     *
     * The stack traits type is inherited from `SA`.
     *
     * @returns The result of `func`.
     */
    template<typename F, typename ...A>
    auto start(F func, A &&...args) -> std::result_of_t<F(A...)> {
        basic_fixedsize_stack<typename SA::traits_type, false> sa{
            detail::stack_main_size()
        };
        return start(
            std::allocator_arg, sa, std::move(func), std::forward<A>(args)...
        );
    }

    void do_spawn(std::function<void()> func) {
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

    void reserve_stacks(std::size_t n) {
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

/** An #ostd::basic_simple_coroutine_scheduler using #ostd::stack_pool. */
using simple_coroutine_scheduler = basic_simple_coroutine_scheduler<stack_pool>;

/** @brief A scheduler that uses a coroutine type for tasks on several threads.
 *
 * Effectively implements the M:N model. Runs on several threads, typically as
 * many as there are physical threads on your CPU(s), so it makes use of
 * multicore systems. The tasks bypass the coroutine_context::current() method,
 * so they're completely hidden from the outside code. This also has several
 * advantages for code using coroutines.
 *
 * @tparam SA The stack allocator to use when requesting stacks. Used for
 *            the tasks as well as for the stack request methods.
 */
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
    /* @brief Creates the scheduler.
     *
     * The number of threads defaults to the number of physical threads.
     *
     * @param[in] thrs The number of threads to use.
     * @param[in] sa The provided stack allocator.
     */
    basic_coroutine_scheduler(
        std::size_t thrs = std::thread::hardware_concurrency(), SA &&sa = SA{}
    ):
        p_threads(thrs), p_stacks(std::move(sa))
    {}

    ~basic_coroutine_scheduler() {}

    /** @brief Starts the scheduler given a set of arguments.
     *
     * Sets the internal current scheduler pointer to this scheduler creates
     * the main task using the separate provided stack allocator. This is
     * useful because the task stacks tend to be rather small and we need
     * a much bigger stack for the main task.
     *
     * After creating the task, creates the requested number of threads and
     * starts the dispatcher on each. Then it waits for all threads to finish
     * and returns the return value of the provided main task function.
     *
     * @returns The result of `func`.
     */
    template<typename TSA, typename F, typename ...A>
    auto start(std::allocator_arg_t, TSA &&sa, F func, A &&...args)
        -> std::result_of_t<F(A...)>
    {
        detail::current_scheduler_owner iface{*this};

        /* start with one task in the queue, this way we can
         * say we've finished when the task queue becomes empty
         */
        using R = std::result_of_t<F(A...)>;

        if constexpr(std::is_same_v<R, void>) {
            spawn_add(
                std::forward<TSA>(sa), std::move(func),
                std::forward<A>(args)...
            );
            /* actually start the thread pool */
            init();
        } else {
            R ret;
            spawn_add(
                std::forward<TSA>(sa),
                [&ret, func = std::move(func)](auto &&...args) {
                    ret = func(std::forward<A>(args)...);
                },
                std::forward<A>(args)...
            );
            init();
            return ret;
        }
    }

    /** @brief Starts the scheduler given a set of arguments.
     *
     * Like start() but uses a fixed size stack that has the same size as
     * the main thread stack.
     *
     * The stack traits type is inherited from `SA`.
     *
     * @returns The result of `func`.
     */
    template<typename F, typename ...A>
    auto start(F func, A &&...args) -> std::result_of_t<F(A...)> {
        /* the default 64 KiB stack won't cut it for main, allocate a stack
         * which matches the size of the process stack outside of the pool
         */
        basic_fixedsize_stack<typename SA::traits_type, false> sa{
            detail::stack_main_size()
        };
        return start(
            std::allocator_arg, sa, std::move(func), std::forward<A>(args)...
        );
    }

    void do_spawn(std::function<void()> func) {
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
        if constexpr(!SA::is_thread_safe) {
            std::lock_guard<std::mutex> l{p_lock};
            return p_stacks.allocate();
        } else {
            return p_stacks.allocate();
        }
    }

    void deallocate_stack(stack_context &st) noexcept {
        if constexpr(!SA::is_thread_safe) {
            std::lock_guard<std::mutex> l{p_lock};
            p_stacks.deallocate(st);
        } else {
            p_stacks.deallocate();
        }
    }

    void reserve_stacks(std::size_t n) {
        if constexpr(!SA::is_thread_safe) {
            std::lock_guard<std::mutex> l{p_lock};
            p_stacks.reserve(n);
        } else {
            p_stacks.reserve(n);
        }
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
        std::size_t size = p_threads;
        std::vector<std::thread> thrs;
        thrs.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            thrs.emplace_back([this]() { thread_run(); });
        }
        for (std::size_t i = 0; i < size; ++i) {
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

    std::size_t p_threads;
    std::condition_variable p_cond;
    std::mutex p_lock;
    SA p_stacks;
    tlist p_available;
    tlist p_waiting;
    tlist p_running;
};

/** An #ostd::basic_coroutine_scheduler using #ostd::stack_pool. */
using coroutine_scheduler = basic_coroutine_scheduler<stack_pool>;

/** @brief Spawns a task on the currently in use scheduler.
 *
 * The arguments are passed to the function. Effectively just calls
 * scheduler::spawn().
 */
template<typename F, typename ...A>
inline void spawn(F &&func, A &&...args) {
    detail::current_scheduler->spawn(
        std::forward<F>(func), std::forward<A>(args)...
    );
}

/** @brief Tells the current scheduler to re-schedule the current task.
 *
 * Effectively calls scheduler::yield().
 */
inline void yield() noexcept {
    detail::current_scheduler->yield();
}

/** @brief Creates a channel with the currently in use scheduler.
 *
 * Effectively calls scheduler::make_channel().
 *
 * @tparam T The type of the channel value.
 *
 */
template<typename T>
inline channel<T> make_channel() {
    return detail::current_scheduler->make_channel<T>();
}

/** @brief Creates a coroutine with the currently in use scheduler.
 *
 * Effectively calls scheduler::make_coroutine().
 *
 * @tparam T The type passed to the coroutine, `Result(Args...)`.
 *
 */
template<typename T, typename F>
inline coroutine<T> make_coroutine(F &&func) {
    return detail::current_scheduler->make_coroutine<T>(std::forward<F>(func));
}

/** @brief Creates a generator with the currently in use scheduler.
 *
 * Effectively calls scheduler::make_generator().
 *
 * @tparam T The value type of the generator.
 *
 */
template<typename T, typename F>
inline generator<T> make_generator(F &&func) {
    return detail::current_scheduler->make_generator<T>(std::forward<F>(func));
}

inline void reserve_stacks(std::size_t n) {
    detail::current_scheduler->reserve_stacks(n);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
