/* Type traits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TRAITS_H
#define OCTA_TRAITS_H

#include "octa/types.h"

namespace octa {
    /* conditional */

    template<bool cond, typename T, typename U>
    struct Conditional {
        typedef T type;
    };

    template<typename T, typename U>
    struct Conditional<false, T, U> {
        typedef U type;
    };

    /* removers */

    template<typename T> struct RemoveReference      { typedef T type; };
    template<typename T> struct RemoveReference<T&>  { typedef T type; };
    template<typename T> struct RemoveReference<T&&> { typedef T type; };

    template<typename T> struct RemoveConst          { typedef T type; };
    template<typename T> struct RemoveConst<const T> { typedef T type; };

    template<typename T> struct RemoveVolatile             { typedef T type; };
    template<typename T> struct RemoveVolatile<volatile T> { typedef T type; };

    template<typename T>
    struct RemoveConstVolatile {
        typedef typename RemoveVolatile<typename RemoveConst<T>::type>::type type;
    };

    /* integral constant */

    template<typename T, T val>
    struct IntegralConstant {
        static const T value = val;

        typedef T value_type;
        typedef IntegralConstant<T, v> type;
    };

    typedef IntegralConstant<bool, true>  true_t;
    typedef IntegralConstant<bool, false> false_t;

    template<typename T, T val> const T IntegralConstant<T, val>::value;

    /* is integer */

    template<typename T> struct IsIntegerBase: false_t {};

    template<> struct IsIntegerBase<bool  >: true_t {};
    template<> struct IsIntegerBase<char  >: true_t {};
    template<> struct IsIntegerBase<uchar >: true_t {};
    template<> struct IsIntegerBase<schar >: true_t {};
    template<> struct IsIntegerBase<short >: true_t {};
    template<> struct IsIntegerBase<ushort>: true_t {};
    template<> struct IsIntegerBase<int   >: true_t {};
    template<> struct IsIntegerBase<uint  >: true_t {};
    template<> struct IsIntegerBase<long  >: true_t {};
    template<> struct IsIntegerBase<ulong >: true_t {};
    template<> struct IsIntegerBase<llong >: true_t {};
    template<> struct IsIntegerBase<ullong>: true_t {};

    template<typename T>
    struct IsInteger: IsIntegerBase<typename RemoveConstVolatile<T>::type> {};

    /* is floating point */

    template<typename T> struct IsFloatBase  : false_t {};

    template<> struct IsFloatBase<float>: true_t {};
    template<> struct IsFloatBase<double>: true_t {};
    template<> struct IsFloatBase<ldouble>: true_t {};

    template<typename T>
    struct IsFloat: IsFloatBase<typename RemoveConstVolatile<T>::type> {};

    /* is pointer */

    template<typename T> struct IsPointerBase: false_t {};

    template<> struct IsPointerBase<T *>: true_t {};

    template<typename T>
    struct IsPointer: IsPointerBase<typename RemoveConstVolatile<T>::type> {};

    /* is POD: currently wrong */

    template<typename T> struct IsPOD: IntegralConstant<bool,
        (IsInteger<T>::value || IsFloat<T>::value || IsPointer<T>::value)
    > {};

    /* type equality */

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