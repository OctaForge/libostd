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
    template<typename R>
    static inline void write_val(R &writer, const char *val) {
        while (*val) writer.put(*val++);
    }

    template<typename R>
    static inline void write_val(R &writer, const octa::String &val) {
        for (octa::Size i = 0; i < val.size(); ++i) writer.put(val[i]);
    }

    template<typename R>
    static inline void write_val(R &writer, char val) {
        writer.put(val);
    }

    template<typename R, typename T>
    static inline void write_val(R &writer, const T &val) {
        write_val(writer, octa::to_string(val));
    }

    template<typename R, typename T, typename ...A>
    static inline void write_idx(R &writer, octa::Size idx, const T &v) {
        if (idx) assert(false && "not enough format args");
        write_val(writer, v);
    }

    template<typename R, typename T, typename ...A>
    static inline void write_idx(R &writer, octa::Size idx, const T &v,
                                 A &&...args) {
        if (idx) {
            write_idx(writer, idx - 1, octa::forward<A>(args)...);
            return;
        }
        write_val(writer, v);
    }
} /* namespace detail */

template<typename R, typename ...A>
static inline octa::Size formatted_write(R writer, const char *fmt,
                                         A &&...args) {
    octa::Size argidx = 0, retn = 0;
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
            octa::detail::write_idx(writer, needidx,
                octa::forward<A>(args)...);
            continue;
        }
        writer.put(*fmt++);
    }
    return retn + 1;
}

template<typename R, typename ...A>
octa::Size formatted_write(R writer, const octa::String &fmt, A &&...args) {
    return formatted_write(writer, fmt.data(), octa::forward<A>(args)...);
}

} /* namespace octa */

#endif