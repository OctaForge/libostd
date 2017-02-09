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
private:
    struct StNat {};

    bool write_impl(ConstCharRange s) {
        return write_bytes(s.data(), s.size()) == s.size();
    }

    template<typename T>
    inline bool write_impl(
        T const &v, std::enable_if_t<
            !std::is_constructible_v<ConstCharRange, T const &>, StNat
        > = StNat()
    ) {
        return write(ostd::to_string(v));
    }

public:
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
    bool write(T const &v) {
        return write_impl(v);
    }

    template<typename T, typename ...A>
    bool write(T const &v, A const &...args) {
        return write(v) && write(args...);
    }

    template<typename T>
    bool writeln(T const &v) {
        return write(v) && putchar('\n');
    }

    template<typename T, typename ...A>
    bool writeln(T const &v, A const &...args) {
        return write(v) && write(args...) && putchar('\n');
    }

    template<typename ...A>
    bool writef(ConstCharRange fmt, A const &...args);

    template<typename ...A>
    bool writefln(ConstCharRange fmt, A const &...args) {
        return writef(fmt, args...) && putchar('\n');
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

template<typename ...A>
inline bool Stream::writef(ConstCharRange fmt, A const &...args) {
    return format(iter(), fmt, args...) >= 0;
}

}

#endif
