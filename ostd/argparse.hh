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

#include <optional>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>

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
    NONE = 0,
    REQUIRED,
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

protected:
    arg_argument(arg_value req = arg_value::NONE, int nargs = 1):
        arg_description(), p_valreq(req), p_nargs(nargs)
    {}
    arg_argument(int nargs):
        arg_description(),
        p_valreq((nargs > 0)
            ? arg_value::REQUIRED
            : ((nargs < 0) ? arg_value::ALL : arg_value::NONE)
        ), p_nargs(nargs)
    {}

    std::string p_helpstr;
    arg_value p_valreq;
    int p_nargs;
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

    arg_value needs_value() const {
        return p_valreq;
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

    arg_optional &add_name(string_range name) {
        p_names.emplace_back(name);
        return *this;
    }

    auto get_names() const {
        return iter(p_names);
    }

protected:
    arg_optional() = delete;

    arg_optional(string_range name, arg_value req, int nargs = 1):
        arg_argument(req, nargs)
    {
        p_names.emplace_back(name);
    }
    arg_optional(string_range name, int nargs):
        arg_argument(nargs)
    {
        p_names.emplace_back(name);
    }

    arg_optional(
        string_range name1, string_range name2, arg_value req, int nargs = 1
    ):
        arg_argument(req, nargs)
    {
        p_names.emplace_back(name1);
        p_names.emplace_back(name2);
    }
    arg_optional(string_range name1, string_range name2, int nargs):
        arg_argument(nargs)
    {
        p_names.emplace_back(name1);
        p_names.emplace_back(name2);
    }

    void set_values(iterator_range<string_range const *> vals) {
        if (p_action) {
            p_action(vals);
        }
        ++p_used;
    }

private:
    std::function<void(iterator_range<string_range const *>)> p_action;
    std::vector<std::string> p_names;
    std::size_t p_used = 0;
};

struct arg_positional: arg_argument {
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

protected:
    arg_positional() = delete;
    arg_positional(
        string_range name, arg_value req = arg_value::REQUIRED, int nargs = 1
    ):
        arg_argument(req, nargs),
        p_name(name)
    {}
    arg_positional(string_range name, int nargs):
        arg_argument(nargs),
        p_name(name)
    {}

private:
    std::string p_name;
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
    basic_arg_parser(string_range progname = string_range{}):
        arg_description_container(), p_progname(progname)
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
            if (!allow_optional) {
                parse_pos(s);
                continue;
            }
            if (starts_with(s, "--")) {
                parse_long(s, args);
                continue;
            }
            if ((s.size() > 1) && (s[0] == '-') && (s != "-")) {
                parse_short(s, args);
                continue;
            }
            parse_pos(s);
        }
    }

    template<typename OutputRange>
    arg_optional &add_help(OutputRange out, string_range msg) {
        auto &opt = add_optional("-h", "--help", arg_value::NONE);
        opt.help(msg);
        opt.action([this, out = std::move(out)](auto) mutable {
            this->print_help(out);
            return true;
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

private:
    template<typename R>
    void parse_long(string_range arg, R &args) {
        std::optional<string_range> val;
        if (auto sv = find(arg, '='); !sv.empty()) {
            arg = arg.slice(0, arg.size() - sv.size());
            sv.pop_front();
            val = sv;
        }
        parse_arg(arg, std::move(val), args);
    }

    template<typename R>
    void parse_short(string_range arg, R &args) {
        std::optional<string_range> val;
        if (arg.size() > 2) {
            val = arg.slice(2);
            arg = arg.slice(0, 2);
        }
        parse_arg(arg, std::move(val), args);
    }

    template<typename R>
    void parse_arg(
        string_range arg, std::optional<string_range> val, R &args
    ) {
        bool arg_val = false;
        std::string argname{arg};
        auto &desc = find_arg<arg_optional>(arg);

        args.pop_front();
        auto needs = desc.needs_value();
        if (needs == arg_value::NONE) {
            if (val) {
                throw arg_error{format(
                    appender<std::string>(), "argument '%s' takes no value",
                    argname
                ).get()};
            }
            desc.set_values(nullptr);
            return;
        }
        if (!val) {
            if (args.empty()) {
                if (needs == arg_value::REQUIRED) {
                    throw arg_error{format(
                        appender<std::string>(), "argument '%s' needs a value",
                        argname
                    ).get()};
                }
                desc.set_values(nullptr);
                return;
            }
            string_range tval = args.front();
            if ((needs != arg_value::OPTIONAL) || !find_arg_ptr(tval)) {
                val = tval;
                arg_val = true;
            }
        }
        if (val) {
            desc.set_values(ostd::iter({ *val }));
            if (arg_val) {
                args.pop_front();
            }
        } else {
            desc.set_values(nullptr);
        }
    }

    void parse_pos(string_range) {
    }

    std::string p_progname;
    HelpFormatter p_helpfmt{*this};
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
            switch (p->type()) {
                case arg_type::OPTIONAL: {
                    auto &opt = static_cast<arg_optional &>(*p);
                    auto names = opt.get_names();
                    std::size_t nl = 0;
                    for (auto &s: names) {
                        nl += s.size();
                    }
                    nl += 2 * (names.size() - 1);
                    opt_namel = std::max(opt_namel, nl);
                    break;
                }
                case arg_type::POSITIONAL:
                    pos_namel = std::max(
                        pos_namel,
                        static_cast<arg_positional &>(*p).get_name().size()
                    );
                    break;
                default:
                    break;
            }
        }

        std::size_t maxpad = std::max(opt_namel, pos_namel);

        if (pos_namel) {
            format(out, "\npositional arguments:\n");
            for (auto &p: p_parser.iter()) {
                if (p->type() != arg_type::POSITIONAL) {
                    continue;
                }
                auto &parg = static_cast<arg_positional &>(*p.get());
                auto name = parg.get_name(), help = parg.get_help();
                format(out, "  %s", name);
                if (help.empty()) {
                    out.put('\n');
                } else {
                    std::size_t nd = maxpad - name.size() + 2;
                    for (std::size_t i = 0; i < nd; ++i) {
                        out.put(' ');
                    }
                    format(out, "%s\n", help);
                }
            }
        }

        if (opt_namel) {
            format(out, "\noptional arguments:\n");
            for (auto &p: p_parser.iter()) {
                if (p->type() != arg_type::OPTIONAL) {
                    continue;
                }
                auto &parg = static_cast<arg_optional &>(*p.get());
                auto names = parg.get_names();
                auto help = parg.get_help();
                format(out, "  ");
                std::size_t nd = 0;
                for (auto &s: names) {
                    if (nd) {
                        nd += 2;
                        format(out, ", ");
                    }
                    nd += s.size();
                    format(out, "%s", s);
                }
                if (help.empty()) {
                    out.put('\n');
                } else {
                    nd = maxpad - nd + 2;
                    for (std::size_t i = 0; i < nd; ++i) {
                        out.put(' ');
                    }
                    format(out, "%s\n", help);
                }
            }
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

/** @} */

} /* namespace ostd */

#endif

/** @} */
