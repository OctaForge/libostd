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

    template<typename> struct __OctaRemoveCv;
    template<typename> struct __OctaAddLr;
    template<typename> struct __OctaAddRr;
    template<typename> struct __OctaAddConst;
    template<typename> struct IsReference;
    template<typename> struct __OctaRemoveReference;
    template<typename> struct __OctaRemoveAllExtents;
    template<typename> struct IsTriviallyDefaultConstructible;

    template<typename...> struct __OctaCommonType;

    template<typename T>
    using RemoveCv = typename __OctaRemoveCv<T>::Type;

    template<typename T>
    using AddLvalueReference = typename __OctaAddLr<T>::Type;

    template<typename T>
    using AddRvalueReference = typename __OctaAddRr<T>::Type;

    template<typename T>
    using AddConst = typename __OctaAddConst<T>::Type;

    template<typename T>
    using RemoveReference = typename __OctaRemoveReference<T>::Type;

    template<typename T>
    using RemoveAllExtents = typename __OctaRemoveAllExtents<T>::Type;

    /* declval also defined here to avoid including utility.h */
    template<typename T> AddRvalueReference<T> __octa_declval();

    /* integral constant */

    template<typename T, T val>
    struct IntegralConstant {
        static constexpr T value = val;

        typedef T ValType;
        typedef IntegralConstant<T, val> Type;

        constexpr operator ValType() const { return value; }
        constexpr ValType operator()() const { return value; }
    };

    typedef IntegralConstant<bool, true>  True;
    typedef IntegralConstant<bool, false> False;

    template<typename T, T val> constexpr T IntegralConstant<T, val>::value;

    /* is void */

    template<typename T> struct __OctaIsVoid      : False {};
    template<          > struct __OctaIsVoid<void>:  True {};

    template<typename T>
    struct IsVoid: __OctaIsVoid<RemoveCv<T>> {};

    /* is null pointer */

    template<typename> struct __OctaIsNullPointer           : False {};
    template<        > struct __OctaIsNullPointer<nullptr_t>:  True {};

    template<typename T> struct IsNullPointer:
        __OctaIsNullPointer<RemoveCv<T>> {};

    /* is integer */

    template<typename T> struct __OctaIsIntegral: False {};

    template<> struct __OctaIsIntegral<bool  >: True {};
    template<> struct __OctaIsIntegral<char  >: True {};
    template<> struct __OctaIsIntegral<uchar >: True {};
    template<> struct __OctaIsIntegral<schar >: True {};
    template<> struct __OctaIsIntegral<short >: True {};
    template<> struct __OctaIsIntegral<ushort>: True {};
    template<> struct __OctaIsIntegral<int   >: True {};
    template<> struct __OctaIsIntegral<uint  >: True {};
    template<> struct __OctaIsIntegral<long  >: True {};
    template<> struct __OctaIsIntegral<ulong >: True {};
    template<> struct __OctaIsIntegral<llong >: True {};
    template<> struct __OctaIsIntegral<ullong>: True {};

    template<> struct __OctaIsIntegral<char16_t>: True {};
    template<> struct __OctaIsIntegral<char32_t>: True {};
    template<> struct __OctaIsIntegral< wchar_t>: True {};

    template<typename T>
    struct IsIntegral: __OctaIsIntegral<RemoveCv<T>> {};

    /* is floating point */

    template<typename T> struct __OctaIsFloatingPoint: False {};

    template<> struct __OctaIsFloatingPoint<float  >: True {};
    template<> struct __OctaIsFloatingPoint<double >: True {};
    template<> struct __OctaIsFloatingPoint<ldouble>: True {};

    template<typename T>
    struct IsFloatingPoint: __OctaIsFloatingPoint<RemoveCv<T>> {};

    /* is array */

    template<typename            > struct IsArray      : False {};
    template<typename T          > struct IsArray<T[] >:  True {};
    template<typename T, size_t N> struct IsArray<T[N]>:  True {};

    /* is pointer */

    template<typename  > struct __OctaIsPointer     : False {};
    template<typename T> struct __OctaIsPointer<T *>:  True {};

    template<typename T>
    struct IsPointer: __OctaIsPointer<RemoveCv<T>> {};

    /* is lvalue reference */

    template<typename  > struct IsLvalueReference     : False {};
    template<typename T> struct IsLvalueReference<T &>:  True {};

    /* is rvalue reference */

    template<typename  > struct IsRvalueReference      : False {};
    template<typename T> struct IsRvalueReference<T &&>:  True {};

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

    template<typename T> struct __OctaIsFunction<T, true>: False {};

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
    struct __OctaIsMemberPointer: False {};

    template<typename T, typename U>
    struct __OctaIsMemberPointer<T U::*>: True {};

    template<typename T>
    struct IsMemberPointer: __OctaIsMemberPointer<RemoveCv<T>> {};

    /* is pointer to member object */

    template<typename>
    struct __OctaIsMemberObjectPointer: False {};

    template<typename T, typename U>
    struct __OctaIsMemberObjectPointer<T U::*>: IntegralConstant<bool,
        !IsFunction<T>::value
    > {};

    template<typename T> struct IsMemberObjectPointer:
        __OctaIsMemberObjectPointer<RemoveCv<T>> {};

    /* is pointer to member function */

    template<typename>
    struct __OctaIsMemberFunctionPointer: False {};

    template<typename T, typename U>
    struct __OctaIsMemberFunctionPointer<T U::*>: IntegralConstant<bool,
        IsFunction<T>::value
    > {};

    template<typename T> struct IsMemberFunctionPointer:
        __OctaIsMemberFunctionPointer<RemoveCv<T>> {};

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

    template<typename  > struct IsConst         : False {};
    template<typename T> struct IsConst<const T>:  True {};

    /* is volatile */

    template<typename  > struct IsVolatile            : False {};
    template<typename T> struct IsVolatile<volatile T>:  True {};

    /* is empty */

    template<typename T>
    struct IsEmpty: IntegralConstant<bool, __is_empty(T)> {};

    /* is POD */

    template<typename T> struct IsPod: IntegralConstant<bool, __is_pod(T)> {};

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
    struct IsStandardLayout: IntegralConstant<bool, __is_standard_layout(T)> {};

    /* is literal type */

    template<typename T>
    struct IsLiteralType: IntegralConstant<bool, __is_literal_type(T)> {};

    /* is trivially copyable */

    template<typename T>
    struct IsTriviallyCopyable: IntegralConstant<bool,
        IsScalar<RemoveAllExtents<T>>::value
    > {};

    /* is trivial */

    template<typename T>
    struct IsTrivial: IntegralConstant<bool, __is_trivial(T)> {};

    /* has virtual destructor */

    template<typename T>
    struct HasVirtualDestructor: IntegralConstant<bool,
        __has_virtual_destructor(T)
    > {};

    /* is constructible */

#define __OCTA_MOVE(v) static_cast<RemoveReference<decltype(v)> &&>(v)

    template<typename, typename T> struct __OctaSelect2nd { typedef T Type; };
    struct __OctaAny { __OctaAny(...); };

    template<typename T, typename ...A> typename __OctaSelect2nd<
        decltype(__OCTA_MOVE(T(__octa_declval<A>()...))), True
    >::Type __octa_is_ctible_test(T &&, A &&...);

//#undef __OCTA_MOVE

    template<typename ...A> False __octa_is_ctible_test(__OctaAny, A &&...);

    template<bool, typename T, typename ...A>
    struct __OctaCtibleCore: __OctaCommonType<
        decltype(__octa_is_ctible_test(__octa_declval<T>(),
            __octa_declval<A>()...))
    >::Type {};

    /* function types are not constructible */
    template<typename R, typename ...A1, typename ...A2>
    struct __OctaCtibleCore<false, R(A1...), A2...>: False {};

    /* scalars are default constructible, refs are not */
    template<typename T>
    struct __OctaCtibleCore<true, T>: IsScalar<T> {};

    /* scalars and references are constructible from one arg if
     * implicitly convertible to scalar or reference */
    template<typename T>
    struct __OctaCtibleRef {
        static True  test(T);
        static False test(...);
    };

    template<typename T, typename U>
    struct __OctaCtibleCore<true, T, U>: __OctaCommonType<
        decltype(__OctaCtibleRef<T>::test(__octa_declval<U>()))
    >::Type {};

    /* scalars and references are not constructible from multiple args */
    template<typename T, typename U, typename ...A>
    struct __OctaCtibleCore<true, T, U, A...>: False {};

    /* treat scalars and refs separately */
    template<bool, typename T, typename ...A>
    struct __OctaCtibleVoidCheck: __OctaCtibleCore<
        (IsScalar<T>::value || IsReference<T>::value), T, A...
    > {};

    /* if any of T or A is void, IsConstructible should be false */
    template<typename T, typename ...A>
    struct __OctaCtibleVoidCheck<true, T, A...>: False {};

    template<typename ...A> struct __OctaCtibleContainsVoid;

    template<> struct __OctaCtibleContainsVoid<>: False {};

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
    struct __OctaCtibleCore<false, T[N]>: __OctaCtible<RemoveAllExtents<T>> {};

    /* otherwise array types are not constructible by this syntax */
    template<typename T, size_t N, typename ...A>
    struct __OctaCtibleCore<false, T[N], A...>: False {};

    /* incomplete array types are not constructible */
    template<typename T, typename ...A>
    struct __OctaCtibleCore<false, T[], A...>: False {};

    template<typename T, typename ...A>
    struct IsConstructible: __OctaCtible<T, A...> {};

    /* is default constructible */

    template<typename T> struct IsDefaultConstructible: IsConstructible<T> {};

    /* is copy constructible */

    template<typename T> struct IsCopyConstructible: IsConstructible<T,
        AddLvalueReference<AddConst<T>>
    > {};

    /* is move constructible */

    template<typename T> struct IsMoveConstructible: IsConstructible<T,
        AddRvalueReference<T>
    > {};

    /* is assignable */

    template<typename T, typename U> typename __OctaSelect2nd<
        decltype((__octa_declval<T>() = __octa_declval<U>())), True
    >::Type __octa_assign_test(T &&, U &&);

    template<typename T> False __octa_assign_test(__OctaAny, T &&);

    template<typename T, typename U, bool = IsVoid<T>::value || IsVoid<U>::value>
    struct __OctaIsAssignable: __OctaCommonType<
        decltype(__octa_assign_test(__octa_declval<T>(), __octa_declval<U>()))
    >::Type {};

    template<typename T, typename U>
    struct __OctaIsAssignable<T, U, true>: False {};

    template<typename T, typename U>
    struct IsAssignable: __OctaIsAssignable<T, U> {};

    /* is copy assignable */

    template<typename T> struct IsCopyAssignable: IsAssignable<
        AddLvalueReference<T>,
        AddLvalueReference<AddConst<T>>
    > {};

    /* is move assignable */

    template<typename T> struct IsMoveAssignable: IsAssignable<
        AddLvalueReference<T>,
        const AddRvalueReference<T>
    > {};

    /* is destructible */

    template<typename> struct __OctaIsDtibleApply { typedef int Type; };

    template<typename T> struct IsDestructorWellformed {
        template<typename TT> static char test(typename __OctaIsDtibleApply<
            decltype(__octa_declval<TT &>().~TT())
        >::Type);

        template<typename TT> static int test(...);

        static constexpr bool value = (sizeof(test<T>(12)) == sizeof(char));
    };

    template<typename, bool> struct __OctaDtibleImpl;

    template<typename T>
    struct __OctaDtibleImpl<T, false>: IntegralConstant<bool,
        IsDestructorWellformed<RemoveAllExtents<T>>::value
    > {};

    template<typename T>
    struct __OctaDtibleImpl<T, true>: True {};

    template<typename T, bool> struct __OctaDtibleFalse;

    template<typename T> struct __OctaDtibleFalse<T, false>
        : __OctaDtibleImpl<T, IsReference<T>::value> {};

    template<typename T> struct __OctaDtibleFalse<T, true>: False {};

    template<typename T>
    struct IsDestructible: __OctaDtibleFalse<T, IsFunction<T>::value> {};

    template<typename T> struct IsDestructible<T[] >: False {};
    template<          > struct IsDestructible<void>: False {};

    /* is trivially constructible */

    template<typename T, typename ...A>
    struct IsTriviallyConstructible: False {};

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
        AddLvalueReference<const T>
    > {};

    /* is trivially move constructible */

    template<typename T>
    struct IsTriviallyMoveConstructible: IsTriviallyConstructible<T,
        AddRvalueReference<T>
    > {};

    /* is trivially assignable */

    template<typename T, typename ...A>
    struct IsTriviallyAssignable: False {};

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
        AddLvalueReference<const T>
    > {};

    /* is trivially move assignable */

    template<typename T>
    struct IsTriviallyMoveAssignable: IsTriviallyAssignable<T,
        AddRvalueReference<T>
    > {};

    /* is trivially destructible */

    template<typename T>
    struct IsTriviallyDestructible: IntegralConstant<bool,
        __has_trivial_destructor(T)
    > {};

    /* is base of */

    template<typename B, typename D>
    struct IsBaseOf: IntegralConstant<bool, __is_base_of(B, D)> {};

    /* is convertible */

    template<typename F, typename T, bool = IsVoid<F>::value
        || IsFunction<T>::value || IsArray<T>::value
    > struct __OctaIsConvertible {
        typedef typename IsVoid<T>::Type Type;
    };

    template<typename F, typename T> struct __OctaIsConvertible<F, T, false> {
        template<typename TT> static void test_f(TT);

        template<typename FF, typename TT,
            typename = decltype(test_f<TT>(__octa_declval<FF>()))
        > static True test(int);

        template<typename, typename> static False test(...);

        typedef decltype(test<F, T>(0)) Type;
    };

    template<typename F, typename T>
    struct IsConvertible: __OctaIsConvertible<F, T>::Type {};

    /* type equality */

    template<typename, typename> struct IsSame      : False {};
    template<typename T        > struct IsSame<T, T>:  True {};

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

    template<typename T>
    struct __OctaRemoveConst          { typedef T Type; };
    template<typename T>
    struct __OctaRemoveConst<const T> { typedef T Type; };

    template<typename T>
    struct __OctaRemoveVolatile             { typedef T Type; };
    template<typename T>
    struct __OctaRemoveVolatile<volatile T> { typedef T Type; };

    template<typename T>
    using RemoveConst = typename __OctaRemoveConst<T>::Type;
    template<typename T>
    using RemoveVolatile = typename __OctaRemoveVolatile<T>::Type;

    template<typename T>
    struct __OctaRemoveCv {
        typedef RemoveVolatile<RemoveConst<T>> Type;
    };

    /* add const, volatile, cv */

    template<typename T, bool = IsReference<T>::value
         || IsFunction<T>::value || IsConst<T>::value>
    struct __OctaAddConstBase { typedef T Type; };

    template<typename T> struct __OctaAddConstBase<T, false> {
        typedef const T Type;
    };

    template<typename T> struct __OctaAddConst {
        typedef typename __OctaAddConstBase<T>::Type Type;
    };

    template<typename T, bool = IsReference<T>::value
         || IsFunction<T>::value || IsVolatile<T>::value>
    struct __OctaAddVolatileBase { typedef T Type; };

    template<typename T> struct __OctaAddVolatileBase<T, false> {
        typedef volatile T Type;
    };

    template<typename T> struct __OctaAddVolatile {
        typedef typename __OctaAddVolatileBase<T>::Type Type;
    };

    template<typename T>
    using AddVolatile = typename __OctaAddVolatile<T>::Type;

    template<typename T>
    struct __OctaAddCv {
        typedef AddConst<AddVolatile<T>> Type;
    };

    template<typename T>
    using AddCv = typename __OctaAddCv<T>::Type;

    /* remove reference */

    template<typename T>
    struct __OctaRemoveReference       { typedef T Type; };
    template<typename T>
    struct __OctaRemoveReference<T &>  { typedef T Type; };
    template<typename T>
    struct __OctaRemoveReference<T &&> { typedef T Type; };

    /* remove pointer */

    template<typename T>
    struct __OctaRemovePointer                     { typedef T Type; };
    template<typename T>
    struct __OctaRemovePointer<T *               > { typedef T Type; };
    template<typename T>
    struct __OctaRemovePointer<T * const         > { typedef T Type; };
    template<typename T>
    struct __OctaRemovePointer<T * volatile      > { typedef T Type; };
    template<typename T>
    struct __OctaRemovePointer<T * const volatile> { typedef T Type; };

    template<typename T>
    using RemovePointer = typename __OctaRemovePointer<T>::Type;

    /* add pointer */

    template<typename T> struct __OctaAddPointer {
        typedef RemoveReference<T> *Type;
    };

    template<typename T>
    using AddPointer = typename __OctaAddPointer<T>::Type;

    /* add lvalue reference */

    template<typename T> struct __OctaAddLr       { typedef T &Type; };
    template<typename T> struct __OctaAddLr<T  &> { typedef T &Type; };
    template<typename T> struct __OctaAddLr<T &&> { typedef T &Type; };
    template<> struct __OctaAddLr<void> {
        typedef void Type;
    };
    template<> struct __OctaAddLr<const void> {
        typedef const void Type;
    };
    template<> struct __OctaAddLr<volatile void> {
        typedef volatile void Type;
    };
    template<> struct __OctaAddLr<const volatile void> {
        typedef const volatile void Type;
    };

    /* add rvalue reference */

    template<typename T> struct __OctaAddRr       { typedef T &&Type; };
    template<typename T> struct __OctaAddRr<T  &> { typedef T &&Type; };
    template<typename T> struct __OctaAddRr<T &&> { typedef T &&Type; };
    template<> struct __OctaAddRr<void> {
        typedef void Type;
    };
    template<> struct __OctaAddRr<const void> {
        typedef const void Type;
    };
    template<> struct __OctaAddRr<volatile void> {
        typedef volatile void Type;
    };
    template<> struct __OctaAddRr<const volatile void> {
        typedef const volatile void Type;
    };

    /* remove extent */

    template<typename T>
    struct __OctaRemoveExtent       { typedef T Type; };
    template<typename T>
    struct __OctaRemoveExtent<T[ ]> { typedef T Type; };
    template<typename T, size_t N>
    struct __OctaRemoveExtent<T[N]> { typedef T Type; };

    template<typename T>
    using RemoveExtent = typename __OctaRemoveExtent<T>::Type;

    /* remove all extents */

    template<typename T> struct __OctaRemoveAllExtents { typedef T Type; };

    template<typename T> struct __OctaRemoveAllExtents<T[]> {
        typedef RemoveAllExtents<T> Type;
    };

    template<typename T, size_t N> struct __OctaRemoveAllExtents<T[N]> {
        typedef RemoveAllExtents<T> Type;
    };

    /* make (un)signed
     *
     * this is bad, but i don't see any better way
     * shamelessly copied from graphitemaster @ neothyne
     */

    template<typename T, typename U> struct __OctaTl {
        typedef T first;
        typedef U rest;
    };

    /* not a type */
    struct __OctaNat {
        __OctaNat() = delete;
        __OctaNat(const __OctaNat &) = delete;
        __OctaNat &operator=(const __OctaNat &) = delete;
        ~__OctaNat() = delete;
    };

    typedef __OctaTl<schar,
            __OctaTl<short,
            __OctaTl<int,
            __OctaTl<long,
            __OctaTl<llong, __OctaNat>>>>> stypes;

    typedef __OctaTl<uchar,
            __OctaTl<ushort,
            __OctaTl<uint,
            __OctaTl<ulong,
            __OctaTl<ullong, __OctaNat>>>>> utypes;

    template<typename T, size_t N, bool = (N <= sizeof(typename T::first))>
    struct __OctaTypeFindFirst;

    template<typename T, typename U, size_t N>
    struct __OctaTypeFindFirst<__OctaTl<T, U>, N, true> {
        typedef T Type;
    };

    template<typename T, typename U, size_t N>
    struct __OctaTypeFindFirst<__OctaTl<T, U>, N, false> {
        typedef typename __OctaTypeFindFirst<U, N>::Type Type;
    };

    template<typename T, typename U,
        bool = IsConst<RemoveReference<T>>::value,
        bool = IsVolatile<RemoveReference<T>>::value
    > struct __OctaApplyCv {
        typedef U Type;
    };

    template<typename T, typename U>
    struct __OctaApplyCv<T, U, true, false> { /* const */
        typedef const U Type;
    };

    template<typename T, typename U>
    struct __OctaApplyCv<T, U, false, true> { /* volatile */
        typedef volatile U Type;
    };

    template<typename T, typename U>
    struct __OctaApplyCv<T, U, true, true> { /* const volatile */
        typedef const volatile U Type;
    };

    template<typename T, typename U>
    struct __OctaApplyCv<T &, U, true, false> { /* const */
        typedef const U &Type;
    };

    template<typename T, typename U>
    struct __OctaApplyCv<T &, U, false, true> { /* volatile */
        typedef volatile U &Type;
    };

    template<typename T, typename U>
    struct __OctaApplyCv<T &, U, true, true> { /* const volatile */
        typedef const volatile U &Type;
    };

    template<typename T, bool = IsIntegral<T>::value || IsEnum<T>::value>
    struct __OctaMakeSigned {};

    template<typename T, bool = IsIntegral<T>::value || IsEnum<T>::value>
    struct __OctaMakeUnsigned {};

    template<typename T>
    struct __OctaMakeSigned<T, true> {
        typedef typename __OctaTypeFindFirst<stypes, sizeof(T)>::Type Type;
    };

    template<typename T>
    struct __OctaMakeUnsigned<T, true> {
        typedef typename __OctaTypeFindFirst<utypes, sizeof(T)>::Type Type;
    };

    template<> struct __OctaMakeSigned<bool  , true> {};
    template<> struct __OctaMakeSigned<schar , true> { typedef schar Type; };
    template<> struct __OctaMakeSigned<uchar , true> { typedef schar Type; };
    template<> struct __OctaMakeSigned<short , true> { typedef short Type; };
    template<> struct __OctaMakeSigned<ushort, true> { typedef short Type; };
    template<> struct __OctaMakeSigned<int   , true> { typedef int   Type; };
    template<> struct __OctaMakeSigned<uint  , true> { typedef int   Type; };
    template<> struct __OctaMakeSigned<long  , true> { typedef long  Type; };
    template<> struct __OctaMakeSigned<ulong , true> { typedef long  Type; };
    template<> struct __OctaMakeSigned<llong , true> { typedef llong Type; };
    template<> struct __OctaMakeSigned<ullong, true> { typedef llong Type; };

    template<> struct __OctaMakeUnsigned<bool  , true> {};
    template<> struct __OctaMakeUnsigned<schar , true> { typedef uchar  Type; };
    template<> struct __OctaMakeUnsigned<uchar , true> { typedef uchar  Type; };
    template<> struct __OctaMakeUnsigned<short , true> { typedef ushort Type; };
    template<> struct __OctaMakeUnsigned<ushort, true> { typedef ushort Type; };
    template<> struct __OctaMakeUnsigned<int   , true> { typedef uint   Type; };
    template<> struct __OctaMakeUnsigned<uint  , true> { typedef uint   Type; };
    template<> struct __OctaMakeUnsigned<long  , true> { typedef ulong  Type; };
    template<> struct __OctaMakeUnsigned<ulong , true> { typedef ulong  Type; };
    template<> struct __OctaMakeUnsigned<llong , true> { typedef ullong Type; };
    template<> struct __OctaMakeUnsigned<ullong, true> { typedef ullong Type; };

    template<typename T> struct __OctaMakeSignedBase {
        typedef typename __OctaApplyCv<T,
            typename __OctaMakeSigned<RemoveCv<T>>::Type
        >::Type Type;
    };

    template<typename T> struct __OctaMakeUnsignedBase {
        typedef typename __OctaApplyCv<T,
            typename __OctaMakeUnsigned<RemoveCv<T>>::Type
        >::Type Type;
    };

    template<typename T>
    using MakeSigned = typename __OctaMakeSignedBase<T>::Type;
    template<typename T>
    using MakeUnsigned = typename __OctaMakeUnsignedBase<T>::Type;

    /* conditional */

    template<bool cond, typename T, typename U>
    struct __OctaConditional {
        typedef T Type;
    };

    template<typename T, typename U>
    struct __OctaConditional<false, T, U> {
        typedef U Type;
    };

    template<bool cond, typename T, typename U>
    using Conditional = typename __OctaConditional<cond, T, U>::Type;

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

    template<typename T> struct __OctaResultOfBase: __OctaResultOf<T> {};

    template<typename T>
    using ResultOf = typename __OctaResultOfBase<T>::Type;

    /* enable if */

    template<bool B, typename T = void> struct __OctaEnableIf {};

    template<typename T> struct __OctaEnableIf<true, T> { typedef T Type; };

    template<bool B, typename T = void>
    using EnableIf = typename __OctaEnableIf<B, T>::Type;

    /* decay */

    template<typename T>
    struct __OctaDecay {
    private:
        typedef RemoveReference<T> U;
    public:
        typedef Conditional<IsArray<U>::value,
            RemoveExtent<U> *,
            Conditional<IsFunction<U>::value, AddPointer<U>, RemoveCv<U>>
        > Type;
    };

    template<typename T>
    using Decay = typename __OctaDecay<T>::Type;

    /* common type */

    template<typename ...T> struct __OctaCommonType;

    template<typename T> struct __OctaCommonType<T> {
        typedef Decay<T> Type;
    };

    template<typename T, typename U> struct __OctaCommonType<T, U> {
        typedef Decay<decltype(true ? __octa_declval<T>()
            : __octa_declval<U>())> Type;
    };

    template<typename T, typename U, typename ...V>
    struct __OctaCommonType<T, U, V...> {
        typedef typename __OctaCommonType<typename __OctaCommonType<T, U>::Type,
            V...>::Type Type;
    };

    template<typename T, typename U, typename ...V>
    using CommonType = typename __OctaCommonType<T, U, V...>::Type;

    /* aligned storage */

    template<size_t N> struct __OctaAlignedTest {
        union type {
            uchar data[N];
            octa::max_align_t align;
        };
    };

    template<size_t N, size_t A> struct __OctaAlignedStorage {
        struct type {
            alignas(A) uchar data[N];
        };
    };

    template<size_t N, size_t A
        = alignof(typename __OctaAlignedTest<N>::Type)
    > using AlignedStorage = typename __OctaAlignedStorage<N, A>::Type;

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

    template<size_t N, typename ...T> struct __OctaAlignedUnion {
        static constexpr size_t alignment_value
            = __OctaAlignMax<alignof(T)...>::value;

        struct type {
            alignas(alignment_value) uchar data[__OctaAlignMax<N,
                sizeof(T)...>::value];
        };
    };

    template<size_t N, typename ...T>
    using AlignedUnion = typename __OctaAlignedUnion<N, T...>::Type;

    /* underlying type */

    /* gotta wrap, in a struct otherwise clang ICEs... */
    template<typename T> struct __OctaUnderlyingType {
        typedef __underlying_type(T) Type;
    };

    template<typename T>
    using UnderlyingType = typename __OctaUnderlyingType<T>::Type;
}

#endif