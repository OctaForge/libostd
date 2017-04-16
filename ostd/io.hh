/** @addtogroup Streams
 * @{
 */

/** @file io.hh
 *
 * @brief File streams and standard output/input/error manipulation.
 *
 * This file implements a file stream structure equivalent to the C `FILE`
 * as well as wrappers over standard input/output/error and global functions
 * for formatted writing into standard output.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_IO_HH
#define OSTD_IO_HH

#include <cstddef>
#include <cstdio>
#include <cerrno>

#include "ostd/platform.hh"
#include "ostd/string.hh"
#include "ostd/stream.hh"
#include "ostd/format.hh"

namespace ostd {

/** @addtogroup Streams
 * @{
 */

/** @brief The mode to open file streams with.
 *
 * Libostd file streams are always opened in binary mode. Text mode is not
 * directly supported (the only way to get it is to encapsulate a C `FILE *`
 * that is already opened in text mode).
 *
 * See the C fopen() function documentation for more info on modes.
*/
enum class stream_mode {
    READ = 0, ///< Reading, equivalent to the C `rb` mode.
    WRITE,    ///< Writing, equivalent to the C `wb` mode.
    APPEND,   ///< Appending, equivalent to the C `ab` mode.
    READ_U,   ///< Read/update, equivalent to the C `rb+` mode.
    WRITE_U,  ///< Write/update, equivalent to the C `wb+` mode.
    APPEND_U  ///< Append/update, equivalent to the C `ab+` mode.
};

/** @brief A file stream.
 *
 * File streams are equivalent to the C `FILE` type. You can open new file
 * streams and you can also create high level file stream over C file streams.
 * File streams are seekable except in special cases (stdin/stdout/stderr).
 *
 * File streams implement a concept of ownership; if they own the underlying
 * stream, which is every time when the path-based constructor or open() are
 * used, they close the underlying stream on destruction (if still open).
 */
struct OSTD_EXPORT file_stream: stream {
    /** @brief Crates an empty file stream.
     *
     * The resulting file stream won't have an associated file. Any operations
     * involving the potential associated file are considered unfedined.
     */
    file_stream(): p_f(), p_owned(false) {}

    file_stream(file_stream const &) = delete;

    /** @brief Creates a file stream by moving.
     *
     * The other file stream is set to an empty state,
     * i.e. it will not have any associated file set.
     */
    file_stream(file_stream &&s): p_f(s.p_f), p_owned(s.p_owned) {
        s.p_f = nullptr;
        s.p_owned = false;
    }

    /** @brief Creates a file stream using a file path.
     *
     * The path is a relative or absolute path, basically anything that
     * can be passed to C fopen(). The path does not need to be null
     * terminated. The construction might fail, if it does, this will
     * not throw an error but instead the stream will be left without
     * an associated state, which you can check for later using is_open().
     *
     * It works by calling open(). The default mode (when none is provided)
     * is a plain reading stream.
     */
    file_stream(string_range path, stream_mode mode = stream_mode::READ):
        p_f()
    {
        open(path, mode);
    }

    /** @brief Creates a file stream using a C `FILE` pointer.
     *
     * You can then manipulate the pointer using the stream, but it will
     * not be owned; you need to manually close it using the correct C
     * function when you're done.
     */
    file_stream(FILE *f): p_f(f), p_owned(false) {}

    /** @brief Calls close() on the stream. */
    ~file_stream() { close(); }

    file_stream &operator=(file_stream const &) = delete;

    /** @brief Assigns another stream to this one by move.
     *
     * If we're currently owning another file, close() is called first.
     * Then the other stream's state is moved here and the other stream
     * is left empty (as if initialized with a default constructor).
     */
    file_stream &operator=(file_stream &&s) {
        close();
        swap(s);
        return *this;
    }

    /** @brief Opens a file stream by file path.
     *
     * If there is currently another file associated with the stream,
     * this just directly returns `false`. Otherwise, it will try to
     * open the file. If that fails for some reason (path too long or
     * fopen() failed for some other reason), `false` is returned.
     * Otherwise, `true` is returned and both is_open() and is_owned()
     * will be true.
     */
    bool open(string_range path, stream_mode mode = stream_mode::READ);

    /** @brief Opens a file stream by C `FILE` pointer.
     *
     * This sets the associated file pointer. If there is currently
     * another file associated with the stream, this just directly
     * returns `false`. Otherwise, it will set the association and
     * returns `true`.
     *
     * In the end, is_open() will be true but is_owned() will be false.
     * You need to manually take care of the pointer because this stream
     * will not close it.
     */
    bool open(FILE *f);

    /** @brief Checks if there is a resource associated with this stream. */
    bool is_open() const { return p_f != nullptr; }

    /** @brief Checks if we're owning the associated resource. */
    bool is_owned() const { return p_owned; }

    /** @brief Closes the associated file.
     *
     * If both is_open() and is_owned() are true, this will close the
     * associated file and set the stream to empty. Otherwise, it will
     * do nothing.
     */
    void close();

    /** @brief Checks if the associated stream has an end of file set.
     *
     * This is not necessarily true if the current stream position is
     * at the end. It becomes true once you've tried reading past the
     * end of the file.
     */
    bool end() const;

    /** @brief Seeks within the stream.
     *
     * File streams are normally seekable. Sometimes they are not
     * though, such as when this represents an stdin/stdout/stderr.
     *
     * @throws ostd::stream_error with errno on failure.
     *
     * @see tell()
     */
    void seek(stream_off_t pos, stream_seek whence = stream_seek::SET);

    /** @brief Tells the current stream position.
     *
     * @throws ostd::stream_error with EIO on failure.
     *
     * @see seek()
     */
    stream_off_t tell() const;

    /** @brief Flushes the associated stream's buffer.
     *
     * @throws ostd::stream_error with errno on failure.
     */
    void flush();

    /** @brief Reads at most a number of bytes from the stream.
     *
     * If an end-of-file was reached during the reading, this will return
     * the amount of bytes actually read. If the reading failed somehow,
     * this will throw an ostd::stream_error with EIO. Otherwise, it should
     * return `count`.
     *
     * @throws ostd::stream_error with EIO on failure (not on EOF).
     *
     * @see write_bytes()
     */
    std::size_t read_bytes(void *buf, std::size_t count);

    /** @brief Writes `count` bytes into the stream.
     *
     * @throws ostd::stream_error with EIO on failure.
     *
     * @see read_bytes()
     */
    void write_bytes(void const *buf, std::size_t count);

    /** @brief Reads a single character from the stream.
     *
     * Does not use read_bytes() like the default implementation. Instead,
     * it uses fgetc() to read the character. If that fails due to a read
     * error or EOF, this will throw.
     *
     * @throws ostd::stream_error with EIO on failure.
     *
     * @see put_char()
     */
    int get_char();

    /** @brief Writes a single character into the stream.
     *
     * Does not use write_bytes() like the default implementation. Instead,
     * it uses fputc() to write the character. If that fails for any reason,
     * it throws.
     *
     * @throws ostd::stream_error with EIO on failure.
     *
     * @see get_char()
     */
    void put_char(int c);

    /** @brief Swaps two file streams including ownership. */
    void swap(file_stream &s) {
        using std::swap;
        swap(p_f, s.p_f);
        swap(p_owned, s.p_owned);
    }

    /** @brief Gets an underlying C `FILE` pointer backing the stream.
     *
     * This returns an associated `FILE` pointer (if opened) or a null
     * pointer (when no resource is associated with this stream).
     *
     * Ownership does not matter in this case. If you're getting a pointer
     * for a file stream that owns it though, make sure not to close it.
     */
    FILE *get_file() const { return p_f; }

private:
    FILE *p_f;
    bool p_owned;
};

/** @brief Swaps two file streams including ownership. */
inline void swap(file_stream &a, file_stream &b) {
    a.swap(b);
}

/** @brief Standard input file stream. */
OSTD_EXPORT extern file_stream cin;

/** @brief Standard output file stream. */
OSTD_EXPORT extern file_stream cout;

/** @brief Standard error file stream. */
OSTD_EXPORT extern file_stream cerr;

/* no need to call anything from file_stream, prefer simple calls... */

namespace detail {
    /* lightweight output range for direct stdout */
    struct stdout_range: output_range<stdout_range> {
        using value_type = char;
        using reference  = char &;
        using size_type  = std::size_t;

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

/** @brief Writes all given values into standard output.
 *
 * Behaves the same as calling ostd::stream::write() on ostd::cout,
 * but with more convenience.
 *
 * @see ostd::writeln(), ostd::writef(), ostd::writefln()
 */
template<typename ...A>
inline void write(A const &...args) {
    format_spec sp{'s', cout.getloc()};
    (sp.format_value(detail::stdout_range{}, args), ...);
}

/** @brief Writes all given values into standard output followed by a newline.
 *
 * Behaves the same as calling ostd::stream::writeln() on ostd::cout,
 * but with more convenience.
 *
 * @see ostd::write(), ostd::writef(), ostd::writefln()
 */
template<typename ...A>
inline void writeln(A const &...args) {
    write(args...);
    if (std::putchar('\n') == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
}

/** @brief Writes a formatted string into standard output.
 *
 * Behaves the same as calling ostd::stream::writef() on ostd::cout,
 * but with more convenience.
 *
 * @see ostd::writefln(), ostd::write(), ostd::writeln()
 */
template<typename ...A>
inline void writef(string_range fmt, A const &...args) {
    format_spec sp{fmt, cout.getloc()};
    sp.format(detail::stdout_range{}, args...);
}

/** @brief Writes a formatted string into standard output followed by a newline.
 *
 * Behaves the same as calling ostd::stream::writefln() on ostd::cout,
 * but with more convenience.
 *
 * @see ostd::writef(), ostd::write(), ostd::writeln()
 */
template<typename ...A>
inline void writefln(string_range fmt, A const &...args) {
    writef(fmt, args...);
    if (std::putchar('\n') == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
