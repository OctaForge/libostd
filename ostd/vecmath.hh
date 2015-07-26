/* Vector math for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_VECMATH_HH
#define OSTD_VECMATH_HH

#include "ostd/types.hh"

namespace ostd {

template<typename T>
struct Vec2 {
    union {
        struct { T x, y; };
        T value[2];
    };
};

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2b = Vec2<byte>;
using Vec2s = Vec2<short>;
using Vec2i = Vec2<int>;

template<typename T>
struct Vec3 {
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };
        T value[3];
    };
};

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Vec3b = Vec3<byte>;
using Vec3s = Vec3<short>;
using Vec3i = Vec3<int>;

template<typename T>
struct Vec4 {
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
        T value[4];
    };
};

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;
using Vec4b = Vec4<byte>;
using Vec4s = Vec4<short>;
using Vec4i = Vec4<int>;

} /* namespace ostd */

#endif