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

    Vec2(): x(0), y(0) {}
    Vec2(const Vec2 &v): x(v.x), y(v.y) {}
    Vec2(T v): x(v), y(v) {}
    Vec2(T x, T y): x(x), y(y) {}

    T &operator[](Size idx)       { return value[idx]; }
    T  operator[](Size idx) const { return value[idx]; }
};

template<typename T>
inline bool operator==(const Vec2<T> &a, const Vec2<T> &b) {
    return (a.x == b.x) && (a.y == b.y);
}

template<typename T>
inline bool operator!=(const Vec2<T> &a, const Vec2<T> &b) {
    return (a.x != b.x) || (a.y != b.y);
}

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

    Vec3(): x(0), y(0), z(0) {}
    Vec3(const Vec3 &v): x(v.x), y(v.y), z(v.z) {}
    Vec3(T v): x(v), y(v), z(v) {}
    Vec3(T x, T y, T z): x(x), y(y), z(z) {}
};

template<typename T>
inline bool operator==(const Vec3<T> &a, const Vec3<T> &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

template<typename T>
inline bool operator!=(const Vec3<T> &a, const Vec3<T> &b) {
    return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

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

    Vec4(): x(0), y(0), z(0), w(0) {}
    Vec4(const Vec3 &v): x(v.x), y(v.y), z(v.z), w(v.w) {}
    Vec4(T v): x(v), y(v), z(v), w(v) {}
    Vec4(T x, T y, T z, T w): x(x), y(y), z(z), w(w) {}
};

template<typename T>
inline bool operator==(const Vec4<T> &a, const Vec4<T> &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}

template<typename T>
inline bool operator!=(const Vec4<T> &a, const Vec4<T> &b) {
    return (a.x != b.x) || (a.y != b.y) || (a.z != b.z) || (a.w != b.w);
}

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;
using Vec4b = Vec4<byte>;
using Vec4s = Vec4<short>;
using Vec4i = Vec4<int>;

} /* namespace ostd */

#endif