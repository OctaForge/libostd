/** @defgroup Build Build system framework
 *
 * @brief A build system framework for build tools.
 *
 * This is a framework that can be used to create elaborate build systems
 * as well as simple standalone tools.
 *
 * @{
 */

/** @file make.hh
 *
 * @brief A dependency tracker core similar to Make.
 *
 * This implements a dependency tracking module that is essentially Make
 * but without any Make syntax or shell invocations. It is extensible and
 * can be adapted to many kinds of scenarios.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_BUILD_MAKE_HH
#define OSTD_BUILD_MAKE_HH

#include <list>
#include <vector>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <stdexcept>
#include <chrono>
#include <type_traits>

#include <ostd/range.hh>
#include <ostd/string.hh>
#include <ostd/thread_pool.hh>
#include <ostd/path.hh>
#include <ostd/io.hh>

namespace ostd {
namespace build {

/** @addtogroup Build
 * @{
 */

struct make_error: std::runtime_error {
    using std::runtime_error::runtime_error;

    template<typename ...A>
    make_error(string_range fmt, A const &...args):
        make_error(
            ostd::format(ostd::appender<std::string>(), fmt, args...).get()
        )
    {}
};

struct OSTD_EXPORT make_pattern {
    make_pattern() = delete;
    make_pattern(string_range target):
        p_target(target)
    {}

    std::pair<std::size_t, std::size_t> match(string_range target);
    std::string replace(string_range dep) const;
private:
    std::string p_target;
    std::vector<string_range> p_subs{};
};

struct make_rule {
    using body_func = std::function<
        void(string_range, iterator_range<string_range *>)
    >;

    make_rule() = delete;
    make_rule(string_range target):
        p_target(target)
    {}

    make_pattern &target() noexcept {
        return p_target;
    }

    make_pattern const &target() const noexcept {
        return p_target;
    }

    bool action() const noexcept {
        return p_action;
    }

    make_rule &action(bool act) noexcept {
        p_action = act;
        return *this;
    }

    make_rule &body(std::function<void()> act_f) noexcept {
        if (act_f) {
            p_body = [act_f = std::move(act_f)](auto, auto) {
                act_f();
            };
        } else {
            p_body = body_func{};
        }
        return *this;
    }

    bool has_body() const noexcept {
        return !!p_body;
    }

    make_rule &body(body_func rule_f) noexcept {
        p_body = std::move(rule_f);
        return *this;
    }

    make_rule &cond(std::function<bool(string_range)> cond_f) noexcept {
        p_cond = std::move(cond_f);
        return *this;
    }

    bool cond(string_range target) const {
        if (!p_cond) {
            return true;
        }
        return p_cond(target);
    }

    iterator_range<std::string const *> depends() const noexcept {
        return iterator_range<std::string const *>(
            p_deps.data(), p_deps.data() + p_deps.size()
        );
    }

    template<typename ...A>
    make_rule &depend(A const &...args) {
        (add_depend(args), ...);
        return *this;
    }

    make_rule &depend(std::initializer_list<string_range> il) {
        add_depend(il);
        return *this;
    }

    void call(string_range target, iterator_range<string_range *> srcs) {
        p_body(target, srcs);
    }

private:
    using rule_body = std::function<
        void(string_range, iterator_range<string_range *>)
    >;

    template<typename R>
    void add_depend(R const &v) {
        if constexpr (std::is_constructible_v<std::string, R const &>) {
            p_deps.emplace_back(v);
        } else {
            R mr{v};
            for (auto const &sv: mr) {
                p_deps.emplace_back(sv);
            }
        }
    }

    make_pattern p_target;
    std::vector<std::string> p_deps{};
    body_func p_body{};
    std::function<bool(string_range)> p_cond{};
    bool p_action = false;
};

struct make_task {
    make_task() {}
    virtual ~make_task();

    virtual bool done() const = 0;
    virtual void resume() = 0;
    virtual std::shared_future<void> add_task(std::future<void> f) = 0;
};

namespace detail {
    struct make_task_simple: make_task {
        make_task_simple() = delete;

        make_task_simple(
            string_range target, std::vector<string_range> deps, make_rule &rl
        ): p_body(
            [target, deps = std::move(deps), &rl]() mutable {
                rl.call(target, iterator_range<string_range *>(
                    deps.data(), deps.data() + deps.size()
                ));
            }
        ) {}

        bool done() const {
            return p_futures.empty();
        }

        void resume() {
            if (p_body) {
                std::exchange(p_body, nullptr)();
            }
            /* go over futures and erase those that are done */
            for (auto it = p_futures.begin(); it != p_futures.end();) {
                auto fs = it->wait_for(std::chrono::seconds(0));
                if (fs == std::future_status::ready) {
                    /* maybe propagate exception */
                    auto f = std::move(*it);
                    it = p_futures.erase(it);
                    try {
                        f.get();
                    } catch (...) {
                        p_futures.clear();
                        throw;
                    }
                    continue;
                }
                ++it;
            }
        }

        std::shared_future<void> add_task(std::future<void> f) {
            auto sh = f.share();
            p_futures.push_back(sh);
            return sh;
        }

    private:
        std::function<void()> p_body;
        std::list<std::shared_future<void>> p_futures{};
    };
}

inline make_task *make_task_simple(
    string_range target, std::vector<string_range> deps, make_rule &rl
) {
    return new detail::make_task_simple{target, std::move(deps), rl};
}

struct OSTD_EXPORT make {
    using task_factory = std::function<
        make_task *(string_range, std::vector<string_range>, make_rule &)
    >;

    make(
        task_factory factory, int threads = std::thread::hardware_concurrency()
    ): p_factory(factory) {
        p_tpool.start(threads);
    }

    void exec(string_range target);

    std::shared_future<void> push_task(std::function<void()> func);

    make_rule &rule(string_range tgt) {
        p_rules.emplace_back(tgt);
        return p_rules.back();
    }

    unsigned int threads() const noexcept {
        return p_tpool.threads();
    }

private:
    struct rule_inst {
        std::vector<std::string> deps;
        make_rule *rule;
    };

    OSTD_LOCAL void wait_rest(std::queue<std::unique_ptr<make_task>> &tasks);

    template<typename F>
    void wait_for(F func) {
        std::queue<std::unique_ptr<make_task>> tasks;
        p_waiting.push(&tasks);
        try {
            func();
        } catch (...) {
            p_waiting.pop();
            throw;
        }
        p_waiting.pop();
        wait_rest(tasks);
    }

    OSTD_LOCAL void exec_rlist(
        string_range tname, std::vector<rule_inst> const &rlist
    );

    OSTD_LOCAL void exec_rule(
        string_range target, string_range from = nullptr
    );

    OSTD_LOCAL void find_rules(
        string_range target, std::vector<rule_inst> &rlist
    );

    std::vector<make_rule> p_rules{};
    std::unordered_map<string_range, std::vector<rule_inst>> p_cache{};

    thread_pool p_tpool{};

    std::mutex p_mtx{};
    std::condition_variable p_cond{};
    std::stack<std::queue<std::unique_ptr<make_task>> *> p_waiting{};
    task_factory p_factory{};
    make_task *p_current = nullptr;
    bool p_avail = false;
};

/** @} */

} /* namespace build */
} /* namespace ostd */

#endif

/** @} */
