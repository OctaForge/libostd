/* Type traits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TRAITS_H
#define OCTA_TRAITS_H

#include <stddef.h>

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
        static constexpr const T value = val;

        typedef T value_type;
        typedef IntegralConstant<T, val> type;
    };

    typedef IntegralConstant<bool, true>  true_t;
    typedef IntegralConstant<bool, false> false_t;

    template<typename T, T val> constexpr const T IntegralConstant<T, val>::value;

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

    template<> struct IsFloatBase<float  >: true_t {};
    template<> struct IsFloatBase<double >: true_t {};
    template<> struct IsFloatBase<ldouble>: true_t {};

    template<typename T>
    struct IsFloat: IsFloatBase<typename RemoveConstVolatile<T>::type> {};

    /* is number */

    template<typename T> struct IsNumber: IntegralConstant<bool,
        (IsInteger<T>::value || IsFloat<T>::value)
    > {};

    /* is pointer */

    template<typename  > struct IsPointerBase     : false_t {};
    template<typename T> struct IsPointerBase<T *>:  true_t {};

    template<typename T>
    struct IsPointer: IsPointerBase<typename RemoveConstVolatile<T>::type> {};

    /* is pointer to member function */

    template<typename  > struct IsMemberPointerBase: false_t {};
    template<typename T, typename U>
    struct IsMemberPointerBase<T U::*>:  true_t {};

    template<typename T>
    struct IsMemberPointer: IsMemberPointerBase<
        typename RemoveConstVolatile<T>::type
    > {};

    /* is null pointer */

    template<typename> struct IsNullPointerBase           : false_t {};
    template<        > struct IsNullPointerBase<nullptr_t>:  true_t {};

    template<typename T>
    struct IsNullPointer: IsNullPointerBase<
        typename RemoveConstVolatile<T>::type
    > {};

    /* is POD: currently wrong */

    template<typename T> struct IsPOD: IntegralConstant<bool,
        (IsInteger<T>::value || IsFloat<T>::value || IsPointer<T>::value)
    > {};

    /* is class */

    struct IsClassBase {
        template<typename T> static char test(void (T::*)(void));
        template<typename  > static  int test(...);
    };

    template<typename T>
    struct IsClass: IntegralConstant<bool,
        sizeof(IsClassBase::test<T>(0)) == 1
    > {};

    /* type equality */

    template<typename, typename> struct IsEqual      : false_t {};
    template<typename T        > struct IsEqual<T, T>:  true_t {};

    /* is lvalue reference */

    template<typename  > struct IsLvalueReference     : false_t {};
    template<typename T> struct IsLvalueReference<T &>:  true_t {};

    /* is rvalue reference */

    template<typename  > struct IsRvalueReference      : false_t {};
    template<typename T> struct IsRvalueReference<T &&>:  true_t {};

    /* is reference */

    template<typename T> struct IsReference: IntegralConstant<bool,
        (IsLvalueReference<T>::value || IsRvalueReference<T>::value)
    > {};

    /* is array */

    template<typename            > struct IsArray      : false_t {};
    template<typename T          > struct IsArray<T[] >:  true_t {};
    template<typename T, size_t N> struct IsArray<T[N]>:  true_t {};

    /* move */

    template<typename T>
    static inline constexpr typename RemoveReference<T>::type &&
    move(T &&v) noexcept {
        return (typename RemoveReference<T>::type &&)v;
    }

    /* forward */

    template<typename T>
    static inline constexpr T &&
    forward(typename RemoveReference<T>::type &v) noexcept {
        return (T &&)v;
    }

    template<typename T>
    static inline constexpr T &&
    forward(typename RemoveReference<T>::type &&v) noexcept {
        return (T &&)v;
    }
}

#endif