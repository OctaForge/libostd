/* Format strings for OctaSTD. Inspired by D's std.format module.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_FORMAT_HH
#define OSTD_FORMAT_HH

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <utility>
#include <stdexcept>

#include "ostd/algorithm.hh"
#include "ostd/string.hh"

namespace ostd {

enum format_flags {
    FMT_FLAG_DASH  = 1 << 0,
    FMT_FLAG_ZERO  = 1 << 1,
    FMT_FLAG_SPACE = 1 << 2,
    FMT_FLAG_PLUS  = 1 << 3,
    FMT_FLAG_HASH  = 1 << 4
};

struct format_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct format_spec;

template<
    typename T, typename R, typename = std::enable_if_t<std::is_same_v<
        decltype(std::declval<T const &>().to_format(
            std::declval<R &>(), std::declval<format_spec const &>()
        )), void
    >>
>
inline void to_format(T const &v, R &writer, format_spec const &fs) {
    v.to_format(writer, fs);
}

/* implementation helpers */
namespace detail {
    inline int parse_fmt_flags(string_range &fmt, int ret) {
        while (!fmt.empty()) {
            switch (fmt.front()) {
                case '-': ret |= FMT_FLAG_DASH; fmt.pop_front(); break;
                case '+': ret |= FMT_FLAG_PLUS; fmt.pop_front(); break;
                case '#': ret |= FMT_FLAG_HASH; fmt.pop_front(); break;
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

    static constexpr char fmt_digits[2][16] = {
        {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        },
        {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
        }
    };

    static constexpr char const *fmt_intpfx[2][4] = {
        { "0B", "0", "", "0X" },
        { "0b", "0", "", "0x" }
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

    /* test whether to_format works */
    template<typename T, typename R>
    static std::true_type test_tofmt(decltype(to_format(
        std::declval<T const &>(), std::declval<R &>(),
        std::declval<format_spec const &>()
    )) *);

    template<typename, typename>
    static std::false_type test_tofmt(...);

    template<typename T, typename R>
    constexpr bool fmt_tofmt_test = decltype(test_tofmt<T, R>(0))::value;
}

struct format_spec {
    format_spec(): p_nested_escape(false), p_fmt() {}
    format_spec(string_range fmt, bool escape = false):
        p_nested_escape(escape), p_fmt(fmt)
    {}

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

    bool is_nested() const { return p_is_nested; }
    bool nested_escape() const { return p_nested_escape; }

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

    int p_width = 0;
    int p_precision = 0;

    bool p_has_width = false;
    bool p_has_precision = false;

    bool p_arg_width = false;
    bool p_arg_precision = false;

    char p_spec = '\0';

    byte p_index = 0;

    bool p_is_nested = false;
    bool p_nested_escape = false;

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

    bool read_spec_range() {
        int sflags = p_flags;
        p_nested_escape = !(sflags & FMT_FLAG_DASH);
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
        p_flags = sflags;
        /* find delimiter or ending */
        string_range begin_delim(p_fmt);
        string_range p = find(begin_delim, '%');
        for (; !p.empty(); p = find(p, '%')) {
            p.pop_front();
            /* escape, skip */
            if (p.front() == '%') {
                p.pop_front();
                continue;
            }
            /* found end, in that case delimiter is after spec */
            if (p.front() == ')') {
                p_nested = begin_inner.slice(
                    0, &begin_delim[0] - &begin_inner[0]
                );
                p_nested_sep = begin_delim.slice(
                    0, &p[0] - &begin_delim[0] - 1
                );
                p.pop_front();
                p_fmt = p;
                p_is_nested = true;
                return true;
            }
            /* found actual delimiter start... */
            if (p.front() == '|') {
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
        p_flags = 0;
        size_t skipd = 0;
        if (havepos || !ndig) {
            p_flags = detail::parse_fmt_flags(p_fmt, 0);
        } else {
            for (size_t i = 0; i < ndig; ++i) {
                if (p_buf[i] != '0') {
                    break;
                }
                ++skipd;
            }
            if (skipd) {
                p_flags = FMT_FLAG_ZERO;
            }
            if (skipd == ndig) {
                p_flags = detail::parse_fmt_flags(p_fmt, p_flags);
            }
        }

        /* range/array formatting */
        if ((p_fmt.front() == '(') && (havepos || !(ndig - skipd))) {
            return read_spec_range();
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
        /* make sure we're testing on a signed byte - our mapping only
         * tests values up to 127 */
        sbyte sp = p_spec;
        return (sp >= 65) && (detail::fmt_specs[sp - 65] != 0);
    }

    template<typename R>
    void write_spaces(R &writer, size_t n, bool left, char c = ' ') const {
        if (left == bool(p_flags & FMT_FLAG_DASH)) {
            return;
        }
        for (int w = p_width - int(n); --w >= 0; writer.put(c));
    }

    template<typename R>
    void build_spec(R &out, string_range spec) const {
        out.put('%');
        if (p_flags & FMT_FLAG_DASH ) {
            out.put('-');
        }
        if (p_flags & FMT_FLAG_ZERO ) {
            out.put('0');
        }
        if (p_flags & FMT_FLAG_SPACE) {
            out.put(' ');
        }
        if (p_flags & FMT_FLAG_PLUS ) {
            out.put('+');
        }
        if (p_flags & FMT_FLAG_HASH ) {
            out.put('#');
        }
        range_put_all(out, string_range{"*.*"});
        range_put_all(out, spec);
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

        int base = detail::fmt_bases[specn];
        if (!val) {
            buf[n++] = '0';
        }
        for (; val; val /= base) {
            buf[n++] = detail::fmt_digits[isp >= 'a'][val % base];
        }

        int fl = flags();
        bool lsgn = fl & FMT_FLAG_PLUS;
        bool lsp  = fl & FMT_FLAG_SPACE;
        bool zero = fl & FMT_FLAG_ZERO;
        bool sign = neg + lsgn + lsp;

        char const *pfx = nullptr;
        size_t pfxlen = 0;
        if (((fl & FMT_FLAG_HASH) || ptr) && isp != 'd') {
            pfx = detail::fmt_intpfx[isp >= 'a'][specn - 3];
            pfxlen = !!pfx[1] + 1;
        }

        if (!zero) {
            write_spaces(writer, n + pfxlen + sign, true, ' ');
        }
        if (sign) {
            writer.put(neg ? '-' : *((" \0+") + lsgn * 2));
        }
        range_put_all(writer, string_range{pfx, pfx + pfxlen});
        if (zero) {
            write_spaces(writer, n + pfxlen + sign, true, '0');
        }

        for (int i = int(n - 1); i >= 0; --i) {
            writer.put(buf[i]);
        }
        write_spaces(writer, n + sign + pfxlen, false);
    }

    /* floating point */
    template<typename R, typename T, bool Long = std::is_same_v<T, ldouble>>
    void write_float(R &writer, T val) const {
        char buf[16], rbuf[128];
        char fmtspec[Long + 1];

        fmtspec[Long] = spec();
        byte specn = detail::fmt_specs[spec() - 65];
        if (specn != 1 && specn != 7) {
            throw format_error{"cannot format floats with the given spec"};
        }
        if (specn == 7) {
            fmtspec[Long] = 'g';
        }
        if (Long) {
            fmtspec[0] = 'L';
        }

        auto bufr = iter(buf);
        build_spec(bufr, fmtspec);
        bufr.put('\0');
        int ret = snprintf(
            rbuf, sizeof(rbuf), buf, width(),
            has_precision() ? precision() : 6, val
        );
        if (ret < 0) {
            /* typically unreachable, build_spec creates valid format */
            throw format_error{"invalid float format"};
        }

        char *dbuf = nullptr;
        if (size_t(ret) >= sizeof(rbuf)) {
            /* this should typically never happen */
            dbuf = new char[ret + 1];
            ret = snprintf(
                dbuf, ret + 1, buf, width(),
                has_precision() ? precision() : 6, val
            );
            if (ret < 0) {
                /* see above */
                throw format_error{"invalid float format"};
            }
            range_put_all(writer, string_range{dbuf, dbuf + ret});
            delete[] dbuf;
        } else {
            range_put_all(writer, string_range{rbuf, rbuf + ret});
        }
    }

    template<typename R, typename T>
    void write_val(R &writer, bool escape, T const &val) const {
        /* stuff fhat can be custom-formatted goes first */
        if constexpr(detail::fmt_tofmt_test<T, noop_output_range<char>>) {
            to_format(val, writer, *this);
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
            write_val(writer, p_nested_escape, val);
        }
    }

    template<typename R, typename T>
    inline void write_range_item(
        R &writer, bool expandval, string_range fmt, T const &item
    ) const {
        if constexpr(detail::is_tuple_like<T>) {
            if (expandval) {
                std::apply([&writer, esc = p_nested_escape, &fmt](
                    auto const &...args
                ) mutable {
                    format_spec sp{fmt, esc};
                    sp.write_fmt(writer, args...);
                }, item);
                return;
            }
        }
        format_spec sp{fmt, p_nested_escape};
        sp.write_fmt(writer, item);
    }

    template<typename R, typename T>
    void write_range_val(
        R &writer, bool expandval, string_range sep, T const &val
    ) const {
        if constexpr(detail::iterable_test<T>) {
            auto range = ostd::iter(val);
            if (range.empty()) {
                return;
            }
            for (;;) {
                write_range_item(writer, expandval, rest(), range.front());
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
            write_range_val(writer, expandval, sep, val);
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
                /* FIXME: figure out a better way */
                format_spec nspec(nested(), nested_escape());
                nspec.write_range(
                    writer, argpos - 1, (flags() & FMT_FLAG_HASH),
                    nested_sep(), args...
                );
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

    string_range p_fmt;
    char p_buf[32];
};

template<typename R, typename ...A>
inline R &&format(R &&writer, string_range fmt, A const &...args) {
    return format_spec{fmt}.format(std::forward<R>(writer), args...);
}

} /* namespace ostd */

#endif
