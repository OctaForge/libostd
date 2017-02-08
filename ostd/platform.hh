/* Platform specific definitions for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_PLATFORM_HH
#define OSTD_PLATFORM_HH

#include <stdlib.h>
#include <cstdint>

#include <type_traits>

#if defined(WIN32) || defined(_WIN32) || (defined(__WIN32) && !defined(__CYGWIN__))
#  define OSTD_PLATFORM_WIN32 1
#  if defined(WIN64) || defined(_WIN64)
#    define OSTD_PLATFORM_WIN64 1
#  endif
#else
#  define OSTD_PLATFORM_POSIX 1
#  if defined(__linux__)
#    define OSTD_PLATFORM_LINUX 1
#  endif
#  if defined(__APPLE__)
#    define OSTD_PLATFORM_OSX 1
#  endif
#  if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#    define OSTD_PLATFORM_FREEBSD 1
#    define OSTD_PLATFORM_BSD 1
#  endif
#  if defined(__NetBSD__)
#    define OSTD_PLATFORM_NETBSD 1
#    define OSTD_PLATFORM_BSD 1
#  endif
#  if defined(__OpenBSD__)
#    define OSTD_PLATFORM_OPENBSD 1
#    define OSTD_PLATFORM_BSD 1
#  endif
#  if defined(__DragonFly__)
#    define OSTD_PLATFORM_DRAGONFLYBSD 1
#    define OSTD_PLATFORM_BSD 1
#  endif
#  if defined(sun) || defined(__sun)
#    define OSTD_PLATFORM_SOLARIS 1
#  endif
#endif

#if defined(__clang__)
#  define OSTD_TOOLCHAIN_CLANG 1
#endif

#if defined(__GNUC__)
#  define OSTD_TOOLCHAIN_GNU 1
#endif

#if defined(_MSC_VER)
#  define OSTD_TOOLCHAIN_MSVC 1
#endif

#define OSTD_ENDIAN_LIL 1234
#define OSTD_ENDIAN_BIG 4321

#ifdef OSTD_PLATFORM_LINUX
#  include <endian.h>
#  define OSTD_BYTE_ORDER __BYTE_ORDER
#else
#  if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__)  || defined(__MIPSEB__) || defined(__MIPSEB) || \
      defined(_MIPSEB) || defined(__ppc__) || defined(__POWERPC__) || \
      defined(_M_PPC) || defined(__sparc__)
#    define OSTD_BYTE_ORDER OSTD_ENDIAN_BIG
#  else
#    define OSTD_BYTE_ORDER OSTD_ENDIAN_LIL
#  endif
#endif

#ifdef OSTD_PLATFORM_WIN32
#  ifdef OSTD_LIBRARY_DLL
#    ifdef OSTD_TOOLCHAIN_GNU
#      define OSTD_EXPORT __attribute__((dllexport))
#    else
#      define OSTD_EXPORT __declspec(dllexport)
#    endif
#  else
#    ifdef OSTD_TOOLCHAIN_GNU
#      define OSTD_EXPORT __attribute__((dllimport))
#    else
#      define OSTD_EXPORT __declspec(dllimport)
#    endif
#  endif
#  define OSTD_LOCAL
#else
#  if __GNUC__ >= 4
#    define OSTD_EXPORT __attribute__((visibility("default")))
#    define OSTD_LOCAL  __attribute__((visibility("hidden")))
#  else
#    define OSTD_EXPORT
#    define OSTD_LOCAL
#  endif
#endif

namespace ostd {

#if defined(OSTD_TOOLCHAIN_GNU)

/* using gcc/clang builtins */
inline std::uint16_t endian_swap16(std::uint16_t x) noexcept {
    return __builtin_bswap16(x);
}
inline std::uint32_t endian_swap32(std::uint32_t x) noexcept {
    return __builtin_bswap32(x);
}
inline std::uint64_t endian_swap64(std::uint64_t x) noexcept {
    return __builtin_bswap64(x);
}

#elif defined(OSTD_TOOLCHAIN_MSVC)

/* using msvc builtins */
inline std::uint16_t endian_swap16(std::uint16_t x) noexcept {
    return _byteswap_ushort(x);
}
inline std::uint32_t endian_swap32(std::uint32_t x) noexcept {
    /* win64 is llp64 */
    return _byteswap_ulong(x);
}
inline std::uint64_t endian_swap64(std::uint64_t x) noexcept {
    return _byteswap_uint64(x);
}

#else

/* fallback */
inline std::uint16_t endian_swap16(std::uint16_t x) noexcept {
    return (x << 8) | (x >> 8);
}
inline std::uint32_t endian_swap32(std::uint32_t x) noexcept {
    return (x << 24) | (x >> 24) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000);
}
inline std::uint64_t endian_swap64(std::uint64_t x) noexcept {
    return endian_swap32(
        std::uint32_t(x >> 32)) |
            (std::uint64_t(endian_swap32(std::uint32_t(x))) << 32
    );
}

#endif

/* endian swap */

template<typename T, size_t N = sizeof(T), bool IsNum = std::is_arithmetic_v<T>>
struct EndianSwap;

template<typename T>
struct EndianSwap<T, 2, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; std::uint16_t sv; } u;
        u.iv = v;
        u.sv = endian_swap16(u.sv);
        return u.iv;
    }
};

template<typename T>
struct EndianSwap<T, 4, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; std::uint32_t sv; } u;
        u.iv = v;
        u.sv = endian_swap32(u.sv);
        return u.iv;
    }
};

template<typename T>
struct EndianSwap<T, 8, true> {
    using Argument = T;
    using Result = T;
    T operator()(T v) const {
        union { T iv; std::uint64_t sv; } u;
        u.iv = v;
        u.sv = endian_swap64(u.sv);
        return u.iv;
    }
};

template<typename T>
T endian_swap(T x) { return EndianSwap<T>()(x); }

namespace detail {
    template<
        typename T, size_t N = sizeof(T), bool IsNum = std::is_arithmetic_v<T>
    >
    struct EndianSame;

    template<typename T>
    struct EndianSame<T, 2, true> {
        using Argument = T;
        using Result = T;
        T operator()(T v) const { return v; }
    };
    template<typename T>
    struct EndianSame<T, 4, true> {
        using Argument = T;
        using Result = T;
        T operator()(T v) const { return v; }
    };
    template<typename T>
    struct EndianSame<T, 8, true> {
        using Argument = T;
        using Result = T;
        T operator()(T v) const { return v; }
    };
}

#if OSTD_BYTE_ORDER == OSTD_ENDIAN_LIL
template<typename T>
struct FromLilEndian: detail::EndianSame<T> {};
template<typename T>
struct FromBigEndian: EndianSwap<T> {};
#else
template<typename T>
struct FromLilEndian: EndianSwap<T> {};
template<typename T>
struct FromBigEndian: detail::EndianSame<T> {};
#endif

template<typename T>
T from_lil_endian(T x) { return FromLilEndian<T>()(x); }
template<typename T>
T from_big_endian(T x) { return FromBigEndian<T>()(x); }

}

#endif
