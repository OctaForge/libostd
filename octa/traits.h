/* Type traits for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_TRAITS_H
#define OCTA_TRAITS_H

#include <stddef.h>

#include "octa/types.h"
#include "octa/utility.h"

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

    template<typename T> using RemoveReference = internal::RemoveReference<T>;

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

        constexpr operator value_type() const { return value; }
        constexpr value_type operator()() const { return value; }
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

    /* is array */

    template<typename            > struct IsArray      : false_t {};
    template<typename T          > struct IsArray<T[] >:  true_t {};
    template<typename T, size_t N> struct IsArray<T[N]>:  true_t {};

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
        struct ResultOf<F(A...), decltype(void(internal::result_of_invoke(
            declval<F>(), declval<A>()...)))> {
            using type = decltype(internal::result_of_invoke(declval<F>(),
                declval<A>()...));
        };
    }

    /* cppreference.com used for reference */
    template<typename T> struct ResultOf: internal::ResultOf<T> {};
}

#endif