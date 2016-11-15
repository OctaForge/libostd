/* Type limits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_LIMITS_HH
#define OSTD_LIMITS_HH

#include <limits.h>
#include <float.h>

#include "ostd/types.hh"
#include "ostd/type_traits.hh"

namespace ostd {

namespace detail {
    template<typename T, bool I = IsIntegral<T>, bool F = IsFloatingPoint<T>>
    struct NumericLimitsBase {
        static constexpr T minv = T{};
        static constexpr T maxv = T{};
        static constexpr T lowv = T{};
        static constexpr bool is_signed = false;
        static constexpr bool is_integer = false;
    };

    /* signed integer types */
    template<typename T, bool S = IsSigned<T>>
    struct IntegerLimitsBase {
        using UT = MakeUnsigned<T>;
        static constexpr T minv = -(~(1ULL << (sizeof(T) * CHAR_BIT - 1))) - 1;
        static constexpr T maxv = T(UT(~UT(0)) >> 1);
        static constexpr T lowv = minv;

        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
    };

    /* unsigned integer types */
    template<typename T>
    struct IntegerLimitsBase<T, false> {
        static constexpr T minv = T(0);
        static constexpr T maxv = ~T(0);
        static constexpr T lowv = minv;

        static constexpr bool is_signed = false;
        static constexpr bool is_integer = true;
    };

    /* all integer types */
    template<typename T>
    struct NumericLimitsBase<T, true, false>: IntegerLimitsBase<T> {};

    /* floating point types */
    template<typename T>
    struct FloatLimitsBase;

    template<>
    struct FloatLimitsBase<float> {
        static constexpr float minv = FLT_MIN;
        static constexpr float maxv = FLT_MAX;
        static constexpr float lowv = -maxv;
    };

    template<>
    struct FloatLimitsBase<double> {
        static constexpr double minv = DBL_MIN;
        static constexpr double maxv = DBL_MAX;
        static constexpr double lowv = -maxv;
    };

    template<>
    struct FloatLimitsBase<long double> {
        static constexpr long double minv = LDBL_MIN;
        static constexpr long double maxv = LDBL_MAX;
        static constexpr long double lowv = -maxv;
    };

    template<typename T>
    struct NumericLimitsBase<T, false, true>: FloatLimitsBase<T> {
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = false;
    };
}

template<typename T>
static constexpr T NumericLimitMin = detail::NumericLimitsBase<T>::minv;

template<typename T>
static constexpr T NumericLimitMax = detail::NumericLimitsBase<T>::maxv;

template<typename T>
static constexpr T NumericLimitLowest = detail::NumericLimitsBase<T>::lowv;

template<typename T>
static constexpr bool NumericLimitIsSigned = detail::NumericLimitsBase<T>::is_signed;

template<typename T>
static constexpr bool NumericLimitIsInteger = detail::NumericLimitsBase<T>::is_integer;

}

#endif
