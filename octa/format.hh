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
    static constexpr const octa::byte fmt_specs[] = {
        /* uppercase spec set */
        2, 0, 0, 0, /* A B C D */
        2, 2, 2, 0, /* E F G H */
        0, 0, 0, 0, /* I J K L */
        0, 0, 0, 0, /* M N O P */
        0, 0, 0, 0, /* Q R S T */
        0, 0, 0, 1, /* U V W X */
        0, 0,       /* Y Z */

        /* ascii filler */
        0, 0, 0, 0, 0, 0,

        /* lowercase spec set */
        2, 1, 1, 1, /* a b c d */
        2, 2, 2, 0, /* e f g h */
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
        if (wret) *wret = written;
        return false;
    }

    template<typename R>
    octa::Size write_ws(R &writer, octa::Size n, bool left) const {
        if (left == bool(flags & FMT_FLAG_DASH)) return 0;
        int r = width - int(n);
        for (int w = width - int(n); --w >= 0; writer.put(' '));
        if (r < 0) return 0;
        return r;
    }

    const char *rest() const {
        return p_fmt;
    }

    void build_spec(char *buf, const char *spec, octa::Size specn) {
        *buf++ = '%';
        if (flags & FMT_FLAG_DASH ) *buf++ = '-';
        if (flags & FMT_FLAG_ZERO ) *buf++ = '0';
        if (flags & FMT_FLAG_SPACE) *buf++ = ' ';
        if (flags & FMT_FLAG_PLUS ) *buf++ = '+';
        if (flags & FMT_FLAG_HASH ) *buf++ = '#';
        memcpy(buf, "*.*", 3);
        memcpy(buf + 3, spec, specn);
        *(buf += specn + 3) = '\0';
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
        return (sp >= 65) && (octa::detail::fmt_specs[sp - 65] != 0);
    }

    const char *p_fmt;
    char p_buf[32];
};

namespace detail {
    template<typename R, typename T>
    static inline octa::Size write_u(R &writer, const FormatSpec *fl,
                                     bool neg, T val) {
        char buf[20];
        octa::Size n = 0;
        octa::Size nws = 0;
        for (; val; val /= 10) buf[n++] = '0' + val % 10;
        bool lsgn = !!(fl->flags & FMT_FLAG_PLUS);
        bool lsp = !!(fl->flags & FMT_FLAG_SPACE);
        bool sign = !!(neg + lsgn + lsp);
        nws += fl->write_ws(writer, n + sign, true);
        if (sign) writer.put(neg ? '-' : *((" \0+") + lsgn * 2));
        for (int i = int(n - 1); i >= 0; --i) {
            writer.put(buf[i]);
        }
        nws += fl->write_ws(writer, n + sign, false);
        return n + sign + nws;
    }

    struct WriteSpec: octa::FormatSpec {
        WriteSpec(): octa::FormatSpec() {}
        WriteSpec(const char *fmt): octa::FormatSpec(fmt) {}

        /* C string */
        template<typename R>
        octa::Ptrdiff write(R &writer, const char *val) {
            octa::Size n = strlen(val);
            octa::Ptrdiff r = n;
            r += this->write_ws(writer, n, true);
            writer.put_n(val, n);
            r += this->write_ws(writer, n, false);
            return r;
        }

        /* OctaSTD string */
        template<typename R, typename A>
        octa::Ptrdiff write(R &writer, const octa::AnyString<A> &val) {
            octa::Size n = val.size();
            octa::Ptrdiff r = n;
            r += this->write_ws(writer, n, true);
            writer.put_n(val.data(), n);
            r += this->write_ws(writer, n, false);
            return r;
        }

        /* character */
        template<typename R>
        octa::Ptrdiff write(R &writer, char val) {
            octa::Ptrdiff r = 1;
            r += this->write_ws(writer, 1, true);
            writer.put(val);
            r += this->write_ws(writer, 1, false);
            return r;
        }

        /* bool */
        template<typename R>
        octa::Ptrdiff write(R &writer, bool val) {
            return write(writer, ("false\0true") + (6 * val));
        }

        /* signed integers */
        template<typename R, typename T>
        octa::Ptrdiff write(R &writer, T val, octa::EnableIf<
            octa::IsIntegral<T>::value && octa::IsSigned<T>::value, bool
        > = true) {
            using UT = octa::MakeUnsigned<T>;
            return octa::detail::write_u(writer, this, val < 0,
                (val < 0) ? UT(-val) : UT(val));
        }

        /* unsigned integers */
        template<typename R, typename T>
        octa::Ptrdiff write(R &writer, T val, octa::EnableIf<
            octa::IsIntegral<T>::value && octa::IsUnsigned<T>::value, bool
        > = true) {
            return octa::detail::write_u(writer, this, false, val);
        }

        template<typename R, typename T,
            bool Long = octa::IsSame<T, octa::ldouble>::value
        > octa::Ptrdiff write(R &writer, T val, octa::EnableIf<
            octa::IsFloatingPoint<T>::value, bool
        > = true) {
            char buf[16], rbuf[128];
            char fmtspec[Long + 1];
            if (octa::detail::fmt_specs[this->spec - 65] > 1)
                fmtspec[Long] = this->spec;
            else
                fmtspec[Long] = 'g';
            if (Long) fmtspec[0] = 'L';
            this->build_spec(buf, fmtspec, sizeof(fmtspec));
            octa::Ptrdiff ret = snprintf(rbuf, sizeof(rbuf), buf, this->width,
                this->has_precision ? this->precision : 6, val);
            char *dbuf = nullptr;
            if (octa::Size(ret) >= sizeof(rbuf)) {
                /* this should typically never happen */
                dbuf = (char *)malloc(ret + 1);
                ret = snprintf(dbuf, ret + 1, buf, this->width,
                    this->has_precision ? this->precision : 6, val);
                writer.put_n(dbuf, ret);
                free(dbuf);
            } else writer.put_n(rbuf, ret);
            return ret;
        }

        /* generic value */
        template<typename R, typename T>
        octa::Ptrdiff write(R &writer, const T &val, octa::EnableIf<
            !octa::IsArithmetic<T>::value, bool
        > = true) {
            return write(writer, octa::to_string(val));
        }

        /* actual writer */
        template<typename R, typename T>
        octa::Ptrdiff write_arg(R &writer, octa::Size idx, const T &val) {
            if (idx) {
                assert(false && "not enough format args");
                return -1;
            }
            return write(writer, val);
        }

        template<typename R, typename T, typename ...A>
        octa::Ptrdiff write_arg(R &writer, octa::Size idx, const T &val,
                             const A &...args) {
            if (idx) return write_arg(writer, idx - 1, args...);
            return write(writer, val);
        }
    };
} /* namespace detail */

template<typename R, typename ...A>
static inline octa::Ptrdiff formatted_write(R writer, octa::Size &fmtn,
                                            const char *fmt, const A &...args) {
    octa::Size argidx = 1, retn = 0, twr = 0;
    octa::Ptrdiff written = 0;
    octa::detail::WriteSpec spec(fmt);
    while (spec.read_until_spec(writer, &twr)) {
        written += twr;
        octa::Size argpos = spec.index;
        if (!argpos) argpos = argidx++;
        octa::Ptrdiff sw = spec.write_arg(writer, argpos - 1, args...);
        if (sw < 0) return sw;
        written += sw;
    }
    written += twr;
    fmtn = retn;
    return written;
}

template<typename R, typename ...A>
static inline octa::Ptrdiff formatted_write(R writer, octa::Size &fmtn,
                                            const char *fmt) {
    octa::Size written = 0;
    octa::detail::WriteSpec spec(fmt);
    if (spec.read_until_spec(writer, &written)) return -1;
    fmtn = 0;
    return written;
}

template<typename R, typename AL, typename ...A>
octa::Ptrdiff formatted_write(R writer, octa::Size &fmtn,
                              const octa::AnyString<AL> &fmt,
                              const A &...args) {
    return formatted_write(writer, fmtn, fmt.data(), args...);
}

template<typename R, typename ...A>
octa::Ptrdiff formatted_write(R writer, const char *fmt, const A &...args) {
    octa::Size fmtn = 0;
    return formatted_write(writer, fmtn, fmt, args...);
}

template<typename R, typename AL, typename ...A>
octa::Ptrdiff formatted_write(R writer, const octa::AnyString<AL> &fmt,
                              const A &...args) {
    octa::Size fmtn = 0;
    return formatted_write(writer, fmtn, fmt.data(), args...);
}

} /* namespace octa */

#endif