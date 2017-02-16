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
        if ((p < 0) || !seek(0, stream_seek::END)) {
            return -1;
        }
        offset_type e = tell();
        return ((p == e) || seek(p, stream_seek::SET)) ? e : -1;
    }

    virtual bool seek(offset_type, stream_seek = stream_seek::SET) {
        return false;
    }

    virtual offset_type tell() const { return -1; }

    virtual bool flush() { return true; }

    virtual size_t read_bytes(void *, size_t) { return 0; }
    virtual size_t write_bytes(void const *, size_t) { return 0; }

    virtual int getchar() {
        byte c;
        return (read_bytes(&c, 1) == 1) ? c : -1;
    }

    virtual bool putchar(int c) {
        byte wc = byte(c);
        return write_bytes(&wc, 1) == 1;
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
        if (!putchar('\n')) {
            /* consistency with format module */
            throw format_error{"stream EOF"};
        }
    }

    template<typename T, typename ...A>
    void writeln(T const &v, A const &...args) {
        write(v);
        write(args...);
        if (!putchar('\n')) {
            throw format_error{"stream EOF"};
        }
    }

    template<typename ...A>
    void writef(string_range fmt, A const &...args);

    template<typename ...A>
    void writefln(string_range fmt, A const &...args) {
        writef(fmt, args...);
        if (!putchar('\n')) {
            throw format_error{"stream EOF"};
        }
    }

    template<typename T = char>
    stream_range<T> iter();

    template<typename T>
    size_t put(T const *v, size_t count) {
        return write_bytes(v, count * sizeof(T)) / sizeof(T);
    }

    template<typename T>
    bool put(T v) {
        return write_bytes(&v, sizeof(T)) == sizeof(T);
    }

    template<typename T>
    size_t get(T *v, size_t count) {
        return read_bytes(v, count * sizeof(T)) / sizeof(T);
    }

    template<typename T>
    bool get(T &v) {
        return read_bytes(&v, sizeof(T)) == sizeof(T);
    }

    template<typename T>
    T get() {
        T r;
        return get(r) ? r : T();
    }
};

template<typename T>
size_t range_put_n(stream_range<T> &range, T const *p, size_t n);

template<typename T>
struct stream_range<T, true>: input_range<stream_range<T>> {
    using range_category  = input_range_tag;
    using value_type      = T;
    using reference       = T;
    using size_type       = size_t;
    using difference_type = stream_off_t;

    template<typename TT>
    friend size_t range_put_n(stream_range<TT> &range, TT const *p, size_t n);

    stream_range() = delete;
    stream_range(stream &s): p_stream(&s), p_size(s.size()) {}
    stream_range(stream_range const &r): p_stream(r.p_stream), p_size(r.p_size) {}

    bool empty() const {
        return (p_size - p_stream->tell()) < stream_off_t(sizeof(T));
    }

    bool pop_front() {
        if (empty()) {
            return false;
        }
        T val;
        return !!p_stream->read_bytes(&val, sizeof(T));
    }

    T front() const {
        T val;
        p_stream->seek(-p_stream->read_bytes(&val, sizeof(T)), stream_seek::CUR);
        return val;
    }

    bool equals_front(stream_range const &s) const {
        return p_stream->tell() == s.p_stream->tell();
    }

    bool put(T val) {
        size_t v = p_stream->write_bytes(&val, sizeof(T));
        p_size += v;
        return (v == sizeof(T));
    }

    size_t copy(std::remove_cv_t<T> *p, size_t n = -1) {
        if (n == size_t(-1)) {
            n = p_stream->size() / sizeof(T);
        }
        return p_stream->get(p, n);
    }

private:
    stream *p_stream;
    stream_off_t p_size;
};

template<typename T>
inline size_t range_put_n(stream_range<T> &range, T const *p, size_t n) {
    return range.p_stream->put(p, n);
}

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

        fmt_stream_range(stream &s): p_s(s) {}
        bool put(char c) {
            return p_s.write_bytes(&c, 1) == 1;
        }
        stream &p_s;
    };

    inline size_t range_put_n(fmt_stream_range &range, char const *p, size_t n) {
        return range.p_s.write_bytes(p, n);
    }
}

template<typename T>
inline void stream::write(T const &v) {
    format(detail::fmt_stream_range{*this}, format_spec{'s'}, v);
}

template<typename ...A>
inline void stream::writef(string_range fmt, A const &...args) {
    format(detail::fmt_stream_range{*this}, fmt, args...);
}

}

#endif
