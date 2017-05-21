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
    CATEGORY
};

enum class arg_value {
    EXACTLY,
    OPTIONAL,
    ALL,
    REST
};

struct arg_description {
    virtual ~arg_description() {}

    virtual arg_type type() const = 0;

    virtual bool is_arg(string_range name) const = 0;

protected:
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

    arg_type type() const {
        return arg_type::OPTIONAL;
    }

    bool is_arg(string_range name) const {
        for (auto const &nm: p_names) {
            if (name == iter(nm)) {
                return true;
            }
        }
        return false;
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

    void set_values(
        string_range argname, iterator_range<string_range const *> vals
    ) {
        if (p_limit && (p_used == p_limit)) {
            throw arg_error{format(
                appender<std::string>(),
                "argument '%s' can be used at most %d times", argname, p_limit
            ).get()};
        }
        if (p_action) {
            p_action(vals);
        }
        ++p_used;
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

    bool is_arg(string_range name) const {
        return (name == ostd::citer(p_name));
    }

    string_range get_name() const {
        return p_name;
    }

    template<typename F>
    arg_positional &action(F func) {
        p_action = func;
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

struct arg_category: arg_description {
    friend struct arg_description_container;

    arg_type type() const {
        return arg_type::CATEGORY;
    }

    bool is_arg(string_range) const {
        return false;
    }

protected:
    arg_category() = delete;
    arg_category(
        string_range name
    ):
        p_name(name)
    {}

private:
    std::string p_name;
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
    arg_category &add_category(A &&...args) {
        arg_description *p = new arg_category(std::forward<A>(args)...);
        return static_cast<arg_category &>(*p_opts.emplace_back(p));
    }

    auto iter() const {
        return ostd::citer(p_opts);
    }

protected:
    arg_description_container() {}

    arg_description *find_arg_ptr(string_range name) {
        for (auto &p: p_opts) {
            if (p->is_arg(name)) {
                return &*p;
            }
        }
        return nullptr;
    }

    template<typename AT>
    AT &find_arg(string_range name) {
        auto p = static_cast<AT *>(find_arg_ptr(name));
        if (p) {
            return *p;
        }
        throw arg_error{format(
            appender<std::string>(), "unknown argument '%s'", name
        ).get()};
    }

    std::vector<std::unique_ptr<arg_description>> p_opts;
};

template<typename HelpFormatter>
struct basic_arg_parser: arg_description_container {
    basic_arg_parser(
        string_range progname = string_range{}, bool posix = false
    ):
        arg_description_container(), p_progname(progname), p_posix(posix)
    {}

    void parse(int argc, char **argv) {
        if (p_progname.empty()) {
            p_progname = argv[0];
        }
        parse(ostd::iter(&argv[1], &argv[argc]));
    }

    template<typename InputRange>
    void parse(InputRange args) {
        bool allow_optional = true;
        while (!args.empty()) {
            string_range s{args.front()};
            if (s == "--") {
                args.pop_front();
                allow_optional = false;
                continue;
            }
            if (allow_optional && is_optarg(s)) {
                parse_opt(s, args);
                continue;
            }
            if (p_posix) {
                allow_optional = false;
            }
            parse_pos(s, args, allow_optional);
        }
    }

    template<typename OutputRange>
    arg_optional &add_help(OutputRange out, string_range msg) {
        auto &opt = add_optional("-h", "--help", 0);
        opt.help(msg);
        opt.action([this, out = std::move(out)](auto) mutable {
            this->print_help(out);
        });
        return opt;
    }

    arg_optional &add_help(string_range msg) {
        return add_help(cout.iter(), msg);
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
        return find_arg<arg_argument>(name);
    }

    std::size_t used(string_range name) {
        auto &arg = find_arg<arg_optional>(name);
        return arg.p_used;
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

        auto &desc = find_arg<arg_optional>(arg);
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
        format(out, "usage: %s [opts] [args]\n", progname);
    }

    template<typename OutputRange>
    void format_options(OutputRange &out) {
        std::size_t opt_namel = 0, pos_namel = 0;
        for (auto &p: p_parser.iter()) {
            auto cs = counting_sink(noop_sink<char>());
            switch (p->type()) {
                case arg_type::OPTIONAL: {
                    format_option(cs, static_cast<arg_optional &>(*p));
                    opt_namel = std::max(opt_namel, cs.get_written());
                    break;
                }
                case arg_type::POSITIONAL:
                    format_option(cs, static_cast<arg_positional &>(*p));
                    pos_namel = std::max(pos_namel, cs.get_written());
                    break;
                default:
                    break;
            }
        }
        std::size_t maxpad = std::max(opt_namel, pos_namel);

        auto write_help = [maxpad](
            auto &out, arg_argument &arg, std::size_t len
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

        if (pos_namel) {
            format(out, "\npositional arguments:\n");
            for (auto &p: p_parser.iter()) {
                if (p->type() != arg_type::POSITIONAL) {
                    continue;
                }
                format(out, "  ");
                auto &parg = static_cast<arg_positional &>(*p.get());
                auto cr = counting_sink(out);
                format_option(cr, parg);
                out = std::move(cr.get_range());
                write_help(out, parg, cr.get_written());
            }
        }

        if (opt_namel) {
            format(out, "\noptional arguments:\n");
            for (auto &p: p_parser.iter()) {
                if (p->type() != arg_type::OPTIONAL) {
                    continue;
                }
                format(out, "  ");
                auto &oarg = static_cast<arg_optional &>(*p.get());
                auto cr = counting_sink(out);
                format_option(cr, oarg);
                out = std::move(cr.get_range());
                write_help(out, oarg, cr.get_written());
            }
        }
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
