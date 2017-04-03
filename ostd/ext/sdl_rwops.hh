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

#include "ostd/stream.hh"

namespace ostd {
namespace sdl {

#ifdef OSTD_EXT_SDL_USE_SDL1
using sdl_rwops_off_t = int;
#else
using sdl_rwops_off_t = int64_t;
#endif

inline SDL_RWops *stream_to_rwops(stream &s) {
    SDL_RWops *rwr = SDL_AllocRW();
    if (!rwr) {
        return nullptr;
    }

    rwr->hidden.unknown.data1 = &s;

    rwr->size = [](SDL_RWops *rw) -> sdl_rwops_off_t {
        auto &is = *static_cast<stream *>(rw->hidden.unknown.data1);
        try {
            return static_cast<sdl_rwops_off_t>(is.size());
        } catch (stream_error const &) {
            return -1;
        }
    };

    rwr->seek = [](SDL_RWops *rw, sdl_rwops_off_t pos, int whence) ->
        sdl_rwops_off_t
    {
        auto &is = *static_cast<stream *>(rw->hidden.unknown.data1);
        try {
            if (!pos && whence == SEEK_CUR) {
                return static_cast<sdl_rwops_off_t(is.tell());
            }
            if (is->seek((stream_off_t(pos), stream_seek(whence)))) {
                return static_cast<sdl_rwops_off_t>(is.tell());
            }
        } catch (stream_error const &) {
            return -1;
        }
        return -1;
    };

    rwr->read = [](SDL_RWops *rw, void *buf, size_t size, size_t nb) ->
        size_t
    {
        auto &is = *static_cast<stream *>(rw->hidden.unknown.data1);
        try {
            return is.read_bytes(buf, size * nb) / size;
        } catch (stream_error const &) {
            return 0;
        }
    };

    rwr->write = [](SDL_RWops *rw, void const *buf, size_t size, size_t nb) ->
        size_t
    {
        stream &is = *static_cast<stream *>(rw->hidden.unknown.data1);
        try {
            return is->write_bytes(buf, size * nb) / size;
        } catch (stream_error const &) {
            return 0;
        }
    };

    rwr->close = [](SDL_RWops *) -> int {
        return 0;
    };

    return rwr;
}

}
}

#endif
