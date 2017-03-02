/* Generic stream implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_STREAM_HH
#define OSTD_STREAM_HH

#include <sys/types.h>
#include <type_traits>
#include <locale>
#include <optional>

#include "ostd/platform.hh"
#include "ostd/types.hh"
#include "ostd/range.hh"
#include "ostd/string.hh"
#include "ostd/format.hh"

namespace ostd {

#ifndef OSTD_PLATFORM_WIN32
using stream_off_t = off_t;
#else
using stream_off_t = __int64;
#endif

enum class stream_seek {
    CUR = SEEK_CUR,
    END = SEEK_END,
    SET = SEEK_SET
};

template<typename T = char, bool = std::is_pod_v<T>>
struct stream_range;

struct stream_error: std::system_error {
    using std::system_error::system_error;
};

template<typename T = char, typename TC = std::basic_string<T>>
struct stream_line_range;

struct stream {
    using offset_type = stream_off_t;

    virtual ~stream() {}

    virtual void close() = 0;

    virtual bool end() const = 0;

    virtual offset_type size() {
        offset_type p = tell();
        seek(0, stream_seek::END);
        offset_type e = tell();
        if (p == e) {
            return e;
        }
        seek(p, stream_seek::SET);
        return e;
    }

    virtual void seek(offset_type, stream_seek = stream_seek::SET) {}

    virtual offset_type tell() const { return -1; }

    virtual void flush() {}

    virtual void read_bytes(void *, size_t) {}
    virtual void write_bytes(void const *, size_t) {}

    virtual int get_char() {
        byte c;
        read_bytes(&c, 1);
        return c;
    }

    virtual void put_char(int c) {
        byte wc = byte(c);
        write_bytes(&wc, 1);
    }

    template<typename T = char, typename R>
    void get_line(R &&writer, bool keep_nl = false) {
        bool cr = false;
        /* read one char, if it fails to read at all just propagate errors */
        T c = get<T>();
        bool gotc = false;
        do {
            if (cr) {
                writer.put('\r');
                cr = false;
            }
            if (c == '\r') {
                cr = true;
                continue;
            }
            writer.put(c);
            gotc = safe_get<T>(c);
        } while (gotc && (c != '\n'));
        if (cr && (!gotc || keep_nl)) {
            /* we had carriage return and either reached EOF
             * or were told to keep separator, write the CR
             */
            writer.put('\r');
        }
        if (gotc && keep_nl) {
            writer.put('\n');
        }
    }

    template<typename T>
    void write(T const &v);

    template<typename T, typename ...A>
    void write(T const &v, A const &...args) {
        write(v);
        write(args...);
    }

    template<typename T>
    void writeln(T const &v) {
        write(v);
        put_char('\n');
    }

    template<typename T, typename ...A>
    void writeln(T const &v, A const &...args) {
        write(v);
        write(args...);
        put_char('\n');
    }

    template<typename ...A>
    void writef(string_range fmt, A const &...args);

    template<typename ...A>
    void writefln(string_range fmt, A const &...args) {
        writef(fmt, args...);
        put_char('\n');
    }

    template<typename T = char>
    stream_range<T> iter();

    template<typename T = char, typename TC = std::basic_string<T>>
    stream_line_range<T, TC> iter_lines(bool keep_nl = false);

    template<typename T>
    void put(T const *v, size_t count) {
        write_bytes(v, count * sizeof(T));
    }

    template<typename T>
    void put(T v) {
        write_bytes(&v, sizeof(T));
    }

    template<typename T>
    void get(T *v, size_t count) {
        read_bytes(v, count * sizeof(T));
    }

    template<typename T>
    void get(T &v) {
        read_bytes(&v, sizeof(T));
    }

    template<typename T>
    T get() {
        T r;
        get(r);
        return r;
    }

    std::locale imbue(std::locale const &loc) {
        std::locale ret{p_loc};
        p_loc = loc;
        return ret;
    }

    std::locale getloc() const {
        return p_loc;
    }

private:
    /* helper for get_line, so we don't catch errors from output range put */
    template<typename T>
    bool safe_get(T &c) {
        try {
            c = get<T>();
            return true;
        } catch (stream_error const &) {
            return false;
        }
    }

    std::locale p_loc;
};

template<typename T>
struct stream_range<T, true>: input_range<stream_range<T>> {
    using range_category  = input_range_tag;
    using value_type      = T;
    using reference       = T;
    using size_type       = size_t;
    using difference_type = stream_off_t;

    stream_range() = delete;
    stream_range(stream &s): p_stream(&s) {}
    stream_range(stream_range const &r):
        p_stream(r.p_stream), p_item(r.p_item)
    {}

    bool empty() const {
        if (!p_item.has_value()) {
            try {
                p_item = p_stream->get<T>();
            } catch (stream_error const &) {
                return true;
            }
        }
        return false;
    }

    void pop_front() {
        if (p_item.has_value()) {
            p_item = std::nullopt;
        } else {
            p_stream->get<T>();
        }
    }

    T front() const {
        if (p_item.has_value()) {
            return p_item.value();
        } else {
            return (p_item = p_stream->get<T>()).value();
        }
    }

    bool equals_front(stream_range const &s) const {
        return p_stream == s.p_stream;
    }

    void put(T val) {
        p_stream->put(val);
    }

private:
    stream *p_stream;
    mutable std::optional<T> p_item;
};

template<typename T>
inline stream_range<T> stream::iter() {
    return stream_range<T>{*this};
}

template<typename T, typename TC>
struct stream_line_range: input_range<stream_line_range<T, TC>> {
    using range_category  = input_range_tag;
    using value_type      = TC;
    using reference       = TC &;
    using size_type       = size_t;
    using difference_type = stream_off_t;

    stream_line_range() = delete;
    stream_line_range(stream &s, bool keep_nl = false):
        p_stream(&s), p_has_item(false), p_keep_nl(keep_nl)
    {}
    stream_line_range(stream_line_range const &r):
        p_stream(r.p_stream), p_item(r.p_item),
        p_has_item(r.p_has_item), p_keep_nl(r.p_keep_nl)
    {}

    bool empty() const {
        if (!p_has_item) {
            try {
                p_item.clear();
                p_stream->get_line(p_item, p_keep_nl);
                p_has_item = true;
            } catch (stream_error const &) {
                return true;
            }
        }
        return false;
    }

    void pop_front() {
        if (p_has_item) {
            p_item.clear();
            p_has_item = false;
        } else {
            p_stream->get_line(noop_output_range<T>{});
        }
    }

    reference front() const {
        if (p_has_item) {
            return p_item.get();
        } else {
            p_stream->get_line(p_item, p_keep_nl);
            p_has_item = true;
            return p_item.get();
        }
    }

    bool equals_front(stream_line_range const &s) const {
        return p_stream == s.p_stream;
    }

private:
    stream *p_stream;
    mutable appender_range<TC> p_item;
    mutable bool p_has_item;
    bool p_keep_nl;
};

template<typename T, typename TC>
stream_line_range<T, TC> stream::iter_lines(bool keep_nl) {
    return stream_line_range<T, TC>{*this, keep_nl};
}

template<typename T>
inline void stream::write(T const &v) {
    format_spec{'s', p_loc}.format_value(iter(), v);
}

template<typename ...A>
inline void stream::writef(string_range fmt, A const &...args) {
    format_spec sp{fmt, p_loc};
    sp.format(iter(), args...);
}

}

#endif
