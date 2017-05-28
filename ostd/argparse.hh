/** @addtogroup Utilities
 * @{
 */

/** @file argparse.hh
 *
 * @brief Portable argument parsing.
 *
 * Provides a powerful argument parser that can handle a wide variety of
 * cases, including POSIX and GNU argument ordering, different argument
 * formats, optional values and type conversions.
 *
 * @include argparse.cc
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
#include <type_traits>

#include "ostd/algorithm.hh"
#include "ostd/format.hh"
#include "ostd/string.hh"
#include "ostd/io.hh"

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

/** @brief The error thrown on parsing and other failures. */
struct arg_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

/** @brief The type of an argument class. */
enum class arg_type {
    OPTIONAL = 0,            ///< An optional argument.
    POSITIONAL,              ///< A positional argument.
    GROUP,                   ///< A group of arguments.
    MUTUALLY_EXCLUSIVE_GROUP ///< A group of mutually exclusive optionals.
};

/** @brief Defines the value requirements of an argument.
 *
 * The value requirement is paired with an integer defining the actual
 * number of values. This number is valid for `EXACTLY` (defining the
 * actual number of values) and for `ALL` (defining the minimum number
 * of values) but not for the other two.
 *
 * See ostd::basic_arg_parser for detailed behavior.
 */
enum class arg_value {
    EXACTLY,  ///< An exact number of arguments.
    OPTIONAL, ///< A single optional argument.
    ALL,      ///< All arguments until an optional.
    REST      ///< The rest of the arguments.
};

/** @brief A base class for all argument types.
 *
 * This base class is abstract so it cannot be instantiated.
 */
struct arg_description {
    friend struct arg_description_container;
    friend struct arg_mutually_exclusive_group;
    friend struct arg_group;

    /** @brief The base class contains no data. */
    virtual ~arg_description() {}

    /** @brief Gets an ostd::arg_type for the class. */
    virtual arg_type type() const noexcept = 0;

protected:
    /** @brief Finds an argument within the argument including itself.
     *
     * Given a name and optionally a type, this finds an optional or
     * a positional argument of the given name. If the argument itself
     * matches the parameters, it may return itself. For groups, it
     * will search in the group (recursively) until something is
     * found. If nothing is found, `nullptr` is returned.
     *
     * The bool here indicates whether we're currently parsing.
     */
    virtual arg_description *find_arg(
        string_range, std::optional<arg_type>, bool
    ) {
        return nullptr;
    }

    /** @brief Does nothing. */
    arg_description() {}
};

/** @brief A base class for optional and positional arguments.
 *
 * Optionals and positionals derive from this but not groups. It implements
 * things common for both positional and optional arguments, including
 * handling of ostd::arg_value. help and metavars.
 *
 * It's not instantiated directly.
 */
struct arg_argument: arg_description {
    /** @brief Sets the help for the argument.
     *
     * The help string is stored internally. Returns itself.
     */
    arg_argument &help(string_range str) {
        p_helpstr = std::string{str};
        return *this;
    }

    /** @brief Gets the help string set by help(). */
    string_range help() const noexcept {
        return p_helpstr;
    }

    /** @brief Sets the metavar for the argument.
     *
     * A metavar is a string displayed in help listing either in place
     * of a value (for optionals) or as the argument name itself (for
     * positionals). See ostd::default_help_formatter for more.
     */
    arg_argument &metavar(string_range str) {
        p_metavar = std::string{str};
        return *this;
    }

    /** @brief Gets the metavar string set by metavar(). */
    string_range metavar() const noexcept {
        return p_metavar;
    }

    /** @brief Gets the value requirement for the argument */
    arg_value needs_value() const noexcept {
        return p_valreq;
    }

    /** @brief Gets the number of values for needs_value(). */
    std::size_t nargs() const noexcept {
        return p_nargs;
    }

protected:
    /** @brief A helper constructor with ostd::arg_value.
     *
     * This is called by ostd::arg_optional and ostd::arg_positional.
     * This version specifies an explicit value requirement plus a
     * number of arguments.
     */
    arg_argument(arg_value req, std::size_t nargs):
        arg_description(), p_valreq(req), p_nargs(nargs)
    {}

    /** @brief A helper constructor with a number of arguments.
     *
     * The ostd::arg_value requirement is always `EXACTLY`.
     */
    arg_argument(std::size_t nargs):
        arg_description(),
        p_valreq(arg_value::EXACTLY),
        p_nargs(nargs)
    {}

private:
    std::string p_helpstr, p_metavar;
    arg_value p_valreq;
    std::size_t p_nargs;
};

/** @brief An optional argument class.
 *
 * An optional argument is composed of a prefix (frequently `-` or `--`,
 * but can be anything allowed by the parser), a name and optionally a
 * value or several values. Optional arguments are called optional because
 * they don't have to be present. You can however limit how many times they
 * can be used.
 *
 * An optional argument can have multiple names defining short and long
 * arguments. Short arguments frequently have a format like `-a` while
 * long ones often look like `--arg`. There is no restriction here as
 * long as the name begins with at least one allowed prefix character,
 * so arguments like `-arg` or `--a` or even `---x` are allowed.
 *
 *See ostd::basic_arg_parser for more.
 */
struct arg_optional: arg_argument {
    template<typename HelpFormatter>
    friend struct basic_arg_parser;
    friend struct arg_description_container;
    friend struct arg_group;
    friend struct arg_mutually_exclusive_group;

    /** @brief Gets the argument class type (ostd::arg_type).
     *
     * The value is always `OPTIONAL`.
     */
    arg_type type() const noexcept {
        return arg_type::OPTIONAL;
    }

    /** @brief Gets how many times the argument has been specified. */
    std::size_t used() const noexcept {
        return p_used;
    }

    /** @brief Sets the action to run when the argument is used.
     *
     * The function is called with a finite random access range
     * of ostd::string_range, each containing a value.  It's not
     * expected to return anything.
     */
    template<typename F>
    arg_optional &action(F func) {
        p_action = func;
        return *this;
    }

    /** @brief Like ostd::arg_argument::help(). */
    arg_optional &help(string_range str) {
        arg_argument::help(str);
        return *this;
    }

    /** @brief Gets the metavar string set by help(). */
    string_range help() const noexcept {
        return arg_argument::help();
    }

    /** @brief Like ostd::arg_argument::metavar(). */
    arg_optional &metavar(string_range str) {
        arg_argument::metavar(str);
        return *this;
    }

    /** @brief Gets the metavar string set by metavar(). */
    string_range metavar() const noexcept {
        return arg_argument::metavar();
    }

    /** @brief Gets the actual metavar used in help listing.
     *
     * Unlike just metavar(), it accounts for the case when the
     * provided metavar is empty, falling back to transforming
     * a suitable argument name into one.
     */
    std::string real_metavar() const {
        std::string mt{metavar()};
        if (mt.empty()) {
            string_range fb = longest_name();
            if (!fb.empty()) {
                char pfx = fb[0];
                while (!fb.empty() && (fb.front() == pfx)) {
                    fb.pop_front();
                }
            }
            if (fb.empty()) {
                mt = "VALUE";
            } else {
                mt.append(fb.data(), fb.size());
                std::transform(mt.begin(), mt.end(), mt.begin(), toupper);
            }
        }
        return mt;
    }

    /** @brief Sets the limit on how many times this can be used.
     *
     * By default there is no limit (the value is 0). You can use this
     * to restrict this to for example 1 use. The parsing will then
     * throw ostd::arg_error if it's used more.
     */
    arg_optional &limit(std::size_t n) noexcept {
        p_limit = n;
        return *this;
    }

    /** @brief Adds an extra optional name.
     *
     * Using the constructor, you can add 1 or 2 names. If you want
     * this argument to be accessible with more names, you use this.
     */
    arg_optional &add_name(string_range name) {
        p_names.emplace_back(name);
        return *this;
    }

    /** @brief Gets a read-only finite random access range of names.
     *
     * The value type of this range is an `std::string const`.
     */
    auto names() const noexcept {
        return iter(p_names);
    }

    /** @brief Gets the longest name this option can be referred to by.
     *
     * This is used in error messages and fallback metavar in help listing.
     */
    string_range longest_name() const noexcept {
        string_range ret;
        for (auto &s: p_names) {
            if (s.size() > ret.size()) {
                ret = s;
            }
        }
        return ret;
    }

    /** @brief Checks if the optional argument must be specified.
     *
     * In vast majority of cases, optional arguments are optional.
     */
    bool required() const noexcept {
        return p_required;
    }

protected:
    arg_optional() = delete;

    /** @brief Constructs the optional argument with one name.
     *
     * This version takes an explicit value requirement, see the
     * appropriate constructors of ostd::arg_argument. The value
     * requirement can be `EXACTLY`, `OPTIONAL` or `ALL` but never
     * `REST`. If it's not one of the allowed ones, ostd::arg_error
     * is thrown.
     *
     * The `required` argument specifies whether this optional
     * argument must be specified. In most cases optional
     * arguments are optional, but not always.
     */
    arg_optional(
        string_range name, arg_value req, std::size_t nargs = 1,
        bool required = false
    ):
        arg_argument(req, nargs), p_required(required)
    {
        validate_req(req);
        p_names.emplace_back(name);
    }

    /** @brief Constructs the optional argument with one name.
     *
     * This version takes only a number, see the appropriate
     * constructors of ostd::arg_argument.
     *
     * The `required` argument specifies whether this optional
     * argument must be specified. In most cases optional
     * arguments are optional, but not always.
     */
    arg_optional(string_range name, std::size_t nargs, bool required = false):
        arg_argument(nargs), p_required(required)
    {
        p_names.emplace_back(name);
    }

    /** @brief Constructs the optional argument with two names.
     *
     * The typical combination is a short and a long name.
     *
     * This version takes an explicit value requirement, see the
     * appropriate constructors of ostd::arg_argument. The value
     * requirement can be `EXACTLY`, `OPTIONAL` or `ALL` but never
     * `REST`. If it's not one of the allowed ones, ostd::arg_error
     * is thrown.
     *
     * The `required` argument specifies whether this optional
     * argument must be specified. In most cases optional
     * arguments are optional, but not always.
     */
    arg_optional(
        string_range name1, string_range name2, arg_value req,
        std::size_t nargs = 1, bool required = false
    ):
        arg_argument(req, nargs), p_required(required)
    {
        validate_req(req);
        p_names.emplace_back(name1);
        p_names.emplace_back(name2);
    }

    /** @brief Constructs the optional argument with two names.
     *
     * The typical combination is a short and a long name.
     *
     * This version takes only a number, see the appropriate
     * constructors of ostd::arg_argument.
     *
     * The `required` argument specifies whether this optional
     * argument must be specified. In most cases optional
     * arguments are optional, but not always.
     */
    arg_optional(
        string_range name1, string_range name2, std::size_t nargs,
        bool required = false
    ):
        arg_argument(nargs), p_required(required)
    {
        p_names.emplace_back(name1);
        p_names.emplace_back(name2);
    }

    /** @brief See arg_description::find_arg().
     *
     * If `tp` is given, it will be checked. All names are iterated
     * and compared against `name`. If any matches, `this` will be
     * returned. Otherwise `nullptr` will be returned.
     */
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

    /** @brief Marks the argument as used.
     *
     * If the number of uses is limited and the limit has already been
     * reached, ostd::arg_error is thrown. Otherwise, the usage counter
     * is incremented and if an action has been set, it's called with
     * the given range of values. How many values the range contains
     * depends on the value requirement of the argument, so it can
     * be anything - empty, a single value or several.
     *
     * The action can throw, it's not caught and gets propagated all
     * the way to the outside.
     */
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

    /** @brief Resets the usage counter. */
    void reset() {
        p_used = 0;
    }

private:
    void validate_req(arg_value req) const noexcept {
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
    bool p_required;
};

/** @brief A positional argument class.
 *
 * A positional argument is not prefixed. It also doesn't have to always
 * be used, so positional arguments with optional values are possible.
 * The important part is that positional arguments have a strictly
 * defined order and don't have a specific format. They simply capture
 * values in the list that are in their positions. Also, obviously they
 * are only used once so their usage is a boolean, not a counter.
 *
 *See ostd::basic_arg_parser for more.
 */
struct arg_positional: arg_argument {
    template<typename HelpFormatter>
    friend struct basic_arg_parser;
    friend struct arg_description_container;

    /** @brief Gets the argument class type (ostd::arg_type).
     *
     * The value is always `POSITIONAL`.
     */
    arg_type type() const noexcept {
        return arg_type::POSITIONAL;
    }

    /** @brief Gets the name of the positional argument.
     *
     * A name is just a unique label. It's also used in place
     * of metavar() when no metavar is explicitly set.
     */
    string_range name() const noexcept {
        return p_name;
    }

    /** @brief Sets the action to run when the argument is used.
     *
     * The function is called with a finite random access range
     * of ostd::string_range, each containing a value.  It's not
     * expected to return anything.
     */
    template<typename F>
    arg_positional &action(F func) {
        p_action = func;
        return *this;
    }

    /** @brief Like ostd::arg_argument::help(). */
    arg_positional &help(string_range str) {
        arg_argument::help(str);
        return *this;
    }

    /** @brief Gets the metavar string set by help(). */
    string_range help() const noexcept {
        return arg_argument::help();
    }

    /** @brief Like ostd::arg_argument::metavar(). */
    arg_positional &metavar(string_range str) {
        arg_argument::metavar(str);
        return *this;
    }

    /** @brief Gets the metavar string set by metavar(). */
    string_range metavar() const noexcept {
        return arg_argument::metavar();
    }

    /** @brief Checks if the positional arg has been used.
     *
     * If the argument requirement is `EXACTLY` or if it's `ALL` with at
     * least 1 argument, this will always be true after parsing unless
     * an error happens. In other cases it can be false.
     */
    bool used() const noexcept {
        return p_used;
    }

    /** @brief Resets the usage flag. */
    void reset() {
        p_used = false;
    }

protected:
    arg_positional() = delete;

    /** @brief Constructs the positional argument.
     *
     * This version takes an explicit value requirement,
     * see the appropriate constructors of ostd::arg_argument.
     */
    arg_positional(
        string_range name, arg_value req = arg_value::EXACTLY,
        std::size_t nargs = 1
    ):
        arg_argument(req, nargs),
        p_name(name)
    {}

    /** @brief Constructs the positional argument.
     *
     * This version takes only a number, see the appropriate
     * constructors of ostd::arg_argument.
     */
    arg_positional(string_range name, std::size_t nargs):
        arg_argument(nargs),
        p_name(name)
    {}

    /** @brief See arg_description::find_arg().
     *
     * If `tp` is given, it will be checked. The given name is checked
     * against the argument's name. Then it will appropriately return
     * either `this` or `nullptr`.
     */
    arg_description *find_arg(
        string_range name, std::optional<arg_type> tp, bool
    ) {
        if ((tp && (*tp != type())) || (name != ostd::citer(p_name))) {
            return nullptr;
        }
        return this;
    }

    /** @brief Marks the argument as used.
     *
     * The used flag is set and if an action has been set, it's called
     * with the given range of values. How many values the range contains
     * depends on the value requirement of the argument, so it can be
     * anything - empty, a single value or several.
     *
     * The action can throw, it's not caught and gets propagated all
     * the way to the outside.
     */
    void set_values(iterator_range<string_range const *> vals) {
        p_used = true;
        if (p_action) {
            p_action(vals);
        }
    }

private:
    std::function<void(iterator_range<string_range const *>)> p_action;
    std::string p_name;
    bool p_used = false;
};

/** @brief A group of mutually exclusive optional arguments.
 *
 * This is a group of optional arguments that cannot be used together.
 * Mutually exclusive groups allow all of their arguments to be used,
 * but if more than one is used, an ostd::arg_error is thrown and
 * parsing is aborted.
 *
 * A mutually exclusive group can also have the `required` flag set
 * in which case one of the arguments must always be used.
 */
struct arg_mutually_exclusive_group: arg_description {
    friend struct arg_description_container;

    /** @brief Gets the argument class type (ostd::arg_type).
     *
     * The value is always `MUTUALLY_EXCLUSIVE_GROUP`.
     */
    arg_type type() const noexcept {
        return arg_type::MUTUALLY_EXCLUSIVE_GROUP;
    }

    /** @brief Constructs an optional argument in the group.
     *
     * The arguments are perfect-forwarded to the ostd::arg_optional
     * constructors. The constructors are protected but may be used
     * from here.
     */
    template<typename ...A>
    arg_optional &add_optional(A &&...args) {
        auto *opt = new arg_optional(std::forward<A>(args)...);
        std::unique_ptr<arg_description> p{opt};
        if (opt->required()) {
            throw arg_error{
                "required optional arguments not allowed "
                "in mutually exclusive groups"
            };
        }
        p_opts.push_back(std::move(p));
        return *opt;
    }

    /** @brief Calls `func` for each argument in the group.
     *
     * The function takes a reference to ostd::arg_description `const`.
     * It returns `true` if it's meant to continue or `false` if the
     * loop should be aborted.
     *
     * This function returns `false` if the loop was aborted and `true`
     * if it finished successfully.
     */
    template<typename F>
    bool for_each(F &&func) const {
        for (auto &desc: p_opts) {
            if (!func(*desc)) {
                return false;
            }
        }
        return true;
    }

    /** Checks if at least one argument must be used. */
    bool required() const noexcept {
        return p_required;
    }

protected:
    /** @brief Initializes the group. */
    arg_mutually_exclusive_group(bool required = false):
        p_required(required)
    {}

    /** @brief See arg_description::find_arg().
     *
     * This iterates over its optional arguments and calls their
     * own `find_arg()` on each, returning the first matching
     * result. However, if `parsing` is set, it will also check
     * whether any has been used and if one was and it isn't the
     * one that was matched, it throws ostd::arg_error with the
     * appropriate error message. That's how the mutual exclusion
     * is dealt with.
     */
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
            used = static_cast<arg_optional &>(*opt).longest_name();
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<arg_description>> p_opts;
    bool p_required;
};

/** @brief A container for arguments and groups.
 *
 * Used as a base for ostd::arg_group as well as ostd::basic_arg_parser.
 * It defines common methods for them as well as insertion calls. There
 * is no insertion call for regular argument groups though as those
 * may only be inserted from a parser.
 */
struct arg_description_container {
    /** @brief Constructs an optional argument in the container.
     *
     * The arguments are perfect-forwarded to the ostd::arg_optional
     * constructors. The constructors are protected but may be used
     * from here.
     */
    template<typename ...A>
    arg_optional &add_optional(A &&...args) {
        auto *opt = new arg_optional(std::forward<A>(args)...);
        std::unique_ptr<arg_description> p{opt};
        p_opts.push_back(std::move(p));
        return *opt;
    }

    /** @brief Constructs a positional argument in the container.
     *
     * The arguments are perfect-forwarded to the ostd::arg_positional
     * constructors. The constructors are protected but may be used
     * from here.
     */
    template<typename ...A>
    arg_positional &add_positional(A &&...args) {
        auto *pos = new arg_positional(std::forward<A>(args)...);
        std::unique_ptr<arg_description> p{pos};
        p_opts.push_back(std::move(p));
        return *pos;
    }

    /** @brief Constructs a mutually exclusive group in the container.
     *
     * The arguments are perfect-forwarded to the
     * ostd::arg_mutually_exclusive_group constructor. The
     * constructor is protected but may be used from here.
     */
    template<typename ...A>
    arg_mutually_exclusive_group &add_mutually_exclusive_group(A &&...args) {
        auto *mgrp = new arg_mutually_exclusive_group(std::forward<A>(args)...);
        std::unique_ptr<arg_description> p{mgrp};
        p_opts.push_back(std::move(p));
        return *mgrp;
    }

    /** @brief Calls `func` for each argument in the container.
     *
     * The iteration is optionally recursive. It goes through mutually
     * exclusive groups when `iter_ex` is true and also through normal
     * groups if `iter_grp` is true.
     *
     * The function takes a reference to ostd::arg_description `const`.
     * It returns `true` if it's meant to continue or `false` if the
     * loop should be aborted.
     *
     * This function returns `false` if the loop was aborted and `true`
     * if it finished successfully.
     */
    template<typename F>
    bool for_each(F &&func, bool iter_ex, bool iter_grp) const;

protected:
    /** @brief Initializes the data. */
    arg_description_container() {}

    /** @brief Finds an argument in the container.
     *
     * Iterates through everything, calling their own `find_arg()`
     * on each (see arg_description::find_arg()). If that returns
     * something valid, it's dynamically cast to a pointer to `AT`
     * and if that is still valid, it's dereferenced and returned.
     * Otherwise it goes on to the next one.
     *
     * If nothing is found, ostd::arg_error is thrown with the
     * appropriate error message.
     */
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

    std::vector<std::unique_ptr<arg_description>> p_opts;
};

/** @brief A group of arguments.
 *
 * The arguments in the group can be whatever can be inserted into
 * an ostd::arg_description_container. Actual argument groups show
 * in help listing separately from each other and from general args.
 *
 * A group is named and can optionally have a title. The title is
 * displayed in help listing. If not set, the name is displayed.
 */
struct arg_group: arg_description, arg_description_container {
    template<typename HelpFormatter>
    friend struct basic_arg_parser;

    /** @brief Gets the argument class type (ostd::arg_type).
     *
     * The value is always `GROUP`.
     */
    arg_type type() const noexcept {
        return arg_type::GROUP;
    }

    /** @brief Gets the name of the group. */
    string_range name() const noexcept {
        return p_name;
    }

    /** @brief Gets the title of the group.
     *
     * If title was not set properly, it's like name().
     */
    string_range title() const noexcept {
        if (p_title.empty()) {
            return p_name;
        }
        return p_title;
    }

protected:
    arg_group() = delete;

    /** @brief Constructs the group with a name and an optional title. */
    arg_group(string_range name, string_range title = string_range{}):
        arg_description(), arg_description_container(),
        p_name(name), p_title(title)
    {}

    /** @brief See arg_description::find_arg().
     *
     * This iterates over its arguments and calls their own `find_arg()`
     * on each, returning the first matching result or `nullptr`.
     */
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
    std::string p_name, p_title;
    std::vector<std::unique_ptr<arg_description>> p_opts;
};

template<typename F>
inline bool arg_description_container::for_each(
    F &&func, bool iter_ex, bool iter_grp
) const {
    std::remove_reference_t<F> &fref = func;
    for (auto &desc: p_opts) {
        switch (desc->type()) {
            case arg_type::OPTIONAL:
            case arg_type::POSITIONAL:
                if (!fref(*desc)) {
                    return false;
                }
                break;
            case arg_type::GROUP:
                if (!iter_grp) {
                    if (!fref(*desc)) {
                        return false;
                    }
                    continue;
                }
                if (!static_cast<arg_group const &>(*desc).for_each(
                    fref, iter_ex, iter_grp
                )) {
                    return false;
                }
                break;
            case arg_type::MUTUALLY_EXCLUSIVE_GROUP:
                if (!iter_ex) {
                    if (!fref(*desc)) {
                        return false;
                    }
                    continue;
                }
                if (!static_cast<arg_mutually_exclusive_group const &>(
                    *desc
                ).for_each(fref)) {
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

/** @brief A command line argument parser.
 *
 * This implements a universal parser for command line arguments.
 *
 * It supports positional arguments, optional arguments, groups and mutually
 * exclusive groups. It supports GNU and POSIX style argument ordering. Help
 * formatting is provided using `HelpFormatter`.
 *
 * Optional arguments with arbitrary prefixes constrained by a constructor
 * string are supported. By default, the only allowed prefix character is
 * the `-` character as in POSIX style arguments.
 *
 * The system also supports a special argument that will be skipped but will
 * indicate that all following arguments after it are positional. By default
 * this is a string of two instances of the first allowed prefix character,
 * therefore `--` by default.
 *
 * GNU style argument ordering (default) means that optional and positional
 * arguments can be mixed. POSIX style argument ordering means that optional
 * arguments come first and when a non-optional argument is encountered, any
 * argument after that is considered positional no matter the prefix.
 *
 * Generally, the input command line arguments are like this:
 *
 * ~~~
 * --opt1 --opt2 --opt3 pos1 pos2 pos3
 * ~~~
 *
 * With mixed mode, you can do something like:
 *
 * ~~~
 * --opt1 pos1 --opt2 pos2 --opt3 pos3
 * ~~~
 *
 * Using the special argument you can do this:
 *
 * ~~~
 * --opt1 pos1 -- --pos2 --pos3
 * ~~~
 *
 * With POSIX style:
 *
 * ~~~
 * --opt1 pos1 --pos2 --pos3
 * ~~~
 *
 * Optional arguments are formatted like this:
 *
 * ~~~
 * --opt-with-no-value --opt-with-value=VALUE --opt-with-value VALUE
 * ~~~
 *
 * Keep in mind that if you use the version with a space, you have an
 * argument that requires a value and that is followed by an argument
 * detected as optional (i.e. having at least one optional prefix char),
 * it will not be assuemd to be the value and you will get an error
 * (the parser will throw ostd::arg_error with an appropriate message).
 *
 * ~~~
 * # errors
 * --arg-with-value --foo
 * # passes
 * --arg-with-value=--foo
 * ~~~
 *
 * The reason for this is to avoid potential mistakes.
 *
 * If an argument optionally takes a value and is followed by a value that
 * does not have an optional-like format, it will be used as the value.
 * If the `=` style value assignment is used, it will be assigned to the
 * argument no matter the value's format. If it's followed by a value
 * that looks like an optional, the argument will have no value and the
 * following value will be assumed to be another argument.
 *
 * If an argument takes multiple values, the following can be done:
 *
 * ~~~
 * --arg val1 val2 val3 ...
 * --arg=val1 val2 val3 ...
 * ~~~
 *
 * The `REST` value of ostd::arg_value is useful when coupled with a
 * positional argument. First, all positional arguments preceding the
 * one with `REST` are filled and then every single value no matter the
 * format following the last non-`REST` positional value is used for
 * the `REST` argument. If there is no non-`REST` positional argument
 * before the `REST` one, the first non-optional argument onwards will
 * be used for the `REST` argument. This is also true with GNU style
 * mixed argument ordering.
 *
 * ~~~
 * # multiple positionals followed by REST
 * --foo pos1 pos2 [--part-of-rest part_of_rest --also-part-of-rest]
 * # only REST
 * --foo [part_of_rest --also-part-of-rest]
 * ~~~
 *
 * The parser also makes no distinction between short and long arguments,
 * the only requiremnt is at least one prefix character. Thus you can have
 * arguments like `-arg`, `--arg`, `-a`, `--a` and their value-taking
 * syntax is identical. If you define extra prefix characaters, you can
 * also have syntax like `/arg` (DOS/Windows-style) or `++arg` or anything.
 *
 * The `progname` is used for help formatting. It's passed in during
 * construction, but if you don't and you use the parse() call with
 * `argc` and `argv` rather than range, `argv[0]] will be assumed to
 * be the progname if not already set.
 */
template<typename HelpFormatter>
struct basic_arg_parser: arg_description_container {
private:
    struct parse_stop {};

public:
    /** @brief Constructs the parser.
     *
     * The `progname` argument is used for help listing. If empty, it
     * can also be filled using parse() with `argc`/`argv`. The range
     * based parse() will not set it and a fallback will be used.
     *
     * The `pfx_chars` argument defines allowed characters for optional
     * argument prefixes. By default, only `-` is allowed.
     *
     * When `pos_sep` is not empty, it's the special argument that makes
     * all following values to be positional. When empty, it's set to
     * two instances of the first `pfx_chars` character, i.e. `--` by
     * default.
     *
     * When `pfx_chars` does not contain at least one character,
     * ostd::arg_error is thrown with the appropriate error.
     *
     * The `posix` argument enforces POSIX style argument ordering.
     */
    basic_arg_parser(
        string_range progname = string_range{},
        string_range pfx_chars = "-",
        string_range pos_sep = string_range{},
        bool posix = false
    ):
        arg_description_container(), p_progname(progname),
        p_pfx_chars(pfx_chars), p_pos_sep(), p_posix(posix)
    {
        if (pfx_chars.empty()) {
            throw arg_error{"at least one prefix character needed"};
        }
        if (pos_sep.empty()) {
            p_pos_sep.append(2, pfx_chars[0]);
        } else {
            p_pos_sep.append(pos_sep.data(), pos_sep.size());
        }
    }

    /** @brief Constructs a group in the container.
     *
     * The arguments are perfect-forwarded to the ostd::arg_group constructor.
     * The constructor is protected but may be used from here. Groups cannot
     * be constructed inside ostd::arg_description_container itself.
     */
    template<typename ...A>
    arg_group &add_group(A &&...args) {
        arg_description *p = new arg_group(std::forward<A>(args)...);
        return static_cast<arg_group &>(*p_opts.emplace_back(p));
    }

    /** @brief Parses arguments using `argc` and `argv`.
     *
     * This is a convenience function for usage with standard C++ `main`.
     * If `progname` was empty during construction, it's set from `argv[0]`.
     * The arguments are `argv[1]` onwards.
     *
     * Otherwise the same as parse(InputRange).
     */
    void parse(int argc, char **argv) {
        if (p_progname.empty()) {
            p_progname = argv[0];
        }
        parse(ostd::iter(&argv[1], &argv[argc]));
    }

    /** @brief Parses arguments.
     *
     * Parses arguments according to the rules. Actions are called where
     * necessary and both positional and optional arguments are handled.
     * If an error happens (action throws, mutually exclusive arguments
     * are used or any other error condition), typically ostd::arg_error
     * is thrown. The only times something else is thrown is when an
     * action explicitly throws a different exception.
     */
    template<typename InputRange>
    void parse(InputRange args) {
        /* count positional args until remainder */
        std::size_t npos = 0;
        bool has_rest = false;
        for_each([&has_rest, &npos](auto const &arg) {
            if (arg.type() == arg_type::OPTIONAL) {
                const_cast<arg_optional &>(
                    static_cast<arg_optional const &>(arg)
                ).reset();
            }
            if (arg.type() != arg_type::POSITIONAL) {
                return true;
            }
            auto const &desc = static_cast<arg_positional const &>(arg);
            const_cast<arg_positional &>(desc).reset();
            if (desc.needs_value() == arg_value::REST) {
                has_rest = true;
                return true;
            }
            if (!has_rest) {
                ++npos;
            }
            return true;
        }, true, true);
        bool allow_optional = true;
        while (!args.empty()) {
            string_range s{args.front()};
            if (s == citer(p_pos_sep)) {
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
            try {
                parse_pos(s, args, allow_optional);
            } catch (parse_stop) {
                return;
            }
            if (has_rest && npos) {
                --npos;
                if (!npos && !args.empty()) {
                    /* parse rest after all preceding positionals are filled
                     * if the only positional consumes rest, it will be filled
                     * by the above when the first non-optional is encountered
                     */
                    try {
                        parse_pos(string_range{args.front()}, args, false);
                    } catch (parse_stop) {
                        return;
                    }
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
                    auto const &mopt = static_cast<arg_optional const &>(marg);
                    if (mopt.used()) {
                        cont = true;
                        return false;
                    }
                    names.push_back(mopt.longest_name());
                    return true;
                });
                if (!cont) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "one of the arguments %('%s'%|, %) is required", names
                    ).get()};
                }
                return true;
            }
            if (arg.type() == arg_type::OPTIONAL) {
                auto const &oarg = static_cast<arg_optional const &>(arg);
                if (oarg.required() && !oarg.used()) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "argument '%s' is required", oarg.longest_name()
                    ).get()};
                }
                return true;
            }
            auto const &desc = static_cast<arg_positional const &>(arg);
            auto needs = desc.needs_value();
            auto nargs = desc.nargs();
            if ((needs != arg_value::EXACTLY) && (needs != arg_value::ALL)) {
                return true;
            }
            if (!nargs || desc.used()) {
                return true;
            }
            throw arg_error{"too few arguments"};
        }, false, true);
    }

    /** @brief Formats help into the given output range.
     *
     * It does so using the help formatter. First `format_usage()`
     * is called followed by `format_options()`. The range is then
     * returned, forwarded.
     */
    template<typename OutputRange>
    OutputRange &&print_help(OutputRange &&range) {
        p_helpfmt.format_usage(range);
        p_helpfmt.format_options(range);
        return std::forward<OutputRange>(range);
    }

    /** @brief A convenience print_help() with ostd::cout. */
    void print_help() {
        print_help(cout.iter());
    }

    /** @brief Gets an argument with the given name.
     *
     * It can be either optional or positional,
     * ostd::arg_description_container::find_arg() is used
     * in non-parsing mode.
     */
    arg_argument &get(string_range name) {
        return find_arg<arg_argument>(name, false);
    }

    /** @brief Gets the previously set progname. */
    string_range progname() const noexcept {
        return p_progname;
    }

    /** @brief Checks is POSIX style ordering is used. */
    bool posix_ordering() const noexcept {
        return p_posix;
    }

    /** @brief Sets if POSIX style ordering is used.
     *
     * The previous ordering style is returned.
     */
    bool posix_ordering(bool v) noexcept {
        return std::exchange(p_posix, v);
    }

    /** @brief When called within an action, aborts parsing.
     *
     * Do not call outside, as this throws an exception that is internal
     * and cannot be handled. It's handled internally in the parser, so
     * only use from actions if you want to stop the parsing. This is
     * useful when e.g. doing help printing.
     */
    void stop_parsing() {
        throw parse_stop{};
    }

private:
    bool is_optarg(string_range arg) {
        if (arg.size() <= 1) {
            return false;
        }
        return (p_pfx_chars.find(arg[0]) != std::string::npos);
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
        auto nargs = desc.nargs();

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
        auto nargs = desc.nargs();

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
                    desc.name(), nargs
                ).get()};
            }
        } else if ((needs == arg_value::EXACTLY) && (nargs > 1)) {
            auto reqargs = nargs - 1;
            while (reqargs) {
                if (args.empty() || (allow_opt && is_optarg(args.front()))) {
                    throw arg_error{format(
                        appender<std::string>(),
                        "positional argument '%s' needs exactly %d values",
                        desc.name(), nargs
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

    std::string p_progname, p_pfx_chars, p_pos_sep;
    HelpFormatter p_helpfmt{*this};
    bool p_posix = false;
};

/** @brief The default help formatter class for ostd::basic_arg_parser.
 *
 * It formats help with the following format:
 *
 * ~~~
 * Usage: <progname> [opts] [args]
 *
 * Positional arguments:
 *   <metavar or name>     <help string>
 *   <...>                 <help string>
 *
 * Optional arguments:
 *   -name1, --name2, ...  <help string>
 *   -a ARG, --arg ARG     <help string>
 *   -a [ARG]              <help string>
 *   -a [ARG ...]          <help string>
 *
 * Group title:
 *   <positional>         <help string>
 *   <optional>           <help string>
 * ~~~
 *
 * See the respective methods for more details.
 */
struct default_help_formatter {
    /** @brief Constructs the formatter with a parser. */
    default_help_formatter(basic_arg_parser<default_help_formatter> &p):
        p_parser(p)
    {}

    /** @brief Formats the usage line.
     *
     * If ostd::basic_arg_parser::progname() is empty, `program`
     * is used as a fallback.
     */
    template<typename OutputRange>
    void format_usage(OutputRange &out) {
        string_range progname = p_parser.progname();
        if (progname.empty()) {
            progname = "program";
        }
        format(out, "Usage: %s [opts] [args]\n", progname);
    }

    /** @brief Formats the options (after usage line).
     *
     * Positional arguments not belonging to any group are formatted
     * first, with `\nPositional arguments:\n` header. Same goes with
     * optional arguments, except with `\nOptional arguments:\n`.
     *
     * If either positional or optional arguments without group don't
     * exist, the section is skipped.
     *
     * All arguments, in groups or not, are offset by 2 spaces. All
     * help strings are aligned and offset by 2 spaces from the longest
     * argument string.
     *
     * Group titles are formatted as `\nTITLE:\n`.
     *
     * Within groups, positional arguments come first and optional
     * arguments second. Mutually exclusive groups are expanded.
     * Actual arguments are formatted with format_option().
     */
    template<typename OutputRange>
    void format_options(OutputRange &out) {
        std::size_t opt_namel = 0, pos_namel = 0, grp_namel = 0;

        std::vector<arg_argument const *> allopt;
        std::vector<arg_argument const *> allpos;

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
                        }
                    );
                    break;
                default:
                    break;
            }
            return true;
        }, false, false);
        std::size_t maxpad = std::max({opt_namel, pos_namel, grp_namel});

        auto write_help = [maxpad, this](
            auto &out, std::vector<arg_argument const *> const &args
        ) {
            for (auto p: args) {
                format(out, "  ");
                auto &parg = *p;
                auto cr = counting_sink(out);
                this->format_option(cr, parg);
                out = std::move(cr.get_range());
                auto help = parg.help();
                if (help.empty()) {
                    out.put('\n');
                } else {
                    std::size_t nd = maxpad - cr.get_written() + 2;
                    for (std::size_t i = 0; i < nd; ++i) {
                        out.put(' ');
                    }
                    format(out, "%s\n", help);
                }
            }
        };

        if (!allpos.empty()) {
            format(out, "\nPositional arguments:\n");
            write_help(out, allpos);
        }

        if (!allopt.empty()) {
            format(out, "\nOptional arguments:\n");
            write_help(out, allopt);
        }

        allopt.clear();
        allpos.clear();

        p_parser.for_each([
            &write_help, &out, &allopt, &allpos
        ](auto const &arg) {
            if (arg.type() != arg_type::GROUP) {
                return true;
            }
            auto &garg = static_cast<arg_group const &>(arg);
            format(out, "\n%s:\n", garg.title());
            garg.for_each([
                &write_help, &out, &allopt, &allpos
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
            write_help(out, allpos);
            write_help(out, allopt);
            return true;
        }, false, false);
    }

    /** @brief Formats an optional argument.
     *
     * If a metavar exists (ostd::arg_optional::metavar()) then it's
     * used as-is. Otherwise, the first name longer than 1 character
     * without prefix is uppercased (without prefix) and used as a
     * metavar. If thta fails, `VALUE` is used as a fallback.
     *
     * The option is formatted as `<name> <value>` for each name separated
     * by a comma followed by a space. If the argument does not take any
     * values, ` <value>` is not used. The format of `<value>` is the
     * metavar when taking exactly one value, a space separated list
     * of metavars when taking multiple, `[<metavar>]` with one optional
     * value, `[<metavar> ...]` with all values with no "at least" count,
     * and `<metavar> <metavar> [<metavar> ...]` for example when at least
     * two values are necessary.
     */
    template<typename OutputRange>
    void format_option(OutputRange &out, arg_optional const &arg) {
        std::string mt = arg.real_metavar();
        auto names = arg.names();
        for (;;) {
            format(out, names.front());
            switch (arg.needs_value()) {
                case arg_value::EXACTLY: {
                    for (auto nargs = arg.nargs(); nargs; --nargs) {
                        format(out, " %s", mt);
                    }
                    break;
                }
                case arg_value::OPTIONAL:
                    format(out, " [%s]", mt);
                    break;
                case arg_value::ALL:
                    for (auto nargs = arg.nargs(); nargs; --nargs) {
                        format(out, " %s", mt);
                    }
                    format(out, " [%s ...]", mt);
                    break;
                default:
                    break;
            }
            names.pop_front();
            if (names.empty()) {
                break;
            }
            format(out, ", ");
        }
    }

    /** @brief Formats a positional argument.
     *
     * If a metavar exists (ostd::arg_positional::metavar()) then it's
     * used as-is. Otherwise, the argument's name is used as-is.
     *
     * The argument is formatted simply as `<metavar>`.
     */
    template<typename OutputRange>
    void format_option(OutputRange &out, arg_positional const &arg) {
        auto mt = arg.metavar();
        if (mt.empty()) {
            mt = arg.name();
        }
        format(out, mt);
    }

    /** @brief Formats either a positional or optional argument.
     *
     * The right call is decided according to arg_description::type().
     * If it's neither, ostd::arg_error is thrown.
     */
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

/** @brief A default specialization of ostd::basic_arg_parser. */
using arg_parser = basic_arg_parser<default_help_formatter>;

/** @brief A help-printing argument action.
 *
 * When called with an output range and a reference to parser, this
 * returns a function which can be passed as an argument action.
 * The function prints help and aborts parsing, see
 * ostd::arg_parser::stop_parsing().
 */
template<typename OutputRange>
auto arg_print_help(OutputRange o, arg_parser &p) {
    return [o = std::move(o), &p](iterator_range<string_range const *>)
        mutable
    {
        p.print_help(o);
        p.stop_parsing();
    };
};

/** @brief Like ostd::arg_print_help() with ostd::cout. */
auto arg_print_help(arg_parser &p) {
    return arg_print_help(cout.iter(), p);
}

/** @brief A constant-storing argument action.
 *
 * Given a value and a reference, this returns a function that copies
 * the value into the reference using assignment. Use with arguments
 * with no values from command line.
 */
template<typename T, typename U>
auto arg_store_const(T &&val, U &ref) {
    return [val, &ref](iterator_range<string_range const *>) mutable {
        ref = std::move(val);
    };
}

/** @brief A string-storing argument action.
 *
 * The returne function stores the first given value in the `ref`.
 */
template<typename T>
auto arg_store_str(T &ref) {
    return [&ref](iterator_range<string_range const *> r) mutable {
        ref = T{r[0]};
    };
}

/** @brief Like ostd::arg_store_const() with a `true` value. */
auto arg_store_true(bool &ref) {
    return arg_store_const(true, ref);
}

/** @brief Like ostd::arg_store_const() with a `false` value. */
auto arg_store_false(bool &ref) {
    return arg_store_const(false, ref);
}

/** @brief An unformatting argument action.
 *
 * The returned function takes the first value and unformats it
 * into all the given references using a format string. For example
 * if the given value is `5:10` and you want to unformat it into
 * two integers, you use `%d:%d` as the format string.
 *
 * If the string cannot be unfortmatted exactly, an ostd::arg_error
 * with an appropriate message is thrown.
 */
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
