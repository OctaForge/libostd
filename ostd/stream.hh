/* Generic stream implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_STREAM_HH
#define OSTD_STREAM_HH

#include <sys/types.h>
#include <type_traits>

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

    virtual int getchar() {
        byte c;
        read_bytes(&c, 1);
        return c;
    }

    virtual void putchar(int c) {
        byte wc = byte(c);
        write_bytes(&wc, 1);
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
        putchar('\n');
    }

    template<typename T, typename ...A>
    void writeln(T const &v, A const &...args) {
        write(v);
        write(args...);
        putchar('\n');
    }

    template<typename ...A>
    void writef(string_range fmt, A const &...args);

    template<typename ...A>
    void writefln(string_range fmt, A const &...args) {
        writef(fmt, args...);
        putchar('\n');
    }

    template<typename T = char>
    stream_range<T> iter();

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
};

template<typename T>
struct stream_range<T, true>: input_range<stream_range<T>> {
    using range_category  = input_range_tag;
    using value_type      = T;
    using reference       = T;
    using size_type       = size_t;
    using difference_type = stream_off_t;

    stream_range() = delete;
    stream_range(stream &s): p_stream(&s), p_size(s.size()) {}
    stream_range(stream_range const &r): p_stream(r.p_stream), p_size(r.p_size) {}

    bool empty() const {
        try {
            auto pos = p_stream->tell();
            return (p_size - pos) < stream_off_t(sizeof(T));
        } catch (...) {
            return true;
        }
    }

    void pop_front() {
        T val;
        p_stream->read_bytes(&val, sizeof(T));
    }

    T front() const {
        T val;
        p_stream->read_bytes(&val, sizeof(T));
        p_stream->seek(-sizeof(T), stream_seek::CUR);
        return val;
    }

    bool equals_front(stream_range const &s) const {
        return p_stream->tell() == s.p_stream->tell();
    }

    void put(T val) {
        p_stream->write_bytes(&val, sizeof(T));
        p_size += sizeof(T);
    }

private:
    stream *p_stream;
    stream_off_t p_size;
};

template<typename T>
inline stream_range<T> stream::iter() {
    return stream_range<T>(*this);
}

namespace detail {
    /* lightweight output range for write/writef on streams */
    struct fmt_stream_range: output_range<fmt_stream_range> {
        using value_type      = char;
        using reference       = char &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        fmt_stream_range(stream *s): p_s(s) {}
        void put(char c) {
            p_s->write_bytes(&c, 1);
        }
        stream *p_s;
    };
}

template<typename T>
inline void stream::write(T const &v) {
    format_spec{'s'}.format_value(detail::fmt_stream_range{this}, v);
}

template<typename ...A>
inline void stream::writef(string_range fmt, A const &...args) {
    format(detail::fmt_stream_range{this}, fmt, args...);
}

}

#endif
