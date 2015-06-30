/* Standard I/O implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_IO_HH
#define OCTA_IO_HH

#include <stdio.h>

#include "octa/string.hh"
#include "octa/stream.hh"

namespace octa {

enum class StreamMode {
    read, write, append,
    update = 1 << 2
};

namespace detail {
    static const char *filemodes[] = {
        "rb", "wb", "ab", nullptr, "rb+", "wb+", "ab+"
    };
}

struct FileStream: Stream {
    FileStream(): p_f(), p_owned(false) {}
    FileStream(const FileStream &) = delete;
    FileStream(FileStream &&s): p_f(s.p_f), p_owned(s.p_owned) {
        s.p_f = nullptr;
        s.p_owned = false;
    }

    FileStream(const octa::String &path, StreamMode mode): p_f() {
        open(path, mode);
    }

    FileStream(FILE *f): p_f(f), p_owned(false) {}

    ~FileStream() { close(); }

    FileStream &operator=(const FileStream &) = delete;
    FileStream &operator=(FileStream &&s) {
        close();
        swap(s);
        return *this;
    }

    bool open(const octa::String &path, StreamMode mode) {
        if (p_f) return false;
        p_f = fopen(path.data(), octa::detail::filemodes[octa::Size(mode)]);
        p_owned = true;
        return is_open();
    }

    bool open(FILE *f) {
        if (p_f) return false;
        p_f = f;
        p_owned = false;
        return is_open();
    }

    bool is_open() const { return p_f != nullptr; }
    bool is_owned() const { return p_owned; }

    void close() {
        if (p_f && p_owned) fclose(p_f);
        p_f = nullptr;
        p_owned = false;
    }

    bool end() const {
        return feof(p_f) != 0;
    }

    bool seek(StreamOffset pos, StreamSeek whence = StreamSeek::set) {
        return fseeko(p_f, pos, int(whence)) >= 0;
    }

    StreamOffset tell() const {
        return ftello(p_f);
    }

    bool flush() { return !fflush(p_f); }

    using Stream::read;
    using Stream::write;

    octa::Size read(void *buf, octa::Size count) {
        return fread(buf, 1, count, p_f);
    }

    octa::Size write(const void *buf, octa::Size count) {
        return fwrite(buf, 1, count, p_f);
    }

    void swap(FileStream &s) {
        octa::swap(p_f, s.p_f);
        octa::swap(p_owned, s.p_owned);
    }

    FILE *get_file() { return p_f; }

private:
    FILE *p_f;
    bool p_owned;
};

static FileStream in(::stdin);
static FileStream out(::stdout);
static FileStream err(::stderr);

/* no need to call anything from FileStream, prefer simple calls... */

static inline void write(const char *s) {
    fputs(s, ::stdout);
}

static inline void write(const octa::String &s) {
    fwrite(s.data(), 1, s.size(), ::stdout);
}

static inline void writeln(const char *s) {
    octa::write(s);
    putc('\n', ::stdout);
}

static inline void writeln(const octa::String &s) {
    octa::write(s);
    putc('\n', ::stdout);
}

}

#endif