/** @defgroup Streams
 *
 * @brief A stream system to replace the C++ iostreams.
 *
 * Libostd provides a custom stream system with considerably simpler API
 * and integration with other libostd features (such as ranges).
 *
 * Some string examples:
 *
 * @include stream2.cc
 *
 * And binary examples:
 *
 * @include stream1.cc
 *
 * See the examples provided with the library for further information.
 *
 * @{
 */

/** @file stream.hh
 *
 * @brief A base class for streams plus stream utilities.
 *
 * All streams derive from the basic ostd::stream class, implementing its
 * methods. This class provides the base interface as well as predefined
 * utility methods using the interface. Other utils are also provided,
 * such as by-value and by-line ranges for iteration and algorithms.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_STREAM_HH
#define OSTD_STREAM_HH

#include <cstddef>
#include <cerrno>
#include <cstdlib>
#include <type_traits>
#include <locale>
#include <optional>
#include <stdexcept>
#include <system_error>

#include "ostd/platform.hh"

#if !defined(OSTD_PLATFORM_POSIX) && !defined(OSTD_PLATFORM_WIN32)
#  error "Unsupported platform"
#endif

#ifndef OSTD_PLATFORM_WIN32
#include <sys/types.h>
#endif

#include "ostd/range.hh"
#include "ostd/string.hh"
#include "ostd/format.hh"

namespace ostd {

/** @addtogroup Streams
 * @{
 */

#ifndef OSTD_PLATFORM_WIN32
/** @brief The stream offset type.
 *
 * This is a signed integer type that can represent file sizes and offsets.
 * On POSIX systems, it defaults to the POSIX `off_t` type. On Windows, it's
 * a signed 64-bit integer.
 */
using stream_off_t = off_t;
#else
using stream_off_t = __int64;
#endif

/** @brief Reference position for seeking.
 *
 * Streams don't need to support the end of stream position. Therefore,
 * do not rely on it for generic stream usage (though it's fine for files).
 */
enum class stream_seek {
    CUR = SEEK_CUR, ///< Current position in the stream.
    END = SEEK_END, ///< End of stream position.
    SET = SEEK_SET  ///< Beginning of the stream.
};


template<typename T = char, bool = std::is_pod_v<T>>
struct stream_range;

/** @brief Thrown on stream errors. */
struct stream_error: std::system_error {
    using std::system_error::system_error;
};

template<typename T = char, typename TC = std::basic_string<T>>
struct stream_line_range;

/** @brief A base stream class.
 *
 * All streams derive from this, for example ostd::file_steram.
 * They implement the virtual interface provided by this class.
 */
struct stream {
    /** @brief The stream offset type. */
    using offset_type = stream_off_t;

    /** @brief Does nothing by default. */
    virtual ~stream() {}

    /** @brief Closes the stream.
     *
     * Pure virtual, so an implementation is needed.
     */
    virtual void close() = 0;

    /** @brief Checks if the stream has the end-of-stream indicator set.
     *
     * This is true for example if you try to read past the end of a file.
     * It's pure virtual, so a stream specific implementation is needed.
     */
    virtual bool end() const = 0;

    /** @brief Gets the size of the stream.
     *
     * There is a default implementation which only works on seekable
     * streams with supported end-of-stream reference position as defined
     * in ostd::stream_seek.
     *
     * It first saves the current position, seeks to the end, saves the
     * new position, seeks back to the old position (if the position
     * changed) and returns the position at the end, which is effectively
     * the size of the stream. This will not work on all stream types, so
     * a custom implementation can be provided.
     *
     * @throws ostd::stream_error if seek() or tell() fails.
     */
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

    /** @brief Seeks in the stream.
     *
     * Streams are not required to be seekable. There is a default
     * implementation that throws ostd::stream_error with EINVAL
     * (invalid argument error).
     *
     * None of the arguments are used in the default implementation. In
     * custom implementations the second argument sets the reference point
     * and the first argument the offset to it (positive or negative).
     *
     * The second argument is optional and will always be the beginning
     * of the stream by default, making seeking with ne argument basically
     * seek to a specific exact position.
     *
     * @throws ostd::stream_error
     *
     * @see tell()
     */
    virtual void seek(offset_type, stream_seek = stream_seek::SET) {
        throw stream_error{EINVAL, std::generic_category()};
    }

    /** @brief Tells the position in the stream.
     *
     * This has no meaning in streams that are not seekable, so it will
     * by default throw ostd::stream_error with EINVAL (invalid argument).
     *
     * Seekable stream types need to override this.
     *
     * @throws ostd::stream_error
     *
     * @see seek()
     */
    virtual offset_type tell() const {
        throw stream_error{EINVAL, std::generic_category()};
    }

    /** @brief Flushes the stream.
     *
     * If this is a writeable stream which has a buffer and any unwritten
     * data in it, this will make sure the data is actually written. By
     * default this does nothing, but other impls will want to override it.
     *
     * If the flush of a buffered stream fails, this should actually throw
     * an appropriate ostd::stream_error.
     */
    virtual void flush() {}

    /** @brief Reads at most a number of bytes from the stream.
     *
     * This is a low level function to read the given number of bytes into
     * the buffer given by the first argument. The default implementation
     * will simply throw ostd::stream_error with EINVAL (invalid argument).
     *
     * Custom implementations will also throw ostd::stream_error with some
     * error code on read errors (assuming they can be read from and actually
     * implement this).
     *
     * Keep in mind that this method should only throw on actual read errors
     * when implemented. If an end-of-stream indicator was set and fewer than
     * the provided number of bytes were read, this method will instead just
     * return the number of bytes it has actually read. An end of stream is
     * not an error, which is why it's necessary to do this.
     *
     * Also, typically you will want to use higher level methods, for example
     * via ranges or utility methods provided within this type.
     *
     * @throws ostd::stream_error
     *
     * @returns How many bytes were actually read.
     *
     * @see write_bytes(), get_char(), get(), get_line()
     */
    virtual std::size_t read_bytes(void *, std::size_t) {
        throw stream_error{EINVAL, std::generic_category()};
    }

    /** @brief Writes the given number of bytes from the given buffer.
     *
     * This is a low level function to write the provided number of bytes
     * in the provided buffer into the stream. The default implementation
     * will throw ostd::stream_error with EINVAL (invalid argument).
     *
     * Streams don't need to be writeable, so only writeable streams will
     * override this. Overridden implementations will throw ostd::stream_error
     * on write errors.
     *
     * Keep in mind that this is low level and you will typically want to
     * use either ranges or other utility methods within this type.
     *
     * @throws ostd::stream_error
     *
     * @see read_bytes(), put_char(), put(), write()
     */
    virtual void write_bytes(void const *, std::size_t) {
        throw stream_error{EINVAL, std::generic_category()};
    }

    /** @brief Reads a single byte from the stream.
     *
     * The returned type is an int, but the read value is indeed just one
     * byte. This uses read_bytes() to read the byte, so it will throw
     * ostd::stream_error on failure. Also, if the stream is at the end
     * and an end-of-stream indicator is set (see end()) and nothing is
     * read, this throws ostd::stream_error with EIO (I/O error).
     *
     * @throws ostd::stream_error on failure or end-of-stream.
     *
     * @returns The read byte.
     *
     * @see put_char(), get(), get_line()
     */
    virtual int get_char() {
        unsigned char c;
        if (!read_bytes(&c, 1)) {
            throw stream_error{EIO, std::generic_category()};
        }
        return c;
    }

    /** @brief Writes a single byte into the stream.
     *
     * The provided argument is an int, but it must be a byte value. The
     * value is converted into `unsigned char` with `static_cast`. It is
     * then written using write_bytes().
     *
     * @throws ostd::stream_error on write error.
     *
     * @see get_char(), put(), write()
     */
    virtual void put_char(int c) {
        unsigned char wc = static_cast<unsigned char>(c);
        write_bytes(&wc, 1);
    }

    /** @brief Reads a line from the stream into an output range.
     *
     * This reads bytes from the stream until it encounters either a newline
     * character or the carriage return plus newline sequence. Those are
     * obviously read from the stream but not written into the sink unless
     * `keep_nl` is true (it's false by default).
     *
     * The get() method is used to read the characters. You can also provide
     * a custom character type to use for reading. By default it reads by
     * bytes, but you might also want a bigger type for UTF-16 or UTF-32
     * encoded streams, for example.
     *
     * @param[in] writer An output range to write into.
     * @param[in] keep_nl Whether to keep the newline sequence.
     * @tparam T The character type to use for reading.
     *
     * @throws ostd::stream_error on read error.
     *
     * @see get()
     */
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

    /** @brief Writes all given values into the stream.
     *
     * There is no separator put between the values. It supports any type
     * that can be formatted using ostd::format_spec with the default `%s`
     * format specifier. The stream's locale is passed into the formatter.
     *
     * @throws ostd::stream_error on write error.
     * @throws ostd::format_error if a value cannot be formatted.
     *
     * @see writeln(), writef(), writefln()
     */
    template<typename ...A>
    void write(A const &...args);

    /** @brief Writes all given values into the stream followed by a newline.
     *
     * The same as write(), except writes a newline at the end.
     *
     * @throws ostd::stream_error on write error.
     * @throws ostd::format_error if a value cannot be formatted.
     *
     * @see write(), writef(), writefln()
     */
    template<typename ...A>
    void writeln(A const &...args) {
        write(args...);
        put_char('\n');
    }

    /** @brief Writes a formatted string into the stream.
     *
     * Given a format string and arguments, this is just like using
     * ostd::format() with this stream as the sink. The stream's
     * locale is passed into the formatter.
     *
     * @throws ostd::stream_error on write error.
     * @throws ostd::format_error if the formatting fails.
     *
     * @see writefln(), write(), writeln()
     */
    template<typename ...A>
    void writef(string_range fmt, A const &...args);

    /** @brief Writes a formatted string into the stream followed by a newline.
     *
     * The same as writef(), except writes a newline at the end.
     *
     * @throws ostd::stream_error on write error.
     * @throws ostd::format_error if the formatting fails.
     *
     * @see writef(), write(), writeln()
     */
    template<typename ...A>
    void writefln(string_range fmt, A const &...args) {
        writef(fmt, args...);
        put_char('\n');
    }

    /** @brief Creates a range around the stream.
     *
     * The range stays valid as long as the stream is valid. The range does
     * not own the stream, so you have to track the lifetime correctly.
     *
     * By default, it's a `char` range that can be read from if the stream
     * can be read from and written into if the stream can be written into.
     * You can override the type by passing in the template parameter. The
     * type must always be POD.
     *
     * @tparam T The type to use for reading/writing (`char` by default).
     *
     * @see iter_lines()
     */
    template<typename T = char>
    stream_range<T> iter();

    /** @brief Creates a by-line range around the stream.
     *
     * Same lifetime rules as with iter() apply. The range uses get_line()
     * to read the lines.
     *
     * @tparam TC The string type to use for line storage in the range.
     *
     * @see iter()
     */
    template<typename T = char, typename TC = std::basic_string<T>>
    stream_line_range<T, TC> iter_lines(bool keep_nl = false);

    /** @brief Writes several values into the stream.
     *
     * Uses write_bytes() to write `count` values from `v` into the stream.
     * The type must be POD.
     *
     * @throws ostd::stream_error on write failure.
     */
    template<typename T>
    void put(T const *v, std::size_t count) {
        write_bytes(v, count * sizeof(T));
    }

    /** @brief Writes a single value into the stream.
     *
     * Uses write_bytes() to write the value. The type must be POD.
     *
     * @throws ostd::stream_error on write failure.
     */
    template<typename T>
    void put(T v) {
        write_bytes(&v, sizeof(T));
    }

    /** @brief Reads several values from the stream.
     *
     * Uses read_bytes() to read `count` values into `v` from the stream.
     * If an end-of-stream is encountered while reading, this does not
     * throw, but it returns the number of entire values successfully read.
     * The type must be POD.
     *
     * @returns The number of whole values read.
     *
     * @throws ostd::stream_error on read failure.
     */
    template<typename T>
    std::size_t get(T *v, std::size_t count) {
        /* if eof was reached, at least return how many values succeeded */
        return read_bytes(v, count * sizeof(T)) / sizeof(T);
    }

    /** @brief Reads a single value from the stream.
     *
     * If the value couldn't be read (reading failed or end-of-stream
     * occured), this throws ostd::stream_error. The type must be POD.
     *
     * @throws ostd::stream_error on read failure or end-of-stream.
     */
    template<typename T>
    void get(T &v) {
        if (read_bytes(&v, sizeof(T)) != sizeof(T)) {
            throw stream_error{EIO, std::generic_category()};
        }
    }

    /** @brief Reads a single value from the stream.
     *
     * If the value couldn't be read (reading failed or end-of-stream
     * occured), this throws ostd::stream_error. The type must be POD.
     *
     * @returns The read value.
     *
     * @throws ostd::stream_error on read failure or end-of-stream.
     */
    template<typename T>
    T get() {
        T r;
        get(r);
        return r;
    }

    /** @brief Sets a new locale for the stream.
     *
     * Replaces the old locale. The old locale is returned.
     *
     * @returns The old locale.
     *
     * @see getloc()
     */
    std::locale imbue(std::locale const &loc) {
        std::locale ret{p_loc};
        p_loc = loc;
        return ret;
    }

    /** @brief Gets the stream's locale.
     *
     * @returns The locale.
     *
     * @see imbue()
     */
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

/** @brief A range type for streams.
 *
 * This is an input range (ostd::input_range_tag) which is also mutable
 * (can be an output range) when the stream can be written into.
 *
 * It keeps a pointer to the stream internally. It does not own the stream
 * so its lifetime ends when the stream's lifetime ends and after that it
 * is no longer safe to use the range.
 *
 * As it's an input range, it's not safe to use it with multipass algorithms.
 * If you do, expect strange semantics, as the state is shared between all
 * ranges over a single stream.
 *
 * The range caches the most recently read value. On a call to any method
 * that manipulates the range (empty(), front() or pop_front()) it will try
 * to read a value from the stream unless it already has one. The value will
 * therefore not be readable from the stream again (from another range for
 * example).
 *
 * This template is a specialization of undefined base type because stream
 * ranges only work with POD types.
 *
 * @see stream_line_range
 */
template<typename T>
struct stream_range<T, true>: input_range<stream_range<T>> {
    using range_category = input_range_tag;
    using value_type     = T;
    using reference      = T;
    using size_type      = std::size_t;

    stream_range() = delete;

    /** @brief Creates a stream range using a stream. */
    stream_range(stream &s): p_stream(&s) {}

    /** @brief Stream ranges can be copied, the cached value is also copied. */
    stream_range(stream_range const &r):
        p_stream(r.p_stream), p_item(r.p_item)
    {}

    /** @brief Checks if the range (stream) is empty.
     *
     * If there is a value cached in this range, this returns false.
     * Otherwise it will attempt to read a value from the stream and
     * cache it. If that reading fails, the exception is discarded
     * and true is returned. Otherwise, false is returned.
     *
     * In short, if a value is cached or can be cached, the range and
     * therefore stream is not empty. Otherwise it's considered empty.
     */
    bool empty() const noexcept {
        if (!p_item.has_value()) {
            try {
                p_item = p_stream->get<T>();
            } catch (stream_error const &) {
                return true;
            }
        }
        return false;
    }

    /** @brief Either clears the cached value or reads one.
     *
     * If a value is currently cached, it gets discarded. Otherwise
     * it will read a value from the stream without caching it.
     */
    void pop_front() {
        if (p_item.has_value()) {
            p_item = std::nullopt;
        } else {
            p_stream->get<T>();
        }
    }

    /** @brief Gets the cached value.
     *
     * If we have a cached value, it's returned. If we don't,
     * a new value is read, cached and then returned.
     */
    reference front() const {
        if (p_item.has_value()) {
            return p_item.value();
        } else {
            return (p_item = p_stream->get<T>()).value();
        }
    }

    /** @brief Writes a value into the stream. */
    void put(value_type val) {
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

/** @brief A range type for streams to read by line.
 *
 * This is an input range (ostd::input_range_tag) which is not mutable,
 * so it can only be used to read.
 *
 * It keeps a pointer to the stream internally. It does not own the stream
 * so its lifetime ends when the stream's lifetime ends and after that it
 * is no longer safe to use the range.
 *
 * As it's an input range, it's not safe to use it with multipass algorithms.
 * If you do, expect strange semantics, as the state is shared between all
 * ranges over a single stream.
 *
 * The range caches the most recently read line. On a call to any method
 * that manipulates the range (empty(), front() or pop_front()) it will try
 * to read a line from the stream unless it already has one. The line will
 * therefore not be readable from the stream again (from another range for
 * example).
 *
 * The lines are read using ostd::stream::get_line().
 *
 * You can provide a custom type for characters, by default it's `char`.
 * It must be POD.
 *
 * You can also provide a custom type used to hold the line, which must be a
 * container type over `T` which can append at the end. By default,
 * it's an std::basic_string over `T`.
 *
 * @tparam T The type used for individual characters.
 * @tparam TC The type used to hold the line (in an ostd::appender()).
 *
 * @see stream_range
 */
template<typename T, typename TC>
struct stream_line_range: input_range<stream_line_range<T, TC>> {
    using range_category = input_range_tag;
    using value_type     = TC;
    using reference      = TC &;
    using size_type      = std::size_t;

    stream_line_range() = delete;

    /** @brief Creates a stream line range using a stream.
     *
     * If you set `keep_nl` to true, it will write the newlines.
     */
    stream_line_range(stream &s, bool keep_nl = false):
        p_stream(&s), p_has_item(false), p_keep_nl(keep_nl)
    {}

    /** @brief Stream line ranges are copy constructible.
     *
     * The storage container with the cached line is also copied.
     */
    stream_line_range(stream_line_range const &r):
        p_stream(r.p_stream), p_item(r.p_item),
        p_has_item(r.p_has_item), p_keep_nl(r.p_keep_nl)
    {}

    /** @brief Stream line ranges are move constructible.
     *
     * The storage container with the cached line is also moved.
     */
    stream_line_range(stream_line_range &&r):
        p_stream(r.p_stream), p_item(std::move(r.p_item)),
        p_has_item(r.p_has_item), p_keep_nl(r.p_keep_nl)
    {
        r.p_has_item = false;
    }

    /** @brief Checks if the range (stream) is empty.
     *
     * If there is a line cached in this range, this returns false.
     * Otherwise it will attempt to read a line from the stream and
     * cache it. If that reading fails, the exception is discarded
     * and true is returned. Otherwise, false is returned.
     *
     * In short, if a line is cached or can be cached, the range and
     * therefore stream is not empty. Otherwise it's considered empty.
     */
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

    /** @brief Either clears the cached line or reads one.
     *
     * If a line is currently cached, it gets discarded. Otherwise
     * it will read a line from the stream without caching it.
     */
    void pop_front() {
        if (p_has_item) {
            p_item.clear();
            p_has_item = false;
        } else {
            p_stream->get_line(noop_sink<T>());
        }
    }

    /** @brief Gets the cached line.
     *
     * If we have a cached line, it's returned. If we don't,
     * a new line is read, cached and then returned.
     */
    reference front() const {
        if (p_has_item) {
            return p_item.get();
        } else {
            p_stream->get_line(p_item, p_keep_nl);
            p_has_item = true;
            return p_item.get();
        }
    }

private:
    stream *p_stream;
    mutable decltype(appender<TC>()) p_item;
    mutable bool p_has_item;
    bool p_keep_nl;
};

template<typename T, typename TC>
inline stream_line_range<T, TC> stream::iter_lines(bool keep_nl) {
    return stream_line_range<T, TC>{*this, keep_nl};
}

template<typename ...A>
inline void stream::write(A const &...args) {
    format_spec sp{'s', p_loc};
    (sp.format_value(iter(), args), ...);
}

template<typename ...A>
inline void stream::writef(string_range fmt, A const &...args) {
    format_spec{fmt, p_loc}.format(iter(), args...);
}

/** @} */

}

#endif

/** @} */
