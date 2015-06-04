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

namespace detail {
    template<typename> struct RemoveCv;
    template<typename> struct AddLr;
    template<typename> struct AddRr;
    template<typename> struct AddConst;
    template<typename> struct RemoveReference;
    template<typename> struct RemoveAllExtents;

    template<typename ...> struct CommonTypeBase;
}

    template<typename> struct __OctaAddLr;
    template<typename> struct __OctaAddRr;
    template<typename> struct __OctaAddConst;
    template<typename> struct IsReference;
    template<typename> struct __OctaRemoveReference;
    template<typename> struct __OctaRemoveAllExtents;
    template<typename> struct IsTriviallyDefaultConstructible;

    template<typename _T>
    using RemoveCv = typename octa::detail::RemoveCv<_T>::Type;

    template<typename _T>
    using AddLvalueReference = typename __OctaAddLr<_T>::Type;

    template<typename _T>
    using AddRvalueReference = typename __OctaAddRr<_T>::Type;

    template<typename _T>
    using AddConst = typename __OctaAddConst<_T>::Type;

    template<typename _T>
    using RemoveReference = typename __OctaRemoveReference<_T>::Type;

    template<typename _T>
    using RemoveAllExtents = typename __OctaRemoveAllExtents<_T>::Type;

namespace detail {
    template<typename _T> octa::AddRvalueReference<_T> declval_in();
}

    /* integral constant */

    template<typename _T, _T __val>
    struct IntegralConstant {
        static constexpr _T value = __val;

        typedef _T Value;
        typedef IntegralConstant<_T, __val> Type;

        constexpr operator Value() const { return value; }
        constexpr Value operator()() const { return value; }
    };

    typedef IntegralConstant<bool, true>  True;
    typedef IntegralConstant<bool, false> False;

    template<typename _T, _T val> constexpr _T IntegralConstant<_T, val>::value;

    /* is void */

    template<typename _T> struct __OctaIsVoid      : False {};
    template<           > struct __OctaIsVoid<void>:  True {};

    template<typename _T>
    struct IsVoid: __OctaIsVoid<RemoveCv<_T>> {};

    /* is null pointer */

    template<typename> struct __OctaIsNullPointer           : False {};
    template<        > struct __OctaIsNullPointer<nullptr_t>:  True {};

    template<typename _T> struct IsNullPointer:
        __OctaIsNullPointer<RemoveCv<_T>> {};

    /* is integer */

    template<typename _T> struct __OctaIsIntegral: False {};

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

    template<typename _T>
    struct IsIntegral: __OctaIsIntegral<RemoveCv<_T>> {};

    /* is floating point */

    template<typename _T> struct __OctaIsFloatingPoint: False {};

    template<> struct __OctaIsFloatingPoint<float  >: True {};
    template<> struct __OctaIsFloatingPoint<double >: True {};
    template<> struct __OctaIsFloatingPoint<ldouble>: True {};

    template<typename _T>
    struct IsFloatingPoint: __OctaIsFloatingPoint<RemoveCv<_T>> {};

    /* is array */

    template<typename              > struct IsArray        : False {};
    template<typename _T           > struct IsArray<_T[]  >:  True {};
    template<typename _T, size_t _N> struct IsArray<_T[_N]>:  True {};

    /* is pointer */

    template<typename   > struct __OctaIsPointer      : False {};
    template<typename _T> struct __OctaIsPointer<_T *>:  True {};

    template<typename _T>
    struct IsPointer: __OctaIsPointer<RemoveCv<_T>> {};

    /* is lvalue reference */

    template<typename   > struct IsLvalueReference      : False {};
    template<typename _T> struct IsLvalueReference<_T &>:  True {};

    /* is rvalue reference */

    template<typename   > struct IsRvalueReference       : False {};
    template<typename _T> struct IsRvalueReference<_T &&>:  True {};

    /* is enum */

    template<typename _T> struct IsEnum: IntegralConstant<bool, __is_enum(_T)> {};

    /* is union */

    template<typename _T> struct IsUnion: IntegralConstant<bool, __is_union(_T)> {};

    /* is class */

    template<typename _T> struct IsClass: IntegralConstant<bool, __is_class(_T)> {};

    /* is function */

namespace detail {
    struct FunctionTestDummy {};

    template<typename _T> char function_test(_T *);
    template<typename _T> char function_test(FunctionTestDummy);
    template<typename _T> int  function_test(...);

    template<typename _T> _T                 &function_source(int);
    template<typename _T> FunctionTestDummy   function_source(...);

    template<typename _T, bool = octa::IsClass<_T>::value ||
                                 octa::IsUnion<_T>::value ||
                                 octa::IsVoid<_T>::value ||
                                 octa::IsReference<_T>::value ||
                                 octa::IsNullPointer<_T>::value
    > struct IsFunctionBase: IntegralConstant<bool,
        sizeof(function_test<_T>(function_source<_T>(0))) == 1
    > {};

    template<typename _T> struct IsFunctionBase<_T, true>: False {};
} /* namespace detail */

template<typename _T> struct IsFunction: octa::detail::IsFunctionBase<_T> {};

    /* is arithmetic */

    template<typename _T> struct IsArithmetic: IntegralConstant<bool,
        (IsIntegral<_T>::value || IsFloatingPoint<_T>::value)
    > {};

    /* is fundamental */

    template<typename _T> struct IsFundamental: IntegralConstant<bool,
        (IsArithmetic<_T>::value || IsVoid<_T>::value || IsNullPointer<_T>::value)
    > {};

    /* is compound */

    template<typename _T> struct IsCompound: IntegralConstant<bool,
        !IsFundamental<_T>::value
    > {};

    /* is pointer to member */

    template<typename>
    struct __OctaIsMemberPointer: False {};

    template<typename _T, typename _U>
    struct __OctaIsMemberPointer<_T _U::*>: True {};

    template<typename _T>
    struct IsMemberPointer: __OctaIsMemberPointer<RemoveCv<_T>> {};

    /* is pointer to member object */

    template<typename>
    struct __OctaIsMemberObjectPointer: False {};

    template<typename _T, typename _U>
    struct __OctaIsMemberObjectPointer<_T _U::*>: IntegralConstant<bool,
        !IsFunction<_T>::value
    > {};

    template<typename _T> struct IsMemberObjectPointer:
        __OctaIsMemberObjectPointer<RemoveCv<_T>> {};

    /* is pointer to member function */

    template<typename>
    struct __OctaIsMemberFunctionPointer: False {};

    template<typename _T, typename _U>
    struct __OctaIsMemberFunctionPointer<_T _U::*>: IntegralConstant<bool,
        IsFunction<_T>::value
    > {};

    template<typename _T> struct IsMemberFunctionPointer:
        __OctaIsMemberFunctionPointer<RemoveCv<_T>> {};

    /* is reference */

    template<typename _T> struct IsReference: IntegralConstant<bool,
        (IsLvalueReference<_T>::value || IsRvalueReference<_T>::value)
    > {};

    /* is object */

    template<typename _T> struct IsObject: IntegralConstant<bool,
        (!IsFunction<_T>::value && !IsVoid<_T>::value && !IsReference<_T>::value)
    > {};

    /* is scalar */

    template<typename _T> struct IsScalar: IntegralConstant<bool,
        (IsMemberPointer<_T>::value || IsPointer<_T>::value || IsEnum<_T>::value
      || IsNullPointer  <_T>::value || IsArithmetic<_T>::value)
    > {};

    /* is abstract */

    template<typename _T>
    struct IsAbstract: IntegralConstant<bool, __is_abstract(_T)> {};

    /* is const */

    template<typename   > struct IsConst          : False {};
    template<typename _T> struct IsConst<const _T>:  True {};

    /* is volatile */

    template<typename   > struct IsVolatile             : False {};
    template<typename _T> struct IsVolatile<volatile _T>:  True {};

    /* is empty */

    template<typename _T>
    struct IsEmpty: IntegralConstant<bool, __is_empty(_T)> {};

    /* is POD */

    template<typename _T> struct IsPod: IntegralConstant<bool, __is_pod(_T)> {};

    /* is polymorphic */

    template<typename _T>
    struct IsPolymorphic: IntegralConstant<bool, __is_polymorphic(_T)> {};

    /* is signed */

    template<typename _T>
    struct IsSigned: IntegralConstant<bool, _T(-1) < _T(0)> {};

    /* is unsigned */

    template<typename _T>
    struct IsUnsigned: IntegralConstant<bool, _T(0) < _T(-1)> {};

    /* is standard layout */

    template<typename _T>
    struct IsStandardLayout: IntegralConstant<bool, __is_standard_layout(_T)> {};

    /* is literal type */

    template<typename _T>
    struct IsLiteralType: IntegralConstant<bool, __is_literal_type(_T)> {};

    /* is trivially copyable */

    template<typename _T>
    struct IsTriviallyCopyable: IntegralConstant<bool,
        IsScalar<RemoveAllExtents<_T>>::value
    > {};

    /* is trivial */

    template<typename _T>
    struct IsTrivial: IntegralConstant<bool, __is_trivial(_T)> {};

    /* has virtual destructor */

    template<typename _T>
    struct HasVirtualDestructor: IntegralConstant<bool,
        __has_virtual_destructor(_T)
    > {};

    /* is constructible */

namespace detail {
#define OCTA_MOVE(v) static_cast<octa::RemoveReference<decltype(v)> &&>(v)

    template<typename, typename _T> struct Select2nd { typedef _T Type; };
    struct Any { Any(...); };

    template<typename _T, typename ..._A> typename Select2nd<
        decltype(OCTA_MOVE(_T(declval_in<_A>()...))), True
    >::Type is_ctible_test(_T &&, _A &&...);

#undef OCTA_MOVE

    template<typename ..._A> False is_ctible_test(Any, _A &&...);

    template<bool, typename _T, typename ..._A>
    struct CtibleCore: CommonTypeBase<
        decltype(is_ctible_test(declval_in<_T>(), declval_in<_A>()...))
    >::Type {};

    /* function types are not constructible */
    template<typename _R, typename ..._A1, typename ..._A2>
    struct CtibleCore<false, _R(_A1...), _A2...>: False {};

    /* scalars are default constructible, refs are not */
    template<typename _T>
    struct CtibleCore<true, _T>: octa::IsScalar<_T> {};

    /* scalars and references are constructible from one arg if
     * implicitly convertible to scalar or reference */
    template<typename _T>
    struct CtibleRef {
        static True  test(_T);
        static False test(...);
    };

    template<typename _T, typename _U>
    struct CtibleCore<true, _T, _U>: CommonTypeBase<
        decltype(CtibleRef<_T>::test(declval_in<_U>()))
    >::Type {};

    /* scalars and references are not constructible from multiple args */
    template<typename _T, typename _U, typename ..._A>
    struct CtibleCore<true, _T, _U, _A...>: False {};

    /* treat scalars and refs separately */
    template<bool, typename _T, typename ..._A>
    struct CtibleVoidCheck: CtibleCore<
        (octa::IsScalar<_T>::value || octa::IsReference<_T>::value), _T, _A...
    > {};

    /* if any of T or A is void, IsConstructible should be false */
    template<typename _T, typename ..._A>
    struct CtibleVoidCheck<true, _T, _A...>: False {};

    template<typename ..._A> struct CtibleContainsVoid;

    template<> struct CtibleContainsVoid<>: False {};

    template<typename _T, typename ..._A>
    struct CtibleContainsVoid<_T, _A...> {
        static constexpr bool value = octa::IsVoid<_T>::value
           || CtibleContainsVoid<_A...>::value;
    };

    /* entry point */
    template<typename _T, typename ..._A>
    struct Ctible: CtibleVoidCheck<
        CtibleContainsVoid<_T, _A...>::value || octa::IsAbstract<_T>::value,
        _T, _A...
    > {};

    /* array types are default constructible if their element type is */
    template<typename _T, size_t _N>
    struct CtibleCore<false, _T[_N]>: Ctible<octa::RemoveAllExtents<_T>> {};

    /* otherwise array types are not constructible by this syntax */
    template<typename _T, size_t _N, typename ..._A>
    struct CtibleCore<false, _T[_N], _A...>: False {};

    /* incomplete array types are not constructible */
    template<typename _T, typename ..._A>
    struct CtibleCore<false, _T[], _A...>: False {};
} /* namespace detail */

template<typename _T, typename ..._A>
struct IsConstructible: octa::detail::Ctible<_T, _A...> {};

    /* is default constructible */

    template<typename _T> struct IsDefaultConstructible: IsConstructible<_T> {};

    /* is copy constructible */

    template<typename _T> struct IsCopyConstructible: IsConstructible<_T,
        AddLvalueReference<AddConst<_T>>
    > {};

    /* is move constructible */

    template<typename _T> struct IsMoveConstructible: IsConstructible<_T,
        AddRvalueReference<_T>
    > {};

    /* is assignable */

namespace detail {
    template<typename _T, typename _U> typename octa::detail::Select2nd<
        decltype((declval_in<_T>() = declval_in<_U>())), True
    >::Type assign_test(_T &&, _U &&);

    template<typename _T> False assign_test(Any, _T &&);

    template<typename _T, typename _U, bool = octa::IsVoid<_T>::value ||
                                              octa::IsVoid<_U>::value
    > struct IsAssignableBase: CommonTypeBase<
        decltype(assign_test(declval_in<_T>(), declval_in<_U>()))
    >::Type {};

    template<typename _T, typename _U>
    struct IsAssignableBase<_T, _U, true>: False {};
} /* namespace detail */

template<typename _T, typename _U>
struct IsAssignable: octa::detail::IsAssignableBase<_T, _U> {};

    /* is copy assignable */

    template<typename _T> struct IsCopyAssignable: IsAssignable<
        AddLvalueReference<_T>,
        AddLvalueReference<AddConst<_T>>
    > {};

    /* is move assignable */

    template<typename _T> struct IsMoveAssignable: IsAssignable<
        AddLvalueReference<_T>,
        const AddRvalueReference<_T>
    > {};

    /* is destructible */

    template<typename> struct __OctaIsDtibleApply { typedef int Type; };

    template<typename _T> struct IsDestructorWellformed {
        template<typename _TT> static char __test(typename __OctaIsDtibleApply<
            decltype(octa::detail::declval_in<_TT &>().~_TT())
        >::Type);

        template<typename _TT> static int __test(...);

        static constexpr bool value = (sizeof(__test<_T>(12)) == sizeof(char));
    };

    template<typename, bool> struct __OctaDtibleImpl;

    template<typename _T>
    struct __OctaDtibleImpl<_T, false>: IntegralConstant<bool,
        IsDestructorWellformed<RemoveAllExtents<_T>>::value
    > {};

    template<typename _T>
    struct __OctaDtibleImpl<_T, true>: True {};

    template<typename _T, bool> struct __OctaDtibleFalse;

    template<typename _T> struct __OctaDtibleFalse<_T, false>
        : __OctaDtibleImpl<_T, IsReference<_T>::value> {};

    template<typename _T> struct __OctaDtibleFalse<_T, true>: False {};

    template<typename _T>
    struct IsDestructible: __OctaDtibleFalse<_T, IsFunction<_T>::value> {};

    template<typename _T> struct IsDestructible<_T[]>: False {};
    template<           > struct IsDestructible<void>: False {};

    /* is trivially constructible */

    template<typename _T, typename ..._A>
    struct IsTriviallyConstructible: False {};

    template<typename _T>
    struct IsTriviallyConstructible<_T>: IntegralConstant<bool,
        __has_trivial_constructor(_T)
    > {};

    template<typename _T>
    struct IsTriviallyConstructible<_T, _T &>: IntegralConstant<bool,
        __has_trivial_copy(_T)
    > {};

    template<typename _T>
    struct IsTriviallyConstructible<_T, const _T &>: IntegralConstant<bool,
        __has_trivial_copy(_T)
    > {};

    template<typename _T>
    struct IsTriviallyConstructible<_T, _T &&>: IntegralConstant<bool,
        __has_trivial_copy(_T)
    > {};

    /* is trivially default constructible */

    template<typename _T>
    struct IsTriviallyDefaultConstructible: IsTriviallyConstructible<_T> {};

    /* is trivially copy constructible */

    template<typename _T>
    struct IsTriviallyCopyConstructible: IsTriviallyConstructible<_T,
        AddLvalueReference<const _T>
    > {};

    /* is trivially move constructible */

    template<typename _T>
    struct IsTriviallyMoveConstructible: IsTriviallyConstructible<_T,
        AddRvalueReference<_T>
    > {};

    /* is trivially assignable */

    template<typename _T, typename ..._A>
    struct IsTriviallyAssignable: False {};

    template<typename _T>
    struct IsTriviallyAssignable<_T>: IntegralConstant<bool,
        __has_trivial_assign(_T)
    > {};

    template<typename _T>
    struct IsTriviallyAssignable<_T, _T &>: IntegralConstant<bool,
        __has_trivial_copy(_T)
    > {};

    template<typename _T>
    struct IsTriviallyAssignable<_T, const _T &>: IntegralConstant<bool,
        __has_trivial_copy(_T)
    > {};

    template<typename _T>
    struct IsTriviallyAssignable<_T, _T &&>: IntegralConstant<bool,
        __has_trivial_copy(_T)
    > {};

    /* is trivially copy assignable */

    template<typename _T>
    struct IsTriviallyCopyAssignable: IsTriviallyAssignable<_T,
        AddLvalueReference<const _T>
    > {};

    /* is trivially move assignable */

    template<typename _T>
    struct IsTriviallyMoveAssignable: IsTriviallyAssignable<_T,
        AddRvalueReference<_T>
    > {};

    /* is trivially destructible */

    template<typename _T>
    struct IsTriviallyDestructible: IntegralConstant<bool,
        __has_trivial_destructor(_T)
    > {};

    /* is base of */

    template<typename _B, typename _D>
    struct IsBaseOf: IntegralConstant<bool, __is_base_of(_B, _D)> {};

    /* is convertible */

    template<typename _F, typename _T, bool = IsVoid<_F>::value
        || IsFunction<_T>::value || IsArray<_T>::value
    > struct __OctaIsConvertible {
        typedef typename IsVoid<_T>::Type Type;
    };

    template<typename _F, typename _T>
    struct __OctaIsConvertible<_F, _T, false> {
        template<typename _TT> static void __test_f(_TT);

        template<typename _FF, typename _TT,
            typename = decltype(__test_f<_TT>(octa::detail::declval_in<_FF>()))
        > static True __test(int);

        template<typename, typename> static False __test(...);

        typedef decltype(__test<_F, _T>(0)) Type;
    };

    template<typename _F, typename _T>
    struct IsConvertible: __OctaIsConvertible<_F, _T>::Type {};

    /* type equality */

    template<typename, typename> struct IsSame        : False {};
    template<typename _T       > struct IsSame<_T, _T>:  True {};

    /* extent */

    template<typename _T, unsigned _I = 0>
    struct Extent: IntegralConstant<size_t, 0> {};

    template<typename _T>
    struct Extent<_T[], 0>: IntegralConstant<size_t, 0> {};

    template<typename _T, unsigned _I>
    struct Extent<_T[], _I>: IntegralConstant<size_t, Extent<_T, _I - 1>::value> {};

    template<typename _T, size_t _N>
    struct Extent<_T[_N], 0>: IntegralConstant<size_t, _N> {};

    template<typename _T, size_t _N, unsigned _I>
    struct Extent<_T[_N], _I>: IntegralConstant<size_t, Extent<_T, _I - 1>::value> {};

    /* rank */

    template<typename _T> struct Rank: IntegralConstant<size_t, 0> {};

    template<typename _T>
    struct Rank<_T[]>: IntegralConstant<size_t, Rank<_T>::value + 1> {};

    template<typename _T, size_t _N>
    struct Rank<_T[_N]>: IntegralConstant<size_t, Rank<_T>::value + 1> {};

    /* remove const, volatile, cv */

    template<typename _T>
    struct __OctaRemoveConst           { typedef _T Type; };
    template<typename _T>
    struct __OctaRemoveConst<const _T> { typedef _T Type; };

    template<typename _T>
    struct __OctaRemoveVolatile              { typedef _T Type; };
    template<typename _T>
    struct __OctaRemoveVolatile<volatile _T> { typedef _T Type; };

    template<typename _T>
    using RemoveConst = typename __OctaRemoveConst<_T>::Type;
    template<typename _T>
    using RemoveVolatile = typename __OctaRemoveVolatile<_T>::Type;

namespace detail {
    template<typename _T>
    struct RemoveCv {
        typedef octa::RemoveVolatile<octa::RemoveConst<_T>> Type;
    };
}

    /* add const, volatile, cv */

    template<typename _T, bool = IsReference<_T>::value
         || IsFunction<_T>::value || IsConst<_T>::value>
    struct __OctaAddConstBase { typedef _T Type; };

    template<typename _T> struct __OctaAddConstBase<_T, false> {
        typedef const _T Type;
    };

    template<typename _T> struct __OctaAddConst {
        typedef typename __OctaAddConstBase<_T>::Type Type;
    };

    template<typename _T, bool = IsReference<_T>::value
         || IsFunction<_T>::value || IsVolatile<_T>::value>
    struct __OctaAddVolatileBase { typedef _T Type; };

    template<typename _T> struct __OctaAddVolatileBase<_T, false> {
        typedef volatile _T Type;
    };

    template<typename _T> struct __OctaAddVolatile {
        typedef typename __OctaAddVolatileBase<_T>::Type Type;
    };

    template<typename _T>
    using AddVolatile = typename __OctaAddVolatile<_T>::Type;

    template<typename _T>
    struct __OctaAddCv {
        typedef AddConst<AddVolatile<_T>> Type;
    };

    template<typename _T>
    using AddCv = typename __OctaAddCv<_T>::Type;

    /* remove reference */

    template<typename _T>
    struct __OctaRemoveReference        { typedef _T Type; };
    template<typename _T>
    struct __OctaRemoveReference<_T &>  { typedef _T Type; };
    template<typename _T>
    struct __OctaRemoveReference<_T &&> { typedef _T Type; };

    /* remove pointer */

    template<typename _T>
    struct __OctaRemovePointer                      { typedef _T Type; };
    template<typename _T>
    struct __OctaRemovePointer<_T *               > { typedef _T Type; };
    template<typename _T>
    struct __OctaRemovePointer<_T * const         > { typedef _T Type; };
    template<typename _T>
    struct __OctaRemovePointer<_T * volatile      > { typedef _T Type; };
    template<typename _T>
    struct __OctaRemovePointer<_T * const volatile> { typedef _T Type; };

    template<typename _T>
    using RemovePointer = typename __OctaRemovePointer<_T>::Type;

    /* add pointer */

    template<typename _T> struct __OctaAddPointer {
        typedef RemoveReference<_T> *Type;
    };

    template<typename _T>
    using AddPointer = typename __OctaAddPointer<_T>::Type;

    /* add lvalue reference */

    template<typename _T> struct __OctaAddLr        { typedef _T &Type; };
    template<typename _T> struct __OctaAddLr<_T  &> { typedef _T &Type; };
    template<typename _T> struct __OctaAddLr<_T &&> { typedef _T &Type; };
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

    template<typename _T> struct __OctaAddRr        { typedef _T &&Type; };
    template<typename _T> struct __OctaAddRr<_T  &> { typedef _T &&Type; };
    template<typename _T> struct __OctaAddRr<_T &&> { typedef _T &&Type; };
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

    template<typename _T>
    struct __OctaRemoveExtent         { typedef _T Type; };
    template<typename _T>
    struct __OctaRemoveExtent<_T[ ] > { typedef _T Type; };
    template<typename _T, size_t _N>
    struct __OctaRemoveExtent<_T[_N]> { typedef _T Type; };

    template<typename _T>
    using RemoveExtent = typename __OctaRemoveExtent<_T>::Type;

    /* remove all extents */

    template<typename _T> struct __OctaRemoveAllExtents { typedef _T Type; };

    template<typename _T> struct __OctaRemoveAllExtents<_T[]> {
        typedef RemoveAllExtents<_T> Type;
    };

    template<typename _T, size_t _N> struct __OctaRemoveAllExtents<_T[_N]> {
        typedef RemoveAllExtents<_T> Type;
    };

/* make (un)signed
 *
 * this is bad, but i don't see any better way
 * shamelessly copied from graphitemaster @ neothyne
 */

namespace detail {
    template<typename _T, typename _U> struct TypeList {
        typedef _T first;
        typedef _U rest;
    };

    /* not a type */
    struct TlNat {
        TlNat() = delete;
        TlNat(const TlNat &) = delete;
        TlNat &operator=(const TlNat &) = delete;
        ~TlNat() = delete;
    };

    typedef TypeList<schar,
            TypeList<short,
            TypeList<int,
            TypeList<long,
            TypeList<llong, TlNat>>>>> stypes;

    typedef TypeList<uchar,
            TypeList<ushort,
            TypeList<uint,
            TypeList<ulong,
            TypeList<ullong, TlNat>>>>> utypes;

    template<typename _T, size_t _N, bool = (_N <= sizeof(typename _T::first))>
    struct TypeFindFirst;

    template<typename _T, typename _U, size_t _N>
    struct TypeFindFirst<TypeList<_T, _U>, _N, true> {
        typedef _T Type;
    };

    template<typename _T, typename _U, size_t _N>
    struct TypeFindFirst<TypeList<_T, _U>, _N, false> {
        typedef typename TypeFindFirst<_U, _N>::Type Type;
    };

    template<typename _T, typename _U,
        bool = octa::IsConst<octa::RemoveReference<_T>>::value,
        bool = octa::IsVolatile<octa::RemoveReference<_T>>::value
    > struct ApplyCv {
        typedef _U Type;
    };

    template<typename _T, typename _U>
    struct ApplyCv<_T, _U, true, false> { /* const */
        typedef const _U Type;
    };

    template<typename _T, typename _U>
    struct ApplyCv<_T, _U, false, true> { /* volatile */
        typedef volatile _U Type;
    };

    template<typename _T, typename _U>
    struct ApplyCv<_T, _U, true, true> { /* const volatile */
        typedef const volatile _U Type;
    };

    template<typename _T, typename _U>
    struct ApplyCv<_T &, _U, true, false> { /* const */
        typedef const _U &Type;
    };

    template<typename _T, typename _U>
    struct ApplyCv<_T &, _U, false, true> { /* volatile */
        typedef volatile _U &Type;
    };

    template<typename _T, typename _U>
    struct ApplyCv<_T &, _U, true, true> { /* const volatile */
        typedef const volatile _U &Type;
    };

    template<typename _T, bool = octa::IsIntegral<_T>::value ||
                                 octa::IsEnum<_T>::value>
    struct MakeSigned {};

    template<typename _T, bool = octa::IsIntegral<_T>::value ||
                                 octa::IsEnum<_T>::value>
    struct MakeUnsigned {};

    template<typename _T>
    struct MakeSigned<_T, true> {
        typedef typename TypeFindFirst<stypes, sizeof(_T)>::Type Type;
    };

    template<typename _T>
    struct MakeUnsigned<_T, true> {
        typedef typename TypeFindFirst<utypes, sizeof(_T)>::Type Type;
    };

    template<> struct MakeSigned<bool  , true> {};
    template<> struct MakeSigned<schar , true> { typedef schar Type; };
    template<> struct MakeSigned<uchar , true> { typedef schar Type; };
    template<> struct MakeSigned<short , true> { typedef short Type; };
    template<> struct MakeSigned<ushort, true> { typedef short Type; };
    template<> struct MakeSigned<int   , true> { typedef int   Type; };
    template<> struct MakeSigned<uint  , true> { typedef int   Type; };
    template<> struct MakeSigned<long  , true> { typedef long  Type; };
    template<> struct MakeSigned<ulong , true> { typedef long  Type; };
    template<> struct MakeSigned<llong , true> { typedef llong Type; };
    template<> struct MakeSigned<ullong, true> { typedef llong Type; };

    template<> struct MakeUnsigned<bool  , true> {};
    template<> struct MakeUnsigned<schar , true> { typedef uchar  Type; };
    template<> struct MakeUnsigned<uchar , true> { typedef uchar  Type; };
    template<> struct MakeUnsigned<short , true> { typedef ushort Type; };
    template<> struct MakeUnsigned<ushort, true> { typedef ushort Type; };
    template<> struct MakeUnsigned<int   , true> { typedef uint   Type; };
    template<> struct MakeUnsigned<uint  , true> { typedef uint   Type; };
    template<> struct MakeUnsigned<long  , true> { typedef ulong  Type; };
    template<> struct MakeUnsigned<ulong , true> { typedef ulong  Type; };
    template<> struct MakeUnsigned<llong , true> { typedef ullong Type; };
    template<> struct MakeUnsigned<ullong, true> { typedef ullong Type; };

    template<typename _T> struct MakeSignedBase {
        typedef typename ApplyCv<_T,
            typename MakeSigned<octa::RemoveCv<_T>>::Type
        >::Type Type;
    };

    template<typename _T> struct MakeUnsignedBase {
        typedef typename ApplyCv<_T,
            typename MakeUnsigned<octa::RemoveCv<_T>>::Type
        >::Type Type;
    };
} /* namespace detail */

template<typename _T>
using MakeSigned = typename octa::detail::MakeSignedBase<_T>::Type;
template<typename _T>
using MakeUnsigned = typename octa::detail::MakeUnsignedBase<_T>::Type;

/* conditional */

namespace detail {
    template<bool _cond, typename _T, typename _U>
    struct ConditionalBase {
        typedef _T Type;
    };

    template<typename _T, typename _U>
    struct ConditionalBase<false, _T, _U> {
        typedef _U Type;
    };
}

template<bool _cond, typename _T, typename _U>
using Conditional = typename octa::detail::ConditionalBase<_cond, _T, _U>::Type;

/* result of call at compile time */

namespace detail {
#define OCTA_FWD(_T, _v) static_cast<_T &&>(_v)
    template<typename _F, typename ..._A>
    inline auto rof_invoke(_F &&f, _A &&...args) ->
      decltype(OCTA_FWD(_F, f)(OCTA_FWD(_A, args)...)) {
        return OCTA_FWD(_F, f)(OCTA_FWD(_A, args)...);
    }
    template<typename _B, typename _T, typename _D>
    inline auto rof_invoke(_T _B::*pmd, _D &&ref) ->
      decltype(OCTA_FWD(_D, ref).*pmd) {
        return OCTA_FWD(_D, ref).*pmd;
    }
    template<typename _PMD, typename _P>
    inline auto rof_invoke(_PMD &&pmd, _P &&ptr) ->
      decltype((*OCTA_FWD(_P, ptr)).*OCTA_FWD(_PMD, pmd)) {
        return (*OCTA_FWD(_P, ptr)).*OCTA_FWD(_PMD, pmd);
    }
    template<typename _B, typename _T, typename _D, typename ..._A>
    inline auto rof_invoke(_T _B::*pmf, _D &&ref, _A &&...args) ->
      decltype((OCTA_FWD(_D, ref).*pmf)(OCTA_FWD(_A, args)...)) {
        return (OCTA_FWD(_D, ref).*pmf)(OCTA_FWD(_A, args)...);
    }
    template<typename _PMF, typename _P, typename ..._A>
    inline auto rof_invoke(_PMF &&pmf, _P &&ptr, _A &&...args) ->
      decltype(((*OCTA_FWD(_P, ptr)).*OCTA_FWD(_PMF, pmf))
          (OCTA_FWD(_A, args)...)) {
        return ((*OCTA_FWD(_P, ptr)).*OCTA_FWD(_PMF, pmf))
          (OCTA_FWD(_A, args)...);
    }
#undef OCTA_FWD

    template<typename, typename = void>
    struct ResultOfCore {};
    template<typename _F, typename ..._A>
    struct ResultOfCore<_F(_A...), decltype(void(rof_invoke(
    octa::detail::declval_in<_F>(), octa::detail::declval_in<_A>()...)))> {
        using type = decltype(rof_invoke(octa::detail::declval_in<_F>(),
            octa::detail::declval_in<_A>()...));
    };

    template<typename _T> struct ResultOfBase: ResultOfCore<_T> {};
} /* namespace detail */

template<typename _T>
using ResultOf = typename octa::detail::ResultOfBase<_T>::Type;

/* enable if */

namespace detail {
    template<bool _B, typename _T = void> struct EnableIfBase {};

    template<typename _T> struct EnableIfBase<true, _T> { typedef _T Type; };
}

template<bool _B, typename _T = void>
using EnableIf = typename octa::detail::EnableIfBase<_B, _T>::Type;

/* decay */

namespace detail {
    template<typename _T>
    struct DecayBase {
    private:
        typedef octa::RemoveReference<_T> _U;
    public:
        typedef octa::Conditional<octa::IsArray<_U>::value,
            octa::RemoveExtent<_U> *,
            octa::Conditional<octa::IsFunction<_U>::value,
                octa::AddPointer<_U>, octa::RemoveCv<_U>>
        > Type;
    };
}

template<typename _T>
using Decay = typename octa::detail::DecayBase<_T>::Type;

/* common type */

namespace detail {
    template<typename ..._T> struct CommonTypeBase;

    template<typename _T> struct CommonTypeBase<_T> {
        typedef Decay<_T> Type;
    };

    template<typename _T, typename _U> struct CommonTypeBase<_T, _U> {
        typedef Decay<decltype(true ? octa::detail::declval_in<_T>()
            : octa::detail::declval_in<_U>())> Type;
    };

    template<typename _T, typename _U, typename ..._V>
    struct CommonTypeBase<_T, _U, _V...> {
        typedef typename CommonTypeBase<typename CommonTypeBase<_T, _U>::Type,
            _V...>::Type Type;
    };
}

template<typename _T, typename _U, typename ..._V>
using CommonType = typename octa::detail::CommonTypeBase<_T, _U, _V...>::Type;

    /* aligned storage */

    template<size_t _N> struct __OctaAlignedTest {
        union type {
            uchar __data[_N];
            octa::max_align_t __align;
        };
    };

    template<size_t _N, size_t _A> struct __OctaAlignedStorage {
        struct type {
            alignas(_A) uchar __data[_N];
        };
    };

    template<size_t _N, size_t _A
        = alignof(typename __OctaAlignedTest<_N>::Type)
    > using AlignedStorage = typename __OctaAlignedStorage<_N, _A>::Type;

    /* aligned union */

    template<size_t ..._N> struct __OctaAlignMax;

    template<size_t _N> struct __OctaAlignMax<_N> {
        static constexpr size_t value = _N;
    };

    template<size_t _N1, size_t _N2> struct __OctaAlignMax<_N1, _N2> {
        static constexpr size_t value = (_N1 > _N2) ? _N1 : _N2;
    };

    template<size_t _N1, size_t _N2, size_t ..._N>
    struct __OctaAlignMax<_N1, _N2, _N...> {
        static constexpr size_t value
            = __OctaAlignMax<__OctaAlignMax<_N1, _N2>::value, _N...>::value;
    };

    template<size_t _N, typename ..._T> struct __OctaAlignedUnion {
        static constexpr size_t __alignment_value
            = __OctaAlignMax<alignof(_T)...>::value;

        struct type {
            alignas(__alignment_value) uchar __data[__OctaAlignMax<_N,
                sizeof(_T)...>::value];
        };
    };

    template<size_t _N, typename ..._T>
    using AlignedUnion = typename __OctaAlignedUnion<_N, _T...>::Type;

    /* underlying type */

    /* gotta wrap, in a struct otherwise clang ICEs... */
    template<typename _T> struct __OctaUnderlyingType {
        typedef __underlying_type(_T) Type;
    };

    template<typename _T>
    using UnderlyingType = typename __OctaUnderlyingType<_T>::Type;
}

#endif