/** @addtogroup Strings
 * @{
 */

/** @file format.hh
 *
 * @brief APIs for type safe formatting using C-style format strings.
 *
 * OctaSTD provides a powerful formatting system that lets you format into
 * arbitrary output ranges using C-style format strings. It's type safe
 * and supports custom object formatting without heap allocations as well
 * as formatting of ranges, tuples and more.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_FORMAT_HH
#define OSTD_FORMAT_HH

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cmath>

#include <utility>
#include <stdexcept>
#include <locale>
#include <ios>

#include "ostd/algorithm.hh"
#include "ostd/string.hh"

namespace ostd {

/** @addtogroup Strings
 * @{
 */

/** @brief An enumeration defining flags for C-style formatting marks.
 *
 * Used inside ostd::format_spec. The C-style formatting mark has a flags
 * section and each of these enum items represents one. They can be combined
 * using the standard bitwise operators.
 */
enum format_flags {
    FMT_FLAG_DASH  = 1 << 0, ///< The dash (`-`) flag.
    FMT_FLAG_ZERO  = 1 << 1, ///< The zero (`0`) flag.
    FMT_FLAG_SPACE = 1 << 2, ///< The space (` `) flag.
    FMT_FLAG_PLUS  = 1 << 3, ///< The plus (`+`) flag.
    FMT_FLAG_HASH  = 1 << 4, ///< The hash (`#`) flag.
    FMT_FLAG_AT    = 1 << 5  ///< The at (`@`) flag.
};

/** @brief Thrown when format string does not properly match the arguments. */
struct format_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct format_spec;

/** @brief Specialize this to format custom objects.
 *
 * The formatting system provides a way to format arbitrary objects. By default
 * it's empty as all the formatting logic is builtin. To specialize for your
 * own object, you simply do this:
 *
 * ~~~{.cc}
 * template<>
 * struct format_traits<foo> {
 *     template<typename R>
 *     static void to_format(foo const &v, R &writer, ostd::format_spec const &fs) {
 *         // custom formatting here
 *         // writer is just an output range (see ostd::output_range)
 *     }
 * };
 * ~~~
 *
 * Obviously, you can passthrough the formatting, for example when your type
 * contains a member and you want to format your type exactly as if it was
 * the member, you just put this in your `to_format`:
 *
 * ~~~{.cc}
 *     fs.format_value(writer, v.my_member);
 * ~~~
 *
 * Anything that writes into the output range will do. The output range is
 * exactly the same output range the outer format call is formatting into,
 * so for example when someone is formatting into an ostd::appender_range,
 * it will be just that.
 *
 * This may be specialized in other OctaSTD modules as well.
 */
template<typename>
struct format_traits {};

/* implementation helpers */
namespace detail {
    inline int parse_fmt_flags(string_range &fmt, int ret) {
        while (!fmt.empty()) {
            switch (fmt.front()) {
                case '-': ret |= FMT_FLAG_DASH; fmt.pop_front(); break;
                case '+': ret |= FMT_FLAG_PLUS; fmt.pop_front(); break;
                case '#': ret |= FMT_FLAG_HASH; fmt.pop_front(); break;
                case '@': ret |= FMT_FLAG_AT;   fmt.pop_front(); break;
                case '0': ret |= FMT_FLAG_ZERO; fmt.pop_front(); break;
                case ' ': ret |= FMT_FLAG_SPACE; fmt.pop_front(); break;
                default: goto retflags;
            }
        }
    retflags:
        return ret;
    }

    inline size_t read_digits(string_range &fmt, char *buf) {
        size_t ret = 0;
        for (; !fmt.empty() && isdigit(fmt.front()); ++ret) {
            *buf++ = fmt.front();
            fmt.pop_front();
        }
        *buf = '\0';
        return ret;
    }

    /* 0 .. not allowed
     * 1 .. floating point
     * 2 .. character
     * 3 .. binary
     * 4 .. octal
     * 5 .. decimal
     * 6 .. hexadecimal
     * 7 .. string
     * 8 .. custom object
     */
    static constexpr byte const fmt_specs[] = {
        /* uppercase spec set */
        1, 3, 8, 8, /* A B C D */
        1, 1, 1, 8, /* E F G H */
        8, 8, 8, 8, /* I J K L */
        8, 8, 8, 8, /* M N O P */
        8, 8, 8, 8, /* Q R S T */
        8, 8, 8, 6, /* U V W X */
        8, 8,       /* Y Z */

        /* ascii filler */
        0, 0, 0, 0, 0, 0,

        /* lowercase spec set */
        1, 3, 2, 5, /* a b c d */
        1, 1, 1, 8, /* e f g h */
        8, 8, 8, 8, /* i j k l */
        8, 8, 4, 8, /* m n o p */
        8, 8, 7, 8, /* q r s t */
        8, 8, 8, 6, /* u v w x */
        8, 8,       /* y z */

        /* ascii filler */
        0, 0, 0, 0, 0
    };

    static constexpr int const fmt_bases[] = {
        0, 0, 0, 2, 8, 10, 16, 0
    };

    /* non-printable escapes up to 0x20 (space) */
    static constexpr char const *fmt_escapes[] = {
        "\\0"  , "\\x01", "\\x02", "\\x03", "\\x04", "\\x05",
        "\\x06", "\\a"  , "\\b"  , "\\t"  , "\\n"  , "\\v"  ,
        "\\f"  , "\\r"  , "\\x0E", "\\x0F", "\\x10", "\\x11",
        "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17",
        "\\x18", "\\x19", "\\x1A", "\\x1B", "\\x1C", "\\x1D",
        "\\x1E", "\\x1F",
        /* we want to escape double quotes... */
        nullptr, nullptr, "\\\"", nullptr, nullptr, nullptr,
        nullptr, "\\\'"
    };

    inline char const *escape_fmt_char(char v, char quote) {
        if ((v >= 0 && v < 0x20) || (v == quote)) {
            return fmt_escapes[size_t(v)];
        } else if (v == 0x7F) {
            return "\\x7F";
        }
        return nullptr;
    }

    /* retrieve width/precision */
    template<typename T, typename ...A>
    inline int get_arg_param(size_t idx, T const &val, A const &...args) {
        if (idx) {
            if constexpr(!sizeof...(A)) {
                throw format_error{"not enough format args"};
            } else {
                return get_arg_param(idx - 1, args...);
            }
        } else {
            if constexpr(!std::is_integral_v<T>) {
                throw format_error{"invalid argument for width/precision"};
            } else {
                if constexpr(std::is_signed_v<T>) {
                    if (val < 0) {
                        throw format_error{
                            "width/precision cannot be negative"
                        };
                    }
                }
                return int(val);
            }
        }
    }

    /* ugly ass check for whether a type is tuple-like, like tuple itself,
     * pair, array, possibly other types added later or overridden...
     */
    template<typename T>
    std::true_type tuple_like_test(typename std::tuple_size<T>::type *);

    template<typename>
    std::false_type tuple_like_test(...);

    template<typename T>
    constexpr bool is_tuple_like = decltype(tuple_like_test<T>(0))::value;

    /* test if format traits are available for the type */
    template<typename T, typename R>
    static std::true_type test_tofmt(decltype(format_traits<T>::to_format(
        std::declval<T const &>(), std::declval<R &>(),
        std::declval<format_spec const &>()
    )) *);

    template<typename, typename>
    static std::false_type test_tofmt(...);

    template<typename T, typename R>
    constexpr bool fmt_tofmt_test = decltype(test_tofmt<T, R>(0))::value;
}

/** @brief A structure implementing type safe C-style formatting.
 *
 * It can be constructed either to represent a specific format specifier or
 * with a format string to format an entire string (in which case it will
 * parse the string and format it with the individual intermediate markings).
 *
 * It stores information about the current format specifier (when constructed
 * as one or when parsing the format string) as well as the rest of the current
 * format string. See read_until_spec() and rest() for more information.
 *
 * # Regular format specifiers
 *
 * The formatter is considerably more elaborate than C-style printf. Its
 * basic format specifiers superficially look the same:
 *
 * ~~~
 * %[position$][flags][width][.precision]specifier
 * ~~~
 *
 * Position is the optional position of the argument in the pack starting
 * with 1. It can be mixed with format specifiers without explicit position,
 * unlike what POSIX says; the next specifier without explicit position
 * will use the position after the largest explicit position used so far.
 * For example, `%3$s %1$s %s` will use position 4 for the last specifier.
 *
 * ## Flags
 *
 * * The `-` flag will left-justify within the given width (right by default).
 * * The `+` flag applies to numbers and will force sign to always show, even
 *   for positive numbers (by default, only negative ones get a sign).
 * * The ` ` (space) flag applies to numbers and will force a space to be
 *   written in place of sign for positive numbers (no effect with `+`).
 * * The `#` flag applies to integers, floats and ranges. For integers, it
 *   will add the prefixes `0x`, `0X`, `0b` or `0B` when formatted as hex
 *   or binary, lowercase or uppercase. For floats, it will force the output
 *   to always contain a decimal point/comma. For ranges, it will cause
 *   automatic expansion of values into items if the values are tuples.
 * * The `@` flag will escape the value according to the rules.
 * * The '0' flag will left-pad numbers with zeroes instead of spaces when
 *   needed (according to width).
 *
 * ## Width
 *
 * Width can be specified either as a number in the format string or as `*`
 * in which case it will be an integer argument (any integral type, must be
 * equal or larger than zero, otherwise ostd::format_error is thrown). When
 * an argument, the position of the argument is where the actual value should
 * have been if no argument was used, and the actual value follows. The same
 * applies with explicit positions.
 *
 * Width defines the minimum number of characters to be printed. If the value
 * ends up being shorter, it's padded with spaces (or zeroes when formatting
 * a number and the zero flag is used). The value is not truncated if it's
 * actually longer than the width.
 *
 * ## Precision
 *
 * Precision can also be specified as a number or as an argument. When both
 * width and precision are an argument, width is first. For integers, it
 * specifies the default number of digits to be written. If the value is
 * shorter than the precision, the result is padded with leading zeroes.
 * If it's longer, no truncation happens. A precision of 0 means that no
 * character is written for the value 0. For floats, it's the number of
 * digits to be written after decimal point or comma. When not specified,
 * it's 6. For strings, it's the maximum number of characters to be printed.
 * By default all characters are printed. When escaping strings, the quotes
 * are not counted into the precision and escape sequences count as a single
 * character.
 *
 * # Range formatting
 *
 * The system also allows advanced formatting for ranges. The specifier
 * then looks different:
 *
 * ~~~
 * %[flags](contents%)
 * ~~~
 *
 * The `contents` is a format specifier for each item of the range followed
 * by a separator. For example:
 *
 * ~~~
 * %(%s, %)
 * ~~~
 *
 * In this case, `%s` is the specifier and `, ` is the separator. You can
 * also explicitly delimit the separator:
 *
 * ~~~
 * %(%s%|, %)
 * ~~~
 *
 * The first part is used to format items and the separator is put between
 * each two items.
 *
 * Two flags are used by this format. Normally, each item of the range is
 * formatted as is, using a single specifier, even if the item is a tuple-like
 * value. Using the `#` flag you can expand tuple-like items into multiple
 * values. So when formatting a range over an associative map, you can do this:
 *
 * ~~~
 * %#(%s: %s%|, %)
 * ~~~
 *
 * to format key and value separately.
 *
 * You can also use the `@` flag. It will cause the `@` flag to be applied to
 * every item of the range, therefore escaping each one. Nested range formats
 * are also affected by this. There is no way to unapply the flag once you
 * set it.
 *
 * # Tuple formatting
 *
 * Additionally, the system also supports advanced formatting for tuples.
 * The syntax is similar:
 *
 * ~~~
 * %[flags]<contents%>
 * ~~~
 *
 * There are no delimiters here. The `contents` is simply a regular format
 * string, with a format specifier for each tuple item.
 *
 * You can use the `@` flag just like you can use it with ranges. No other
 * flag can be used when formatting tuples.
 *
 * # Specifiers
 *
 * Now for the basic specifiers themselves:
 *
 * * `a`, `A` - hexadecimal float like C printf (lowercase, uppercase).
 * * `b`, `B` - binary integers (lowercase, uppercase).
 * * `c` - character values.
 * * `d` - decimal integers (signed or unsigned).
 * * `e`, `E` - floats in scientific notation (lowercase, uppercase).
 * * `f`, `F` - decimal floating point (lowercase, uppercase).
 * * `g`, `G` - shortest representation (`e`/`E` or `f`/`F`).
 * * `o` - octal octal (signed or unsigned)
 * * `s` - any value with its default format
 * * `x`, `X` - hexadecimal integers (lowercase, uppercase, signed or unsigned).
 *
 * You can use the `s` specifier to format any value that can be formatted
 * at no extra cost. Because the system is type safe, how a value is meant
 * to be formatted is decided from the type that is passed in, not the format
 * specifier itself.
 *
 * All letters (uppercase and lowercase) are available for custom formatting.
 *
 * # Format order and rules
 *
 * The rules for formatting values go as follows:
 *
 * * First it's checked whether the value can be custom formatted using a
 *   specialization of ostd::format_traits. If it can, it's formatted using
 *   that, the current `format_spec` is passed in as it is and no extra
 *   checks are made. Any letter can be used to format custom objects.
 * * Then it's checked if the value is convertible to ostd::string_range.
 *   If it is, it's formatted as a string. Only the `s` specifier is allowed.
 * * Then it's checked if the value is a tuple-like object. The value is one
 *   if `std::tuple_size<T>::value` is valid. If it is, the tuple-like object
 *   is formatted as `<ITEM, ITEM, ITEM, ...>` by default. You need to use
 *   the `s` specifier only to format tuples like this. The items are all
 *   formatted using the `s` specifier.
 * * Then ranges are tested in a similar way. The default format for ranges
 *   is `{ITEM, ITEM, ITEM, ...}`. The `s` specifier must be used. The items
 *   are all formatted with `s` too.
 * * Then bools are formatted. If the `s` specifier is used, the bool is
 *   formatted as `true` or `false`. Otherwise it's converted to `int` and
 *   formatted using the specifier (might error depending on the specifier).
 * * Then character values are formatted. The `c` and `s` specifiers are
 *   allowed.
 * * Pointers are formatted then. If the `s` specifier is used, the pointer
 *   will be formatted as hex with the `0x` prefix. Otherwise it's converted
 *   to `size_t` and formatted with the specifier (might error depending on
 *   the specifier).
 * * Then integers are formatted. Using the `s` specifier is like using the
 *   `d` specifier.
 * * Floats follow. Using `s` is like using `g`.
 * * When everything is exhausted, ostd::format_error is thrown.
 *
 * # Escaping
 *
 * String and character values are subject to escaping if the `@` flag is
 * used. Strings are put into double quotes and any unprintable values in
 * them are converted into escape sequences. Quotes (single and double)
 * are also escaped. Character values are put into single quotes and
 * unprintable characters are converted into escape sequences as well.
 * For known escape sequences, simple readable versions are used, particularly
 * `a`, `b`, `t`, `n`, `v`, `f`, `r`. For other unprintables, the hexadecimal
 * escape format is used.
 *
 * When printing tuples and ranges with the `s` specifier and the `@` flag
 * is used, all of their items are escaped. If the items are tuples or ranges,
 * their own items are also escaped. The `@` flag doesn't escape anything else,
 * unless you implement support for escaping in your own custom objects.
 *
 * # Locale awareness
 *
 * The system also makes use of locales. When formatting integers, thousands
 * grouping rules from the locale apply (no matter the base). When formatting
 * floats, a locale specific decimal separator is used and thousands grouping
 * also applies.
 *
 * # Errors and other remarks
 *
 * Bceause the system is type safe, there is no need to explicitly specify
 * type lengths or any such thing. Any integral type and any floating point
 * type can be formatted using the right specifiers.
 *
 * If a specifier is not allowed for a value, ostd::format_error is thrown.
 */
struct format_spec {
    /** @brief Constructs with a format string and the default locale.
     *
     * If you use this constructor, there won't be a specific formatting
     * specifier set in here so you won't be able to get its properties,
     * but you will be able to format into a range with some arguments.
     * You can also manually parse the format string, see read_until_spec().
     *
     * The locale used here is the default (global) locale.
     */
    format_spec(string_range fmt):
        p_fmt(fmt), p_loc()
    {}

    /** @brief Constructs with a format string and a locale.
     *
     * Like format_spec(string_range), but with an explicit locale.
     */
    format_spec(string_range fmt, std::locale const &loc):
        p_fmt(fmt), p_loc(loc)
    {}

    /** @brief Constructs a specific format specifier.
     *
     * See ostd::format_flags for flags. The `spec` argument is the format
     * specifier (for example `s`). It doesn't support tuple/range formatting
     * nor positional arguments.
     *
     * Uses the default (global) locale. The locale is then potentially used
     * for formatting values.
     */
    format_spec(char spec, int flags = 0):
        p_flags(flags), p_spec(spec), p_loc()
    {}

    /** @brief Constructs a specific format specifier with a locale.
     *
     * Like format_spec(char, int) but uses an explicit locale.
     */
    format_spec(char spec, std::locale const &loc, int flags = 0):
        p_flags(flags), p_spec(spec), p_loc(loc)
    {}

    /** @brief Parses the format string if constructed with one.
     *
     * This reads the format string, writing each character of it into
     * `writer`, until it encounters a valid format specifier. It then
     * stops there and returns `true`. If no format specifier was read,
     * it returns `false`. When a format specifier is read, this structure
     * then represents it.
     *
     * It's used by format() to parse the string.
     */
    template<typename R>
    bool read_until_spec(R &writer) {
        if (p_fmt.empty()) {
            return false;
        }
        while (!p_fmt.empty()) {
            if (p_fmt.front() == '%') {
                p_fmt.pop_front();
                if (p_fmt.front() == '%') {
                    goto plain;
                }
                return read_spec();
            }
    plain:
            writer.put(p_fmt.front());
            p_fmt.pop_front();
        }
        return false;
    }

    /** @brief Gets the yet not parsed portion of the format string.
     *
     * If no read_until_spec() was called, this returns the entire format
     * string. Otherwise, it returns the format string from the point
     * after the format specifier this structure currently represents.
     */
    string_range rest() const {
        return p_fmt;
    }

    /** @brief Overrides the currently set locale.
     *
     * @returns The old locale.
     */
    std::locale imbue(std::locale const &loc) {
        std::locale ret{p_loc};
        p_loc = loc;
        return ret;
    }

    /** @brief Retrieves the currently used locale for the format state. */
    std::locale getloc() const {
        return p_loc;
    }

    /** @brief Gets the width of the format specifier.
     *
     * If explicitly specified (say `%5s`) it will return the number that
     * was in the format specifier. If explicitly set with set_width(),
     * it will return that. If not set at all, it will return 0.
     *
     * @see has_width(), precision()
     */
    int width() const { return p_width; }

    /** @brief Gets the precision of the format specifier.
     *
     * If explicitly specified (say `%.5f`) it will return the number that
     * was in the format specifier. If explicitly set with set_precision(),
     * it will return that. If not set at all, it will return 0.
     *
     * @see has_precision(), width()
     */
    int precision() const { return p_precision; }

    /** @brief Gets whether a width was specified somehow.
     *
     * If the width was provided direclty as part of the format specifier
     * or with an explicit argument (see set_width()), this will return
     * `true`. Otherwise, it will return `false`.
     *
     * You can get the actual width using width().
     *
     * @see has_precision(), arg_width()
     */
    bool has_width() const { return p_has_width; }

    /** @brief Gets whether a precision was specified somehow.
     *
     * If the precision was provided direclty as part of the format specifier
     * or with an explicit argument (see set_precision()), this will return
     * `true`. Otherwise, it will return `false`.
     *
     * You can get the actual width using precision().
     *
     * @see has_width(), arg_precision()
     */
    bool has_precision() const { return p_has_precision; }

    /** @brief Gets whether a width was specified as an explicit argument.
     *
     * This is true if the width was specified using `*` in the format
     * specifier. Also set by set_width(size_t, A const &...).
     *
     * @see has_width()
     */
    bool arg_width() const { return p_arg_width; }

    /** @brief Gets whether a precision was specified as an explicit argument.
     *
     * This is true if the precision was specified using `*` in the format
     * specifier. Also set by set_precision(size_t, A const &...).
     *
     * @see has_width()
     */
    bool arg_precision() const { return p_arg_precision; }

    /** @brief Sets the width from an argument pack.
     *
     * The `idx` parameter specifies the index (starting with 0) of the
     * width argument in the followup pack.
     *
     * The return value of width() will then be the argument's value.
     * It will also make has_width() and arg_width() return true (if
     * they previously didn't).
     *
     * @throws ostd::format_error when `idx` is out of bounds or the argument
     *         has an invalid type.
     *
     * @see set_width(int), set_precision(size_t, A const &...);
     */
    template<typename ...A>
    void set_width(size_t idx, A const &...args) {
        p_width = detail::get_arg_param(idx, args...);
        p_has_width = p_arg_width = true;
    }

    /** @brief Sets the width to an explicit number.
     *
     * The return value of width() will then be the given value. It will
     * also make has_width() return true and arg_width() return false.
     *
     * @see set_width(size_t, A const &...) set_precision(int)
     */
    void set_width(int v) {
        p_width = v;
        p_has_width = true;
        p_arg_width = false;
    }

    /** @brief Sets the precision from an argument pack.
     *
     * The `idx` parameter specifies the index (starting with 0) of the
     * precision argument in the followup pack.
     *
     * The return value of precision() will then be the argument's value.
     * It will also make has_precision() and arg_precision() return true (if
     * they previously didn't).
     *
     * @throws ostd::format_error when `idx` is out of bounds or the argument
     *         has an invalid type.
     *
     * @see set_precision(int), set_width(size_t, A const &...);
     */
    template<typename ...A>
    void set_precision(size_t idx, A const &...args) {
        p_precision = detail::get_arg_param(idx, args...);
        p_has_precision = p_arg_precision = true;
    }

    /** @brief Sets the precision to an explicit number.
     *
     * The return value of precision() will then be the given value. It will
     * also make has_precision() return true and arg_precision() return false.
     *
     * @see set_precision(size_t, A const &...) set_with(int)
     */
    void set_precision(int v) {
        p_precision = v;
        p_has_precision = true;
        p_arg_precision = false;
    }

    /** @brief Gets the combination of flags for the current specifier. */
    int flags() const { return p_flags; }

    /** @brief Gets the base char for the specifier. */
    char spec() const { return p_spec; }

    /** @brief Gets the position of the matching argument in the pack.
     *
     * This applies for when the position in the format specifier was
     * explicitly set (for example `%5$s` will have index 5) to refer
     * to a specific argument in the pack. Keep in mind that these
     * start with 1 (1st argument, 5th argument etc) to match the POSIX
     * conventions on this. If the position was not specified, this just
     * returns 0.
     */
    byte index() const { return p_index; }

    /** @brief Gets the inner part of a range or tuple format specifier.
     *
     * For ranges, this does not include the separator, you need to use
     * nested_sep() to get the separator. For example, given the
     * `%(%s, %)` specifier, this returns `%s` and for `%(%s%|, %)`
     * it returns the same. When formatting tuples, this behaves identically,
     * for example for `%<%s, %f%>` this returns `%s, %f`. For simple
     * specifiers this returns an empty slice.
     */
    string_range nested() const { return p_nested; }

    /** @brief Gets the separator of a complex range format specifier.
     *
     * For example for `%(%s, %)` this returns `, `. With an explicit
     * delimiter, for example for `%(%s%|, %)`, this returns the same
     * thing as well. For simple specifiers and tuple specifiers this
     * returns an empty slice.
     */
    string_range nested_sep() const { return p_nested_sep; }

    /** @brief Returns true if this specifier is for a tuple. */
    bool is_tuple() const { return p_is_tuple; }

    /** @brief Returns true if this specifier is for a tuple or a range. */
    bool is_nested() const { return p_is_nested; }

    /** @brief Formats into a range with the given arguments.
     *
     * When a valid format string is currently present, this formats
     * into the given range using that format string and the provided
     * arguments.
     *
     * @throws ostd::format_error when the format string and args don't match.
     *
     * @see format_value()
     */
    template<typename R, typename ...A>
    R &&format(R &&writer, A const &...args) {
        write_fmt(writer, args...);
        return std::forward<R>(writer);
    }

    /** @brief Formats a single value into a range.
     *
     * When this currently represents a valid format specifier, you can
     * use this to format a single value with that specifier. This is very
     * useful for example when formatting custom objects, see the example
     * in ostd::format_traits.
     *
     * @throws ostd::format_error when the specifier and the value don't match.
     *
     * @see format()
     */
    template<typename R, typename T>
    R &&format_value(R &&writer, T const &val) const {
        write_arg(writer, 0, val);
        return std::forward<R>(writer);
    }

private:
    string_range p_nested;
    string_range p_nested_sep;

    int p_flags = 0;
    /* internal, for initial set of flags */
    int p_gflags = 0;

    int p_width = 0;
    int p_precision = 0;

    bool p_has_width = false;
    bool p_has_precision = false;

    bool p_arg_width = false;
    bool p_arg_precision = false;

    char p_spec = '\0';

    byte p_index = 0;

    bool p_is_tuple = false;
    bool p_is_nested = false;

    bool read_until_dummy() {
        while (!p_fmt.empty()) {
            if (p_fmt.front() == '%') {
                p_fmt.pop_front();
                if (p_fmt.front() == '%') {
                    goto plain;
                }
                return read_spec();
            }
        plain:
            p_fmt.pop_front();
        }
        return false;
    }

    bool read_spec_range(bool tuple = false) {
        int sflags = p_flags;
        p_fmt.pop_front();
        string_range begin_inner(p_fmt);
        if (!read_until_dummy()) {
            p_is_nested = false;
            return false;
        }
        /* skip to the last spec in case multiple specs are present */
        string_range curfmt(p_fmt);
        while (read_until_dummy()) {
            curfmt = p_fmt;
        }
        p_fmt = curfmt;
        /* restore in case the inner spec read changed them */
        p_flags = sflags;
        /* find delimiter or ending */
        string_range begin_delim(p_fmt);
        string_range p = find(begin_delim, '%');
        char need = tuple ? '>' : ')';
        for (; !p.empty(); p = find(p, '%')) {
            p.pop_front();
            /* escape, skip */
            if (p.front() == '%') {
                p.pop_front();
                continue;
            }
            /* found end, in that case delimiter is after spec */
            if (p.front() == need) {
                p_is_tuple = tuple;
                if (tuple) {
                    p_nested = begin_inner.slice(0, &p[0] - &begin_inner[0] - 1);
                    p_nested_sep = nullptr;
                } else {
                    p_nested = begin_inner.slice(
                        0, &begin_delim[0] - &begin_inner[0]
                    );
                    p_nested_sep = begin_delim.slice(
                        0, &p[0] - &begin_delim[0] - 1
                    );
                }
                p.pop_front();
                p_fmt = p;
                p_is_nested = true;
                return true;
            }
            /* found actual delimiter start... */
            if ((p.front() == '|') && !tuple) {
                p_nested = begin_inner.slice(0, &p[0] - &begin_inner[0] - 1);
                p.pop_front();
                p_nested_sep = p;
                for (p = find(p, '%'); !p.empty(); p = find(p, '%')) {
                    p.pop_front();
                    if (p.front() == ')') {
                        p_nested_sep = p_nested_sep.slice(
                            0, &p[0] - &p_nested_sep[0] - 1
                        );
                        p.pop_front();
                        p_fmt = p;
                        p_is_nested = true;
                        return true;
                    }
                }
                p_is_nested = false;
                return false;
            }
        }
        p_is_nested = false;
        return false;
    }

    bool read_spec() {
        size_t ndig = detail::read_digits(p_fmt, p_buf);

        bool havepos = false;
        p_index = 0;
        /* parse index */
        if (p_fmt.front() == '$') {
            if (ndig <= 0) return false; /* no pos given */
            int idx = atoi(p_buf);
            if (idx <= 0 || idx > 255) return false; /* bad index */
            p_index = byte(idx);
            p_fmt.pop_front();
            havepos = true;
        }

        /* parse flags */
        p_flags = p_gflags;
        size_t skipd = 0;
        if (havepos || !ndig) {
            p_flags |= detail::parse_fmt_flags(p_fmt, 0);
        } else {
            for (size_t i = 0; i < ndig; ++i) {
                if (p_buf[i] != '0') {
                    break;
                }
                ++skipd;
            }
            if (skipd) {
                p_flags |= FMT_FLAG_ZERO;
            }
            if (skipd == ndig) {
                p_flags |= detail::parse_fmt_flags(p_fmt, p_flags);
            }
        }

        /* range/array/tuple formatting */
        if (
            ((p_fmt.front() == '(') || (p_fmt.front() == '<')) &&
            (havepos || !(ndig - skipd))
        ) {
            return read_spec_range(p_fmt.front() == '<');
        }

        /* parse width */
        p_width = 0;
        p_has_width = false;
        p_arg_width = false;
        if (!havepos && ndig && (ndig - skipd)) {
            p_width = atoi(p_buf + skipd);
            p_has_width = true;
        } else if (detail::read_digits(p_fmt, p_buf)) {
            p_width = atoi(p_buf);
            p_has_width = true;
        } else if (p_fmt.front() == '*') {
            p_arg_width = p_has_width = true;
            p_fmt.pop_front();
        }

        /* parse precision */
        p_precision = 0;
        p_has_precision = false;
        p_arg_precision = false;
        if (p_fmt.front() != '.') {
            goto fmtchar;
        }
        p_fmt.pop_front();

        if (detail::read_digits(p_fmt, p_buf)) {
            p_precision = atoi(p_buf);
            p_has_precision = true;
        } else if (p_fmt.front() == '*') {
            p_arg_precision = p_has_precision = true;
            p_fmt.pop_front();
        } else {
            return false;
        }

    fmtchar:
        p_spec = p_fmt.front();
        p_fmt.pop_front();
        return ((p_spec | 32) >= 'a') && ((p_spec | 32) <= 'z');
    }

    template<typename R>
    void write_spaces(R &writer, size_t n, bool left, char c = ' ') const {
        if (left == bool(p_flags & FMT_FLAG_DASH)) {
            return;
        }
        for (int w = p_width - int(n); --w >= 0; writer.put(c));
    }

    /* string base writer */
    template<typename R>
    void write_str(R &writer, bool escape, string_range val) const {
        size_t n = val.size();
        if (has_precision()) {
            n = std::min(n, size_t(precision()));
        }
        write_spaces(writer, n, true);
        if (escape) {
            writer.put('"');
            for (size_t i = 0; i < n; ++i) {
                if (val.empty()) {
                    break;
                }
                char c = val.front();
                char const *esc = detail::escape_fmt_char(c, '"');
                if (esc) {
                    range_put_all(writer, string_range{esc});
                } else {
                    writer.put(c);
                }
                val.pop_front();
            }
            writer.put('"');
        } else {
            range_put_all(writer, val.slice(0, n));
        }
        write_spaces(writer, n, false);
    }

    /* char values */
    template<typename R>
    void write_char(R &writer, bool escape, char val) const {
        if (escape) {
            char const *esc = detail::escape_fmt_char(val, '\'');
            if (esc) {
                char buf[6];
                buf[0] = '\'';
                size_t elen = strlen(esc);
                memcpy(buf + 1, esc, elen);
                buf[elen + 1] = '\'';
                write_val(writer, false, ostd::string_range{
                    buf, buf + elen + 2
                });
                return;
            }
        }
        write_spaces(writer, 1 + escape * 2, true);
        if (escape) {
            writer.put('\'');
            writer.put(val);
            writer.put('\'');
        } else {
            writer.put(val);
        }
        write_spaces(writer, 1 + escape * 2, false);
    }

    template<typename R, typename T>
    void write_int(R &writer, bool ptr, bool neg, T val) const {
        /* binary representation is the biggest, assume grouping */
        char buf[sizeof(T) * CHAR_BIT * 2];
        size_t n = 0;

        char isp = spec();
        if (isp == 's') {
            isp = (ptr ? 'x' : 'd');
        }
        byte specn = detail::fmt_specs[isp - 65];
        if (specn <= 2 || specn > 7) {
            throw format_error{"cannot format integers with the given spec"};
        }
        /* 32 for lowercase variants, 0 for uppercase */
        int cmask = ((isp >= 'a') << 5);

        int base = detail::fmt_bases[specn];
        bool zval = !val;
        if (zval) {
            buf[n++] = '0';
        }

        auto const &fac = std::use_facet<std::numpunct<char>>(p_loc);
        auto const &grp = fac.grouping();
        char tsep = fac.thousands_sep();
        auto grpp = reinterpret_cast<unsigned char const *>(grp.data());
        unsigned char grpn = *grpp;
        for (; val; val /= base) {
            if (!ptr && *grpp) {
                if (!grpn) {
                    buf[n++] = tsep;
                    if (*(grpp + 1)) {
                        ++grpp;
                    }
                    grpn = *grpp;
                }
                if (grpn) {
                    --grpn;
                }
            }
            T vb = val % base;
            buf[n++] = (vb + "70"[vb < 10]) | cmask;
        }
        size_t tn = n;
        if (has_precision()) {
            int prec = precision();
            if (size_t(prec) > tn) {
                tn = size_t(prec);
            } else if (!prec && zval) {
                tn = 0;
            }
        }

        int fl = flags();
        bool lsgn = fl & FMT_FLAG_PLUS;
        bool lsp  = fl & FMT_FLAG_SPACE;
        bool zero = fl & FMT_FLAG_ZERO;
        bool sign = neg + lsgn + lsp;

        char pfx = '\0';
        if (((fl & FMT_FLAG_HASH) || ptr) && ((specn == 3) || (specn == 6))) {
            pfx = ("XB"[(specn == 3)]) | cmask;
        }

        if (!zero) {
            write_spaces(writer, tn + (!!pfx * 2) + sign, true, ' ');
        }
        if (sign) {
            writer.put(neg ? '-' : *((" \0+") + lsgn * 2));
        }
        if (pfx) {
            writer.put('0');
            writer.put(pfx);
        }
        if (zero) {
            write_spaces(writer, tn + (!!pfx * 2) + sign, true, '0');
        }
        if (tn) {
            for (size_t i = 0; i < (tn - n); ++i) {
                writer.put('0');
            }
            for (size_t i = 0; i < n; ++i) {
                writer.put(buf[n - i - 1]);
            }
        }
        write_spaces(writer, tn + sign + (!!pfx * 2), false);
    }

    /* floating point */
    template<typename R, typename T>
    void write_float(R &writer, T val) const {
        char isp = spec();
        byte specn = detail::fmt_specs[isp - 65];
        if (specn != 1 && specn != 7) {
            throw format_error{"cannot format floats with the given spec"};
        }

        /* null streambuf because it's only used to read flags etc */
        std::ios st{nullptr};
        st.imbue(p_loc);

        st.width(width());
        st.precision(has_precision() ? precision() : 6);

        typename std::ios_base::fmtflags fl = 0;
        if (!(isp & 32)) {
            fl |= std::ios_base::uppercase;
        }
        /* equivalent of printf 'g' or 'G' by default */
        if ((isp | 32) == 'f') {
            fl |= std::ios_base::fixed;
        } else if ((isp | 32) == 'e') {
            fl |= std::ios_base::scientific;
        } else if ((isp | 32) == 'a') {
            fl |= std::ios_base::fixed | std::ios_base::scientific;
        }
        if (p_flags & FMT_FLAG_DASH) {
            fl |= std::ios_base::right;
        }
        if (p_flags & FMT_FLAG_PLUS) {
            fl |= std::ios_base::showpos;
        } else if ((p_flags & FMT_FLAG_SPACE) && !signbit(val)) {
            /* only if no sign is shown... num_put does not
             * support this so we have to do it on our own
             */
            writer.put(' ');
        }
        if (p_flags & FMT_FLAG_HASH) {
            fl |= std::ios_base::showpoint;
        }
        st.flags(fl);

        fmt_num_put<R> nump;
        nump.put(
            fmt_out<R>{&writer}, st, (p_flags & FMT_FLAG_ZERO) ? '0' : ' ', val
        );
    }

    template<typename R, typename T>
    void write_val(R &writer, bool escape, T const &val) const {
        /* stuff fhat can be custom-formatted goes first */
        if constexpr(detail::fmt_tofmt_test<T, noop_output_range<char>>) {
            format_traits<T>::to_format(val, writer, *this);
            return;
        }
        /* second best, we can convert to string slice */
        if constexpr(std::is_constructible_v<string_range, T const &>) {
            if (spec() != 's') {
                throw format_error{"strings need the '%s' spec"};
            }
            write_str(writer, escape, val);
            return;
        }
        /* tuples */
        if constexpr(detail::is_tuple_like<T>) {
            if (spec() != 's') {
                throw format_error{"ranges need the '%s' spec"};
            }
            writer.put('<');
            write_tuple_val<0, std::tuple_size<T>::value>(
                writer, escape, ", ", val
            );
            writer.put('>');
            return;
        }
        /* ranges */
        if constexpr(detail::iterable_test<T>) {
            if (spec() != 's') {
                throw format_error{"tuples need the '%s' spec"};
            }
            writer.put('{');
            write_range_val(writer, [&writer, escape, this](auto const &rval) {
                format_spec sp{'s', p_loc, escape ? FMT_FLAG_AT : 0};
                sp.write_arg(writer, 0, rval);
            }, ", ", val);
            writer.put('}');
            return;
        }
        /* bools, check if printing as string, otherwise convert to int */
        if constexpr(std::is_same_v<T, bool>) {
            if (spec() == 's') {
                write_val(writer, escape, ("false\0true") + (6 * val));
            } else {
                write_val(writer, escape, int(val));
            }
            return;
        }
        /* character values */
        if constexpr(std::is_same_v<T, char>) {
            if (spec() != 's' && spec() != 'c') {
                throw format_error{"cannot format chars with the given spec"};
            }
            write_char(writer, escape, val);
            return;
        }
        /* pointers, write as pointer with %s and otherwise as unsigned...
         * char pointers are handled by the string case above
         */
        if constexpr(std::is_pointer_v<T>) {
            write_int(writer, (spec() == 's'), false, size_t(val));
            return;
        }
        /* integers */
        if constexpr(std::is_integral_v<T>) {
            if constexpr(std::is_signed_v<T>) {
                /* signed integers */
                using UT = std::make_unsigned_t<T>;
                write_int(
                    writer, false, val < 0,
                    (val < 0) ? static_cast<UT>(-val) : static_cast<UT>(val)
                );
            } else {
                /* unsigned integers */
                write_int(writer, false, false, val);
            }
            return;
        }
        /* floats */
        if constexpr(std::is_floating_point_v<T>) {
            write_float(writer, val);
            return;
        }
        /* we ran out of options, failure */
        throw format_error{"the value cannot be formatted"};
    }

    /* actual writer */
    template<typename R, typename T, typename ...A>
    void write_arg(
        R &writer, size_t idx, T const &val, A const &...args
    ) const {
        if (idx) {
            if constexpr(!sizeof...(A)) {
                throw format_error{"not enough format arguments"};
            } else {
                write_arg(writer, idx - 1, args...);
            }
        } else {
            write_val(writer, p_flags & FMT_FLAG_AT, val);
        }
    }

    template<typename R, typename T>
    inline void write_range_item(
        R &writer, bool escape, bool expandval, string_range fmt, T const &item
    ) const {
        if constexpr(detail::is_tuple_like<T>) {
            if (expandval) {
                std::apply([&writer, escape, &fmt, this](
                    auto const &...args
                ) mutable {
                    format_spec sp{fmt, p_loc};
                    if (escape) {
                        sp.p_gflags |= FMT_FLAG_AT;
                    }
                    sp.write_fmt(writer, args...);
                }, item);
                return;
            }
        }
        format_spec sp{fmt, p_loc};
        if (escape) {
            sp.p_gflags |= FMT_FLAG_AT;
        }
        sp.write_fmt(writer, item);
    }

    template<typename R, typename F, typename T>
    void write_range_val(
        R &writer, F &&func, string_range sep, T const &val
    ) const {
        if constexpr(detail::iterable_test<T>) {
            auto range = ostd::iter(val);
            if (range.empty()) {
                return;
            }
            for (;;) {
                func(range.front());
                range.pop_front();
                if (range.empty()) {
                    break;
                }
                range_put_all(writer, sep);
            }
        } else {
            throw format_error{"invalid value for ranged format"};
        }
    }

    /* range writer */
    template<typename R, typename T, typename ...A>
    void write_range(
        R &writer, size_t idx, bool expandval, string_range sep,
        T const &val, A const &...args
    ) const {
        if (idx) {
            if constexpr(!sizeof...(A)) {
                throw format_error{"not enough format arguments"};
            } else {
                write_range(writer, idx - 1, expandval, sep, args...);
            }
        } else {
            write_range_val(writer, [
                this, &writer, escape = p_gflags & FMT_FLAG_AT, expandval,
                fmt = rest()
            ](auto const &rval) {
                this->write_range_item(
                    writer, escape, expandval, fmt, rval
                );
            }, sep, val);
        }
    }

    template<size_t I, size_t N, typename R, typename T>
    void write_tuple_val(
        R &writer, bool escape, string_range sep, T const &tup
    ) const {
        format_spec sp{'s', p_loc, escape ? FMT_FLAG_AT : 0};
        sp.write_arg(writer, 0, std::get<I>(tup));
        if constexpr(I < (N - 1)) {
            range_put_all(writer, sep);
            write_tuple_val<I + 1, N>(writer, escape, sep, tup);
        }
    }

    template<typename R, typename T, typename ...A>
    void write_tuple(
        R &writer, size_t idx, T const &val, A const &...args
    ) {
        if (idx) {
            if constexpr(!sizeof...(A)) {
                throw format_error{"not enough format arguments"};
            } else {
                write_tuple(writer, idx - 1, args...);
            }
        } else {
            if constexpr(detail::is_tuple_like<T>) {
                std::apply([this, &writer, &val](auto const &...vals) mutable {
                    this->write_fmt(writer, vals...);
                }, val);
            } else {
                throw format_error{"invalid value for tuple format"};
            }
        }
    }

    template<typename R, typename ...A>
    void write_fmt(R &writer, A const &...args) {
        size_t argidx = 1;
        while (read_until_spec(writer)) {
            size_t argpos = index();
            if (is_nested()) {
                if (!argpos) {
                    argpos = argidx++;
                } else if (argpos > argidx) {
                    argidx = argpos + 1;
                }
                format_spec nspec(nested(), p_loc);
                nspec.p_gflags |= (p_flags & FMT_FLAG_AT);
                if (is_tuple()) {
                    nspec.write_tuple(writer, argpos - 1, args...);
                } else {
                    nspec.write_range(
                        writer, argpos - 1, (flags() & FMT_FLAG_HASH),
                        nested_sep(), args...
                    );
                }
                continue;
            }
            if (!argpos) {
                argpos = argidx++;
                if (arg_width()) {
                    set_width(argpos - 1, args...);
                    argpos = argidx++;
                }
                if (arg_precision()) {
                    set_precision(argpos - 1, args...);
                    argpos = argidx++;
                }
            } else {
                bool argprec = arg_precision();
                if (argprec) {
                    if (argpos <= 1) {
                        throw format_error{"argument precision not given"};
                    }
                    set_precision(argpos - 2, args...);
                }
                if (arg_width()) {
                    if (argpos <= (size_t(argprec) + 1)) {
                        throw format_error{"argument width not given"};
                    }
                    set_width(argpos - 2 - argprec, args...);
                }
                if (argpos > argidx) {
                    argidx = argpos + 1;
                }
            }
            write_arg(writer, argpos - 1, args...);
        }
    }

    template<typename R, typename ...A>
    void write_fmt(R &writer) {
        if (read_until_spec(writer)) {
            throw format_error{"format spec without format arguments"};
        }
    }

    template<typename R>
    struct fmt_out {
        using iterator_category = std::output_iterator_tag;
        using value_type = char;
        using pointer = char *;
        using reference = char &;
        using difference_type = typename std::char_traits<char>::off_type;

        fmt_out &operator=(char c) {
            p_out->put(c);
            return *this;
        }

        fmt_out &operator*() { return *this; }
        fmt_out &operator++() { return *this; }
        fmt_out &operator++(int) { return *this; }

        R *p_out;
    };

    template<typename R>
    struct fmt_num_put final: std::num_put<char, fmt_out<R>> {
        fmt_num_put(size_t refs = 0): std::num_put<char, fmt_out<R>>(refs) {}
        ~fmt_num_put() {}
    };

    string_range p_fmt;
    std::locale p_loc;
    char p_buf[32];
};

/** @brief Formats into an output range using a format string and arguments.
 *
 * Uses the default constructed std::locale (the current global locale)
 * for locale specific formatting. There is also a version that takes an
 * explicit locale.
 *
 * This is just a simple wrapper, equivalent to:
 *
 * ~~~{.cc}
 *     return ostd::format_spec{fmt}.format(std::forward<R>(writer), args...);
 * ~~~
 */
template<typename R, typename ...A>
inline R &&format(R &&writer, string_range fmt, A const &...args) {
    return format_spec{fmt}.format(std::forward<R>(writer), args...);
}

/** @brief Formats into an output range using a format string and arguments.
 *
 * This version uses `loc` as a locale. There is also a version that uses
 * the global locale by default.
 *
 * This is just a simple wrapper, equivalent to:
 *
 * ~~~{.cc}
 *     return ostd::format_spec{fmt, loc}.format(std::forward<R>(writer), args...);
 * ~~~
 */
template<typename R, typename ...A>
inline R &&format(
    R &&writer, std::locale const &loc, string_range fmt, A const &...args
) {
    return format_spec{fmt, loc}.format(std::forward<R>(writer), args...);
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
