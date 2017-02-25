/* Format strings for OctaSTD. Inspired by D's std.format module.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
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

enum format_flags {
    FMT_FLAG_DASH  = 1 << 0,
    FMT_FLAG_ZERO  = 1 << 1,
    FMT_FLAG_SPACE = 1 << 2,
    FMT_FLAG_PLUS  = 1 << 3,
    FMT_FLAG_HASH  = 1 << 4,
    FMT_FLAG_AT    = 1 << 5
};

struct format_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct format_spec;

/* empty by default, SFINAE friendly */
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

struct format_spec {
    format_spec(): p_fmt() {}
    format_spec(string_range fmt): p_fmt(fmt) {}

    format_spec(char spec, int flags = 0):
        p_flags(flags),
        p_spec(spec)
    {}

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

    string_range rest() const {
        return p_fmt;
    }

    int width() const { return p_width; }
    int precision() const { return p_precision; }

    bool has_width() const { return p_has_width; }
    bool has_precision() const { return p_has_precision; }

    bool arg_width() const { return p_arg_width; }
    bool arg_precision() const { return p_arg_precision; }

    template<typename ...A>
    void set_width(size_t idx, A const &...args) {
        p_width = detail::get_arg_param(idx, args...);
        p_has_width = p_arg_width = true;
    }

    void set_width(int v) {
        p_width = v;
        p_has_width = true;
        p_arg_width = false;
    }

    template<typename ...A>
    void set_precision(size_t idx, A const &...args) {
        p_precision = detail::get_arg_param(idx, args...);
        p_has_precision = p_arg_precision = true;
    }

    void set_precision(int v) {
        p_precision = v;
        p_has_precision = true;
        p_arg_precision = false;
    }

    int flags() const { return p_flags; }

    char spec() const { return p_spec; }

    byte index() const { return p_index; }

    string_range nested() const { return p_nested; }
    string_range nested_sep() const { return p_nested_sep; }

    bool is_tuple() const { return p_is_tuple; }
    bool is_nested() const { return p_is_nested; }

    template<typename R, typename ...A>
    inline R &&format(R &&writer, A const &...args) {
        write_fmt(writer, args...);
        return std::forward<R>(writer);
    }

    template<typename R, typename T>
    inline R &&format_value(R &&writer, T const &val) {
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
        char buf[20];
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
        for (; val; val /= base) {
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
            write_range_val(writer, [&writer, escape](auto const &rval) {
                format_spec{'s', escape}.write_arg(writer, 0, rval);
            }, ", ", val);
            writer.put('}');
            return;
        }
        /* bools, check if printing as string, otherwise convert to int */
        if constexpr(std::is_same_v<T, bool>) {
            if (spec() == 's') {
                write_val(writer, ("false\0true") + (6 * val));
            } else {
                write_val(writer, int(val));
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
                std::apply([&writer, escape, &fmt](
                    auto const &...args
                ) mutable {
                    format_spec sp{fmt};
                    if (escape) {
                        sp.p_gflags |= FMT_FLAG_AT;
                    }
                    sp.write_fmt(writer, args...);
                }, item);
                return;
            }
        }
        format_spec sp{fmt};
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
        format_spec sp{'s', escape ? FMT_FLAG_AT : 0};
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
                }
                format_spec nspec(nested());
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
    char p_buf[32];
};

template<typename R, typename ...A>
inline R &&format(R &&writer, string_range fmt, A const &...args) {
    return format_spec{fmt}.format(std::forward<R>(writer), args...);
}

} /* namespace ostd */

#endif
