/* Vector math for libostd.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#ifndef OSTD_VECMATH_HH
#define OSTD_VECMATH_HH

namespace ostd {

template<typename T>
struct vec2 {
    union {
        struct { T x, y; };
        T value[2];
    };

    vec2(): x(0), y(0) {}
    vec2(vec2 const &v): x(v.x), y(v.y) {}
    vec2(T v): x(v), y(v) {}
    vec2(T x, T y): x(x), y(y) {}

    T &operator[](size_t idx)       { return value[idx]; }
    T  operator[](size_t idx) const { return value[idx]; }

    vec2 &add(T v) {
        x += v; y += v;
        return *this;
    }
    vec2 &add(vec2 const &o) {
        x += o.x; y += o.y;
        return *this;
    }

    vec2 &sub(T v) {
        x -= v; y -= v;
        return *this;
    }
    vec2 &sub(vec2 const &o) {
        x -= o.x; y -= o.y;
        return *this;
    }

    vec2 &mul(T v) {
        x *= v; y *= v;
        return *this;
    }
    vec2 &mul(vec2 const &o) {
        x *= o.x; y *= o.y;
        return *this;
    }

    vec2 &div(T v) {
        x /= v; y /= v;
        return *this;
    }
    vec2 &div(vec2 const &o) {
        x /= o.x; y /= o.y;
        return *this;
    }

    vec2 &neg() {
        x = -x; y = -y;
        return *this;
    }

    bool is_zero() const {
        return (x == 0) && (y == 0);
    }

    T dot(vec2<T> const &o) const {
        return (x * o.x) + (y * o.y);
    }
};

template<typename T>
inline bool operator==(vec2<T> const &a, vec2<T> const &b) {
    return (a.x == b.x) && (a.y == b.y);
}

template<typename T>
inline bool operator!=(vec2<T> const &a, vec2<T> const &b) {
    return (a.x != b.x) || (a.y != b.y);
}

template<typename T>
inline vec2<T> operator+(vec2<T> const &a, vec2<T> const &b) {
    return vec2<T>(a).add(b);
}

template<typename T>
inline vec2<T> operator+(vec2<T> const &a, T b) {
    return vec2<T>(a).add(b);
}

template<typename T>
inline vec2<T> operator-(vec2<T> const &a, vec2<T> const &b) {
    return vec2<T>(a).sub(b);
}

template<typename T>
inline vec2<T> operator-(vec2<T> const &a, T b) {
    return vec2<T>(a).sub(b);
}

template<typename T>
inline vec2<T> operator*(vec2<T> const &a, vec2<T> const &b) {
    return vec2<T>(a).mul(b);
}

template<typename T>
inline vec2<T> operator*(vec2<T> const &a, T b) {
    return vec2<T>(a).mul(b);
}

template<typename T>
inline vec2<T> operator/(vec2<T> const &a, vec2<T> const &b) {
    return vec2<T>(a).div(b);
}

template<typename T>
inline vec2<T> operator/(vec2<T> const &a, T b) {
    return vec2<T>(a).div(b);
}

template<typename T>
inline vec2<T> operator-(vec2<T> const &a) {
    return vec2<T>(a).neg();
}

using vec2f = vec2<float>;
using vec2d = vec2<double>;
using vec2b = vec2<unsigned char>;
using vec2i = vec2<int>;

template<typename T>
struct vec3 {
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };
        T value[3];
    };

    vec3(): x(0), y(0), z(0) {}
    vec3(vec3 const &v): x(v.x), y(v.y), z(v.z) {}
    vec3(T v): x(v), y(v), z(v) {}
    vec3(T x, T y, T z): x(x), y(y), z(z) {}

    T &operator[](size_t idx)       { return value[idx]; }
    T  operator[](size_t idx) const { return value[idx]; }

    vec3 &add(T v) {
        x += v; y += v; z += v;
        return *this;
    }
    vec3 &add(vec3 const &o) {
        x += o.x; y += o.y; z += o.z;
        return *this;
    }

    vec3 &sub(T v) {
        x -= v; y -= v; z -= v;
        return *this;
    }
    vec3 &sub(vec3 const &o) {
        x -= o.x; y -= o.y; z -= o.z;
        return *this;
    }

    vec3 &mul(T v) {
        x *= v; y *= v; z *= v;
        return *this;
    }
    vec3 &mul(vec3 const &o) {
        x *= o.x; y *= o.y; z *= o.z;
        return *this;
    }

    vec3 &div(T v) {
        x /= v; y /= v; z /= v;
        return *this;
    }
    vec3 &div(vec3 const &o) {
        x /= o.x; y /= o.y; z /= o.z;
        return *this;
    }

    vec3 &neg() {
        x = -x; y = -y; z = -z;
        return *this;
    }

    bool is_zero() const {
        return (x == 0) && (y == 0) && (z == 0);
    }

    T dot(vec3<T> const &o) const {
        return (x * o.x) + (y * o.y) + (z * o.z);
    }
};

template<typename T>
inline bool operator==(vec3<T> const &a, vec3<T> const &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

template<typename T>
inline bool operator!=(vec3<T> const &a, vec3<T> const &b) {
    return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

template<typename T>
inline vec3<T> operator+(vec3<T> const &a, vec3<T> const &b) {
    return vec3<T>(a).add(b);
}

template<typename T>
inline vec3<T> operator+(vec3<T> const &a, T b) {
    return vec3<T>(a).add(b);
}

template<typename T>
inline vec3<T> operator-(vec3<T> const &a, vec3<T> const &b) {
    return vec3<T>(a).sub(b);
}

template<typename T>
inline vec3<T> operator-(vec3<T> const &a, T b) {
    return vec3<T>(a).sub(b);
}

template<typename T>
inline vec3<T> operator*(vec3<T> const &a, vec3<T> const &b) {
    return vec3<T>(a).mul(b);
}

template<typename T>
inline vec3<T> operator*(vec3<T> const &a, T b) {
    return vec3<T>(a).mul(b);
}

template<typename T>
inline vec3<T> operator/(vec3<T> const &a, vec3<T> const &b) {
    return vec3<T>(a).div(b);
}

template<typename T>
inline vec3<T> operator/(vec3<T> const &a, T b) {
    return vec3<T>(a).div(b);
}

template<typename T>
inline vec3<T> operator-(vec3<T> const &a) {
    return vec3<T>(a).neg();
}

using vec3f = vec3<float>;
using vec3d = vec3<double>;
using vec3b = vec3<unsigned char>;
using vec3i = vec3<int>;

template<typename T>
struct vec4 {
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
        T value[4];
    };

    vec4(): x(0), y(0), z(0), w(0) {}
    vec4(vec4 const &v): x(v.x), y(v.y), z(v.z), w(v.w) {}
    vec4(T v): x(v), y(v), z(v), w(v) {}
    vec4(T x, T y, T z, T w): x(x), y(y), z(z), w(w) {}

    T &operator[](size_t idx)       { return value[idx]; }
    T  operator[](size_t idx) const { return value[idx]; }

    vec4 &add(T v) {
        x += v; y += v; z += v; w += v;
        return *this;
    }
    vec4 &add(vec4 const &o) {
        x += o.x; y += o.y; z += o.z; w += o.w;
        return *this;
    }

    vec4 &sub(T v) {
        x -= v; y -= v; z -= v; w -= v;
        return *this;
    }
    vec4 &sub(vec4 const &o) {
        x -= o.x; y -= o.y; z -= o.z; w -= o.w;
        return *this;
    }

    vec4 &mul(T v) {
        x *= v; y *= v; z *= v; w *= v;
        return *this;
    }
    vec4 &mul(vec4 const &o) {
        x *= o.x; y *= o.y; z *= o.z; w *= o.w;
        return *this;
    }

    vec4 &div(T v) {
        x /= v; y /= v; z /= v; w /= v;
        return *this;
    }
    vec4 &div(vec4 const &o) {
        x /= o.x; y /= o.y; z /= o.z; w /= o.w;
        return *this;
    }

    vec4 &neg() {
        x = -x; y = -y; z = -z; w = -w;
        return *this;
    }

    bool is_zero() const {
        return (x == 0) && (y == 0) && (z == 0) && (w == 0);
    }

    T dot(vec4<T> const &o) const {
        return (x * o.x) + (y * o.y) + (z * o.z) + (w * o.w);
    }
};

template<typename T>
inline bool operator==(vec4<T> const &a, vec4<T> const &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}

template<typename T>
inline bool operator!=(vec4<T> const &a, vec4<T> const &b) {
    return (a.x != b.x) || (a.y != b.y) || (a.z != b.z) || (a.w != b.w);
}

template<typename T>
inline vec4<T> operator+(vec4<T> const &a, vec4<T> const &b) {
    return vec4<T>(a).add(b);
}

template<typename T>
inline vec4<T> operator+(vec4<T> const &a, T b) {
    return vec4<T>(a).add(b);
}

template<typename T>
inline vec4<T> operator-(vec4<T> const &a, vec4<T> const &b) {
    return vec4<T>(a).sub(b);
}

template<typename T>
inline vec4<T> operator-(vec4<T> const &a, T b) {
    return vec4<T>(a).sub(b);
}

template<typename T>
inline vec4<T> operator*(vec4<T> const &a, vec4<T> const &b) {
    return vec4<T>(a).mul(b);
}

template<typename T>
inline vec4<T> operator*(vec4<T> const &a, T b) {
    return vec4<T>(a).mul(b);
}

template<typename T>
inline vec4<T> operator/(vec4<T> const &a, vec4<T> const &b) {
    return vec4<T>(a).div(b);
}

template<typename T>
inline vec4<T> operator/(vec4<T> const &a, T b) {
    return vec4<T>(a).div(b);
}

template<typename T>
inline vec4<T> operator-(vec4<T> const &a) {
    return vec4<T>(a).neg();
}

using vec4f = vec4<float>;
using vec4d = vec4<double>;
using vec4b = vec4<unsigned char>;
using vec4i = vec4<int>;

} /* namespace ostd */

#endif
