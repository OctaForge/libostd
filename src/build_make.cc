/* Build system implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/build/make.hh"

namespace ostd {
namespace build {

/* place the vtable in here */
make_task::~make_task() {}

static bool check_exec(
    string_range tname, std::vector<string_range> const &deps
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

void make::exec_rlist(string_range tname, std::vector<rule_inst> const &rlist) {
    std::vector<string_range> rdeps;
    if ((rlist.size() > 1) || !rlist[0].deps.empty()) {
        wait_for([&rlist, &rdeps, &tname, this]() {
            for (auto &sr: rlist) {
                for (auto &tgt: sr.deps) {
                    rdeps.push_back(tgt);
                    exec_rule(tgt, tname);
                }
            }
        });
    }
    make_rule *rl = nullptr;
    for (auto &sr: rlist) {
        if (sr.rule->has_body()) {
            rl = sr.rule;
            break;
        }
    }
    if (rl && (rl->action() || check_exec(tname, rdeps))) {
        std::unique_ptr<make_task> t{
            p_factory(tname, std::move(rdeps), *rl)
        };
        p_current = t.get();
        t->resume();
        if (!t->done()) {
            p_waiting.top()->push(std::move(t));
        }
    }
}

void make::exec_rule(string_range target, string_range from) {
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

void make::find_rules(string_range target, std::vector<rule_inst> &rlist) {
    if (!rlist.empty()) {
        return;
    }
    rule_inst *frule = nullptr;
    bool exact = false;
    string_range prev_sub{};
    for (auto &rule: p_rules) {
        if (target == string_range{rule.target()}) {
            rlist.emplace_back();
            rule_inst &sr = rlist.back();
            sr.rule = &rule;
            sr.deps.reserve(rule.depends().size());
            for (auto &d: rule.depends()) {
                sr.deps.push_back(d);
            }
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
        string_range sub = match_pattern(target, rule.target());
        if (!sub.empty()) {
            rlist.emplace_back();
            rule_inst &sr = rlist.back();
            sr.rule = &rule;
            sr.deps.reserve(rule.depends().size());
            for (auto &d: rule.depends()) {
                string_range dp = d;
                auto lp = ostd::find(dp, '%');
                if (!lp.empty()) {
                    auto repd = std::string{dp.slice(0, &lp[0] - &dp[0])};
                    repd.append(sub);
                    lp.pop_front();
                    repd.append(lp);
                    sr.deps.push_back(std::move(repd));
                } else {
                    sr.deps.push_back(d);
                }
            }
            if (frule) {
                if (sub.size() == prev_sub.size()) {
                    throw make_error{"redefinition of rule '%s'", target};
                }
                if (sub.size() < prev_sub.size()) {
                    *frule = sr;
                    rlist.pop_back();
                }
            } else {
                frule = &sr;
                prev_sub = sub;
            }
        }
    }
}

void make::wait_rest(std::queue<std::unique_ptr<make_task>> &tasks) {
    if (tasks.empty()) {
        /* nothing to wait for, so return */
        return;
    }
    /* cycle until tasks are done */
    std::unique_lock<std::mutex> lk{p_mtx};
    while (!p_avail) {
        p_cond.wait(lk);
    }
    std::queue<std::unique_ptr<make_task>> atasks;
    while (!tasks.empty()) {
        p_avail = false;
        while (!tasks.empty()) {
            try {
                auto t = std::move(tasks.front());
                tasks.pop();
                p_current = t.get();
                t->resume();
                if (!t->done()) {
                    /* still not dead, re-push */
                    atasks.push(std::move(t));
                }
            } catch (make_error const &) {
                writeln("waiting for the remaining tasks to finish...");
                for (; !tasks.empty(); tasks.pop()) {
                    try {
                        auto t = std::move(tasks.front());
                        tasks.pop();
                        while (!t->done()) {
                            p_current = t.get();
                            t->resume();
                        }
                    } catch (make_error const &) {
                        /* no rethrow */
                    }
                }
                throw;
            }
        }
        if (atasks.empty()) {
            break;
        }
        tasks.swap(atasks);
        /* so we're not busylooping */
        while (!p_avail) {
            p_cond.wait(lk);
        }
    }
}

OSTD_EXPORT void make::exec(string_range target) {
    wait_for([&target, this]() {
        exec_rule(target);
    });
}

OSTD_EXPORT std::shared_future<void> make::push_task(
    std::function<void()> func
) {
    return p_current->add_task(
        p_tpool.push([func = std::move(func), this]() {
            func();
            {
                std::lock_guard<std::mutex> l{p_mtx};
                p_avail = true;
            }
            p_cond.notify_one();
        })
    );
}

} /* namespace build */
} /* namespace ostd */
