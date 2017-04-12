/** @defgroup Platform Platform support
 *
 * @brief Abstractions for platform (OS, toolchain) specific code.
 *
 * libostd is not only a simple utility library, it also aims to make writing
 * cross-platform code as simple as possible (while sticking to native features
 * and therefore not making your code feel foreign on the platform). This
 * module represents the base layer to achieve this.
 *
 * @{
 */

/** @file platform.hh
 *
 * @brief Platform specific definitions.
 *
 * This file defines various helper macros to deal with system and compiler
 * checks as well as API exports and byte order.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_PLATFORM_HH
#define OSTD_PLATFORM_HH

#include <cstddef>
#include <cstdint>

#include <type_traits>

/* only expanded when generating documentation, define all macros */
#ifdef OSTD_GENERATING_DOC

/// Defined on Windows x86 and x86_64, otherwise undefined.
#define OSTD_PLATFORM_WIN32

/// Defined on Windows x64 only, otherwise undefined.
#define OSTD_PLATFORM_WIN64

/// Defined on all POSIX compliant systems, otherwise undefined.
#define OSTD_PLATFORM_POSIX

/// Defined on Linux.
#define OSTD_PLATFORM_LINUX

/// Defined on macOS.
#define OSTD_PLATFORM_OSX

/// Defined on DragonflyBSD.
#define OSTD_PLATFORM_DRAGONFLYBSD

/// Defined on FreeBSD.
#define OSTD_PLATFORM_FREEBSD

/// Defined on OpenBSD.
#define OSTD_PLATFORM_OPENBSD

/// Defined on NetBSD.
#define OSTD_PLATFORM_NETBSD

/// Defined on FreeBSD, NetBSD, OpenBSD and DragonflyBSD.
#define OSTD_PLATFORM_BSD

/// Defined on Solaris, Illumos and derivatives.
#define OSTD_PLATFORM_SOLARIS

/// Defined when compiling with Clang.
#define OSTD_TOOLCHAIN_CLANG

/// Defined when compiling with GCC or a compatible compiler such as Clang.
#define OSTD_TOOLCHAIN_GNU

/// Defined when compiling with Microsoft Visual Studio.
#define OSTD_TOOLCHAIN_MSVC

/// The value of #OSTD_BYTE_ORDER on little-endian systems.
#define OSTD_ENDIAN_LIL

/// The value of #OSTD_BYTE_ORDER on big-endian systems.
#define OSTD_ENDIAN_BIG

/// The system's byte order, either #OSTD_ENDIAN_LIL or #OSTD_ENDIAN_BIG.
#define OSTD_BYTE_ORDER

/** @brief Use this to annotate externally visible API.
 *
 * On Windows, this will expand differently when building a library and when
 * using the resulting API. You can define `OSTD_BUILD_LIB` to declare that
 * you're currently building a static library (in which case this macro will
 * expand to nothing) or `OSTD_BUILD_DLL` to declare you're building a DLL
 * (in which case it will expand to `__declspec(dllexport)`).
 *
 * When not building a library, this will expand to `__declspec(dllimport)`.
 *
 * On POSIX compliant systems, there are two possibilities. If your compiler
 * supports it, it can expand to `__attribute__((visibility("default")))`.
 * Otherwise, it will be empty.
 *
 * ~~~{.cc}
 * OSTD_EXPORT void foo();
 *
 * class OSTD_EXPORT bar {
 *     OSTD_LOCAL void private_method(); // internal to the DSO
 *
 * public:
 *     void public_method();
 * };
 *
 * OSTD_EXPORT extern baz external_variable;
 * ~~~
 *
 * @see OSTD_LOCAL
 */
#define OSTD_EXPORT

/** @brief Use this to annotate APIs internal to the library.
 *
 * On Windows, this will always expand to nothing. On POSIX compliant systems,
 * it will expand to `__attribute__((visibility("hidden")))` on compliant
 * compilers, otherwise also nothing. This macro is useful typically to hide
 * private methods in classes that have been exported as whole. Otherwise,
 * APIs that are not annontated can be considered local by default (though
 * the actual visibility depends on your compiler flags).
 */
#define OSTD_LOCAL

/** @brief A convenience macro for the `cdecl` calling convention on Win32.
 *
 * This will expand to nothing on all platforms except 32-bit Windows, where
 * default calling convention can be overridden. You can use this to enforce
 * the `cdecl` calling convention for external function declarations where
 * necessary without having to write Windows specific code.
 */
#define OSTD_CDECL

#else /* OSTD_GENERATING_DOC */
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
#  if defined(OSTD_BUILD_LIB) || defined(OSTD_BUILD_DLL)
#    ifdef OSTD_BUILD_DLL
#      define OSTD_EXPORT __declspec(dllexport)
#    else
#      define OSTD_EXPORT
#    endif
#  else
#    define OSTD_EXPORT __declspec(dllimport)
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

#if defined(OSTD_PLATFORM_WIN32) && !defined(OSTD_PLATFORM_WIN64)
#  define OSTD_CDECL __cdecl
#else
#  define OSTD_CDECL
#endif
#endif /* OSTD_GENERATING_DOC */

namespace ostd {

/** @addtogroup Platform
 * @{
 */

#if defined(OSTD_TOOLCHAIN_GNU) && !defined(OSTD_GENERATING_DOC)

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

#elif defined(OSTD_TOOLCHAIN_MSVC) && !defined(OSTD_GENERATING_DOC)

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

/** @brief 16-bit byte swap. */
inline std::uint16_t endian_swap16(std::uint16_t x) noexcept {
    return (x << 8) | (x >> 8);
}

/** @brief 32-bit byte swap. */
inline std::uint32_t endian_swap32(std::uint32_t x) noexcept {
    return (x << 24) | (x >> 24) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000);
}

/** @brief 64-bit byte swap. */
inline std::uint64_t endian_swap64(std::uint64_t x) noexcept {
    return endian_swap32(
        std::uint32_t(x >> 32)) |
            (std::uint64_t(endian_swap32(std::uint32_t(x))) << 32
    );
}

#endif

/* endian swap */

template<
    typename T, std::size_t N = sizeof(T),
    bool IsNum = std::is_arithmetic_v<T>
>
struct endian_swap;

template<typename T>
struct endian_swap<T, 2, true> {
    T operator()(T v) const {
        union { T iv; std::uint16_t sv; } u;
        u.iv = v;
        u.sv = endian_swap16(u.sv);
        return u.iv;
    }
};

template<typename T>
struct endian_swap<T, 4, true> {
    T operator()(T v) const {
        union { T iv; std::uint32_t sv; } u;
        u.iv = v;
        u.sv = endian_swap32(u.sv);
        return u.iv;
    }
};

template<typename T>
struct endian_swap<T, 8, true> {
    T operator()(T v) const {
        union { T iv; std::uint64_t sv; } u;
        u.iv = v;
        u.sv = endian_swap64(u.sv);
        return u.iv;
    }
};

template<typename T, bool IsNum = std::is_arithmetic_v<T>>
struct from_lil_endian;

template<typename T>
struct from_lil_endian<T, true> {
    T operator()(T v) const {
        if constexpr(OSTD_BYTE_ORDER == OSTD_ENDIAN_LIL) {
            return v;
        } else {
            return endian_swap<T>{}(v);
        }
    }
};

template<typename T, bool IsNum = std::is_arithmetic_v<T>>
struct from_big_endian;

template<typename T>
struct from_big_endian<T, true> {
    T operator()(T v) const {
        if constexpr(OSTD_BYTE_ORDER != OSTD_ENDIAN_LIL) {
            return v;
        } else {
            return endian_swap<T>{}(v);
        }
    }
};

/** @} */

}

#endif

/** @} */
