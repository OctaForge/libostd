/* SDL RWops integration.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_EXT_SDL_RWOPS_HH
#define OSTD_EXT_SDL_RWOPS_HH

#ifdef OSTD_EXT_SDL_USE_SDL1
#include <SDL/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "ostd/types.hh"
#include "ostd/stream.hh"

namespace ostd {
namespace sdl {

#ifdef OSTD_EXT_SDL_USE_SDL1
using SDLRWopsOffset = int;
#else
using SDLRWopsOffset = Int64;
#endif

inline SDL_RWops *stream_to_rwops(Stream &s) {
    SDL_RWops *rwr = SDL_AllocRW();
    if (!rwr) {
        return nullptr;
    }
    rwr->hidden.unknown.data1 = &s;
    rwr->size = [](SDL_RWops *rw) -> SDLRWopsOffset {
        Stream *is = static_cast<Stream *>(rw->hidden.unknown.data1);
        return static_cast<SDLRWopsOffset>(is->size());
    };
    rwr->seek = [](SDL_RWops *rw, SDLRWopsOffset pos, int whence) -> SDLRWopsOffset {
        Stream *is = static_cast<Stream *>(rw->hidden.unknown.data1);
        if (!pos && whence == SEEK_CUR)
            return static_cast<SDLRWopsOffset>(is->tell());
        if (is->seek(((StreamOffset)pos, (StreamSeek)whence))
            return static_cast<SDLRWopsOffset>(is->tell());
        return -1;
    };
    rwr->read = [](SDL_RWops *rw, void *buf, Size size, Size nb) -> Size {
        Stream *is = static_cast<Stream *>(rw->hidden.unknown.data1);
        return is->read_bytes(buf, size * nb) / size;
    };
    rwr->write = [](SDL_RWops *rw, const void *buf, Size size, Size nb) -> Size {
        Stream *is = static_cast<Stream *>(rw->hidden.unknown.data1);
        return is->write_bytes(buf, size * nb) / size;
    };
    rwr->close = [](SDL_RWops *) -> int { return 0; };
    return rwr;
}

}
}

#endif
