/* Standard I/O implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_IO_HH
#define OSTD_IO_HH

#include <cstdio>
#include <cerrno>

#include "ostd/platform.hh"
#include "ostd/string.hh"
#include "ostd/stream.hh"
#include "ostd/format.hh"

namespace ostd {

enum class stream_mode {
    READ = 0, WRITE, APPEND, READ_U, WRITE_U, APPEND_U
};

struct OSTD_EXPORT file_stream: stream {
    file_stream(): p_f(), p_owned(false) {}
    file_stream(file_stream const &) = delete;
    file_stream(file_stream &&s): p_f(s.p_f), p_owned(s.p_owned) {
        s.p_f = nullptr;
        s.p_owned = false;
    }

    file_stream(string_range path, stream_mode mode = stream_mode::READ):
        p_f()
    {
        open(path, mode);
    }

    file_stream(FILE *f): p_f(f), p_owned(false) {}

    ~file_stream() { close(); }

    file_stream &operator=(file_stream const &) = delete;
    file_stream &operator=(file_stream &&s) {
        close();
        swap(s);
        return *this;
    }

    bool open(string_range path, stream_mode mode = stream_mode::READ);

    bool open(FILE *f);

    bool is_open() const { return p_f != nullptr; }
    bool is_owned() const { return p_owned; }

    void close();
    bool end() const;

    void seek(stream_off_t pos, stream_seek whence = stream_seek::SET);

    stream_off_t tell() const;

    void flush();

    void read_bytes(void *buf, size_t count);
    void write_bytes(void const *buf, size_t count);

    int get_char();
    void put_char(int c);

    void swap(file_stream &s) {
        using std::swap;
        swap(p_f, s.p_f);
        swap(p_owned, s.p_owned);
    }

    FILE *get_file() const { return p_f; }

private:
    FILE *p_f;
    bool p_owned;
};

inline void swap(file_stream &a, file_stream &b) {
    a.swap(b);
}

OSTD_EXPORT extern file_stream cin;
OSTD_EXPORT extern file_stream cout;
OSTD_EXPORT extern file_stream cerr;

/* no need to call anything from file_stream, prefer simple calls... */

namespace detail {
    /* lightweight output range for direct stdout */
    struct stdout_range: output_range<stdout_range> {
        using value_type      = char;
        using reference       = char &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        stdout_range() {}
        void put(char c) {
            if (std::putchar(c) == EOF) {
                throw stream_error{EIO, std::generic_category()};
            }
        }
    };

    template<typename R>
    inline void range_put_all(stdout_range &r, R range) {
        if constexpr(
            is_contiguous_range<R> &&
            std::is_same_v<std::remove_const_t<range_value_t<R>>, char>
        ) {
            if (std::fwrite(range.data(), 1, range.size(), stdout) != range.size()) {
                throw stream_error{EIO, std::generic_category()};
            }
        } else {
            for (; !range.empty(); range.pop_front()) {
                r.put(range.front());
            }
        }
    }
}

template<typename ...A>
inline void write(A const &...args) {
    format_spec sp{'s', cout.getloc()};
    (sp.format_value(detail::stdout_range{}, args), ...);
}

template<typename ...A>
inline void writeln(A const &...args) {
    write(args...);
    if (std::putchar('\n') == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
}

template<typename ...A>
inline void writef(string_range fmt, A const &...args) {
    format_spec sp{fmt, cout.getloc()};
    sp.format(detail::stdout_range{}, args...);
}

template<typename ...A>
inline void writefln(string_range fmt, A const &...args) {
    writef(fmt, args...);
    if (std::putchar('\n') == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
}

} /* namespace ostd */

#endif
