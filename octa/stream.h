/* Generic stream implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_STREAM_H
#define OCTA_STREAM_H

#include <stdio.h>
#include <sys/types.h>

#include "octa/types.h"
#include "octa/range.h"
#include "octa/string.h"
#include "octa/type_traits.h"

namespace octa {

/* off_t is POSIX - will also work on windows with mingw/clang, but FIXME */
using StreamOffset = off_t;

enum class StreamSeek {
    cur = SEEK_CUR,
    end = SEEK_END,
    set = SEEK_SET
};

template<typename T> struct Stream: InputRange<
    Stream<T>, octa::InputRangeTag, T, T, octa::Size, StreamOffset
> {
    using Offset = StreamOffset;

    virtual ~Stream() {}

    virtual void close() = 0;

    virtual bool end() const = 0;

    virtual Offset size() {
        Offset p = tell();
        if ((p < 0) || !seek(0, StreamSeek::end)) return -1;
        Offset e = tell();
        return (p == e) || (seek(p, StreamSeek::set) ? e : -1);
    }

    virtual bool seek(Offset, StreamSeek = StreamSeek::set) {
        return false;
    }

    virtual Offset tell() const { return -1; }

    virtual bool flush() { return true; }

    virtual octa::Size read(T *, octa::Size) { return 0; }
    virtual octa::Size write(const T *, octa::Size) { return 0; }

    /* range interface */

    bool empty() const { return end(); }

    bool pop_front() {
        if (empty()) return false;
        T val;
        return read(&val, 1) == sizeof(T);
    }

    T front() const {
        Stream *f = (Stream *)this;
        T val;
        f->seek(-f->read(&val, 1), StreamSeek::cur);
        return val;
    }

    virtual bool equals_front(const Stream &s) const {
        return s.tell() == tell();
    }

    void put(const T &val) {
        write(&val, 1);
    }
};

enum class StreamMode {
    read, write, append,
    update = 1 << 2
};

namespace detail {
    static const char *filemodes[] = {
        "rb", "wb", "ab", nullptr, "rb+", "wb+", "ab+"
    };
}

template<typename T> struct FileStream: Stream<T> {
    FileStream(): p_f(), p_owned(false) {}
    FileStream(const FileStream &s): p_f(s.p_f), p_owned(false) {}

    FileStream(const octa::String &path, StreamMode mode): p_f(), p_owned(false) {
        open(path, mode);
    }

    ~FileStream() { close(); }

    bool open(const octa::String &path, StreamMode mode) {
        if (p_f && !p_owned) return false;
        p_f = fopen(path.data(), octa::detail::filemodes[octa::Size(mode)]);
        p_owned = true;
        return p_f != nullptr;
    }

    void close() {
        if (p_owned && p_f) fclose(p_f);
        p_f = nullptr;
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

    octa::Size read(T *buf, octa::Size count) {
        return fread((void *)buf, 1, count * sizeof(T), p_f);
    }

    octa::Size write(const T *buf, octa::Size count) {
        return fwrite((const void *)buf, 1, count * sizeof(T), p_f);
    }

private:
    FILE *p_f;
    bool p_owned;
};

}

#endif