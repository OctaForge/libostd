/** @addtogroup Concurrency
 * @{
 */

/** @file thread_pool.hh
 *
 * @brief A pool of threads with workers.
 *
 * This file implements a regular thread pool with worker threads. It does
 * not do any elaborate stuff with coroutines or task scheduling.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_THREAD_POOL_HH
#define OSTD_THREAD_POOL_HH

#include <cstddef>
#include <type_traits>
#include <functional>
#include <utility>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

namespace ostd {

/** @addtogroup Concurrency
 * @{
 */

namespace detail {
    struct tpool_func_base {
        tpool_func_base() {}
        virtual ~tpool_func_base();
        virtual void clone(tpool_func_base *func) = 0;
        virtual void call() = 0;
    };

    template<typename F>
    struct tpool_func_impl: tpool_func_base {
        tpool_func_impl(F &&func): p_func(std::move(func)) {}

        void clone(tpool_func_base *p) {
            new (p) tpool_func_impl(std::move(p_func));
        }

        void call() {
            p_func();
        }

    private:
        F p_func;
    };

    struct tpool_func {
        tpool_func() = delete;
        tpool_func(tpool_func const &) = delete;
        tpool_func &operator=(tpool_func const &) = delete;

        tpool_func(tpool_func &&func) {
            if (static_cast<void *>(func.p_func) == &func.p_buf) {
                p_func = reinterpret_cast<tpool_func_base *>(&p_buf);
                func.p_func->clone(p_func);
            } else {
                p_func = func.p_func;
                func.p_func = nullptr;
            }
        }

        template<typename F>
        tpool_func(F &&func) {
            if (sizeof(tpool_func_impl<F>) <= sizeof(p_buf)) {
                p_func = ::new(reinterpret_cast<void *>(&p_buf))
                    tpool_func_impl<F>{std::move(func)};
            } else {
                p_func = new tpool_func_impl<F>{std::move(func)};
            }
        }

        ~tpool_func() {
            if (static_cast<void *>(p_func) == &p_buf) {
                p_func->~tpool_func_base();
            } else {
                delete p_func;
            }
        }

        void operator()() {
            p_func->call();
        }
    private:
        std::aligned_storage_t<
            sizeof(tpool_func_impl<std::packaged_task<void()>>),
            alignof(tpool_func_impl<std::packaged_task<void()>>)
        > p_buf;
        tpool_func_base *p_func;
    };
}

/** @brief A thread pool.
 *
 * A simple thread pool that lets you start a specified number of threads
 * and queue tasks onto them. No elaborate scheduling is performed, tasks
 * are called on threads as they become available and are assumed completed
 * once they return.
 */
struct thread_pool {
    /** @brief Starts the thread pool.
     *
     * Creates the threads and marks the pool as running. The number of
     * threads defaults to the number of hardware threads in your system.
     *
     * @param[in] size The number of threads to use.
     */
    void start(std::size_t size = std::thread::hardware_concurrency()) {
        p_running = true;
        auto tf = [this]() {
            thread_run();
        };
        for (std::size_t i = 0; i < size; ++i) {
            p_thrs.push_back(std::thread{tf});
        }
    }

    /** @brief Calls destroy(). */
    ~thread_pool() {
        destroy();
    }

    /** @brief Destroys the thread pool.
     *
     * If the pool is not running, this method simply returns. Otherwise
     * it notifies all threads to run any remaining queued tasks and
     * proceeds to wait for every thread to finish, notifying the rest
     * every time after a thread successfully exits.
     */
    void destroy() {
        {
            std::lock_guard<std::mutex> l{p_lock};
            if (!p_running) {
                return;
            }
            p_running = false;
        }
        p_cond.notify_all();
        for (auto &tid: p_thrs) {
            tid.join();
            p_cond.notify_all();
        }
        p_thrs.clear();
    }

    /** @brief Queues a new task for execution.
     *
     * Queues the given function for execution. Any extra passed parameters
     * are bound to the function first before queuing. It also lets you
     * retrieve the return value of the task at later point, via the
     * returned future.
     *
     * The function's argument types and the provided arguments must match.
     *
     * @param[in] func The function to queue.
     * @param[in] args A parameter pack matching the function's arguments.
     *
     * @returns A future to the return type of the task.
     *
     * @throws std::runtime_error if the pool is not running.
     */
    template<typename F, typename ...A>
    auto push(F &&func, A &&...args) ->
        std::future<std::result_of_t<F(A...)>>
    {
        using R = std::result_of_t<F(A...)>;
        std::packaged_task<R()> t;
        if constexpr(sizeof...(A) == 0) {
            t = std::packaged_task<R()>{std::forward<F>(func)};
        } else {
            t = std::packaged_task<R()>{
                std::bind(std::forward<F>(func), std::forward<A>(args)...)
            };
        }
        auto ret = t.get_future();
        {
            std::lock_guard<std::mutex> l{p_lock};
            if (!p_running) {
                throw std::runtime_error{"push on stopped thread_pool"};
            }
            p_tasks.emplace(std::move(t));
        }
        p_cond.notify_one();
        return ret;
    }

    /** @brief Gets the number of threads in the pool. */
    unsigned int threads() const noexcept {
        return p_thrs.size();
    }

private:
    void thread_run() {
        for (;;) {
            std::unique_lock<std::mutex> l{p_lock};
            while (p_running && p_tasks.empty()) {
                p_cond.wait(l);
            }
            if (!p_running && p_tasks.empty()) {
                return;
            }
            auto t{std::move(p_tasks.front())};
            p_tasks.pop();
            l.unlock();
            t();
        }
    }

    std::condition_variable p_cond;
    std::mutex p_lock;
    std::vector<std::thread> p_thrs;
    std::queue<detail::tpool_func> p_tasks;
    bool p_running = false;
};

/** @} */

} /* namespace ostd */

#endif

/** @} */
