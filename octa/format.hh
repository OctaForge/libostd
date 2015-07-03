/* Format strings for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_FORMAT_HH
#define OCTA_FORMAT_HH

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "octa/algorithm.hh"
#include "octa/string.hh"

namespace octa {

enum FormatFlags {
    FMT_FLAG_DASH  = 1 << 0,
    FMT_FLAG_ZERO  = 1 << 1,
    FMT_FLAG_SPACE = 1 << 2,
    FMT_FLAG_PLUS  = 1 << 3,
    FMT_FLAG_HASH  = 1 << 4
};

namespace detail {
    static inline int parse_fmt_flags(const char *&fmt, int ret) {
        while (*fmt) {
            switch (*fmt) {
            case '-': ret |= FMT_FLAG_DASH; ++fmt; break;
            case '+': ret |= FMT_FLAG_PLUS; ++fmt; break;
            case '#': ret |= FMT_FLAG_HASH; ++fmt; break;
            case '0': ret |= FMT_FLAG_ZERO; ++fmt; break;
            case ' ': ret |= FMT_FLAG_SPACE; ++fmt; break;
            default: goto retflags;
            }
        }
    retflags:
        return ret;
    }

    static inline octa::Size read_digits(const char *&fmt, char *buf) {
        octa::Size ret = 0;
        for (; isdigit(*fmt); ++ret)
            *buf++ = *fmt++;
        *buf = '\0';
        return ret;
    }
}

namespace detail {
    static constexpr const bool fmt_specs[] = {
        /* uppercase spec set */
        1, 0, 0, 0, /* A B C D */
        1, 1, 1, 0, /* E F G H */
        0, 0, 0, 0, /* I J K L */
        0, 0, 0, 0, /* M N O P */
        0, 0, 0, 0, /* Q R S T */
        0, 0, 0, 1, /* U V W X */
        0, 0,       /* Y Z */

        /* ascii filler */
        0, 0, 0, 0, 0, 0,

        /* lowercase spec set */
        1, 1, 1, 1, /* a b c d */
        1, 1, 1, 0, /* e f g h */
        0, 0, 0, 0, /* i j k l */
        0, 0, 1, 0, /* m n o p */
        0, 0, 1, 0, /* q r s t */
        0, 0, 0, 1, /* u v w x */
        0, 0,       /* y z */

        /* ascii filler */
        0, 0, 0, 0, 0
    };
}

struct FormatSpec {
    FormatSpec(): p_fmt(nullptr) {}
    FormatSpec(const char *fmt): p_fmt(fmt) {}

    int width = 0;
    int precision = 0;

    bool has_width = false;
    bool has_precision = false;

    bool arg_width = false;
    bool arg_precision = false;

    int flags = 0;

    char spec = '\0';

    octa::byte index = 0;

    template<typename R>
    bool read_until_spec(R &writer, octa::Size *wret) {
        octa::Size written = 0;
        if (!p_fmt) return false;
        while (*p_fmt) {
            if (*p_fmt == '%') {
                ++p_fmt;
                if (*p_fmt == '%') goto plain;
                bool r = read_spec();
                if (wret) *wret = written;
                return r;
            }
        plain:
            ++written;
            writer.put(*p_fmt++);
        }
        if (wret) *wret = 0;
        return false;
    }

    template<typename R>
    void write_ws(R &writer, octa::Size n, bool left) const {
        if (left == bool(flags & FMT_FLAG_DASH)) return;
        for (int w = width - int(n); --w >= 0; writer.put(' '));
    }

    const char *rest() const {
        return p_fmt;
    }

private:

    bool read_spec() {
        octa::Size ndig = octa::detail::read_digits(p_fmt, p_buf);

        bool havepos = false;
        index = 0;
        /* parse index */
        if (*p_fmt == '$') {
            if (ndig <= 0) return false; /* no pos given */
            int idx = atoi(p_buf);
            if (idx <= 0 || idx > 255) return false; /* bad index */
            index = octa::byte(idx);
            ++p_fmt;
            havepos = true;
        }

        /* parse flags */
        flags = 0;
        octa::Size skipd = 0;
        if (havepos || !ndig) {
            flags = octa::detail::parse_fmt_flags(p_fmt, 0);
        } else {
            for (octa::Size i = 0; i < ndig; ++i) {
                if (p_buf[i] != '0') break;
                ++skipd;
            }
            if (skipd) flags = FMT_FLAG_ZERO;
            if (skipd == ndig)
                flags = octa::detail::parse_fmt_flags(p_fmt, flags);
        }

        /* parse width */
        width = 0;
        has_width = false;
        arg_width = false;
        if (!havepos && ndig && (ndig - skipd)) {
            width = atoi(p_buf + skipd);
            has_width = true;
        } else if (octa::detail::read_digits(p_fmt, p_buf)) {
            width = atoi(p_buf);
            has_width = true;
        } else if (*p_fmt == '*') {
            arg_width = true;
            ++p_fmt;
        }

        /* parse precision */
        precision = 0;
        has_precision = false;
        arg_precision = false;
        if (*p_fmt != '.') goto fmtchar;
        ++p_fmt;

        if (octa::detail::read_digits(p_fmt, p_buf)) {
            precision = atoi(p_buf);
            has_precision = true;
        } else if (*p_fmt == '*') {
            arg_precision = true;
            ++p_fmt;
        } else return false;

    fmtchar:
        spec = *p_fmt++;
        /* make sure we're testing on a signed byte - our mapping only
         * tests values up to 127 */
        octa::sbyte sp = spec;
        return (sp >= 65) && octa::detail::fmt_specs[sp - 65];
    }

    const char *p_fmt;
    char p_buf[32];
};

namespace detail {
    /* C string */

    template<typename R>
    static inline octa::Size write_val(R &writer, const FormatSpec &fl,
                                       const char *val) {
        octa::Size n = strlen(val);
        fl.write_ws(writer, n, true);
        writer.put_n(val, n);
        fl.write_ws(writer, n, false);
        return n;
    }

    /* OctaSTD string */

    template<typename R, typename A>
    static inline octa::Size
    write_val(R &writer, const FormatSpec &fl, const octa::AnyString<A> &val) {
        octa::Size n = val.size();
        fl.write_ws(writer, n, true);
        writer.put_n(val.data(), n);
        fl.write_ws(writer, n, false);
        return n;
    }

    /* A character */

    template<typename R>
    static inline octa::Size write_val(R &writer, const FormatSpec &fl,
                                       char val) {
        fl.write_ws(writer, 1, true);
        writer.put(val);
        fl.write_ws(writer, 1, false);
        return 1;
    }

    /* boolean */

    template<typename R>
    static inline octa::Size write_val(R &writer, const FormatSpec &fl,
                                       bool val) {
        return write_val(writer, fl, ("false\0true") + (6 * val));
    }

    /* Integer helpers */

    template<typename R, typename T>
    static inline octa::Size write_u(R &writer, const FormatSpec &fl,
                                     bool neg, T val) {
        char buf[20];
        octa::Size n = 0;
        for (; val; val /= 10) buf[n++] = '0' + val % 10;
        bool lsgn = !!(fl.flags & FMT_FLAG_PLUS);
        bool lsp = !!(fl.flags & FMT_FLAG_SPACE);
        bool sign = !!(neg + lsgn + lsp);
        fl.write_ws(writer, n + sign, true);
        if (sign) writer.put(neg ? '-' : *((" \0+") + lsgn * 2));
        for (int i = int(n - 1); i >= 0; --i) {
            writer.put(buf[i]);
        }
        fl.write_ws(writer, n + sign, false);
        return n + sign;
    }

    template<typename R, typename T, bool = octa::IsIntegral<T>::value,
                                     bool = octa::IsSigned<T>::value>
    struct WriteIntVal {
        static inline octa::Size write(R &writer, const FormatSpec &fl,
                                       const T &val) {
            return octa::detail::write_val(writer, fl,
                                           octa::to_string(val));
        }
    };

    template<typename R, typename T> struct WriteIntVal<R, T, true, true> {
        static inline octa::Size write(R &writer, const FormatSpec &fl, T val) {
            using UT = octa::MakeUnsigned<T>;
            return octa::detail::write_u(writer, fl, val < 0,
                (val < 0) ? UT(-val) : UT(val));
        }
    };

    template<typename R, typename T> struct WriteIntVal<R, T, true, false> {
        static inline octa::Size write(R &writer, const FormatSpec &fl, T val) {
            return octa::detail::write_u(writer, fl, false, val);
        }
    };

    /* Integers or generic value */

    template<typename R, typename T>
    static inline octa::Size write_val(R &writer, const FormatSpec &fl,
                                       const T &val) {
        return octa::detail::WriteIntVal<R, T>::write(writer, fl, val);
    }

    /* Writer entry point */

    template<typename R, typename T>
    static inline octa::Size write_idx(R &writer, octa::Size idx,
                                       const FormatSpec &fl, const T &v) {
        if (idx) {
            assert(false && "not enough format args");
            return -1;
        }
        return octa::detail::write_val(writer, fl, v);
    }

    template<typename R, typename T, typename ...A>
    static inline octa::Size write_idx(R &writer, octa::Size idx,
                                       const FormatSpec &fl, const T &v,
                                       const A &...args) {
        if (idx) return octa::detail::write_idx(writer, idx - 1, fl,
            args...);
        return octa::detail::write_val(writer, fl, v);
    }

    template<typename T>
    static inline int conv_idx_num(const T &v, octa::EnableIf<
        octa::IsIntegral<T>::value, bool
    > = true) {
        return int(v);
    }

    template<typename T>
    static inline int conv_idx_num(const T &, octa::EnableIf<
        !octa::IsIntegral<T>::value, bool
    > = true) {
        assert(false && "argument is not an integer");
        return -2;
    }

    template<typename T>
    static inline int read_idx_num(octa::Size idx, const T &v) {
        if (idx) {
            assert(false && "not enough format args");
            return -2;
        }
        return conv_idx_num(v);
    }

    template<typename T, typename ...A>
    static inline int read_idx_num(octa::Size idx, const T &v, const A &...args) {
        if (idx) return octa::detail::read_idx_num(idx - 1, args...);
        return conv_idx_num(v);
    }

} /* namespace detail */

template<typename R, typename ...A>
static inline octa::Size formatted_write(R writer, octa::Size &fmtn,
                                         const char *fmt, const A &...args) {
    octa::Size argidx = 1, written = 0, retn = 0, twr = 0;
    FormatSpec spec(fmt);
    while (spec.read_until_spec(writer, &twr)) {
        written += twr;
        octa::Size argpos = spec.index;
        if (!argpos) argpos = argidx++;
        octa::Size wl = octa::detail::write_idx(writer, argpos - 1, spec, args...);
        written += octa::max(spec.width, int(wl));
    }
    fmtn = retn;
    return written;
}

template<typename R, typename AL, typename ...A>
octa::Size formatted_write(R writer, octa::Size &fmtn,
                           const octa::AnyString<AL> &fmt,
                           const A &...args) {
    return formatted_write(writer, fmtn, fmt.data(), args...);
}

template<typename R, typename ...A>
octa::Size formatted_write(R writer, const char *fmt, const A &...args) {
    octa::Size fmtn = 0;
    return formatted_write(writer, fmtn, fmt, args...);
}

template<typename R, typename AL, typename ...A>
octa::Size formatted_write(R writer, const octa::AnyString<AL> &fmt,
                           const A &...args) {
    octa::Size fmtn = 0;
    return formatted_write(writer, fmtn, fmt.data(), args...);
}

} /* namespace octa */

#endif