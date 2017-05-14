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

struct arg_argument {
    friend struct arg_parser;

    virtual ~arg_argument() {}

    virtual arg_type type() const = 0;

    virtual bool is_arg(string_range name) const = 0;

protected:
    arg_argument(arg_value req = arg_value::NONE, int nargs = 1):
        p_valreq(req), p_nargs(nargs)
    {}

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

    void set_value(string_range val) {
        p_value = std::string{val};
    }

protected:
    arg_optional() = delete;
    arg_optional(
        char short_name, string_range long_name,
        arg_value req, int nargs = 1
    ):
        arg_argument(req, nargs), p_lname(long_name), p_sname(short_name)
    {}

private:
    std::optional<std::string> p_value;
    std::string p_lname;
    char p_sname;
};

struct arg_positional: arg_argument {
    friend struct arg_parser;

    arg_type type() const {
        return arg_type::POSITIONAL;
    }

    bool is_arg(string_range) const {
        return false;
    }

protected:
    arg_positional() = delete;
    arg_positional(
        string_range name, arg_value req = arg_value::REQUIRED, int nargs = 1
    ):
        arg_argument(req, nargs),
        p_name(name)
    {}

private:
    std::string p_name;
};

struct arg_category: arg_argument {
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
        arg_argument *p = new arg_optional{std::forward<A>(args)...};
        return static_cast<arg_optional &>(*p_opts.emplace_back(p));
    }

    template<typename ...A>
    arg_positional &add_positional(A &&...args) {
        arg_argument *p = new arg_positional{std::forward<A>(args)...};
        return static_cast<arg_positional &>(*p_opts.emplace_back(p));
    }

    template<typename ...A>
    arg_category &add_category(A &&...args) {
        arg_argument *p = new arg_category{std::forward<A>(args)...};
        return static_cast<arg_category &>(*p_opts.emplace_back(p));
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
        bool has_opt = false, has_pos = false;
        for (auto &p: p_opts) {
            switch (p->type()) {
                case arg_type::OPTIONAL:
                    has_opt = true;
                    break;
                case arg_type::POSITIONAL:
                    has_pos = true;
                    break;
                default:
                    break;
            }
        }

        if (has_pos) {
            format(out, "\npositional arguments:\n");
            for (auto &p: p_opts) {
                if (p->type() != arg_type::POSITIONAL) {
                    continue;
                }
                auto &parg = static_cast<arg_positional &>(*p.get());
                format(out, "  %s\n", parg.p_name);
            }
        }

        if (has_opt) {
            format(out, "\noptional arguments:\n");
            for (auto &p: p_opts) {
                if (p->type() != arg_type::OPTIONAL) {
                    continue;
                }
                auto &parg = static_cast<arg_optional &>(*p.get());
                format(out, "  ");
                if (parg.p_sname) {
                    format(out, "-%c", parg.p_sname);
                    if (!parg.p_lname.empty()) {
                        format(out, ", ");
                    }
                }
                if (!parg.p_lname.empty()) {
                    format(out, "--%s", parg.p_lname);
                }
                out.put('\n');
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

        auto &desc = static_cast<arg_optional &>(find_arg(arg));

        args.pop_front();
        auto needs = desc.needs_value();
        if (needs == arg_value::NONE) {
            if (has_val) {
                throw arg_error{format(
                    appender<std::string>(), "argument '--%s' takes no value",
                    desc.p_lname
                ).get()};
            }
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
                return;
            }
            string_range tval = args.front();
            if ((needs != arg_value::OPTIONAL) || !find_arg_ptr(tval)) {
                val = tval;
                has_val = arg_val = true;
            }
        }
        if (has_val) {
            desc.set_value(val);
            if (arg_val) {
                args.pop_front();
            }
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

        auto &desc = static_cast<arg_optional &>(find_arg(arg));

        args.pop_front();
        auto needs = desc.needs_value();
        if (needs == arg_value::NONE) {
            if (has_val) {
                throw arg_error{format(
                    appender<std::string>(), "argument '-%c' takes no value",
                    desc.p_sname
                ).get()};
            }
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
                return;
            }
            string_range tval = args.front();
            if ((needs != arg_value::OPTIONAL) || !find_arg_ptr(tval)) {
                val = tval;
                has_val = arg_val = true;
            }
        }
        if (has_val) {
            desc.set_value(val);
            if (arg_val) {
                args.pop_front();
            }
        }
    }

    void parse_pos(string_range) {
    }

    arg_argument *find_arg_ptr(string_range name) {
        for (auto &p: p_opts) {
            if (p->is_arg(name)) {
                return &*p;
            }
        }
        return nullptr;
    }

    arg_argument &find_arg(string_range name) {
        auto p = find_arg_ptr(name);
        if (p) {
            return *p;
        }
        throw arg_error{format(
            appender<std::string>(), "unknown argument '%s'", name
        ).get()};
    }

    std::vector<std::unique_ptr<arg_argument>> p_opts;
    std::string p_progname;
};

/** @} */

} /* namespace ostd */

#endif

/** @} */
