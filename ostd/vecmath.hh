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
    Vec2(Vec2 const &v): x(v.x), y(v.y) {}
    Vec2(T v): x(v), y(v) {}
    Vec2(T x, T y): x(x), y(y) {}

    T &operator[](Size idx)       { return value[idx]; }
    T  operator[](Size idx) const { return value[idx]; }

    Vec2 &add(T v) {
        x += v; y += v;
        return *this;
    }
    Vec2 &add(Vec2 const &o) {
        x += o.x; y += o.y;
        return *this;
    }

    Vec2 &sub(T v) {
        x -= v; y -= v;
        return *this;
    }
    Vec2 &sub(Vec2 const &o) {
        x -= o.x; y -= o.y;
        return *this;
    }

    Vec2 &mul(T v) {
        x *= v; y *= v;
        return *this;
    }
    Vec2 &mul(Vec2 const &o) {
        x *= o.x; y *= o.y;
        return *this;
    }

    Vec2 &div(T v) {
        x /= v; y /= v;
        return *this;
    }
    Vec2 &div(Vec2 const &o) {
        x /= o.x; y /= o.y;
        return *this;
    }

    Vec2 &neg() {
        x = -x; y = -y;
        return *this;
    }

    bool is_zero() const {
        return (x == 0) && (y == 0);
    }

    T dot(Vec2<T> const &o) const {
        return (x * o.x) + (y * o.y);
    }
};

template<typename T>
inline bool operator==(Vec2<T> const &a, Vec2<T> const &b) {
    return (a.x == b.x) && (a.y == b.y);
}

template<typename T>
inline bool operator!=(Vec2<T> const &a, Vec2<T> const &b) {
    return (a.x != b.x) || (a.y != b.y);
}

template<typename T>
inline Vec2<T> operator+(Vec2<T> const &a, Vec2<T> const &b) {
    return Vec2<T>(a).add(b);
}

template<typename T>
inline Vec2<T> operator+(Vec2<T> const &a, T b) {
    return Vec2<T>(a).add(b);
}

template<typename T>
inline Vec2<T> operator-(Vec2<T> const &a, Vec2<T> const &b) {
    return Vec2<T>(a).sub(b);
}

template<typename T>
inline Vec2<T> operator-(Vec2<T> const &a, T b) {
    return Vec2<T>(a).sub(b);
}

template<typename T>
inline Vec2<T> operator*(Vec2<T> const &a, Vec2<T> const &b) {
    return Vec2<T>(a).mul(b);
}

template<typename T>
inline Vec2<T> operator*(Vec2<T> const &a, T b) {
    return Vec2<T>(a).mul(b);
}

template<typename T>
inline Vec2<T> operator/(Vec2<T> const &a, Vec2<T> const &b) {
    return Vec2<T>(a).div(b);
}

template<typename T>
inline Vec2<T> operator/(Vec2<T> const &a, T b) {
    return Vec2<T>(a).div(b);
}

template<typename T>
inline Vec2<T> operator-(Vec2<T> const &a) {
    return Vec2<T>(a).neg();
}

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2b = Vec2<byte>;
using Vec2i = Vec2<int>;

template<typename T>
struct Vec3 {
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };
        T value[3];
    };

    Vec3(): x(0), y(0), z(0) {}
    Vec3(Vec3 const &v): x(v.x), y(v.y), z(v.z) {}
    Vec3(T v): x(v), y(v), z(v) {}
    Vec3(T x, T y, T z): x(x), y(y), z(z) {}

    T &operator[](Size idx)       { return value[idx]; }
    T  operator[](Size idx) const { return value[idx]; }

    Vec3 &add(T v) {
        x += v; y += v; z += v;
        return *this;
    }
    Vec3 &add(Vec3 const &o) {
        x += o.x; y += o.y; z += o.z;
        return *this;
    }

    Vec3 &sub(T v) {
        x -= v; y -= v; z -= v;
        return *this;
    }
    Vec3 &sub(Vec3 const &o) {
        x -= o.x; y -= o.y; z -= o.z;
        return *this;
    }

    Vec3 &mul(T v) {
        x *= v; y *= v; z *= v;
        return *this;
    }
    Vec3 &mul(Vec3 const &o) {
        x *= o.x; y *= o.y; z *= o.z;
        return *this;
    }

    Vec3 &div(T v) {
        x /= v; y /= v; z /= v;
        return *this;
    }
    Vec3 &div(Vec3 const &o) {
        x /= o.x; y /= o.y; z /= o.z;
        return *this;
    }

    Vec3 &neg() {
        x = -x; y = -y; z = -z;
        return *this;
    }

    bool is_zero() const {
        return (x == 0) && (y == 0) && (z == 0);
    }

    T dot(Vec3<T> const &o) const {
        return (x * o.x) + (y * o.y) + (z * o.z);
    }
};

template<typename T>
inline bool operator==(Vec3<T> const &a, Vec3<T> const &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

template<typename T>
inline bool operator!=(Vec3<T> const &a, Vec3<T> const &b) {
    return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

template<typename T>
inline Vec3<T> operator+(Vec3<T> const &a, Vec3<T> const &b) {
    return Vec3<T>(a).add(b);
}

template<typename T>
inline Vec3<T> operator+(Vec3<T> const &a, T b) {
    return Vec3<T>(a).add(b);
}

template<typename T>
inline Vec3<T> operator-(Vec3<T> const &a, Vec3<T> const &b) {
    return Vec3<T>(a).sub(b);
}

template<typename T>
inline Vec3<T> operator-(Vec3<T> const &a, T b) {
    return Vec3<T>(a).sub(b);
}

template<typename T>
inline Vec3<T> operator*(Vec3<T> const &a, Vec3<T> const &b) {
    return Vec3<T>(a).mul(b);
}

template<typename T>
inline Vec3<T> operator*(Vec3<T> const &a, T b) {
    return Vec3<T>(a).mul(b);
}

template<typename T>
inline Vec3<T> operator/(Vec3<T> const &a, Vec3<T> const &b) {
    return Vec3<T>(a).div(b);
}

template<typename T>
inline Vec3<T> operator/(Vec3<T> const &a, T b) {
    return Vec3<T>(a).div(b);
}

template<typename T>
inline Vec3<T> operator-(Vec3<T> const &a) {
    return Vec3<T>(a).neg();
}

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Vec3b = Vec3<byte>;
using Vec3i = Vec3<int>;

template<typename T>
struct Vec4 {
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
        T value[4];
    };

    Vec4(): x(0), y(0), z(0), w(0) {}
    Vec4(Vec4 const &v): x(v.x), y(v.y), z(v.z), w(v.w) {}
    Vec4(T v): x(v), y(v), z(v), w(v) {}
    Vec4(T x, T y, T z, T w): x(x), y(y), z(z), w(w) {}

    T &operator[](Size idx)       { return value[idx]; }
    T  operator[](Size idx) const { return value[idx]; }

    Vec4 &add(T v) {
        x += v; y += v; z += v; w += v;
        return *this;
    }
    Vec4 &add(Vec4 const &o) {
        x += o.x; y += o.y; z += o.z; w += o.w;
        return *this;
    }

    Vec4 &sub(T v) {
        x -= v; y -= v; z -= v; w -= v;
        return *this;
    }
    Vec4 &sub(Vec4 const &o) {
        x -= o.x; y -= o.y; z -= o.z; w -= o.w;
        return *this;
    }

    Vec4 &mul(T v) {
        x *= v; y *= v; z *= v; w *= v;
        return *this;
    }
    Vec4 &mul(Vec4 const &o) {
        x *= o.x; y *= o.y; z *= o.z; w *= o.w;
        return *this;
    }

    Vec4 &div(T v) {
        x /= v; y /= v; z /= v; w /= v;
        return *this;
    }
    Vec4 &div(Vec4 const &o) {
        x /= o.x; y /= o.y; z /= o.z; w /= o.w;
        return *this;
    }

    Vec4 &neg() {
        x = -x; y = -y; z = -z; w = -w;
        return *this;
    }

    bool is_zero() const {
        return (x == 0) && (y == 0) && (z == 0) && (w == 0);
    }

    T dot(Vec4<T> const &o) const {
        return (x * o.x) + (y * o.y) + (z * o.z) + (w * o.w);
    }
};

template<typename T>
inline bool operator==(Vec4<T> const &a, Vec4<T> const &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}

template<typename T>
inline bool operator!=(Vec4<T> const &a, Vec4<T> const &b) {
    return (a.x != b.x) || (a.y != b.y) || (a.z != b.z) || (a.w != b.w);
}

template<typename T>
inline Vec4<T> operator+(Vec4<T> const &a, Vec4<T> const &b) {
    return Vec4<T>(a).add(b);
}

template<typename T>
inline Vec4<T> operator+(Vec4<T> const &a, T b) {
    return Vec4<T>(a).add(b);
}

template<typename T>
inline Vec4<T> operator-(Vec4<T> const &a, Vec4<T> const &b) {
    return Vec4<T>(a).sub(b);
}

template<typename T>
inline Vec4<T> operator-(Vec4<T> const &a, T b) {
    return Vec4<T>(a).sub(b);
}

template<typename T>
inline Vec4<T> operator*(Vec4<T> const &a, Vec4<T> const &b) {
    return Vec4<T>(a).mul(b);
}

template<typename T>
inline Vec4<T> operator*(Vec4<T> const &a, T b) {
    return Vec4<T>(a).mul(b);
}

template<typename T>
inline Vec4<T> operator/(Vec4<T> const &a, Vec4<T> const &b) {
    return Vec4<T>(a).div(b);
}

template<typename T>
inline Vec4<T> operator/(Vec4<T> const &a, T b) {
    return Vec4<T>(a).div(b);
}

template<typename T>
inline Vec4<T> operator-(Vec4<T> const &a) {
    return Vec4<T>(a).neg();
}

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;
using Vec4b = Vec4<byte>;
using Vec4i = Vec4<int>;

} /* namespace ostd */

#endif
