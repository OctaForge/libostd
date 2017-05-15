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

struct arg_parser;

struct arg_description {
    friend struct arg_parser;

    virtual ~arg_description() {}

    virtual arg_type type() const = 0;

    virtual bool is_arg(string_range name) const = 0;

protected:
    arg_description() {}
};

struct arg_argument: arg_description {
    friend struct arg_parser;

    arg_argument &help(string_range str) {
        p_helpstr = std::string{str};
        return *this;
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
    friend struct arg_parser;

    arg_type type() const {
        return arg_type::OPTIONAL;
    }

    bool is_arg(string_range name) const {
        if (starts_with(name, "--")) {
            return name.slice(2) == ostd::citer(p_lname);
        }
        if (name[0] == '-') {
            return name[1] == p_sname;
        }
        return false;
    }

    arg_value needs_value() const {
        return p_valreq;
    }

    bool used() const {
        return p_used;
    }

    template<typename F>
    arg_optional &action(F func) {
        p_action = [this, func = std::move(func)](
            iterator_range<string_range const *> vals
        ) mutable {
            func(vals);
            p_used = true;
        };
        return *this;
    }

    arg_optional &help(string_range str) {
        arg_argument::help(str);
        return *this;
    }

protected:
    arg_optional() = delete;
    arg_optional(
        char short_name, string_range long_name,
        arg_value req, int nargs = 1
    ):
        arg_argument(req, nargs), p_lname(long_name), p_sname(short_name)
    {}
    arg_optional(char short_name, string_range long_name, int nargs):
        arg_argument(nargs), p_lname(long_name), p_sname(short_name)
    {}

    void set_values(iterator_range<string_range const *> vals) {
        if (p_action) {
            p_action(vals);
        } else {
            p_used = true;
        }
    }

private:
    std::function<void(iterator_range<string_range const *>)> p_action;
    std::string p_lname;
    char p_sname;
    bool p_used = false;
};

struct arg_positional: arg_argument {
    friend struct arg_parser;

    arg_type type() const {
        return arg_type::POSITIONAL;
    }

    bool is_arg(string_range name) const {
        return (name == ostd::citer(p_name));
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
    friend struct arg_parser;

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

struct arg_parser {
    arg_parser(string_range progname = string_range{}):
        p_progname(progname)
    {}

    void parse(int argc, char **argv) {
        if (p_progname.empty()) {
            p_progname = argv[0];
        }
        parse(iter(&argv[1], &argv[argc]));
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

    template<typename ...A>
    arg_optional &add_optional(A &&...args) {
        arg_description *p = new arg_optional{std::forward<A>(args)...};
        return static_cast<arg_optional &>(*p_opts.emplace_back(p));
    }

    template<typename ...A>
    arg_positional &add_positional(A &&...args) {
        arg_description *p = new arg_positional{std::forward<A>(args)...};
        return static_cast<arg_positional &>(*p_opts.emplace_back(p));
    }

    template<typename ...A>
    arg_category &add_category(A &&...args) {
        arg_description *p = new arg_category{std::forward<A>(args)...};
        return static_cast<arg_category &>(*p_opts.emplace_back(p));
    }

    template<typename OutputRange>
    arg_optional &add_help(OutputRange out, string_range msg) {
        auto &opt = add_optional('h', "help", arg_value::NONE);
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
        print_usage_impl(range);
        print_help_impl(range);
        return std::forward<OutputRange>(range);
    }

    void print_help() {
        print_help(cout.iter());
    }

    arg_argument &get(string_range name) {
        return find_arg<arg_argument>(name);
    }

    bool used(string_range name) {
        auto &arg = find_arg<arg_optional>(name);
        return arg.p_used;
    }

private:
    template<typename OR>
    void print_usage_impl(OR &out) {
        string_range progname = p_progname;
        if (progname.empty()) {
            progname = "program";
        }
        format(out, "usage: %s [opts] [args]\n", progname);
    }

    template<typename OR>
    void print_help_impl(OR &out) {
        std::size_t opt_namel = 0, pos_namel = 0;
        for (auto &p: p_opts) {
            switch (p->type()) {
                case arg_type::OPTIONAL: {
                    auto &opt = static_cast<arg_optional &>(*p);
                    std::size_t nl = 0;
                    if (opt.p_sname) {
                        nl += 2;
                    }
                    if (!opt.p_lname.empty()) {
                        if (opt.p_sname) {
                            nl += 2;
                        }
                        nl += opt.p_lname.size() + 2;
                    }
                    opt_namel = std::max(opt_namel, nl);
                    break;
                }
                case arg_type::POSITIONAL:
                    pos_namel = std::max(
                        pos_namel,
                        static_cast<arg_positional &>(*p).p_name.size()
                    );
                    break;
                default:
                    break;
            }
        }

        std::size_t maxpad = std::max(opt_namel, pos_namel);

        if (pos_namel) {
            format(out, "\npositional arguments:\n");
            for (auto &p: p_opts) {
                if (p->type() != arg_type::POSITIONAL) {
                    continue;
                }
                auto &parg = static_cast<arg_positional &>(*p.get());
                format(out, "  %s", parg.p_name);
                if (parg.p_helpstr.empty()) {
                    out.put('\n');
                } else {
                    std::size_t nd = maxpad - parg.p_name.size() + 2;
                    for (std::size_t i = 0; i < nd; ++i) {
                        out.put(' ');
                    }
                    format(out, "%s\n", parg.p_helpstr);
                }
            }
        }

        if (opt_namel) {
            format(out, "\noptional arguments:\n");
            for (auto &p: p_opts) {
                if (p->type() != arg_type::OPTIONAL) {
                    continue;
                }
                auto &parg = static_cast<arg_optional &>(*p.get());
                format(out, "  ");
                std::size_t nd = 0;
                if (parg.p_sname) {
                    nd += 2;
                    format(out, "-%c", parg.p_sname);
                    if (!parg.p_lname.empty()) {
                        nd += 2;
                        format(out, ", ");
                    }
                }
                if (!parg.p_lname.empty()) {
                    nd += parg.p_lname.size() + 2;
                    format(out, "--%s", parg.p_lname);
                }
                if (parg.p_helpstr.empty()) {
                    out.put('\n');
                } else {
                    nd = maxpad - nd + 2;
                    for (std::size_t i = 0; i < nd; ++i) {
                        out.put(' ');
                    }
                    format(out, "%s\n", parg.p_helpstr);
                }
            }
        }
    }

    template<typename R>
    void parse_long(string_range arg, R &args) {
        string_range val = find(arg, '=');
        bool has_val = !val.empty(), arg_val = false;
        if (has_val) {
            arg = arg.slice(0, arg.size() - val.size());
            val.pop_front();
        }

        auto &desc = find_arg<arg_optional>(arg);

        args.pop_front();
        auto needs = desc.needs_value();
        if (needs == arg_value::NONE) {
            if (has_val) {
                throw arg_error{format(
                    appender<std::string>(), "argument '--%s' takes no value",
                    desc.p_lname
                ).get()};
            }
            desc.set_values(nullptr);
            return;
        }
        if (!has_val) {
            if (args.empty()) {
                if (needs == arg_value::REQUIRED) {
                    throw arg_error{format(
                        appender<std::string>(), "argument '--%s' needs a value",
                        desc.p_lname
                    ).get()};
                }
                desc.set_values(nullptr);
                return;
            }
            string_range tval = args.front();
            if ((needs != arg_value::OPTIONAL) || !find_arg_ptr(tval)) {
                val = tval;
                has_val = arg_val = true;
            }
        }
        if (has_val) {
            desc.set_values(iter({ val }));
            if (arg_val) {
                args.pop_front();
            }
        } else {
            desc.set_values(nullptr);
        }
    }

    template<typename R>
    void parse_short(string_range arg, R &args) {
        string_range val;
        bool has_val = (arg.size() > 2), arg_val = false;
        if (has_val) {
            val = arg.slice(2);
            arg = arg.slice(0, 2);
        }

        auto &desc = find_arg<arg_optional>(arg);

        args.pop_front();
        auto needs = desc.needs_value();
        if (needs == arg_value::NONE) {
            if (has_val) {
                throw arg_error{format(
                    appender<std::string>(), "argument '-%c' takes no value",
                    desc.p_sname
                ).get()};
            }
            desc.set_values(nullptr);
            return;
        }
        if (!has_val) {
            if (args.empty()) {
                if (needs == arg_value::REQUIRED) {
                    throw arg_error{format(
                        appender<std::string>(), "argument '-%c' needs a value",
                        desc.p_sname
                    ).get()};
                }
                desc.set_values(nullptr);
                return;
            }
            string_range tval = args.front();
            if ((needs != arg_value::OPTIONAL) || !find_arg_ptr(tval)) {
                val = tval;
                has_val = arg_val = true;
            }
        }
        if (has_val) {
            desc.set_values(iter({ val }));
            if (arg_val) {
                args.pop_front();
            }
        } else {
            desc.set_values(nullptr);
        }
    }

    void parse_pos(string_range) {
    }

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
    std::string p_progname;
};

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
