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

namespace octa {

/* off_t is POSIX - will also work on windows with mingw/clang, but FIXME */
using StreamOffset = off_t;

enum class Seek {
    cur = SEEK_CUR,
    end = SEEK_END,
    set = SEEK_SET
};

struct Stream {
    using Offset = StreamOffset;

    virtual void close() = 0;

    virtual bool end() = 0;

    virtual Offset size() {
        Offset p = tell();
        if ((p < 0) || !seek(0, Seek::end)) return -1;
        Offset e = tell();
        return (p == e) || (seek(p, Seek::set) ? e : -1);
    }

    virtual bool seek(Offset pos, Seek whence = Seek::set) { return false; }

    virtual Offset tell() { return -1; }

    virtual bool flush() { return true; }
};

}

#endif