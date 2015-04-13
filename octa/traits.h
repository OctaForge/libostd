/* Type traits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TRAITS_H
#define OCTA_TRAITS_H

#include "octa/types.h"

namespace octa {
    template<typename T, T val>
    struct IntegralConstant {
        static const T value = val;

        typedef T value_type;
        typedef IntegralConstant<T, v> type;
    };

    typedef IntegralConstant<bool, true>  true_t;
    typedef IntegralConstant<bool, false> false_t;

    template<typename T, T val> const T IntegralConstant<T, val>::value;

    template<typename T> struct IsInteger: false_t {};
    template<typename T> struct IsFloat  : false_t {};
    template<typename T> struct IsPointer: false_t {};

    template<> struct IsInteger<bool  >: true_t {};
    template<> struct IsInteger<char  >: true_t {};
    template<> struct IsInteger<uchar >: true_t {};
    template<> struct IsInteger<schar >: true_t {};
    template<> struct IsInteger<short >: true_t {};
    template<> struct IsInteger<ushort>: true_t {};
    template<> struct IsInteger<int   >: true_t {};
    template<> struct IsInteger<uint  >: true_t {};
    template<> struct IsInteger<long  >: true_t {};
    template<> struct IsInteger<ulong >: true_t {};
    template<> struct IsInteger<llong >: true_t {};
    template<> struct IsInteger<ullong>: true_t {};

    template<> struct IsFloat<float>: true_t {};
    template<> struct IsFloat<double>: true_t {};
    template<> struct IsFloat<ldouble>: true_t {};

    template<> struct IsPointer<T *>: true_t {};

    template<typename T> struct IsPOD: IntegralConstant<bool,
        (IsInteger<T>::value || IsFloat<T>::value || IsPointer<T>::value)
    > {};

    template<typename, typename>
    struct IsEqual {
        static const bool value = false;
    };

    template<typename T>
    struct IsEqual<T, T> {
        static const bool value = true;
    };
}

#endif