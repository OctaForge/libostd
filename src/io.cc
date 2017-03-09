/* I/O streams implementation bits.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include "ostd/io.hh"

namespace ostd {

static char const *filemodes[] = {
    "rb", "wb", "ab", "rb+", "wb+", "ab+"
};

bool file_stream::open(string_range path, stream_mode mode) {
    if (p_f || (path.size() > FILENAME_MAX)) {
        return false;
    }
    char buf[FILENAME_MAX + 1];
    std::memcpy(buf, &path[0], path.size());
    buf[path.size()] = '\0';
    p_owned = false;
#ifndef OSTD_PLATFORM_WIN32
    p_f = std::fopen(buf, filemodes[size_t(mode)]);
#else
    if (fopen_s(&p_f, buf, filemodes[size_t(mode)]) != 0) {
        return false;
    }
#endif
    p_owned = !!p_f;
    return is_open();
}

bool file_stream::open(FILE *f) {
    if (p_f) {
        return false;
    }
    p_f = f;
    p_owned = false;
    return is_open();
}

void file_stream::close() {
    if (p_f && p_owned) {
        std::fclose(p_f);
    }
    p_f = nullptr;
    p_owned = false;
}

bool file_stream::end() const {
    return std::feof(p_f) != 0;
}

void file_stream::seek(stream_off_t pos, stream_seek whence) {
#ifndef OSTD_PLATFORM_WIN32
    if (fseeko(p_f, pos, int(whence)))
#else
    if (_fseeki64(p_f, pos, int(whence)))
#endif
    {
        throw stream_error{errno, std::generic_category()};
    }
}

stream_off_t file_stream::tell() const {
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

void file_stream::flush() {
    if (std::fflush(p_f)) {
        throw stream_error{EIO, std::generic_category()};
    }
}

void file_stream::read_bytes(void *buf, size_t count) {
    if (std::fread(buf, 1, count, p_f) != count) {
        throw stream_error{EIO, std::generic_category()};
    }
}

void file_stream::write_bytes(void const *buf, size_t count) {
    if (std::fwrite(buf, 1, count, p_f) != count) {
        throw stream_error{EIO, std::generic_category()};
    }
}

int file_stream::get_char() {
    int ret = std::fgetc(p_f);
    if (ret == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
    return ret;
}

void file_stream::put_char(int c) {
    if (std::fputc(c, p_f) == EOF) {
        throw stream_error{EIO, std::generic_category()};
    }
}

OSTD_EXPORT file_stream cin{stdin};
OSTD_EXPORT file_stream cout{stdout};
OSTD_EXPORT file_stream cerr{stderr};

} /* namespace ostd */
