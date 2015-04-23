/* Type traits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TRAITS_H
#define OCTA_TRAITS_H

#include <stddef.h>

#include "octa/types.h"

namespace octa {
    /* forward declarations */

    template<typename> struct RemoveCV;
    template<typename> struct AddLvalueReference;
    template<typename> struct AddRvalueReference;
    template<typename> struct AddConst;
    template<typename> struct IsReference;
    template<typename> struct RemoveReference;
    template<typename> struct RemoveAllExtents;
    template<typename> struct IsTriviallyDefaultConstructible;

    template<typename...> struct CommonType;

    /* declval also defined here to avoid including utility.h */
    template<typename T> typename AddRvalueReference<T>::type __octa_declval();

    /* integral constant */

    template<typename T, T val>
    struct IntegralConstant {
        static constexpr T value = val;

        typedef T value_type;
        typedef IntegralConstant<T, val> type;

        constexpr operator value_type() const { return value; }
        constexpr value_type operator()() const { return value; }
    };

    typedef IntegralConstant<bool, true>  true_t;
    typedef IntegralConstant<bool, false> false_t;

    template<typename T, T val> constexpr T IntegralConstant<T, val>::value;

    /* is void */

    template<typename T> struct __OctaIsVoid      : false_t {};
    template<          > struct __OctaIsVoid<void>:  true_t {};

    template<typename T>
    struct IsVoid: __OctaIsVoid<typename RemoveCV<T>::type> {};

    /* is null pointer */

    template<typename> struct __OctaIsNullPointer           : false_t {};
    template<        > struct __OctaIsNullPointer<nullptr_t>:  true_t {};

    template<typename T> struct IsNullPointer: __OctaIsNullPointer<
        typename RemoveCV<T>::type
    > {};

    /* is integer */

    template<typename T> struct __OctaIsIntegral: false_t {};

    template<> struct __OctaIsIntegral<bool  >: true_t {};
    template<> struct __OctaIsIntegral<char  >: true_t {};
    template<> struct __OctaIsIntegral<uchar >: true_t {};
    template<> struct __OctaIsIntegral<schar >: true_t {};
    template<> struct __OctaIsIntegral<short >: true_t {};
    template<> struct __OctaIsIntegral<ushort>: true_t {};
    template<> struct __OctaIsIntegral<int   >: true_t {};
    template<> struct __OctaIsIntegral<uint  >: true_t {};
    template<> struct __OctaIsIntegral<long  >: true_t {};
    template<> struct __OctaIsIntegral<ulong >: true_t {};
    template<> struct __OctaIsIntegral<llong >: true_t {};
    template<> struct __OctaIsIntegral<ullong>: true_t {};

    template<typename T>
    struct IsIntegral: __OctaIsIntegral<typename RemoveCV<T>::type> {};

    /* is floating point */

    template<typename T> struct __OctaIsFloatingPoint: false_t {};

    template<> struct __OctaIsFloatingPoint<float  >: true_t {};
    template<> struct __OctaIsFloatingPoint<double >: true_t {};
    template<> struct __OctaIsFloatingPoint<ldouble>: true_t {};

    template<typename T>
    struct IsFloatingPoint: __OctaIsFloatingPoint<typename RemoveCV<T>::type> {};

    /* is array */

    template<typename            > struct IsArray      : false_t {};
    template<typename T          > struct IsArray<T[] >:  true_t {};
    template<typename T, size_t N> struct IsArray<T[N]>:  true_t {};

    /* is pointer */

    template<typename  > struct __OctaIsPointer     : false_t {};
    template<typename T> struct __OctaIsPointer<T *>:  true_t {};

    template<typename T>
    struct IsPointer: __OctaIsPointer<typename RemoveCV<T>::type> {};

    /* is lvalue reference */

    template<typename  > struct IsLvalueReference     : false_t {};
    template<typename T> struct IsLvalueReference<T &>:  true_t {};

    /* is rvalue reference */

    template<typename  > struct IsRvalueReference      : false_t {};
    template<typename T> struct IsRvalueReference<T &&>:  true_t {};

    /* is enum */

    template<typename T> struct IsEnum: IntegralConstant<bool, __is_enum(T)> {};

    /* is union */

    template<typename T> struct IsUnion: IntegralConstant<bool, __is_union(T)> {};

    /* is class */

    template<typename T> struct IsClass: IntegralConstant<bool, __is_class(T)> {};

    /* is function */

    struct __OctaFunctionTestDummy {};

    template<typename T> char __octa_function_test(T *);
    template<typename T> char __octa_function_test(__OctaFunctionTestDummy);
    template<typename T> int  __octa_function_test(...);

    template<typename T> T                       &__octa_function_source(int);
    template<typename T> __OctaFunctionTestDummy  __octa_function_source(...);

    template<typename T, bool = IsClass<T>::value || IsUnion<T>::value
                              || IsVoid<T>::value || IsReference<T>::value
                              || IsNullPointer<T>::value
    > struct __OctaIsFunction: IntegralConstant<bool,
        sizeof(__octa_function_test<T>(__octa_function_source<T>(0))) == 1
    > {};

    template<typename T> struct __OctaIsFunction<T, true>: false_t {};

    template<typename T> struct IsFunction: __OctaIsFunction<T> {};

    /* is arithmetic */

    template<typename T> struct IsArithmetic: IntegralConstant<bool,
        (IsIntegral<T>::value || IsFloatingPoint<T>::value)
    > {};

    /* is fundamental */

    template<typename T> struct IsFundamental: IntegralConstant<bool,
        (IsArithmetic<T>::value || IsVoid<T>::value || IsNullPointer<T>::value)
    > {};

    /* is compound */

    template<typename T> struct IsCompound: IntegralConstant<bool,
        !IsFundamental<T>::value
    > {};

    /* is pointer to member */

    template<typename>
    struct __OctaIsMemberPointer: false_t {};

    template<typename T, typename U>
    struct __OctaIsMemberPointer<T U::*>: true_t {};

    template<typename T>
    struct IsMemberPointer: __OctaIsMemberPointer<
        typename RemoveCV<T>::type
    > {};

    /* is pointer to member object */

    template<typename>
    struct __OctaIsMemberObjectPointer: false_t {};

    template<typename T, typename U>
    struct __OctaIsMemberObjectPointer<T U::*>: IntegralConstant<bool,
        !IsFunction<T>::value
    > {};

    template<typename T> struct IsMemberObjectPointer:
        __OctaIsMemberObjectPointer<typename RemoveCV<T>::type> {};

    /* is pointer to member function */

    template<typename>
    struct __OctaIsMemberFunctionPointer: false_t {};

    template<typename T, typename U>
    struct __OctaIsMemberFunctionPointer<T U::*>: IntegralConstant<bool,
        IsFunction<T>::value
    > {};

    template<typename T> struct IsMemberFunctionPointer:
        __OctaIsMemberFunctionPointer<typename RemoveCV<T>::type> {};

    /* is reference */

    template<typename T> struct IsReference: IntegralConstant<bool,
        (IsLvalueReference<T>::value || IsRvalueReference<T>::value)
    > {};

    /* is object */

    template<typename T> struct IsObject: IntegralConstant<bool,
        (!IsFunction<T>::value && !IsVoid<T>::value && !IsReference<T>::value)
    > {};

    /* is scalar */

    template<typename T> struct IsScalar: IntegralConstant<bool,
        (IsMemberPointer<T>::value || IsPointer<T>::value || IsEnum<T>::value
      || IsNullPointer  <T>::value || IsArithmetic<T>::value)
    > {};

    /* is abstract */

    template<typename T>
    struct IsAbstract: IntegralConstant<bool, __is_abstract(T)> {};

    /* is const */

    template<typename  > struct IsConst         : false_t {};
    template<typename T> struct IsConst<const T>:  true_t {};

    /* is volatile */

    template<typename  > struct IsVolatile            : false_t {};
    template<typename T> struct IsVolatile<volatile T>:  true_t {};

    /* is empty */

    template<typename T>
    struct IsEmpty: IntegralConstant<bool, __is_empty(T)> {};

    /* is POD */

    template<typename T> struct IsPOD: IntegralConstant<bool, __is_pod(T)> {};

    /* is polymorphic */

    template<typename T>
    struct IsPolymorphic: IntegralConstant<bool, __is_polymorphic(T)> {};

    /* is signed */

    template<typename T>
    struct IsSigned: IntegralConstant<bool, T(-1) < T(0)> {};

    /* is unsigned */

    template<typename T>
    struct IsUnsigned: IntegralConstant<bool, T(0) < T(-1)> {};

    /* is standard layout */

    template<typename T>
    struct IsStandardLayout: IntegralConstant<bool,
        IsScalar<typename RemoveAllExtents<T>::type>::value
    > {};

    /* is literal type */

    template<typename T>
    struct IsLiteralType: IntegralConstant<bool,
        IsReference<typename RemoveAllExtents<T>::type>::value
     || IsStandardLayout<T>::value
    > {};

    /* is trivially copyable */

    template<typename T>
    struct IsTriviallyCopyable: IntegralConstant<bool,
        IsScalar<typename RemoveAllExtents<T>::type>::value
    > {};

    /* is trivial */

    template<typename T>
    struct IsTrivial: IntegralConstant<bool,
        (IsTriviallyCopyable<T>::value && IsTriviallyDefaultConstructible<T>::value)
    > {};

    /* has virtual destructor */

    template<typename T>
    struct HasVirtualDestructor: IntegralConstant<bool,
        __has_virtual_destructor(T)
    > {};

    /* is constructible */

#define __OCTA_MOVE(v) static_cast<typename RemoveReference<decltype(v)>::type &&>(v)

    template<typename, typename T> struct __OctaSelect2nd { typedef T type; };
    struct __OctaAny { __OctaAny(...); };

    template<typename T, typename ...A> typename __OctaSelect2nd<
        decltype(__OCTA_MOVE(T(__octa_declval<A>()...))), true_t
    >::type __octa_is_ctible_test(T &&, A &&...);

#undef __OCTA_MOVE

    template<typename ...A> false_t __octa_is_ctible_test(__OctaAny, A &&...);

    template<bool, typename T, typename ...A>
    struct __OctaCtibleCore: CommonType<
        decltype(__octa_is_ctible_test(__octa_declval<T>(),
            __octa_declval<A>()...))
    >::type {};

    /* function types are not constructible */
    template<typename R, typename ...A1, typename ...A2>
    struct __OctaCtibleCore<false, R(A1...), A2...>: false_t {};

    /* scalars are default constructible, refs are not */
    template<typename T>
    struct __OctaCtibleCore<true, T>: IsScalar<T> {};

    /* scalars and references are constructible from one arg if
     * implicitly convertible to scalar or reference */
    template<typename T>
    struct __OctaCtibleRef {
        static true_t  test(T);
        static false_t test(...);
    };

    template<typename T, typename U>
    struct __OctaCtibleCore<true, T, U>: CommonType<
        decltype(__OctaCtibleRef<T>::test(__octa_declval<U>()))
    >::type {};

    /* scalars and references are not constructible from multiple args */
    template<typename T, typename U, typename ...A>
    struct __OctaCtibleCore<true, T, U, A...>: false_t {};

    /* treat scalars and refs separately */
    template<bool, typename T, typename ...A>
    struct __OctaCtibleVoidCheck: __OctaCtibleCore<
        (IsScalar<T>::value || IsReference<T>::value), T, A...
    > {};

    /* if any of T or A is void, IsConstructible should be false */
    template<typename T, typename ...A>
    struct __OctaCtibleVoidCheck<true, T, A...>: false_t {};

    template<typename ...A> struct __OctaCtibleContainsVoid;

    template<> struct __OctaCtibleContainsVoid<>: false_t {};

    template<typename T, typename ...A>
    struct __OctaCtibleContainsVoid<T, A...> {
        static constexpr bool value = IsVoid<T>::value
           || __OctaCtibleContainsVoid<A...>::value;
    };

    /* entry point */
    template<typename T, typename ...A>
    struct __OctaCtible: __OctaCtibleVoidCheck<
        __OctaCtibleContainsVoid<T, A...>::value || IsAbstract<T>::value,
        T, A...
    > {};

    /* array types are default constructible if their element type is */
    template<typename T, size_t N>
    struct __OctaCtibleCore<false, T[N]>: __OctaCtible<
        typename RemoveAllExtents<T>::type
    > {};

    /* otherwise array types are not constructible by this syntax */
    template<typename T, size_t N, typename ...A>
    struct __OctaCtibleCore<false, T[N], A...>: false_t {};

    /* incomplete array types are not constructible */
    template<typename T, typename ...A>
    struct __OctaCtibleCore<false, T[], A...>: false_t {};

    template<typename T, typename ...A>
    struct IsConstructible: __OctaCtible<T, A...> {};

    /* is default constructible */

    template<typename T> struct IsDefaultConstructible: IsConstructible<T> {};

    /* is copy constructible */

    template<typename T> struct IsCopyConstructible: IsConstructible<T,
        typename AddLvalueReference<typename AddConst<T>::type>::type
    > {};

    /* is move constructible */

    template<typename T> struct IsMoveConstructible: IsConstructible<T,
        typename AddRvalueReference<T>::type
    > {};

    /* is assignable */

    template<typename T, typename U> typename __OctaSelect2nd<
        decltype((__octa_declval<T>() = __octa_declval<U>())), true_t
    >::type __octa_assign_test(T &&, U &&);

    template<typename T> false_t __octa_assign_test(__OctaAny, T &&);

    template<typename T, typename U, bool = IsVoid<T>::value || IsVoid<U>::value>
    struct __OctaIsAssignable: CommonType<
        decltype(__octa_assign_test(__octa_declval<T>(), __octa_declval<U>()))
    >::type {};

    template<typename T, typename U>
    struct __OctaIsAssignable<T, U, true>: false_t {};

    template<typename T, typename U>
    struct IsAssignable: __OctaIsAssignable<T, U> {};

    /* is copy assignable */

    template<typename T> struct IsCopyAssignable: IsAssignable<
        typename AddLvalueReference<T>::type,
        typename AddLvalueReference<typename AddConst<T>::type>::type
    > {};

    /* is move assignable */

    template<typename T> struct IsMoveAssignable: IsAssignable<
        typename AddLvalueReference<T>::type,
        const typename AddRvalueReference<T>::type
    > {};

    /* is destructible */

    template<typename> struct __OctaIsDtibleApply { typedef int type; };

    template<typename T> struct IsDestructorWellformed {
        template<typename TT> static char test(typename __OctaIsDtibleApply<
            decltype(__octa_declval<TT &>().~TT())
        >::type);

        template<typename TT> static int test(...);

        static constexpr bool value = (sizeof(test<T>(12)) == sizeof(char));
    };

    template<typename, bool> struct __OctaDtibleImpl;

    template<typename T>
    struct __OctaDtibleImpl<T, false>: IntegralConstant<bool,
        IsDestructorWellformed<typename RemoveAllExtents<T>::type>::value
    > {};

    template<typename T>
    struct __OctaDtibleImpl<T, true>: true_t {};

    template<typename T, bool> struct __OctaDtibleFalse;

    template<typename T> struct __OctaDtibleFalse<T, false>
        : __OctaDtibleImpl<T, IsReference<T>::value> {};

    template<typename T> struct __OctaDtibleFalse<T, true>: false_t {};

    template<typename T>
    struct IsDestructible: __OctaDtibleFalse<T, IsFunction<T>::value> {};

    template<typename T> struct IsDestructible<T[] >: false_t {};
    template<          > struct IsDestructible<void>: false_t {};

    /* is trivially constructible */

    template<typename T, typename ...A>
    struct IsTriviallyConstructible: false_t {};

    template<typename T>
    struct IsTriviallyConstructible<T>: IntegralConstant<bool,
        __has_trivial_constructor(T)
    > {};

    template<typename T>
    struct IsTriviallyConstructible<T, T &>: IntegralConstant<bool,
        __has_trivial_copy(T)
    > {};

    template<typename T>
    struct IsTriviallyConstructible<T, const T &>: IntegralConstant<bool,
        __has_trivial_copy(T)
    > {};

    template<typename T>
    struct IsTriviallyConstructible<T, T &&>: IntegralConstant<bool,
        __has_trivial_copy(T)
    > {};

    /* is trivially default constructible */

    template<typename T>
    struct IsTriviallyDefaultConstructible: IsTriviallyConstructible<T> {};

    /* is trivially copy constructible */

    template<typename T>
    struct IsTriviallyCopyConstructible: IsTriviallyConstructible<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is trivially move constructible */

    template<typename T>
    struct IsTriviallyMoveConstructible: IsTriviallyConstructible<T,
        typename AddRvalueReference<T>::type
    > {};

    /* is trivially assignable */

    template<typename T, typename ...A>
    struct IsTriviallyAssignable: false_t {};

    template<typename T>
    struct IsTriviallyAssignable<T>: IntegralConstant<bool,
        __has_trivial_assign(T)
    > {};

    template<typename T>
    struct IsTriviallyAssignable<T, T &>: IntegralConstant<bool,
        __has_trivial_copy(T)
    > {};

    template<typename T>
    struct IsTriviallyAssignable<T, const T &>: IntegralConstant<bool,
        __has_trivial_copy(T)
    > {};

    template<typename T>
    struct IsTriviallyAssignable<T, T &&>: IntegralConstant<bool,
        __has_trivial_copy(T)
    > {};

    /* is trivially copy assignable */

    template<typename T>
    struct IsTriviallyCopyAssignable: IsTriviallyAssignable<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is trivially move assignable */

    template<typename T>
    struct IsTriviallyMoveAssignable: IsTriviallyAssignable<T,
        typename AddRvalueReference<T>::type
    > {};

    /* is trivially destructible */

    template<typename T>
    struct IsTriviallyDestructible: IntegralConstant<bool,
        __has_trivial_destructor(T)
    > {};

    /* is nothrow constructible */

    template<typename T, typename ...A>
    struct IsNothrowConstructible: false_t {};

    template<typename T>
    struct IsNothrowConstructible<T>: IntegralConstant<bool,
        __has_nothrow_constructor(T)
    > {};

    template<typename T>
    struct IsNothrowConstructible<T, T &>: IntegralConstant<bool,
        __has_nothrow_copy(T)
    > {};

    template<typename T>
    struct IsNothrowConstructible<T, const T &>: IntegralConstant<bool,
        __has_nothrow_copy(T)
    > {};

    template<typename T>
    struct IsNothrowConstructible<T, T &&>: IntegralConstant<bool,
        __has_nothrow_copy(T)
    > {};

    /* is nothrow default constructible */

    template<typename T>
    struct IsNothrowDefaultConstructible: IsNothrowConstructible<T> {};

    /* is nothrow copy constructible */

    template<typename T>
    struct IsNothrowCopyConstructible: IsNothrowConstructible<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is nothrow move constructible */

    template<typename T>
    struct IsNothrowMoveConstructible: IsNothrowConstructible<T,
        typename AddRvalueReference<T>::type
    > {};

    /* is nothrow assignable */

    template<typename T, typename ...A>
    struct IsNothrowAssignable: false_t {};

    template<typename T>
    struct IsNothrowAssignable<T>: IntegralConstant<bool,
        __has_nothrow_assign(T)
    > {};

    template<typename T>
    struct IsNothrowAssignable<T, T &>: IntegralConstant<bool,
        __has_nothrow_copy(T)
    > {};

    template<typename T>
    struct IsNothrowAssignable<T, const T &>: IntegralConstant<bool,
        __has_nothrow_copy(T)
    > {};

    template<typename T>
    struct IsNothrowAssignable<T, T &&>: IntegralConstant<bool,
        __has_nothrow_copy(T)
    > {};

    /* is nothrow copy assignable */

    template<typename T>
    struct IsNothrowCopyAssignable: IsNothrowAssignable<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is nothrow move assignable */

    template<typename T>
    struct IsNothrowMoveAssignable: IsNothrowAssignable<T,
        typename AddRvalueReference<T>::type
    > {};

    /* is nothrow destructible */

    template<typename, bool> struct __OctaIsNothrowDtible;

    template<typename T>
    struct __OctaIsNothrowDtible<T, false>: IntegralConstant<bool,
        noexcept(__octa_declval<T>().~T())
    > {};

    template<typename T>
    struct IsNothrowDestructible: __OctaIsNothrowDtible<T,
        IsDestructible<T>::value
    > {};

    template<typename T, size_t N>
    struct IsNothrowDestructible<T[N]>: IsNothrowDestructible<T> {};

    template<typename T>
    struct IsNothrowDestructible<T &>: IsNothrowDestructible<T> {};

    template<typename T>
    struct IsNothrowDestructible<T &&>: IsNothrowDestructible<T> {};

    /* is base of */

    template<typename B, typename D>
    struct IsBaseOf: IntegralConstant<bool, __is_base_of(B, D)> {};

    /* is convertible */

    template<typename F, typename T, bool = IsVoid<F>::value
        || IsFunction<T>::value || IsArray<T>::value
    > struct __OctaIsConvertible {
        typedef typename IsVoid<T>::type type;
    };

    template<typename F, typename T> struct __OctaIsConvertible<F, T, false> {
        template<typename TT> static void test_f(TT);

        template<typename FF, typename TT,
            typename = decltype(test_f<TT>(__octa_declval<FF>()))
        > static true_t test(int);

        template<typename, typename> static false_t test(...);

        typedef decltype(test<F, T>(0)) type;
    };

    template<typename F, typename T>
    struct IsConvertible: __OctaIsConvertible<F, T>::type {};

    /* type equality */

    template<typename, typename> struct IsSame      : false_t {};
    template<typename T        > struct IsSame<T, T>:  true_t {};

    /* extent */

    template<typename T, unsigned I = 0>
    struct Extent: IntegralConstant<size_t, 0> {};

    template<typename T>
    struct Extent<T[], 0>: IntegralConstant<size_t, 0> {};

    template<typename T, unsigned I>
    struct Extent<T[], I>: IntegralConstant<size_t, Extent<T, I - 1>::value> {};

    template<typename T, size_t N>
    struct Extent<T[N], 0>: IntegralConstant<size_t, N> {};

    template<typename T, size_t N, unsigned I>
    struct Extent<T[N], I>: IntegralConstant<size_t, Extent<T, I - 1>::value> {};

    /* rank */

    template<typename T> struct Rank: IntegralConstant<size_t, 0> {};

    template<typename T>
    struct Rank<T[]>: IntegralConstant<size_t, Rank<T>::value + 1> {};

    template<typename T, size_t N>
    struct Rank<T[N]>: IntegralConstant<size_t, Rank<T>::value + 1> {};

    /* remove const, volatile, cv */

    template<typename T> struct RemoveConst          { typedef T type; };
    template<typename T> struct RemoveConst<const T> { typedef T type; };

    template<typename T> struct RemoveVolatile             { typedef T type; };
    template<typename T> struct RemoveVolatile<volatile T> { typedef T type; };

    template<typename T>
    struct RemoveCV {
        typedef typename RemoveVolatile<typename RemoveConst<T>::type>::type type;
    };

    /* add const, volatile, cv */

    template<typename T, bool = IsReference<T>::value
         || IsFunction<T>::value || IsConst<T>::value>
    struct __OctaAddConst { typedef T type; };

    template<typename T> struct __OctaAddConst<T, false> {
        typedef const T type;
    };

    template<typename T> struct AddConst {
        typedef typename __OctaAddConst<T>::type type;
    };

    template<typename T, bool = IsReference<T>::value
         || IsFunction<T>::value || IsVolatile<T>::value>
    struct __OctaAddVolatile { typedef T type; };

    template<typename T> struct __OctaAddVolatile<T, false> {
        typedef volatile T type;
    };

    template<typename T> struct AddVolatile {
        typedef typename __OctaAddVolatile<T>::type type;
    };

    template<typename T>
    struct AddCV {
        typedef typename AddConst<typename AddVolatile<T>::type>::type type;
    };

    /* remove reference */

    template<typename T> struct RemoveReference       { typedef T type; };
    template<typename T> struct RemoveReference<T &>  { typedef T type; };
    template<typename T> struct RemoveReference<T &&> { typedef T type; };

    /* remove pointer */

    template<typename T> struct RemovePointer                     { typedef T type; };
    template<typename T> struct RemovePointer<T *               > { typedef T type; };
    template<typename T> struct RemovePointer<T * const         > { typedef T type; };
    template<typename T> struct RemovePointer<T * volatile      > { typedef T type; };
    template<typename T> struct RemovePointer<T * const volatile> { typedef T type; };

    /* add pointer */

    template<typename T> struct AddPointer {
        typedef typename RemoveReference<T>::type *type;
    };

    /* add lvalue reference */

    template<typename T> struct AddLvalueReference       { typedef T &type; };
    template<typename T> struct AddLvalueReference<T  &> { typedef T &type; };
    template<typename T> struct AddLvalueReference<T &&> { typedef T &type; };
    template<> struct AddLvalueReference<void> {
        typedef void type;
    };
    template<> struct AddLvalueReference<const void> {
        typedef const void type;
    };
    template<> struct AddLvalueReference<volatile void> {
        typedef volatile void type;
    };
    template<> struct AddLvalueReference<const volatile void> {
        typedef const volatile void type;
    };

    /* add rvalue reference */

    template<typename T> struct AddRvalueReference       { typedef T &&type; };
    template<typename T> struct AddRvalueReference<T  &> { typedef T &&type; };
    template<typename T> struct AddRvalueReference<T &&> { typedef T &&type; };
    template<> struct AddRvalueReference<void> {
        typedef void type;
    };
    template<> struct AddRvalueReference<const void> {
        typedef const void type;
    };
    template<> struct AddRvalueReference<volatile void> {
        typedef volatile void type;
    };
    template<> struct AddRvalueReference<const volatile void> {
        typedef const volatile void type;
    };

    /* remove extent */

    template<typename T          > struct RemoveExtent       { typedef T type; };
    template<typename T          > struct RemoveExtent<T[ ]> { typedef T type; };
    template<typename T, size_t N> struct RemoveExtent<T[N]> { typedef T type; };

    /* remove all extents */

    template<typename T> struct RemoveAllExtents { typedef T type; };

    template<typename T> struct RemoveAllExtents<T[]> {
        typedef typename RemoveAllExtents<T>::type type;
    };

    template<typename T, size_t N> struct RemoveAllExtents<T[N]> {
        typedef typename RemoveAllExtents<T>::type type;
    };

    /* make (un)signed
     *
     * this is bad, but i don't see any better way
     * shamelessly copied from graphitemaster @ neothyne
     */

    template<typename T, typename U> struct __OctaTL {
        typedef T first;
        typedef U rest;
    };

    /* not a type */
    struct __OctaNAT {
        __OctaNAT() = delete;
        __OctaNAT(const __OctaNAT &) = delete;
        __OctaNAT &operator=(const __OctaNAT &) = delete;
        ~__OctaNAT() = delete;
    };

    typedef __OctaTL<schar,
            __OctaTL<short,
            __OctaTL<int,
            __OctaTL<long,
            __OctaTL<llong, __OctaNAT>>>>> stypes;

    typedef __OctaTL<uchar,
            __OctaTL<ushort,
            __OctaTL<uint,
            __OctaTL<ulong,
            __OctaTL<ullong, __OctaNAT>>>>> utypes;

    template<typename T, size_t N, bool = (N <= sizeof(typename T::first))>
    struct __OctaTypeFindFirst;

    template<typename T, typename U, size_t N>
    struct __OctaTypeFindFirst<__OctaTL<T, U>, N, true> {
        typedef T type;
    };

    template<typename T, typename U, size_t N>
    struct __OctaTypeFindFirst<__OctaTL<T, U>, N, false> {
        typedef typename __OctaTypeFindFirst<U, N>::type type;
    };

    template<typename T, typename U,
        bool = IsConst<typename RemoveReference<T>::type>::value,
        bool = IsVolatile<typename RemoveReference<T>::type>::value
    > struct __OctaApplyCV {
        typedef U type;
    };

    template<typename T, typename U>
    struct __OctaApplyCV<T, U, true, false> { /* const */
        typedef const U type;
    };

    template<typename T, typename U>
    struct __OctaApplyCV<T, U, false, true> { /* volatile */
        typedef volatile U type;
    };

    template<typename T, typename U>
    struct __OctaApplyCV<T, U, true, true> { /* const volatile */
        typedef const volatile U type;
    };

    template<typename T, typename U>
    struct __OctaApplyCV<T &, U, true, false> { /* const */
        typedef const U &type;
    };

    template<typename T, typename U>
    struct __OctaApplyCV<T &, U, false, true> { /* volatile */
        typedef volatile U &type;
    };

    template<typename T, typename U>
    struct __OctaApplyCV<T &, U, true, true> { /* const volatile */
        typedef const volatile U &type;
    };

    template<typename T, bool = IsIntegral<T>::value || IsEnum<T>::value>
    struct __OctaMakeSigned {};

    template<typename T, bool = IsIntegral<T>::value || IsEnum<T>::value>
    struct __OctaMakeUnsigned {};

    template<typename T>
    struct __OctaMakeSigned<T, true> {
        typedef typename __OctaTypeFindFirst<stypes, sizeof(T)>::type type;
    };

    template<typename T>
    struct __OctaMakeUnsigned<T, true> {
        typedef typename __OctaTypeFindFirst<utypes, sizeof(T)>::type type;
    };

    template<> struct __OctaMakeSigned<bool  , true> {};
    template<> struct __OctaMakeSigned<schar , true> { typedef schar type; };
    template<> struct __OctaMakeSigned<uchar , true> { typedef schar type; };
    template<> struct __OctaMakeSigned<short , true> { typedef short type; };
    template<> struct __OctaMakeSigned<ushort, true> { typedef short type; };
    template<> struct __OctaMakeSigned<int   , true> { typedef int   type; };
    template<> struct __OctaMakeSigned<uint  , true> { typedef int   type; };
    template<> struct __OctaMakeSigned<long  , true> { typedef long  type; };
    template<> struct __OctaMakeSigned<ulong , true> { typedef long  type; };
    template<> struct __OctaMakeSigned<llong , true> { typedef llong type; };
    template<> struct __OctaMakeSigned<ullong, true> { typedef llong type; };

    template<> struct __OctaMakeUnsigned<bool  , true> {};
    template<> struct __OctaMakeUnsigned<schar , true> { typedef uchar  type; };
    template<> struct __OctaMakeUnsigned<uchar , true> { typedef uchar  type; };
    template<> struct __OctaMakeUnsigned<short , true> { typedef ushort type; };
    template<> struct __OctaMakeUnsigned<ushort, true> { typedef ushort type; };
    template<> struct __OctaMakeUnsigned<int   , true> { typedef uint   type; };
    template<> struct __OctaMakeUnsigned<uint  , true> { typedef uint   type; };
    template<> struct __OctaMakeUnsigned<long  , true> { typedef ulong  type; };
    template<> struct __OctaMakeUnsigned<ulong , true> { typedef ulong  type; };
    template<> struct __OctaMakeUnsigned<llong , true> { typedef ullong type; };
    template<> struct __OctaMakeUnsigned<ullong, true> { typedef ullong type; };

    template<typename T> struct MakeSigned {
        typedef typename __OctaApplyCV<T,
            typename __OctaMakeSigned<
                typename RemoveCV<T>::type
            >::type
        >::type type;
    };

    template<typename T> struct MakeUnsigned {
        typedef typename __OctaApplyCV<T,
            typename __OctaMakeUnsigned<
                typename RemoveCV<T>::type
            >::type
        >::type type;
    };

    /* conditional */

    template<bool cond, typename T, typename U>
    struct Conditional {
        typedef T type;
    };

    template<typename T, typename U>
    struct Conditional<false, T, U> {
        typedef U type;
    };

    /* result of call at compile time */

#define __OCTA_FWD(T, v) static_cast<T &&>(v)
    template<typename F, typename ...A>
    inline auto __octa_rof_invoke(F &&f, A &&...args) ->
      decltype(__OCTA_FWD(F, f)(__OCTA_FWD(A, args)...)) {
        return __OCTA_FWD(F, f)(__OCTA_FWD(A, args)...);
    }
    template<typename B, typename T, typename D>
    inline auto __octa_rof_invoke(T B::*pmd, D &&ref) ->
      decltype(__OCTA_FWD(D, ref).*pmd) {
        return __OCTA_FWD(D, ref).*pmd;
    }
    template<typename PMD, typename P>
    inline auto __octa_rof_invoke(PMD &&pmd, P &&ptr) ->
      decltype((*__OCTA_FWD(P, ptr)).*__OCTA_FWD(PMD, pmd)) {
        return (*__OCTA_FWD(P, ptr)).*__OCTA_FWD(PMD, pmd);
    }
    template<typename B, typename T, typename D, typename ...A>
    inline auto __octa_rof_invoke(T B::*pmf, D &&ref, A &&...args) ->
      decltype((__OCTA_FWD(D, ref).*pmf)(__OCTA_FWD(A, args)...)) {
        return (__OCTA_FWD(D, ref).*pmf)(__OCTA_FWD(A, args)...);
    }
    template<typename PMF, typename P, typename ...A>
    inline auto __octa_rof_invoke(PMF &&pmf, P &&ptr, A &&...args) ->
      decltype(((*__OCTA_FWD(P, ptr)).*__OCTA_FWD(PMF, pmf))(__OCTA_FWD(A, args)...)) {
        return ((*__OCTA_FWD(P, ptr)).*__OCTA_FWD(PMF, pmf))(__OCTA_FWD(A, args)...);
    }
#undef __OCTA_FWD

    template<typename, typename = void>
    struct __OctaResultOf {};
    template<typename F, typename ...A>
    struct __OctaResultOf<F(A...), decltype(void(__octa_rof_invoke(
    __octa_declval<F>(), __octa_declval<A>()...)))> {
        using type = decltype(__octa_rof_invoke(__octa_declval<F>(),
            __octa_declval<A>()...));
    };

    template<typename T> struct ResultOf: __OctaResultOf<T> {};

    /* enable if */

    template<bool B, typename T = void> struct EnableIf {};

    template<typename T> struct EnableIf<true, T> { typedef T type; };

    /* decay */

    template<typename T>
    struct Decay {
    private:
        typedef typename RemoveReference<T>::type U;
    public:
        typedef typename Conditional<IsArray<U>::value,
            typename RemoveExtent<U>::type *,
            typename Conditional<IsFunction<U>::value,
                typename AddPointer<U>::type,
                typename RemoveCV<U>::type
            >::type
        >::type type;
    };

    /* common type */

    template<typename ...T> struct CommonType;

    template<typename T> struct CommonType<T> {
        typedef Decay<T> type;
    };

    template<typename T, typename U> struct CommonType<T, U> {
        typedef Decay<decltype(true ? __octa_declval<T>() : __octa_declval<U>())> type;
    };

    template<typename T, typename U, typename ...V>
    struct CommonType<T, U, V...> {
        typedef typename CommonType<typename CommonType<T, U>::type, V...>::type type;
    };

    /* aligned storage */

    template<size_t N> struct __OctaAlignedTest {
        union type {
            uchar data[N];
            octa::max_align_t align;
        };
    };

    template<size_t N, size_t A
        = alignof(typename __OctaAlignedTest<N>::type)
    > struct AlignedStorage {
        struct type {
            alignas(A) uchar data[N];
        };
    };

    /* aligned union */

    template<size_t ...N> struct __OctaAlignMax;

    template<size_t N> struct __OctaAlignMax<N> {
        static constexpr size_t value = N;
    };

    template<size_t N1, size_t N2> struct __OctaAlignMax<N1, N2> {
        static constexpr size_t value = (N1 > N2) ? N1 : N2;
    };

    template<size_t N1, size_t N2, size_t ...N>
    struct __OctaAlignMax<N1, N2, N...> {
        static constexpr size_t value
            = __OctaAlignMax<__OctaAlignMax<N1, N2>::value, N...>::value;
    };

    template<size_t N, typename ...T> struct AlignedUnion {
        static constexpr size_t alignment_value
            = __OctaAlignMax<alignof(T)...>::value;

        struct type {
            alignas(alignment_value) uchar data[__OctaAlignMax<N,
                sizeof(T)...>::value];
        };
    };

    /* underlying type */

    template<typename T, bool = IsEnum<T>::value> struct __OctaUnderlyingType;

    template<typename T> struct __OctaUnderlyingType<T, true> {
        typedef typename Conditional<IsSigned<T>::value,
            typename MakeSigned<T>::type,
            typename MakeUnsigned<T>::type
        >::type type;
    };

    template<typename T> struct UnderlyingType {
        typedef typename __OctaUnderlyingType<T>::type type;
    };
}

#endif