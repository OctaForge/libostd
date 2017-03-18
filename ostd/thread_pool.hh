/* A thread pool that can be used standalone or within a more elaborate module.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_THREAD_POOL_HH
#define OSTD_THREAD_POOL_HH

#include <type_traits>
#include <functional>
#include <utility>
#include <vector>
#include <queue>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

namespace ostd {

struct thread_pool {
    void start(size_t size = std::thread::hardware_concurrency()) {
        p_running = true;
        auto tf = [this]() {
            thread_run();
        };
        for (size_t i = 0; i < size; ++i) {
            std::thread tid{tf};
            if (!tid.joinable()) {
                throw std::runtime_error{"thread_pool worker failed"};
            }
            p_thrs.push_back(std::move(tid));
        }
    }

    ~thread_pool() {
        destroy();
    }

    void destroy() {
        std::unique_lock<std::mutex> l{p_lock};
        if (!p_running) {
            return;
        }
        p_running = false;
        l.unlock();
        p_cond.notify_all();
        for (auto &tid: p_thrs) {
            tid.join();
            p_cond.notify_all();
        }
    }

    template<typename F, typename ...A>
    auto push(F &&func, A &&...args) -> std::result_of_t<F(A...)> {
        using R = std::result_of_t<F(A...)>;
        if constexpr(std::is_same_v<R, void>) {
            /* void-returning funcs return void */
            std::unique_lock<std::mutex> l{p_lock};
            if (!p_running) {
                throw std::runtime_error{"push on stopped thread_pool"};
            }
            p_tasks.push(
                std::bind(std::forward<F>(func), std::forward<A>(args)...)
            );
            p_cond.notify_one();
        } else {
            /* non-void-returning funcs return a future */
            std::packaged_task<R> t{
                std::bind(std::forward<F>(func), std::forward<A>(args)...)
            };
            auto ret = t.get_future();
            std::unique_lock<std::mutex> l{p_lock};
            if (!p_running) {
                throw std::runtime_error{"push on stopped thread_pool"};
            }
            p_tasks.emplace([t = std::move(t)]() {
                t();
            });
            p_cond.notify_one();
            return ret;
        }
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
            auto t = std::move(p_tasks.front());
            p_tasks.pop();
            l.unlock();
            t();
        }
    }

    std::condition_variable p_cond;
    std::mutex p_lock;
    std::vector<std::thread> p_thrs;
    std::queue<std::function<void()>> p_tasks;
    bool p_running = false;
};

} /* namespace ostd */

#endif
