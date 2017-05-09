/* I/O streams implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include "ostd/io.hh"

namespace ostd {

static char const *filemodes[] = {
    "rb", "wb", "ab", "rb+", "wb+", "ab+"
};

static void default_close(FILE *f) {
    std::fclose(f);
}

OSTD_EXPORT bool file_stream::open(string_range path, stream_mode mode) {
    if (p_f || (path.size() > FILENAME_MAX)) {
        return false;
    }
    char buf[FILENAME_MAX + 1];
    std::memcpy(buf, &path[0], path.size());
    buf[path.size()] = '\0';
    p_closef = nullptr;
#ifndef OSTD_PLATFORM_WIN32
    p_f = std::fopen(buf, filemodes[std::size_t(mode)]);
#else
    if (fopen_s(&p_f, buf, filemodes[std::size_t(mode)]) != 0) {
        return false;
    }
#endif
    if (p_f) {
        p_closef = default_close;
    }
    return is_open();
}

OSTD_EXPORT bool file_stream::open(FILE *fptr, close_function f) {
    if (p_f) {
        return false;
    }
    p_f = fptr;
    p_closef = f;
    return is_open();
}

OSTD_EXPORT void file_stream::close() {
    if (p_f && p_closef) {
        p_closef(p_f);
    }
    p_f = nullptr;
    p_closef = nullptr;
}

OSTD_EXPORT bool file_stream::end() const {
    return std::feof(p_f) != 0;
}

OSTD_EXPORT void file_stream::seek(stream_off_t pos, stream_seek whence) {
#ifndef OSTD_PLATFORM_WIN32
    if (fseeko(p_f, pos, int(whence)))
#else
    if (_fseeki64(p_f, pos, int(whence)))
#endif
    {
        throw stream_error{errno, std::generic_category()};
    }
}

OSTD_EXPORT stream_off_t file_stream::tell() const {
#ifndef OSTD_PLATFORM_WIN32
    auto ret = ftello(p_f);
#else
    auto ret = _ftelli64(p_f);
#endif
    if (ret < 0) {
        throw stream_error{errno, std::generic_category()};
    }
    return ret;
}

OSTD_EXPORT void file_stream::flush() {
    if (std::fflush(p_f)) {
        throw stream_error{EIO, std::generic_category()};
    }
}

OSTD_EXPORT std::size_t file_stream::read_bytes(void *buf, std::size_t count) {
    std::size_t readn = std::fread(buf, 1, count, p_f);
    if (readn != count) {
        if (std::feof(p_f) != 0) {
            return readn;
        }
        throw stream_error{EIO, std::generic_category()};
    }
    return readn;
}

OSTD_EXPORT void file_stream::write_bytes(void const *buf, std::size_t count) {
    if (std::fwrite(buf, 1, count, p_f) != count) {
        throw stream_error{EIO, std::generic_category()};
    }
}

OSTD_EXPORT int file_stream::get_char() {
    int ret = std::fgetc(p_f);
    if (ret == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
    return ret;
}

OSTD_EXPORT void file_stream::put_char(int c) {
    if (std::fputc(c, p_f) == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
}

OSTD_EXPORT file_stream cin{stdin};
OSTD_EXPORT file_stream cout{stdout};
OSTD_EXPORT file_stream cerr{stderr};

} /* namespace ostd */
