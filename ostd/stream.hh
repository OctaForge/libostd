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
#include "ostd/utility.hh"
#include "ostd/format.hh"

namespace ostd {

#ifndef OSTD_PLATFORM_WIN32
using StreamOffset = off_t;
#else
using StreamOffset = __int64;
#endif

enum class StreamSeek {
    cur = SEEK_CUR,
    end = SEEK_END,
    set = SEEK_SET
};

template<typename T = char, bool = std::is_pod_v<T>>
struct StreamRange;

struct Stream {
    using Offset = StreamOffset;

    virtual ~Stream() {}

    virtual void close() = 0;

    virtual bool end() const = 0;

    virtual Offset size() {
        Offset p = tell();
        if ((p < 0) || !seek(0, StreamSeek::end)) {
            return -1;
        }
        Offset e = tell();
        return ((p == e) || seek(p, StreamSeek::set)) ? e : -1;
    }

    virtual bool seek(Offset, StreamSeek = StreamSeek::set) {
        return false;
    }

    virtual Offset tell() const { return -1; }

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
    void writef(ConstCharRange fmt, A const &...args);

    template<typename ...A>
    void writefln(ConstCharRange fmt, A const &...args) {
        writef(fmt, args...);
        if (!putchar('\n')) {
            throw format_error{"stream EOF"};
        }
    }

    template<typename T = char>
    StreamRange<T> iter();

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
struct StreamRange<T, true>: InputRange<
    StreamRange<T>, InputRangeTag, T, T, size_t, StreamOffset
> {
    StreamRange() = delete;
    StreamRange(Stream &s): p_stream(&s), p_size(s.size()) {}
    StreamRange(StreamRange const &r): p_stream(r.p_stream), p_size(r.p_size) {}

    bool empty() const {
        return (p_size - p_stream->tell()) < StreamOffset(sizeof(T));
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
        p_stream->seek(-p_stream->read_bytes(&val, sizeof(T)), StreamSeek::cur);
        return val;
    }

    bool equals_front(StreamRange const &s) const {
        return p_stream->tell() == s.p_stream->tell();
    }

    bool put(T val) {
        size_t v = p_stream->write_bytes(&val, sizeof(T));
        p_size += v;
        return (v == sizeof(T));
    }

    size_t put_n(T const *p, size_t n) {
        return p_stream->put(p, n);
    }

    size_t copy(std::remove_cv_t<T> *p, size_t n = -1) {
        if (n == size_t(-1)) {
            n = p_stream->size() / sizeof(T);
        }
        return p_stream->get(p, n);
    }

private:
    Stream *p_stream;
    StreamOffset p_size;
};

template<typename T>
inline StreamRange<T> Stream::iter() {
    return StreamRange<T>(*this);
}

namespace detail {
    /* lightweight output range for write/writef on streams */
    struct FmtStreamRange: OutputRange<FmtStreamRange, char> {
        FmtStreamRange(Stream &s): p_s(s) {}
        bool put(char c) {
            return p_s.write_bytes(&c, 1) == 1;
        }
        size_t put_n(char const *p, size_t n) {
            return p_s.write_bytes(p, n);
        }
        Stream &p_s;
    };
}

template<typename T>
inline void Stream::write(T const &v) {
    format(detail::FmtStreamRange{*this}, FormatSpec{'s'}, v);
}

template<typename ...A>
inline void Stream::writef(ConstCharRange fmt, A const &...args) {
    format(detail::FmtStreamRange{*this}, fmt, args...);
}

}

#endif
