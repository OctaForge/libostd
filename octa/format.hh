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

namespace detail {
    enum FormatFlags {
        FMT_LEFT_JUSTIFY  = 1 << 0,
        FMT_LEADING_SIGN  = 1 << 1,
        FMT_FORMAT_NUMBER = 1 << 2,
        FMT_LEADING_ZEROS = 1 << 3,
        FMT_LEADING_SPACE = 1 << 4
    };

    struct ArgParams {
        int flags;
        int width;
        int precision;

        template<typename R>
        octa::Size write_ws(R &writer, octa::Size n, bool left) const {
            if (left == bool(flags & FMT_LEFT_JUSTIFY)) return 0;
            int w = (int(n) > width) ? 0 : (width - int(n));
            octa::Size ret = w;
            for (; w--; writer.put(' '));
            return ret;
        }
    };

    /* C string */

    template<typename R>
    static inline octa::Size write_val(R &writer, const ArgParams &fl,
                                       const char *val) {
        octa::Size n = strlen(val);
        octa::Size r = n;
        r += fl.write_ws(writer, n, true);
        while (*val) writer.put(*val++);
        r += fl.write_ws(writer, n, false);
        return r;
    }

    /* OctaSTD string */

    template<typename R, typename A>
    static inline octa::Size
    write_val(R &writer, const ArgParams &fl, const octa::AnyString<A> &val) {
        octa::Size n = val.size();
        octa::Size r = n;
        r += fl.write_ws(writer, n, true);
        for (octa::Size i = 0; i < n; ++i) writer.put(val[i]);
        r += fl.write_ws(writer, n, false);
        return n;
    }

    /* A character */

    template<typename R>
    static inline octa::Size write_val(R &writer, const ArgParams &fl,
                                       char val) {
        octa::Size r = 1;
        r += fl.write_ws(writer, 1, true);
        writer.put(val);
        r += fl.write_ws(writer, 1, false);
        return r;
    }

    /* boolean */

    template<typename R>
    static inline octa::Size write_val(R &writer, const ArgParams &fl,
                                       bool val) {
        write_val(writer, fl, val ? "true" : "false");
        return 1;
    }

    /* Integer helpers */

    template<typename R>
    static inline octa::Size write_u(R &writer, const ArgParams &,
                                     octa::Uintmax val) {
        char buf[20];
        octa::Size n = 0;
        for (; val; val /= 10) buf[n++] = '0' + val % 10;
        for (int i = int(n - 1); i >= 0; --i) {
            writer.put(buf[i]);
        }
        return n;
    }

    template<typename R>
    static inline octa::Size write_i(R &writer, const ArgParams &fl,
                                     octa::Intmax val) {
        octa::Uintmax uv;
        int neg = 0;
        if (val < 0) {
            uv = -val;
            writer.put('-');
            neg = 1;
        } else uv = val;
        return octa::detail::write_u(writer, fl, uv) + neg;
    }

    template<typename R, typename T, bool = octa::IsIntegral<T>::value,
                                     bool = octa::IsSigned<T>::value>
    struct WriteIntVal {
        static inline octa::Size write(R &writer, const ArgParams &fl,
                                       const T &val) {
            return octa::detail::write_val(writer, fl,
                                           octa::to_string(val));
        }
    };

    template<typename R, typename T> struct WriteIntVal<R, T, true, true> {
        static inline octa::Size write(R &writer, const ArgParams &fl, T val) {
            return octa::detail::write_i(writer, fl, val);
        }
    };

    template<typename R, typename T> struct WriteIntVal<R, T, true, false> {
        static inline octa::Size write(R &writer, const ArgParams &fl, T val) {
            return octa::detail::write_u(writer, fl, val);
        }
    };

    /* Integers or generic value */

    template<typename R, typename T>
    static inline octa::Size write_val(R &writer, const ArgParams &fl,
                                       const T &val) {
        return octa::detail::WriteIntVal<R, T>::write(writer, fl, val);
    }

    /* Writer entry point */

    template<typename R, typename T>
    static inline octa::Size write_idx(R &writer, octa::Size idx,
                                       const ArgParams &fl, const T &v) {
        if (idx) assert(false && "not enough format args");
        return octa::detail::write_val(writer, fl, v);
    }

    template<typename R, typename T, typename ...A>
    static inline octa::Size write_idx(R &writer, octa::Size idx,
                                       const ArgParams &fl, const T &v,
                                       const A &...args) {
        if (idx) return octa::detail::write_idx(writer, idx - 1, fl,
            args...);
        return octa::detail::write_val(writer, fl, v);
    }

    static inline int parse_fmt_flags(const char *&fmt, int ret) {
        while (*fmt) {
            switch (*fmt) {
            case '-': ret |= FMT_LEFT_JUSTIFY; ++fmt; break;
            case '+': ret |= FMT_LEADING_SIGN; ++fmt; break;
            case '#': ret |= FMT_FORMAT_NUMBER; ++fmt; break;
            case '0': ret |= FMT_LEADING_ZEROS; ++fmt; break;
            case ' ': ret |= FMT_LEADING_SPACE; ++fmt; break;
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
        return 0;
    }

    template<typename T>
    static inline int read_idx_num(octa::Size idx, const T &v) {
        if (idx) assert(false && "not enough format args");
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
    char buf[32];
    octa::Size argidx = 0, written = 0, retn = 0;
    while (*fmt) {
        if (*fmt == '%') {
            ++fmt;
            if (*fmt == '%') {
                goto plain;
            }
            /* read digits: can be position, flags or width */
            octa::Size ndig = octa::detail::read_digits(fmt, buf);

            /* try if it's arg position */
            octa::Size argpos = argidx;
            bool havepos = false;
            if (*fmt == '$') {
                assert((ndig > 0) && "no arg position given");
                if (ndig > 0) argpos = atoi(buf);
                ++fmt;
                havepos = true;
            } else ++argidx;

            /* try and parse flags */
            int flags = 0;
            octa::Size skipd = 0;
            if (havepos || !ndig) {
                flags = octa::detail::parse_fmt_flags(fmt, 0);
            } else {
                for (octa::Size i = 0; i < ndig; ++i) {
                    if (buf[i] != '0') break;
                    ++skipd;
                }
                if (skipd) flags = octa::detail::FMT_LEADING_ZEROS;
                if (skipd == ndig)
                    flags = octa::detail::parse_fmt_flags(fmt, flags);
            }

            /* try and parse width */
            int width = 0;
            if (!havepos && ndig && (ndig - skipd))
                width = atoi(buf + skipd);
            else if (octa::detail::read_digits(fmt, buf))
                width = atoi(buf);
            else if (*fmt == '*') {
                width = -1;
                ++fmt;
            }

            /* precision */
            int precision = 0;
            if (*fmt != '.') goto fmtchar;
            ++fmt;
            if (octa::detail::read_digits(fmt, buf))
                precision = atoi(buf);
            else if (*fmt == '*') {
                precision = -1;
                ++fmt;
            } else assert(false && "no precision given");

            /* expand precision and width */
            if (width < 0 || precision < 0) {
                bool both = width < 0 && precision < 0;
                if (havepos) {
                    if (argpos < (both ? 2 : 1)) {
                        if (width < 0) width = 0;
                        if (precision < 0) precision = 0;
                        assert(false && "invalid width argument");
                    } else if (width < 0) {
                        if (precision < 0) {
                            width = octa::detail::read_idx_num(
                                argpos - 2, args...);
                            precision =octa::detail::read_idx_num(
                                argpos - 1, args...);
                        } else width = octa::detail::read_idx_num(
                            argpos - 1, args...);
                    } else precision = octa::detail::read_idx_num(
                        argpos - 1, args...);
                } else {
                    if (width < 0) {
                        width = octa::detail::read_idx_num(argpos++,
                            args...);
                        ++argidx;
                    }
                    if (precision < 0) {
                        precision = octa::detail::read_idx_num(argpos++,
                            args...);
                        ++argidx;
                    }
                }
            }

        fmtchar:
            /* parse needed position */
            assert((*fmt == 's') && "malformed format string");
            ++fmt;
            retn = octa::max(argpos, retn);
            octa::detail::ArgParams afl = { flags, width, precision };
            written += octa::detail::write_idx(writer, argpos, afl, args...);
            continue;
        }
    plain:
        writer.put(*fmt++);
        ++written;
    }
    fmtn = retn + 1;
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