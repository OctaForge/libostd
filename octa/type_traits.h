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
    template<typename> struct RemoveCvBase;
    template<typename> struct AddLr;
    template<typename> struct AddRr;
    template<typename> struct AddConstBase;
    template<typename> struct RemoveReferenceBase;
    template<typename> struct RemoveAllExtentsBase;

    template<typename ...> struct CommonTypeBase;
}

template<typename> struct IsReference;
template<typename> struct IsTriviallyDefaultConstructible;

template<typename _T>
using RemoveCv = typename octa::detail::RemoveCvBase<_T>::Type;

template<typename _T>
using AddLvalueReference = typename octa::detail::AddLr<_T>::Type;

template<typename _T>
using AddRvalueReference = typename octa::detail::AddRr<_T>::Type;

template<typename _T>
using AddConst = typename octa::detail::AddConstBase<_T>::Type;

template<typename _T>
using RemoveReference = typename octa::detail::RemoveReferenceBase<_T>::Type;

template<typename _T>
using RemoveAllExtents = typename octa::detail::RemoveAllExtentsBase<_T>::Type;

namespace detail {
    template<typename _T> octa::AddRvalueReference<_T> declval_in();
}

/* integral constant */

template<typename _T, _T val>
struct IntegralConstant {
    static constexpr _T value = val;

    typedef _T Value;
    typedef IntegralConstant<_T, val> Type;

    constexpr operator Value() const { return value; }
    constexpr Value operator()() const { return value; }
};

typedef IntegralConstant<bool, true>  True;
typedef IntegralConstant<bool, false> False;

template<typename _T, _T val> constexpr _T IntegralConstant<_T, val>::value;

/* is void */

namespace detail {
    template<typename _T> struct IsVoidBase      : False {};
    template<           > struct IsVoidBase<void>:  True {};
}

    template<typename _T>
    struct IsVoid: octa::detail::IsVoidBase<RemoveCv<_T>> {};

/* is null pointer */

namespace detail {
    template<typename> struct IsNullPointerBase           : False {};
    template<        > struct IsNullPointerBase<nullptr_t>:  True {};
}

template<typename _T> struct IsNullPointer:
    octa::detail::IsNullPointerBase<RemoveCv<_T>> {};

/* is integer */

namespace detail {
    template<typename _T> struct IsIntegralBase: False {};

    template<> struct IsIntegralBase<bool  >: True {};
    template<> struct IsIntegralBase<char  >: True {};
    template<> struct IsIntegralBase<uchar >: True {};
    template<> struct IsIntegralBase<schar >: True {};
    template<> struct IsIntegralBase<short >: True {};
    template<> struct IsIntegralBase<ushort>: True {};
    template<> struct IsIntegralBase<int   >: True {};
    template<> struct IsIntegralBase<uint  >: True {};
    template<> struct IsIntegralBase<long  >: True {};
    template<> struct IsIntegralBase<ulong >: True {};
    template<> struct IsIntegralBase<llong >: True {};
    template<> struct IsIntegralBase<ullong>: True {};

    template<> struct IsIntegralBase<char16_t>: True {};
    template<> struct IsIntegralBase<char32_t>: True {};
    template<> struct IsIntegralBase< wchar_t>: True {};
}

template<typename _T>
struct IsIntegral: octa::detail::IsIntegralBase<RemoveCv<_T>> {};

/* is floating point */

namespace detail {
    template<typename _T> struct IsFloatingPointBase: False {};

    template<> struct IsFloatingPointBase<float  >: True {};
    template<> struct IsFloatingPointBase<double >: True {};
    template<> struct IsFloatingPointBase<ldouble>: True {};
}

template<typename _T>
struct IsFloatingPoint: octa::detail::IsFloatingPointBase<RemoveCv<_T>> {};

/* is array */

template<typename              > struct IsArray        : False {};
template<typename _T           > struct IsArray<_T[]  >:  True {};
template<typename _T, size_t _N> struct IsArray<_T[_N]>:  True {};

/* is pointer */

namespace detail {
    template<typename   > struct IsPointerBase      : False {};
    template<typename _T> struct IsPointerBase<_T *>:  True {};
}

template<typename _T>
struct IsPointer: octa::detail::IsPointerBase<RemoveCv<_T>> {};

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

namespace detail {
    template<typename>
    struct IsMemberPointerBase: False {};

    template<typename _T, typename _U>
    struct IsMemberPointerBase<_T _U::*>: True {};
}

template<typename _T>
struct IsMemberPointer: octa::detail::IsMemberPointerBase<RemoveCv<_T>> {};

/* is pointer to member object */

namespace detail {
    template<typename>
    struct IsMemberObjectPointerBase: False {};

    template<typename _T, typename _U>
    struct IsMemberObjectPointerBase<_T _U::*>: IntegralConstant<bool,
        !octa::IsFunction<_T>::value
    > {};
}

template<typename _T> struct IsMemberObjectPointer:
    octa::detail::IsMemberObjectPointerBase<RemoveCv<_T>> {};

/* is pointer to member function */

namespace detail {
    template<typename>
    struct IsMemberFunctionPointerBase: False {};

    template<typename _T, typename _U>
    struct IsMemberFunctionPointerBase<_T _U::*>: IntegralConstant<bool,
        octa::IsFunction<_T>::value
    > {};
}

template<typename _T> struct IsMemberFunctionPointer:
    octa::detail::IsMemberFunctionPointerBase<RemoveCv<_T>> {};

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

namespace detail {
    template<typename> struct IsDtibleApply { typedef int Type; };

    template<typename _T> struct IsDestructorWellformed {
        template<typename _TT> static char test(typename IsDtibleApply<
            decltype(octa::detail::declval_in<_TT &>().~_TT())
        >::Type);

        template<typename _TT> static int test(...);

        static constexpr bool value = (sizeof(test<_T>(12)) == sizeof(char));
    };

    template<typename, bool> struct DtibleImpl;

    template<typename _T>
    struct DtibleImpl<_T, false>: octa::IntegralConstant<bool,
        IsDestructorWellformed<octa::RemoveAllExtents<_T>>::value
    > {};

    template<typename _T>
    struct DtibleImpl<_T, true>: True {};

    template<typename _T, bool> struct DtibleFalse;

    template<typename _T> struct DtibleFalse<_T, false>
        : DtibleImpl<_T, octa::IsReference<_T>::value> {};

    template<typename _T> struct DtibleFalse<_T, true>: False {};
} /* namespace detail */

template<typename _T>
struct IsDestructible: octa::detail::DtibleFalse<_T, IsFunction<_T>::value> {};

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

namespace detail {
    template<typename _F, typename _T, bool = octa::IsVoid<_F>::value
        || octa::IsFunction<_T>::value || octa::IsArray<_T>::value
    > struct IsConvertibleBase {
        typedef typename octa::IsVoid<_T>::Type Type;
    };

    template<typename _F, typename _T>
    struct IsConvertibleBase<_F, _T, false> {
        template<typename _TT> static void test_f(_TT);

        template<typename _FF, typename _TT,
            typename = decltype(test_f<_TT>(declval_in<_FF>()))
        > static octa::True test(int);

        template<typename, typename> static octa::False test(...);

        typedef decltype(test<_F, _T>(0)) Type;
    };
}

template<typename _F, typename _T>
struct IsConvertible: octa::detail::IsConvertibleBase<_F, _T>::Type {};

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

namespace detail {
    template<typename _T>
    struct RemoveConstBase           { typedef _T Type; };
    template<typename _T>
    struct RemoveConstBase<const _T> { typedef _T Type; };

    template<typename _T>
    struct RemoveVolatileBase              { typedef _T Type; };
    template<typename _T>
    struct RemoveVolatileBase<volatile _T> { typedef _T Type; };
}

template<typename _T>
using RemoveConst = typename octa::detail::RemoveConstBase<_T>::Type;
template<typename _T>
using RemoveVolatile = typename octa::detail::RemoveVolatileBase<_T>::Type;

namespace detail {
    template<typename _T>
    struct RemoveCvBase {
        typedef octa::RemoveVolatile<octa::RemoveConst<_T>> Type;
    };
}

/* add const, volatile, cv */

namespace detail {
    template<typename _T, bool = octa::IsReference<_T>::value
         || octa::IsFunction<_T>::value || octa::IsConst<_T>::value>
    struct AddConstCore { typedef _T Type; };

    template<typename _T> struct AddConstCore<_T, false> {
        typedef const _T Type;
    };

    template<typename _T> struct AddConstBase {
        typedef typename AddConstCore<_T>::Type Type;
    };

    template<typename _T, bool = octa::IsReference<_T>::value
         || octa::IsFunction<_T>::value || octa::IsVolatile<_T>::value>
    struct AddVolatileCore { typedef _T Type; };

    template<typename _T> struct AddVolatileCore<_T, false> {
        typedef volatile _T Type;
    };

    template<typename _T> struct AddVolatileBase {
        typedef typename AddVolatileCore<_T>::Type Type;
    };
}

template<typename _T>
using AddVolatile = typename octa::detail::AddVolatileBase<_T>::Type;

namespace detail {
    template<typename _T>
    struct AddCvBase {
        typedef octa::AddConst<octa::AddVolatile<_T>> Type;
    };
}

template<typename _T>
using AddCv = typename octa::detail::AddCvBase<_T>::Type;

/* remove reference */

namespace detail {
    template<typename _T>
    struct RemoveReferenceBase        { typedef _T Type; };
    template<typename _T>
    struct RemoveReferenceBase<_T &>  { typedef _T Type; };
    template<typename _T>
    struct RemoveReferenceBase<_T &&> { typedef _T Type; };
}

/* remove pointer */

namespace detail {
    template<typename _T>
    struct RemovePointerBase                      { typedef _T Type; };
    template<typename _T>
    struct RemovePointerBase<_T *               > { typedef _T Type; };
    template<typename _T>
    struct RemovePointerBase<_T * const         > { typedef _T Type; };
    template<typename _T>
    struct RemovePointerBase<_T * volatile      > { typedef _T Type; };
    template<typename _T>
    struct RemovePointerBase<_T * const volatile> { typedef _T Type; };
}

template<typename _T>
using RemovePointer = typename octa::detail::RemovePointerBase<_T>::Type;

/* add pointer */

namespace detail {
    template<typename _T> struct AddPointerBase {
        typedef octa::RemoveReference<_T> *Type;
    };
}

template<typename _T>
using AddPointer = typename octa::detail::AddPointerBase<_T>::Type;

/* add lvalue reference */

namespace detail {
    template<typename _T> struct AddLr        { typedef _T &Type; };
    template<typename _T> struct AddLr<_T  &> { typedef _T &Type; };
    template<typename _T> struct AddLr<_T &&> { typedef _T &Type; };
    template<> struct AddLr<void> {
        typedef void Type;
    };
    template<> struct AddLr<const void> {
        typedef const void Type;
    };
    template<> struct AddLr<volatile void> {
        typedef volatile void Type;
    };
    template<> struct AddLr<const volatile void> {
        typedef const volatile void Type;
    };
}

/* add rvalue reference */

namespace detail {
    template<typename _T> struct AddRr        { typedef _T &&Type; };
    template<typename _T> struct AddRr<_T  &> { typedef _T &&Type; };
    template<typename _T> struct AddRr<_T &&> { typedef _T &&Type; };
    template<> struct AddRr<void> {
        typedef void Type;
    };
    template<> struct AddRr<const void> {
        typedef const void Type;
    };
    template<> struct AddRr<volatile void> {
        typedef volatile void Type;
    };
    template<> struct AddRr<const volatile void> {
        typedef const volatile void Type;
    };
}

/* remove extent */

namespace detail {
    template<typename _T>
    struct RemoveExtentBase         { typedef _T Type; };
    template<typename _T>
    struct RemoveExtentBase<_T[ ] > { typedef _T Type; };
    template<typename _T, size_t _N>
    struct RemoveExtentBase<_T[_N]> { typedef _T Type; };
}

template<typename _T>
using RemoveExtent = typename octa::detail::RemoveExtentBase<_T>::Type;

/* remove all extents */

namespace detail {
    template<typename _T> struct RemoveAllExtentsBase { typedef _T Type; };

    template<typename _T> struct RemoveAllExtentsBase<_T[]> {
        typedef RemoveAllExtentsBase<_T> Type;
    };

    template<typename _T, size_t _N> struct RemoveAllExtentsBase<_T[_N]> {
        typedef RemoveAllExtentsBase<_T> Type;
    };
}

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

namespace detail {
    template<size_t _N> struct AlignedTest {
        union type {
            uchar data[_N];
            octa::max_align_t align;
        };
    };

    template<size_t _N, size_t _A> struct AlignedStorageBase {
        struct type {
            alignas(_A) uchar data[_N];
        };
    };
}

template<size_t _N, size_t _A
    = alignof(typename octa::detail::AlignedTest<_N>::Type)
> using AlignedStorage = typename octa::detail::AlignedStorageBase<_N, _A>::Type;

/* aligned union */

namespace detail {
    template<size_t ..._N> struct AlignMax;

    template<size_t _N> struct AlignMax<_N> {
        static constexpr size_t value = _N;
    };

    template<size_t _N1, size_t _N2> struct AlignMax<_N1, _N2> {
        static constexpr size_t value = (_N1 > _N2) ? _N1 : _N2;
    };

    template<size_t _N1, size_t _N2, size_t ..._N>
    struct AlignMax<_N1, _N2, _N...> {
        static constexpr size_t value
            = AlignMax<AlignMax<_N1, _N2>::value, _N...>::value;
    };

    template<size_t _N, typename ..._T> struct AlignedUnionBase {
        static constexpr size_t alignment_value
            = AlignMax<alignof(_T)...>::value;

        struct type {
            alignas(alignment_value) uchar data[AlignMax<_N,
                sizeof(_T)...>::value];
        };
    };
} /* namespace detail */

template<size_t _N, typename ..._T>
using AlignedUnion = typename octa::detail::AlignedUnionBase<_N, _T...>::Type;

/* underlying type */

namespace detail {
    /* gotta wrap, in a struct otherwise clang ICEs... */
    template<typename _T> struct UnderlyingTypeBase {
        typedef __underlying_type(_T) Type;
    };
}

template<typename _T>
using UnderlyingType = typename octa::detail::UnderlyingTypeBase<_T>::Type;
}

#endif