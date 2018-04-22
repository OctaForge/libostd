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
#include <stdexcept>
#include <chrono>
#include <type_traits>

#include <ostd/range.hh>
#include <ostd/string.hh>
#include <ostd/coroutine.hh>
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

namespace detail {
    static bool check_exec(
        string_range tname, std::vector<std::string> const &deps
    ) {
        if (!fs::exists(tname)) {
            return true;
        }
        for (auto &dep: deps) {
            if (!fs::exists(dep)) {
                return true;
            }
        }
        auto get_ts = [](string_range fname) {
            path p{fname};
            if (!fs::is_regular_file(p)) {
                return fs::file_time_t{};
            }
            return fs::last_write_time(p);
        };
        auto tts = get_ts(tname);
        if (tts == fs::file_time_t{}) {
            return true;
        }
        for (auto &dep: deps) {
            auto sts = get_ts(dep);
            if ((sts != fs::file_time_t{}) && (tts < sts)) {
                return true;
            }
        }
        return false;
    }

    /* this lets us properly match % patterns in target names */
    static string_range match_pattern(
        string_range expanded, string_range toexpand
    ) {
        auto rep = ostd::find(toexpand, '%');
        /* no subst found */
        if (rep.empty()) {
            return nullptr;
        }
        /* get the part before % */
        auto fp = toexpand.slice(0, &rep[0] - &toexpand[0]);
        /* part before % does not compare, so ignore */
        if (expanded.size() <= fp.size()) {
            return nullptr;
        }
        if (expanded.slice(0, fp.size()) != fp) {
            return nullptr;
        }
        /* pop out front part */
        expanded = expanded.slice(fp.size(), expanded.size());
        /* part after % */
        ++rep;
        if (rep.empty()) {
            return expanded;
        }
        /* part after % does not compare, so ignore */
        if (expanded.size() <= rep.size()) {
            return nullptr;
        }
        size_t es = expanded.size();
        if (expanded.slice(es - rep.size(), es) != rep) {
            return nullptr;
        }
        /* cut off latter part */
        expanded = expanded.slice(0, expanded.size() - rep.size());
        /* we got what we wanted... */
        return expanded;
    }
}

struct make_rule {
    using body_func = std::function<
        void(string_range, iterator_range<string_range *>)
    >;

    make_rule() = delete;
    make_rule(string_range target):
        p_target(target)
    {}

    string_range target() const noexcept {
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
        p_body = [act_f = std::move(act_f)](auto, auto) {
            act_f();
        };
        return *this;
    }

    bool has_body() const noexcept {
        return !!p_body;
    }

    make_rule &body(body_func rule_f) noexcept {
        p_body = std::move(rule_f);
        return *this;
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
            for (auto &sv: v) {
                p_deps.emplace_back(sv);
            }
        }
    }

    std::string p_target;
    std::vector<std::string> p_deps{};
    body_func p_body{};
    bool p_action = false;
};

struct make {
    make(int threads = std::thread::hardware_concurrency()) {
        p_tpool.start(threads);
    }

    void exec(string_range target) {
        wait_for([&target, this]() {
            exec_rule(target);
        });
    }

    void push_task(std::function<void()> func) {
        auto f = p_tpool.push([func = std::move(func), this]() {
            func();
            {
                std::lock_guard<std::mutex> l{p_mtx};
                p_avail = true;
            }
            p_cond.notify_one();
        });
        for (;;) {
            auto fs = f.wait_for(std::chrono::seconds(0));
            if (fs != std::future_status::ready) {
                /* keep yielding until ready */
                auto &cc = static_cast<coroutine<void()> &>(
                    *coroutine_context::current()
                );
                (coroutine<void()>::yield_type(cc))();
            } else {
                break;
            }
        }
    }

    make_rule &rule(string_range tgt) {
        p_rules.emplace_back(tgt);
        return p_rules.back();
    }

private:

    struct rule_inst {
        string_range sub;
        make_rule *rule;
    };

    template<typename F>
    void wait_for(F func) {
        std::queue<std::unique_ptr<coroutine<void()>>> coros;
        p_waiting.push(&coros);
        try {
            func();
        } catch (...) {
            p_waiting.pop();
            throw;
        }
        p_waiting.pop();
        if (coros.empty()) {
            /* nothing to wait for, so return */
            return;
        }
        /* cycle until coroutines are done */
        std::unique_lock<std::mutex> lk{p_mtx};
        while (!p_avail) {
            p_cond.wait(lk);
        }
        std::queue<std::unique_ptr<coroutine<void()>>> acoros;
        while (!coros.empty()) {
            p_avail = false;
            while (!coros.empty()) {
                try {
                    auto c = std::move(coros.front());
                    coros.pop();
                    c->resume();
                    if (*c) {
                        /* still not dead, re-push */
                        acoros.push(std::move(c));
                    }
                } catch (make_error const &) {
                    writeln("waiting for the remaining tasks to finish...");
                    for (; !coros.empty(); coros.pop()) {
                        try {
                            auto c = std::move(coros.front());
                            coros.pop();
                            while (*c) {
                                c->resume();
                            }
                        } catch (make_error const &) {
                            /* no rethrow */
                        }
                    }
                    throw;
                }
            }
            if (acoros.empty()) {
                break;
            }
            coros.swap(acoros);
            /* so we're not busylooping */
            while (!p_avail) {
                p_cond.wait(lk);
            }
        }
    }

    void exec_deps(
        std::vector<rule_inst> const &rlist, std::vector<std::string> &rdeps,
        string_range tname
    ) {
        std::string repd;
        for (auto &sr: rlist) {
            for (auto &target: sr.rule->depends()) {
                string_range atgt = ostd::iter(target);
                repd.clear();
                auto lp = ostd::find(atgt, '%');
                if (!lp.empty()) {
                    repd.append(atgt.slice(0, &lp[0] - &atgt[0]));
                    repd.append(sr.sub);
                    ++lp;
                    if (!lp.empty()) {
                        repd.append(lp);
                    }
                    atgt = ostd::iter(repd);
                }
                rdeps.push_back(std::string{atgt});
                exec_rule(atgt, tname);
            }
        }
    }

    void exec_rlist(string_range tname, std::vector<rule_inst> const &rlist) {
        std::vector<std::string> rdeps;
        if ((rlist.size() > 1) || !rlist[0].rule->depends().empty()) {
            wait_for([&rlist, &rdeps, &tname, this]() {
                exec_deps(rlist, rdeps, tname);
            });
        }
        make_rule *rl = nullptr;
        for (auto &sr: rlist) {
            if (sr.rule->has_body()) {
                rl = sr.rule;
                break;
            }
        }
        if (rl && (rl->action() || detail::check_exec(tname, rdeps))) {
            auto coro = std::make_unique<coroutine<void()>>(
                [tname, rdeps, rl](auto) {
                    std::vector<string_range> rdepsl;
                    for (auto &s: rdeps) {
                        rdepsl.push_back(s);
                    }
                    rl->call(tname, iterator_range<string_range *>(
                        rdepsl.data(), rdepsl.data() + rdepsl.size()
                    ));
                }
            );
            coro->resume();
            if (*coro) {
                p_waiting.top()->push(std::move(coro));
            }
        }
    }

    void exec_rule(string_range target, string_range from = nullptr) {
        std::vector<rule_inst> &rlist = p_cache[target];
        find_rules(target, rlist);
        if (rlist.empty()) {
            if (fs::exists(target)) {
                return;
            }
            if (from.empty()) {
                throw make_error{"no rule to exec target '%s'", target};
            } else {
                throw make_error{
                    "no rule to exec target '%s' (needed by '%s')", target, from
                };
            }
        }
        exec_rlist(target, rlist);
    }

    void find_rules(string_range target, std::vector<rule_inst> &rlist) {
        if (!rlist.empty()) {
            return;
        }
        rule_inst *frule = nullptr;
        bool exact = false;
        for (auto &rule: p_rules) {
            if (target == string_range{rule.target()}) {
                rlist.emplace_back();
                rlist.back().rule = &rule;
                if (rule.has_body()) {
                    if (frule && exact) {
                        throw make_error{"redefinition of rule '%s'", target};
                    }
                    if (!frule) {
                        frule = &rlist.back();
                    } else {
                        *frule = rlist.back();
                        rlist.pop_back();
                    }
                    exact = true;
                }
                continue;
            }
            if (exact || !rule.has_body()) {
                continue;
            }
            string_range sub = detail::match_pattern(target, rule.target());
            if (!sub.empty()) {
                rlist.emplace_back();
                rule_inst &sr = rlist.back();
                sr.rule = &rule;
                sr.sub = sub;
                if (frule) {
                    if (sub.size() == frule->sub.size()) {
                        throw make_error{"redefinition of rule '%s'", target};
                    }
                    if (sub.size() < frule->sub.size()) {
                        *frule = sr;
                        rlist.pop_back();
                    }
                } else {
                    frule = &sr;
                }
            }
        }
    }

    std::vector<make_rule> p_rules{};
    std::unordered_map<string_range, std::vector<rule_inst>> p_cache{};

    thread_pool p_tpool{};

    std::mutex p_mtx{};
    std::condition_variable p_cond{};
    std::stack<std::queue<std::unique_ptr<coroutine<void()>>> *> p_waiting{};
    bool p_avail = false;
};

/** @} */

} /* namespace build */
} /* namespace ostd */

#endif

/** @} */
