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
    /* C string */

    template<typename R>
    static inline octa::Size write_val(R &writer, const char *val) {
        octa::Size n = 0;
        while (*val) {
            writer.put(*val++);
            ++n;
        }
        return n;
    }

    /* OctaSTD string */

    template<typename R>
    static inline octa::Size write_val(R &writer, const octa::String &val) {
        for (octa::Size i = 0; i < val.size(); ++i) writer.put(val[i]);
        return val.size();
    }

    /* A character */

    template<typename R>
    static inline octa::Size write_val(R &writer, char val) {
        writer.put(val);
        return 1;
    }

    /* Integer helpers */

    template<typename R>
    static inline octa::Size write_u(R &writer, octa::Uintmax val) {
        char buf[20];
        octa::Size n = 0;
        for (; val; val /= 10) buf[n++] = '0' + val % 10;
        for (int i = int(n - 1); i >= 0; --i) {
            writer.put(buf[i]);
        }
        return n;
    }

    template<typename R>
    static inline octa::Size write_i(R &writer, octa::Intmax val) {
        octa::Uintmax uv;
        int neg = 0;
        if (val < 0) {
            uv = -val;
            writer.put('-');
            neg = 1;
        } else uv = val;
        return octa::detail::write_u(writer, uv) + neg;
    }

    template<typename R, typename T, bool = octa::IsIntegral<T>::value,
                                     bool = octa::IsSigned<T>::value>
    struct WriteIntVal {
        static inline octa::Size write(R &writer, const T &val) {
            return octa::detail::write_val(writer, octa::to_string(val));
        }
    };

    template<typename R, typename T> struct WriteIntVal<R, T, true, true> {
        static inline octa::Size write(R &writer, T val) {
            return octa::detail::write_i(writer, val);
        }
    };

    template<typename R, typename T> struct WriteIntVal<R, T, true, false> {
        static inline octa::Size write(R &writer, T val) {
            return octa::detail::write_u(writer, val);
        }
    };

    /* Integers or generic value */

    template<typename R, typename T>
    static inline octa::Size write_val(R &writer, const T &val) {
        return octa::detail::WriteIntVal<R, T>::write(writer, val);
    }

    /* Writer entry point */

    template<typename R, typename T, typename ...A>
    static inline octa::Size write_idx(R &writer, octa::Size idx, const T &v) {
        if (idx) assert(false && "not enough format args");
        return octa::detail::write_val(writer, v);
    }

    template<typename R, typename T, typename ...A>
    static inline octa::Size write_idx(R &writer, octa::Size idx, const T &v,
                                       const A &...args) {
        if (idx)
            return octa::detail::write_idx(writer, idx - 1, args...);
        return octa::detail::write_val(writer, v);
    }
} /* namespace detail */

template<typename R, typename ...A>
static inline octa::Size formatted_write(R writer, octa::Size &fmtn,
                                         const char *fmt, const A &...args) {
    octa::Size argidx = 0, written = 0, retn = 0;
    while (*fmt) {
        if (*fmt == '{') {
            octa::Size needidx = argidx;
            if (*++fmt != '}') {
                char idx[8] = { '\0' };
                for (octa::Size i = 0; isdigit(*fmt); ++i)
                    idx[i] = *fmt++;
                assert((*fmt == '}') && "malformed format string"); 
                needidx = atoi(idx);
            } else ++argidx;
            ++fmt;
            retn = octa::max(needidx, retn);
            written += octa::detail::write_idx(writer, needidx,
                args...);
            continue;
        }
        writer.put(*fmt++);
        ++written;
    }
    fmtn = retn + 1;
    return written;
}

template<typename R, typename ...A>
octa::Size formatted_write(R writer, octa::Size &fmtn, const octa::String &fmt,
                           const A &...args) {
    return formatted_write(writer, fmtn, fmt.data(), args...);
}

template<typename R, typename ...A>
octa::Size formatted_write(R writer, const char *fmt, const A &...args) {
    octa::Size fmtn = 0;
    return formatted_write(writer, fmtn, fmt, args...);
}

template<typename R, typename ...A>
octa::Size formatted_write(R writer, const octa::String &fmt,
                           const A &...args) {
    octa::Size fmtn = 0;
    return formatted_write(writer, fmtn, fmt.data(), args...);
}

} /* namespace octa */

#endif