/** @addtogroup Utilities
 * @{
 */

/** @file argúparse.hh
 *
 * @brief Portable argument parsing.
 *
 * Provides a powerful argument parser that can handle a wide variety of
 * cases, including POSIX and GNU argument ordering, different argument
 * formats, optional values and type conversions.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_ARGPARSE_HH
#define OSTD_ARGPARSE_HH

#include <cstdio>
#include <cctype>
#include <algorithm>
#include <optional>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>
#include <functional>

#include "ostd/algorithm.hh"
#include "ostd/format.hh"
#include "ostd/string.hh"
#include "ostd/io.hh"

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

struct arg_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class arg_type {
    OPTIONAL = 0,
    POSITIONAL,
    GROUP,
    MUTUALLY_EXCLUSIVE_GROUP
};

enum class arg_value {
    EXACTLY,
    OPTIONAL,
    ALL,
    REST
};

struct arg_description {
    friend struct arg_description_container;
    friend struct arg_mutually_exclusive_group;
    friend struct arg_group;

    virtual ~arg_description() {}

    virtual arg_type type() const = 0;

protected:
    virtual arg_description *find_arg(
        string_range, std::optional<arg_type>, bool
    ) {
        return nullptr;
    }

    arg_description() {}
};

struct arg_argument: arg_description {
    arg_argument &help(string_range str) {
        p_helpstr = std::string{str};
        return *this;
    }

    string_range get_help() const {
        return p_helpstr;
    }

    arg_argument &metavar(string_range str) {
        p_metavar = std::string{str};
        return *this;
    }

    string_range get_metavar() const {
        return p_metavar;
    }

    arg_value needs_value() const {
        return p_valreq;
    }

    std::size_t get_nargs() const {
        return p_nargs;
    }

protected:
    arg_argument(arg_value req, std::size_t nargs):
        arg_description(), p_valreq(req), p_nargs(nargs)
    {}
    arg_argument(std::size_t nargs):
        arg_description(),
        p_valreq(arg_value::EXACTLY),
        p_nargs(nargs)
    {}

    std::string p_helpstr, p_metavar;
    arg_value p_valreq;
    std::size_t p_nargs;
};

struct arg_optional: arg_argument {
    template<typename HelpFormatter>
    friend struct basic_arg_parser;
    friend struct arg_description_container;
    friend struct arg_group;
    friend struct arg_mutually_exclusive_group;

    arg_type type() const {
        return arg_type::OPTIONAL;
    }

    std::size_t used() const {
        return p_used;
    }

    template<typename F>
    arg_optional &action(F func) {
        p_action = func;
        return *this;
    }

    arg_optional &help(string_range str) {
        arg_argument::help(str);
        return *this;
    }

    arg_optional &metavar(string_range str) {
        arg_argument::metavar(str);
        return *this;
    }

    arg_optional &limit(std::size_t n) {
        p_limit = n;
        return *this;
    }

    arg_optional &add_name(string_range name) {
        p_names.emplace_back(name);
        return *this;
    }

    auto get_names() const {
        return iter(p_names);
    }

protected:
    arg_optional() = delete;

    arg_optional(string_range name, arg_value req, std::size_t nargs = 1):
        arg_argument(req, nargs)
    {
        validate_req(req);
        p_names.emplace_back(name);
    }
    arg_optional(string_range name, std::size_t nargs):
        arg_argument(nargs)
    {
        p_names.emplace_back(name);
    }

    arg_optional(
        string_range name1, string_range name2, arg_value req,
        std::size_t nargs = 1
    ):
        arg_argument(req, nargs)
    {
        validate_req(req);
        p_names.emplace_back(name1);
        p_names.emplace_back(name2);
    }
    arg_optional(string_range name1, string_range name2, std::size_t nargs):
        arg_argument(nargs)
    {
        p_names.emplace_back(name1);
        p_names.emplace_back(name2);
    }

    arg_description *find_arg(
        string_range name, std::optional<arg_type> tp, bool
    ) {
        if (tp && (*tp != type())) {
            return nullptr;
        }
        for (auto const &nm: p_names) {
            if (name == iter(nm)) {
                return this;
            }
        }
        return nullptr;
    }

    void set_values(
        string_range argname, iterator_range<string_range const *> vals
    ) {
        if (p_limit && (p_used == p_limit)) {
            throw arg_error{format(
                appender<std::string>(),
                "argument '%s' can be used at most %d times", argname, p_limit
            ).get()};
        }
        ++p_used;
        if (p_action) {
            p_action(vals);
        }
    }

private:
    void validate_req(arg_value req) {
        switch (req) {
            case arg_value::EXACTLY:
            case arg_value::OPTIONAL:
            case arg_value::ALL:
                break;
            default:
                throw arg_error{"invalid argument requirement"};
        }
    }

    std::function<void(iterator_range<string_range const *>)> p_action;
    std::vector<std::string> p_names;
    std::size_t p_used = 0, p_limit = 0;
};

struct arg_positional: arg_argument {
    template<typename HelpFormatter>
    friend struct basic_arg_parser;
    friend struct arg_description_container;

    arg_type type() const {
        return arg_type::POSITIONAL;
    }

    string_range get_name() const {
        return p_name;
    }

    template<typename F>
    arg_positional &action(F func) {
        p_action = func;
        return *this;
    }

    arg_positional &help(string_range str) {
        arg_argument::help(str);
        return *this;
    }

    arg_positional &metavar(string_range str) {
        arg_argument::metavar(str);
        return *this;
    }

    bool used() const {
        return p_used;
    }

protected:
    arg_positional() = delete;
    arg_positional(
        string_range name, arg_value req = arg_value::EXACTLY,
        std::size_t nargs = 1
    ):
        arg_argument(req, nargs),
        p_name(name)
    {}
    arg_positional(string_range name, std::size_t nargs):
        arg_argument(nargs),
        p_name(name)
    {}

    arg_description *find_arg(
        string_range name, std::optional<arg_type> tp, bool
    ) {
        if ((tp && (*tp != type())) || (name != ostd::citer(p_name))) {
            return nullptr;
        }
        return this;
    }

    void set_values(iterator_range<string_range const *> vals) {
        if (p_action) {
            p_action(vals);
        }
        p_used = true;
    }

private:
    std::function<void(iterator_range<string_range const *>)> p_action;
    std::string p_name;
    bool p_used = false;
};

struct arg_mutually_exclusive_group: arg_description {
    friend struct arg_description_container;

    arg_type type() const {
        return arg_type::MUTUALLY_EXCLUSIVE_GROUP;
    }

    template<typename ...A>
    arg_optional &add_optional(A &&...args) {
        arg_description *p = new arg_optional(std::forward<A>(args)...);
        return static_cast<arg_optional &>(*p_opts.emplace_back(p));
    }

    template<typename F>
    bool for_each(F &&func, bool iter_ex, bool iter_grp) const {
        return for_each_impl(func, iter_ex, iter_grp);
    }

    bool required() const {
        return p_required;
    }

protected:
    arg_mutually_exclusive_group(bool required = false):
        p_required(required)
    {}

    arg_description *find_arg(
        string_range name, std::optional<arg_type> tp, bool parsing
    ) {
        string_range used;
        for (auto &opt: p_opts) {
            if (auto *p = opt->find_arg(name, tp, parsing); p) {
                if (parsing && !used.empty()) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "argument '%s' not allowed with argument '%s'",
                        name, used
                    ).get()};
                }
                return p;
            }
            auto &optr = static_cast<arg_optional &>(*opt);
            if (optr.used()) {
                for (auto &n: optr.get_names()) {
                    if (n.size() > used.size()) {
                        used = n;
                    }
                }
            }
        }
        return nullptr;
    }

private:
    template<typename F>
    bool for_each_impl(F &func, bool, bool) const {
        for (auto &desc: p_opts) {
            if (!func(*desc)) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::unique_ptr<arg_description>> p_opts;
    bool p_required;
};

struct arg_description_container {
    template<typename ...A>
    arg_optional &add_optional(A &&...args) {
        arg_description *p = new arg_optional(std::forward<A>(args)...);
        return static_cast<arg_optional &>(*p_opts.emplace_back(p));
    }

    template<typename ...A>
    arg_positional &add_positional(A &&...args) {
        arg_description *p = new arg_positional(std::forward<A>(args)...);
        return static_cast<arg_positional &>(*p_opts.emplace_back(p));
    }

    template<typename ...A>
    arg_mutually_exclusive_group &add_mutually_exclusive_group(A &&...args) {
        arg_description *p = new arg_mutually_exclusive_group(
            std::forward<A>(args)...
        );
        return static_cast<arg_mutually_exclusive_group &>(
            *p_opts.emplace_back(p)
        );
    }

    template<typename F>
    bool for_each(F &&func, bool iter_ex, bool iter_grp) const {
        return for_each_impl(func, iter_ex, iter_grp);
    }

protected:
    arg_description_container() {}

    template<typename AT>
    AT &find_arg(string_range name, bool parsing) {
        for (auto &p: p_opts) {
            auto *pp = p->find_arg(name, std::nullopt, parsing);
            if (!pp) {
                continue;
            }
            if (auto *r = dynamic_cast<AT *>(pp); r) {
                return *r;
            }
            break;
        }
        throw arg_error{format(
            appender<std::string>(), "unknown argument '%s'", name
        ).get()};
    }

    template<typename F>
    bool for_each_impl(F &func, bool iter_ex, bool iter_grp) const;

    std::vector<std::unique_ptr<arg_description>> p_opts;
};

struct arg_group: arg_description, arg_description_container {
    template<typename HelpFormatter>
    friend struct basic_arg_parser;

    arg_type type() const {
        return arg_type::GROUP;
    }

    string_range get_name() const {
        return p_name;
    }

protected:
    arg_group() = delete;
    arg_group(string_range name):
        arg_description(), arg_description_container(), p_name(name)
    {}

    arg_description *find_arg(
        string_range name, std::optional<arg_type> tp, bool parsing
    ) {
        for (auto &opt: p_opts) {
            if (auto *p = opt->find_arg(name, tp, parsing); p) {
                return p;
            }
        }
        return nullptr;
    }

private:
    std::string p_name;
    std::vector<std::unique_ptr<arg_description>> p_opts;
};

template<typename F>
inline bool arg_description_container::for_each_impl(
    F &func, bool iter_ex, bool iter_grp
) const {
    for (auto &desc: p_opts) {
        switch (desc->type()) {
            case arg_type::OPTIONAL:
            case arg_type::POSITIONAL:
                if (!func(*desc)) {
                    return false;
                }
                break;
            case arg_type::GROUP:
                if (!iter_grp) {
                    if (!func(*desc)) {
                        return false;
                    }
                    continue;
                }
                if (!static_cast<arg_group const &>(*desc).for_each(
                    func, iter_ex, iter_grp
                )) {
                    return false;
                }
                break;
            case arg_type::MUTUALLY_EXCLUSIVE_GROUP:
                if (!iter_ex) {
                    if (!func(*desc)) {
                        return false;
                    }
                    continue;
                }
                if (!static_cast<arg_mutually_exclusive_group const &>(
                    *desc
                ).for_each(func, iter_ex, iter_grp)) {
                    return false;
                }
                break;
            default:
                /* should never happen */
                throw arg_error{"invalid argument type"};
        }
    }
    return true;
}

template<typename HelpFormatter>
struct basic_arg_parser: arg_description_container {
private:
    struct parse_stop {};

public:
    basic_arg_parser(
        string_range progname = string_range{}, bool posix = false
    ):
        arg_description_container(), p_progname(progname), p_posix(posix)
    {}

    template<typename ...A>
    arg_group &add_group(A &&...args) {
        arg_description *p = new arg_group(std::forward<A>(args)...);
        return static_cast<arg_group &>(*p_opts.emplace_back(p));
    }

    void parse(int argc, char **argv) {
        if (p_progname.empty()) {
            p_progname = argv[0];
        }
        parse(ostd::iter(&argv[1], &argv[argc]));
    }

    template<typename InputRange>
    void parse(InputRange args) {
        /* count positional args until remainder */
        std::size_t npos = 0;
        bool has_rest = false;
        for_each([&has_rest, &npos](auto const &arg) {
            if (arg.type() != arg_type::POSITIONAL) {
                return true;
            }
            auto const &desc = static_cast<arg_positional const &>(arg);
            if (desc.needs_value() == arg_value::REST) {
                has_rest = true;
                return false;
            }
            ++npos;
            return true;
        }, true, true);
        bool allow_optional = true;
        while (!args.empty()) {
            string_range s{args.front()};
            if (s == "--") {
                args.pop_front();
                allow_optional = false;
                continue;
            }
            if (allow_optional && is_optarg(s)) {
                try {
                    parse_opt(s, args);
                } catch (parse_stop) {
                    return;
                }
                continue;
            }
            if (p_posix) {
                allow_optional = false;
            }
            parse_pos(s, args, allow_optional);
            if (has_rest && npos) {
                --npos;
                if (!npos && !args.empty()) {
                    /* parse rest after all preceding positionals are filled
                     * if the only positional consumes rest, it will be filled
                     * by the above when the first non-optional is encountered
                     */
                    parse_pos(string_range{args.front()}, args, false);
                }
            }
        }
        for_each([](auto const &arg) {
            if (arg.type() == arg_type::MUTUALLY_EXCLUSIVE_GROUP) {
                auto &mgrp = static_cast<
                    arg_mutually_exclusive_group const &
                >(arg);
                if (!mgrp.required()) {
                    return true;
                }
                std::vector<string_range> names;
                bool cont = false;
                mgrp.for_each([&names, &cont](auto const &marg) {
                    string_range cn;
                    auto const &mopt = static_cast<arg_optional const &>(marg);
                    for (auto &n: mopt.get_names()) {
                        if (n.size() > cn.size()) {
                            cn = n;
                        }
                    }
                    if (!cn.empty()) {
                        names.push_back(cn);
                    }
                    if (mopt.used()) {
                        cont = true;
                        return false;
                    }
                    return true;
                }, true, true);
                if (!cont) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "one of the arguments %('%s'%|, %) is required", names
                    ).get()};
                }
                return true;
            }
            if (arg.type() != arg_type::POSITIONAL) {
                return true;
            }
            auto const &desc = static_cast<arg_positional const &>(arg);
            auto needs = desc.needs_value();
            auto nargs = desc.get_nargs();
            if ((needs != arg_value::EXACTLY) && (needs != arg_value::ALL)) {
                return true;
            }
            if (!nargs || desc.used()) {
                return true;
            }
            throw arg_error{"too few arguments"};
        }, false, true);
    }

    template<typename OutputRange>
    OutputRange &&print_help(OutputRange &&range) {
        p_helpfmt.format_usage(range);
        p_helpfmt.format_options(range);
        return std::forward<OutputRange>(range);
    }

    void print_help() {
        print_help(cout.iter());
    }

    arg_argument &get(string_range name) {
        return find_arg<arg_argument>(name, false);
    }

    string_range get_progname() const {
        return p_progname;
    }

    bool posix_ordering() const {
        return p_posix;
    }

    bool posix_ordering(bool v) {
        return std::exchange(p_posix, v);
    }

    void stop_parsing() {
        throw parse_stop{};
    }

private:
    static bool is_optarg(string_range arg) {
        return (!arg.empty() && (arg[0] == '-') && (arg != "-"));
    }

    template<typename R>
    void parse_opt(string_range argr, R &args) {
        std::vector<std::string> vals;
        if (auto sv = find(argr, '='); !sv.empty()) {
            argr = argr.slice(0, argr.size() - sv.size());
            sv.pop_front();
            vals.emplace_back(sv);
        }
        args.pop_front();
        std::string arg{argr};

        auto &desc = find_arg<arg_optional>(arg, true);
        auto needs = desc.needs_value();
        auto nargs = desc.get_nargs();

        /* optional argument takes no values */
        if ((needs == arg_value::EXACTLY) && !nargs) {
            /* value was provided through = */
            if (!vals.empty()) {
                throw arg_error{format(
                    appender<std::string>(), "argument '%s' takes no value",
                    arg
                ).get()};
            }
            desc.set_values(arg, nullptr);
            return;
        }
        if (
            vals.empty() ||
            (needs == arg_value::ALL) ||
            ((needs == arg_value::EXACTLY) && (nargs > 1))
        ) {
            auto rargs = nargs;
            if ((needs == arg_value::EXACTLY) && !vals.empty()) {
                --rargs;
            }
            for (;;) {
                bool pval = !args.empty() && !is_optarg(args.front());
                if ((needs == arg_value::EXACTLY) && rargs && !pval) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "argument '%s' needs exactly %d values",
                        arg, nargs
                    ).get()};
                }
                if (!pval || ((needs == arg_value::EXACTLY) && !rargs)) {
                    break;
                }
                vals.emplace_back(args.front());
                args.pop_front();
                if (rargs) {
                    --rargs;
                }
            }
        }
        if ((needs == arg_value::ALL) && (nargs > vals.size())) {
            throw arg_error{format(
                appender<std::string>(),
                "argument '%s' needs at least %d values", arg, nargs
            ).get()};
        }
        if (!vals.empty()) {
            std::vector<string_range> srvals;
            for (auto const &s: vals) {
                srvals.push_back(s);
            }
            desc.set_values(
                arg, ostd::iter(&srvals[0], &srvals[srvals.size()])
            );
        } else {
            desc.set_values(arg, nullptr);
        }
    }

    template<typename R>
    void parse_pos(string_range argr, R &args, bool allow_opt) {
        arg_positional *descp = nullptr;
        for (auto &popt: p_opts) {
            if (popt->type() != arg_type::POSITIONAL) {
                continue;
            }
            arg_positional &o = *static_cast<arg_positional *>(popt.get());
            if (o.used()) {
                continue;
            }
            descp = &o;
            break;
        }

        if (!descp) {
            throw arg_error{format(
                appender<std::string>(), "unexpected argument '%s'", argr
            ).get()};
        }

        arg_positional &desc = *descp;
        auto needs = desc.needs_value();
        auto nargs = desc.get_nargs();

        std::vector<std::string> vals;
        vals.emplace_back(argr);
        args.pop_front();

        if (needs == arg_value::REST) {
            for (; !args.empty(); args.pop_front()) {
                vals.emplace_back(args.front());
            }
        } else if (needs == arg_value::ALL) {
            for (; !args.empty(); args.pop_front()) {
                string_range v = args.front();
                if (allow_opt && is_optarg(v)) {
                    break;
                }
                vals.emplace_back(v);
            }
            if (nargs > vals.size()) {
                throw arg_error{format(
                    appender<std::string>(),
                    "positional argument '%s' needs at least %d values",
                    desc.get_name(), nargs
                ).get()};
            }
        } else if ((needs == arg_value::EXACTLY) && (nargs > 1)) {
            auto reqargs = nargs - 1;
            while (reqargs) {
                if (args.empty() || (allow_opt && is_optarg(args.front()))) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "positional argument '%s' needs exactly %d values",
                        desc.get_name(), nargs
                    ).get()};
                }
                vals.emplace_back(args.front());
                args.pop_front();
            }
        } /* else is OPTIONAL and we already have an arg */

        std::vector<string_range> srvals;
        for (auto const &s: vals) {
            srvals.push_back(s);
        }
        desc.set_values(ostd::iter(&srvals[0], &srvals[srvals.size()]));
    }

    std::string p_progname;
    HelpFormatter p_helpfmt{*this};
    bool p_posix = false;
};

struct default_help_formatter {
    default_help_formatter(basic_arg_parser<default_help_formatter> &p):
        p_parser(p)
    {}

    template<typename OutputRange>
    void format_usage(OutputRange &out) {
        string_range progname = p_parser.get_progname();
        if (progname.empty()) {
            progname = "program";
        }
        format(out, "Usage: %s [opts] [args]\n", progname);
    }

    template<typename OutputRange>
    void format_options(OutputRange &out) {
        std::size_t opt_namel = 0, pos_namel = 0, grp_namel = 0;

        std::vector<arg_optional const *> allopt;
        std::vector<arg_positional const *> allpos;

        p_parser.for_each([
            &allopt, &allpos, &opt_namel, &pos_namel, &grp_namel, this
        ](auto const &parg) {
            auto cs = counting_sink(noop_sink<char>());
            switch (parg.type()) {
                case arg_type::OPTIONAL: {
                    auto &opt = static_cast<arg_optional const &>(parg);
                    this->format_option(cs, opt);
                    opt_namel = std::max(opt_namel, cs.get_written());
                    allopt.push_back(&opt);
                    break;
                }
                case arg_type::POSITIONAL: {
                    auto &opt = static_cast<arg_positional const &>(parg);
                    this->format_option(cs, opt);
                    pos_namel = std::max(pos_namel, cs.get_written());
                    allpos.push_back(&opt);
                    break;
                }
                case arg_type::GROUP:
                    static_cast<arg_group const &>(parg).for_each(
                        [&cs, &grp_namel, this](auto const &arg) {
                            auto ccs = cs;
                            this->format_option(ccs, arg);
                            grp_namel = std::max(grp_namel, ccs.get_written());
                            return true;
                        }, true, true
                    );
                    break;
                case arg_type::MUTUALLY_EXCLUSIVE_GROUP:
                    static_cast<arg_mutually_exclusive_group const &>(
                        parg
                    ).for_each(
                        [&cs, &opt_namel, &allopt, this](auto const &arg) {
                            auto ccs = cs;
                            this->format_option(ccs, arg);
                            opt_namel = std::max(opt_namel, ccs.get_written());
                            allopt.push_back(static_cast<
                                arg_optional const *
                            >(&arg));
                            return true;
                        }, true, true
                    );
                    break;
                default:
                    break;
            }
            return true;
        }, false, false);
        std::size_t maxpad = std::max({opt_namel, pos_namel, grp_namel});

        auto write_help = [maxpad](
            auto &out, arg_argument const &arg, std::size_t len
        ) {
            auto help = arg.get_help();
            if (help.empty()) {
                out.put('\n');
            } else {
                std::size_t nd = maxpad - len + 2;
                for (std::size_t i = 0; i < nd; ++i) {
                    out.put(' ');
                }
                format(out, "%s\n", help);
            }
        };

        if (!allpos.empty()) {
            format(out, "\nPositional arguments:\n");
            for (auto p: allpos) {
                format(out, "  ");
                auto &parg = *p;
                auto cr = counting_sink(out);
                format_option(cr, parg);
                out = std::move(cr.get_range());
                write_help(out, parg, cr.get_written());
            }
        }

        if (!allopt.empty()) {
            format(out, "\nOptional arguments:\n");
            for (auto &p: allopt) {
                format(out, "  ");
                auto &oarg = *p;
                auto cr = counting_sink(out);
                format_option(cr, oarg);
                out = std::move(cr.get_range());
                write_help(out, oarg, cr.get_written());
            }
        }

        allopt.clear();
        allpos.clear();

        p_parser.for_each([
            &write_help, &out, &allopt, &allpos, this
        ](auto const &arg) {
            if (arg.type() != arg_type::GROUP) {
                return true;
            }
            auto &garg = static_cast<arg_group const &>(arg);
            format(out, "\n%s:\n", garg.get_name());
            garg.for_each([
                &write_help, &out, &allopt, &allpos, this
            ](auto const &marg) {
                switch (marg.type()) {
                    case arg_type::OPTIONAL:
                        allopt.push_back(
                            static_cast<arg_optional const *>(&marg)
                        );
                        break;
                    case arg_type::POSITIONAL:
                        allpos.push_back(
                            static_cast<arg_positional const *>(&marg)
                        );
                        break;
                    default:
                        /* should never happen */
                        throw arg_error{"invalid argument type"};
                }
                return true;
            }, true, false);
            if (!allpos.empty()) {
                for (auto p: allpos) {
                    format(out, "  ");
                    auto &parg = *p;
                    auto cr = counting_sink(out);
                    format_option(cr, parg);
                    out = std::move(cr.get_range());
                    write_help(out, parg, cr.get_written());
                }
            }
            if (!allopt.empty()) {
                for (auto &p: allopt) {
                    format(out, "  ");
                    auto &oarg = *p;
                    auto cr = counting_sink(out);
                    format_option(cr, oarg);
                    out = std::move(cr.get_range());
                    write_help(out, oarg, cr.get_written());
                }
            }
            return true;
        }, false, false);
    }

    template<typename OutputRange>
    void format_option(OutputRange &out, arg_optional const &arg) {
        auto names = arg.get_names();
        std::string mt{arg.get_metavar()};
        if (mt.empty()) {
            for (auto &s: names) {
                if (!starts_with(s, "--")) {
                    continue;
                }
                string_range mtr = s;
                while (!mtr.empty() && (mtr.front() == '-')) {
                    mtr.pop_front();
                }
                mt = std::string{mtr};
                break;
            }
            if (mt.empty()) {
                mt = "VALUE";
            } else {
                std::transform(mt.begin(), mt.end(), mt.begin(), toupper);
            }
        }
        bool first = true;
        for (auto &s: names) {
            if (!first) {
                format(out, ", ");
            }
            format(out, s);
            switch (arg.needs_value()) {
                case arg_value::EXACTLY: {
                    for (auto nargs = arg.get_nargs(); nargs; --nargs) {
                        format(out, " %s", mt);
                    }
                    break;
                }
                case arg_value::OPTIONAL:
                    format(out, " [%s]", mt);
                    break;
                case arg_value::ALL:
                    for (auto nargs = arg.get_nargs(); nargs; --nargs) {
                        format(out, " %s", mt);
                    }
                    format(out, " [%s ...]", mt);
                    break;
                default:
                    break;
            }
            first = false;
        }
    }

    template<typename OutputRange>
    void format_option(OutputRange &out, arg_positional const &arg) {
        auto mt = arg.get_metavar();
        if (mt.empty()) {
            mt = arg.get_name();
        }
        format(out, mt);
    }

    template<typename OutputRange>
    void format_option(OutputRange &out, arg_description const &arg) {
        switch (arg.type()) {
            case arg_type::OPTIONAL:
                format_option(out, static_cast<arg_optional const &>(arg));
                break;
            case arg_type::POSITIONAL:
                format_option(out, static_cast<arg_positional const &>(arg));
                break;
            default:
                /* should never happen */
                throw arg_error{"invalid argument type"};
        }
    }

private:
    basic_arg_parser<default_help_formatter> &p_parser;
};

using arg_parser = basic_arg_parser<default_help_formatter>;

template<typename OutputRange>
auto arg_print_help(OutputRange o, arg_parser &p) {
    return [o = std::move(o), &p](iterator_range<string_range const *>)
        mutable
    {
        p.print_help(o);
        p.stop_parsing();
    };
};

auto arg_print_help(arg_parser &p) {
    return arg_print_help(cout.iter(), p);
}

template<typename T, typename U>
auto arg_store_const(T &&val, U &ref) {
    return [val, &ref](iterator_range<string_range const *>) mutable {
        ref = std::move(val);
    };
}

template<typename T>
auto arg_store_str(T &ref) {
    return [&ref](iterator_range<string_range const *> r) mutable {
        ref = T{r[0]};
    };
}

auto arg_store_true(bool &ref) {
    return arg_store_const(true, ref);
}

auto arg_store_false(bool &ref) {
    return arg_store_const(false, ref);
}

template<typename ...A>
auto arg_store_format(string_range fmt, A &...args) {
    /* TODO: use ostd::format once it supports reading */
    return [fmts = std::string{fmt}, argst = std::tie(args...)](
        iterator_range<string_range const *> r
    ) mutable {
        std::apply([&fmts, istr = std::string{r[0]}](auto &...refs) {
            if (sscanf(istr.data(), fmts.data(), &refs...) != sizeof...(A)) {
                throw arg_error{format(
                    appender<std::string>(),
                    "argument requires format '%s' (got '%s')", fmts, istr
                ).get()};
            }
        }, argst);
    };
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
