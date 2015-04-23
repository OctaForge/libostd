/* Type traits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TRAITS_H
#define OCTA_TRAITS_H

#include <stddef.h>

#include "octa/types.h"
#include "octa/utility.h"

/* libc++ and cppreference.com occasionally used for reference */

/* missing:
 *
 * UnderlyingType<T>
 */

namespace octa {
    template<typename> struct RemoveConstVolatile;

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

    template<typename T> struct IsVoidBase      : false_t {};
    template<          > struct IsVoidBase<void>: true_t  {};

    template<typename T>
    struct IsVoid: IsVoidBase<typename RemoveConstVolatile<T>::type> {};

    /* is null pointer */

    template<typename> struct IsNullPointerBase           : false_t {};
    template<        > struct IsNullPointerBase<nullptr_t>:  true_t {};

    template<typename T>
    struct IsNullPointer: IsNullPointerBase<
        typename RemoveConstVolatile<T>::type
    > {};

    /* is integer */

    template<typename T> struct IsIntegralBase: false_t {};

    template<> struct IsIntegralBase<bool  >: true_t {};
    template<> struct IsIntegralBase<char  >: true_t {};
    template<> struct IsIntegralBase<uchar >: true_t {};
    template<> struct IsIntegralBase<schar >: true_t {};
    template<> struct IsIntegralBase<short >: true_t {};
    template<> struct IsIntegralBase<ushort>: true_t {};
    template<> struct IsIntegralBase<int   >: true_t {};
    template<> struct IsIntegralBase<uint  >: true_t {};
    template<> struct IsIntegralBase<long  >: true_t {};
    template<> struct IsIntegralBase<ulong >: true_t {};
    template<> struct IsIntegralBase<llong >: true_t {};
    template<> struct IsIntegralBase<ullong>: true_t {};

    template<typename T>
    struct IsIntegral: IsIntegralBase<typename RemoveConstVolatile<T>::type> {};

    /* is floating point */

    template<typename T> struct IsFloatingPointBase  : false_t {};

    template<> struct IsFloatingPointBase<float  >: true_t {};
    template<> struct IsFloatingPointBase<double >: true_t {};
    template<> struct IsFloatingPointBase<ldouble>: true_t {};

    template<typename T>
    struct IsFloatingPoint: IsFloatingPointBase<typename RemoveConstVolatile<T>::type> {};

    /* is array */

    template<typename            > struct IsArray      : false_t {};
    template<typename T          > struct IsArray<T[] >:  true_t {};
    template<typename T, size_t N> struct IsArray<T[N]>:  true_t {};

    /* is pointer */

    template<typename  > struct IsPointerBase     : false_t {};
    template<typename T> struct IsPointerBase<T *>:  true_t {};

    template<typename T>
    struct IsPointer: IsPointerBase<typename RemoveConstVolatile<T>::type> {};

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

    template<typename> struct IsReference;

    namespace internal {
        struct FunctionTestDummy {};

        template<typename T> char function_test(T *);
        template<typename T> char function_test(FunctionTestDummy);
        template<typename T> int  function_test(...);

        template<typename T> T                 &function_source(int);
        template<typename T> FunctionTestDummy  function_source(...);
    }

    template<typename T, bool = IsClass<T>::value || IsUnion<T>::value
                              || IsVoid<T>::value || IsReference<T>::value
                              || IsNullPointer<T>::value
    > struct IsFunctionBase: IntegralConstant<bool,
        sizeof(internal::function_test<T>(internal::function_source<T>(0))) == 1
    > {};

    template<typename T> struct IsFunctionBase<T, true>: false_t {};

    template<typename T> struct IsFunction: IsFunctionBase<T> {};

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

    template<typename> struct IsMemberPointerBase: false_t {};
    template<typename T, typename U>
    struct IsMemberPointerBase<T U::*>: true_t {};

    template<typename T>
    struct IsMemberPointer: IsMemberPointerBase<
        typename RemoveConstVolatile<T>::type
    > {};

    /* is pointer to member object */

    template<typename> struct IsMemberObjectPointerBase: false_t {};

    template<typename T, typename U>
    struct IsMemberObjectPointerBase<T U::*>: IntegralConstant<bool,
        !IsFunction<T>::value
    > {};

    template<typename T> struct IsMemberObjectPointer:
        IsMemberObjectPointerBase<typename RemoveConstVolatile<T>::type> {};

    /* is pointer to member function */

    template<typename> struct IsMemberFunctionPointerBase: false_t {};

    template<typename T, typename U>
    struct IsMemberFunctionPointerBase<T U::*>: IntegralConstant<bool,
        IsFunction<T>::value
    > {};

    template<typename T> struct IsMemberFunctionPointer:
        IsMemberFunctionPointerBase<typename RemoveConstVolatile<T>::type> {};

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

    template<typename T> struct IsEmpty: IntegralConstant<bool, __is_empty(T)> {};

    /* is POD */

    template<typename T> struct IsPOD: IntegralConstant<bool, __is_pod(T)> {};

    /* is polymorphic */

    template<typename T>
    struct IsPolymorphic: IntegralConstant<bool, __is_polymorphic(T)> {};

    /* is signed */

    template<typename T> struct IsSigned: IntegralConstant<bool, T(-1) < T(0)> {};

    /* is unsigned */

    template<typename T> struct IsUnsigned: IntegralConstant<bool, T(0) < T(-1)> {};

    /* is standard layout */

    template<typename T> struct RemoveAllExtents;
    template<typename T> struct IsStandardLayout: IntegralConstant<bool,
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

    template<typename> struct IsTriviallyDefaultConstructible;

    template<typename T>
    struct IsTrivial: IntegralConstant<bool,
        (IsTriviallyCopyable<T>::value && IsTriviallyDefaultConstructible<T>::value)
    > {};

    /* has virtual destructor */

    template<typename T>
    struct HasVirtualDestructor: IntegralConstant<bool, __has_virtual_destructor(T)> {};

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

    template<typename> struct AddLvalueReference;

    template<typename T>
    struct IsTriviallyCopyConstructible: IsTriviallyConstructible<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is trivially move constructible */

    template<typename T>
    struct IsTriviallyMoveConstructible: IsTriviallyConstructible<T,
        typename internal::AddRvalueReference<T>::type
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

    template<typename> struct AddLvalueReference;

    template<typename T>
    struct IsTriviallyCopyAssignable: IsTriviallyAssignable<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is trivially move assignable */

    template<typename T>
    struct IsTriviallyMoveAssignable: IsTriviallyAssignable<T,
        typename internal::AddRvalueReference<T>::type
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

    template<typename> struct AddLvalueReference;

    template<typename T>
    struct IsNothrowCopyConstructible: IsNothrowConstructible<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is nothrow move constructible */

    template<typename T>
    struct IsNothrowMoveConstructible: IsNothrowConstructible<T,
        typename internal::AddRvalueReference<T>::type
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

    template<typename> struct AddLvalueReference;

    template<typename T>
    struct IsNothrowCopyAssignable: IsNothrowAssignable<T,
        typename AddLvalueReference<const T>::type
    > {};

    /* is nothrow move assignable */

    template<typename T>
    struct IsNothrowMoveAssignable: IsNothrowAssignable<T,
        typename internal::AddRvalueReference<T>::type
    > {};

    /* is base of */

    template<typename B, typename D>
    struct IsBaseOf: IntegralConstant<bool, __is_base_of(B, D)> {};

    /* is convertible */

    template<typename F, typename T, bool = IsVoid<F>::value
        || IsFunction<T>::value || IsArray<T>::value
    > struct IsConvertibleBase {
        typedef typename IsVoid<T>::type type;
    };

    template<typename F, typename T> struct IsConvertibleBase<F, T, false> {
        template<typename TT> static void test_f(TT);

        template<typename FF, typename TT,
            typename = decltype(test_f<TT>(declval<FF>()))
        > static true_t test(int);

        template<typename, typename> static false_t test(...);

        typedef decltype(test<F, T>(0)) type;
    };

    template<typename F, typename T>
    struct IsConvertible: IsConvertibleBase<F, T>::type {};

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
    struct RemoveConstVolatile {
        typedef typename RemoveVolatile<typename RemoveConst<T>::type>::type type;
    };

    /* add const, volatile, cv */

    template<typename T, bool = IsReference<T>::value
         || IsFunction<T>::value || IsConst<T>::value>
    struct AddConstBase { typedef T type; };

    template<typename T> struct AddConstBase<T, false> {
        typedef const T type;
    };

    template<typename T> struct AddConst {
        typedef typename AddConstBase<T>::type type;
    };

    template<typename T, bool = IsReference<T>::value
         || IsFunction<T>::value || IsVolatile<T>::value>
    struct AddVolatileBase { typedef T type; };

    template<typename T> struct AddVolatileBase<T, false> {
        typedef volatile T type;
    };

    template<typename T> struct AddVolatile {
        typedef typename AddVolatileBase<T>::type type;
    };

    template<typename T>
    struct AddConstVolatile {
        typedef typename AddConst<typename AddVolatile<T>::type>::type type;
    };

    /* remove reference */

    template<typename T> using RemoveReference = internal::RemoveReference<T>;

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

    template<typename T> using AddRvalueReference = internal::AddRvalueReference<T>;

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

    namespace internal {
        template<typename T, typename U> struct TypeList {
            typedef T first;
            typedef U rest;
        };

        /* not a type */
        struct NAT {
            NAT() = delete;
            NAT(const NAT &) = delete;
            NAT &operator=(const NAT &) = delete;
            ~NAT() = delete;
        };

        typedef TypeList<schar,
                TypeList<short,
                TypeList<int,
                TypeList<long,
                TypeList<llong, NAT>>>>> stypes;

        typedef TypeList<uchar,
                TypeList<ushort,
                TypeList<uint,
                TypeList<ulong,
                TypeList<ullong, NAT>>>>> utypes;

        template<typename T, size_t N, bool = (N <= sizeof(typename T::first))>
        struct TypeFindFirst;

        template<typename T, typename U, size_t N>
        struct TypeFindFirst<TypeList<T, U>, N, true> {
            typedef T type;
        };

        template<typename T, typename U, size_t N>
        struct TypeFindFirst<TypeList<T, U>, N, false> {
            typedef typename TypeFindFirst<U, N>::type type;
        };

        template<typename T, typename U,
            bool = IsConst<typename RemoveReference<T>::type>::value,
            bool = IsVolatile<typename RemoveReference<T>::type>::value
        > struct ApplyConstVolatile {
            typedef U type;
        };

        template<typename T, typename U>
        struct ApplyConstVolatile<T, U, true, false> { /* const */
            typedef const U type;
        };

        template<typename T, typename U>
        struct ApplyConstVolatile<T, U, false, true> { /* volatile */
            typedef volatile U type;
        };

        template<typename T, typename U>
        struct ApplyConstVolatile<T, U, true, true> { /* const volatile */
            typedef const volatile U type;
        };

        template<typename T, typename U>
        struct ApplyConstVolatile<T &, U, true, false> { /* const */
            typedef const U &type;
        };

        template<typename T, typename U>
        struct ApplyConstVolatile<T &, U, false, true> { /* volatile */
            typedef volatile U &type;
        };

        template<typename T, typename U>
        struct ApplyConstVolatile<T &, U, true, true> { /* const volatile */
            typedef const volatile U &type;
        };

        template<typename T, bool = IsIntegral<T>::value || IsEnum<T>::value>
        struct MakeSigned {};

        template<typename T, bool = IsIntegral<T>::value || IsEnum<T>::value>
        struct MakeUnsigned {};

        template<typename T>
        struct MakeSigned<T, true> {
            typedef typename TypeFindFirst<stypes, sizeof(T)>::type type;
        };

        template<typename T>
        struct MakeUnsigned<T, true> {
            typedef typename TypeFindFirst<utypes, sizeof(T)>::type type;
        };

        template<> struct MakeSigned<bool  , true> {};
        template<> struct MakeSigned<schar , true> { typedef schar type; };
        template<> struct MakeSigned<uchar , true> { typedef schar type; };
        template<> struct MakeSigned<short , true> { typedef short type; };
        template<> struct MakeSigned<ushort, true> { typedef short type; };
        template<> struct MakeSigned<int   , true> { typedef int   type; };
        template<> struct MakeSigned<uint  , true> { typedef int   type; };
        template<> struct MakeSigned<long  , true> { typedef long  type; };
        template<> struct MakeSigned<ulong , true> { typedef long  type; };
        template<> struct MakeSigned<llong , true> { typedef llong type; };
        template<> struct MakeSigned<ullong, true> { typedef llong type; };

        template<> struct MakeUnsigned<bool  , true> {};
        template<> struct MakeUnsigned<schar , true> { typedef uchar  type; };
        template<> struct MakeUnsigned<uchar , true> { typedef uchar  type; };
        template<> struct MakeUnsigned<short , true> { typedef ushort type; };
        template<> struct MakeUnsigned<ushort, true> { typedef ushort type; };
        template<> struct MakeUnsigned<int   , true> { typedef uint   type; };
        template<> struct MakeUnsigned<uint  , true> { typedef uint   type; };
        template<> struct MakeUnsigned<long  , true> { typedef ulong  type; };
        template<> struct MakeUnsigned<ulong , true> { typedef ulong  type; };
        template<> struct MakeUnsigned<llong , true> { typedef ullong type; };
        template<> struct MakeUnsigned<ullong, true> { typedef ullong type; };
    }

    template<typename T> struct MakeSigned {
        typedef typename internal::ApplyConstVolatile<T,
            typename internal::MakeSigned<
                typename RemoveConstVolatile<T>::type
            >::type
        >::type type;
    };

    template<typename T> struct MakeUnsigned {
        typedef typename internal::ApplyConstVolatile<T,
            typename internal::MakeUnsigned<
                typename RemoveConstVolatile<T>::type
            >::type
        >::type type;
    };

    /* common type */

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

    namespace internal {
        template<typename F, typename ...A>
        inline auto result_of_invoke(F &&f, A &&...args) ->
          decltype(forward<F>(f)(forward<A>(args)...)) {
            return forward<F>(f)(forward<A>(args)...);
        }
        template<typename B, typename T, typename D>
        inline auto result_of_invoke(T B::*pmd, D &&ref) ->
          decltype(forward<D>(ref).*pmd) {
            return forward<D>(ref).*pmd;
        }
        template<typename PMD, typename P>
        inline auto result_of_invoke(PMD &&pmd, P &&ptr) ->
          decltype((*forward<P>(ptr)).*forward<PMD>(pmd)) {
            return (*forward<P>(ptr)).*forward<PMD>(pmd);
        }
        template<typename B, typename T, typename D, typename ...A>
        inline auto result_of_invoke(T B::*pmf, D &&ref, A &&...args) ->
          decltype((forward<D>(ref).*pmf)(forward<A>(args)...)) {
            return (forward<D>(ref).*pmf)(forward<A>(args)...);
        }
        template<typename PMF, typename P, typename ...A>
        inline auto result_of_invoke(PMF &&pmf, P &&ptr, A &&...args) ->
          decltype(((*forward<P>(ptr)).*forward<PMF>(pmf))(forward<A>(args)...)) {
            return ((*forward<P>(ptr)).*forward<PMF>(pmf))(forward<A>(args)...);
        }

        template<typename, typename = void>
        struct ResultOf {};
        template<typename F, typename ...A>
        struct ResultOf<F(A...), decltype(void(result_of_invoke(declval<F>(),
        declval<A>()...)))> {
            using type = decltype(result_of_invoke(declval<F>(), declval<A>()...));
        };
    }

    template<typename T> struct ResultOf: internal::ResultOf<T> {};

    /* enable_if */

    template<bool B, typename T = void> struct enable_if {};

    template<typename T> struct enable_if<true, T> { typedef T type; };

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
                typename RemoveConstVolatile<U>::type
            >::type
        >::type type;
    };

    /* common type */

    template<typename ...T> struct CommonType;

    template<typename T> struct CommonType<T> {
        typedef Decay<T> type;
    };

    template<typename T, typename U> struct CommonType<T, U> {
        typedef Decay<decltype(true ? declval<T>() : declval<U>())> type;
    };

    template<typename T, typename U, typename ...V>
    struct CommonType<T, U, V...> {
        typedef typename CommonType<typename CommonType<T, U>::type, V...>::type type;
    };

    /* aligned storage */

    namespace internal {
        template<size_t N> struct AlignedStorageTest {
            union type {
                uchar data[N];
                octa::max_align_t align;
            };
        };
    };

    template<size_t N, size_t A
        = alignof(typename internal::AlignedStorageTest<N>::type)
    > struct AlignedStorage {
        struct type {
            alignas(A) uchar data[N];
        };
    };

    /* aligned union */

    namespace internal {
        template<size_t ...N> struct AlignMax;

        template<size_t N> struct AlignMax<N> {
            static constexpr size_t value = N;
        };

        template<size_t N1, size_t N2> struct AlignMax<N1, N2> {
            static constexpr size_t value = (N1 > N2) ? N1 : N2;
        };

        template<size_t N1, size_t N2, size_t ...N>
        struct AlignMax<N1, N2, N...> {
            static constexpr size_t value
                = AlignMax<AlignMax<N1, N2>::value, N...>::value;
        };
    }

    template<size_t N, typename ...T> struct AlignedUnion {
        static constexpr size_t alignment_value
            = internal::AlignMax<alignof(T)...>::value;

        struct type {
            alignas(alignment_value) uchar data[internal::AlignMax<N,
                sizeof(T)...>::value];
        };
    };

    /* underlying type */

    template<typename T, bool = IsEnum<T>::value> struct UnderlyingTypeBase;

    template<typename T> struct UnderlyingTypeBase<T, true> {
        typedef typename Conditional<IsSigned<T>::value,
            typename MakeSigned<T>::type,
            typename MakeUnsigned<T>::type
        >::type type;
    };

    template<typename T> struct UnderlyingType {
        typedef typename UnderlyingTypeBase<T>::type type;
    };
}

#endif