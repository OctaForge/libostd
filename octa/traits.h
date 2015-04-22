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

    /* is number */

    template<typename T> struct IsNumber: IntegralConstant<bool,
        (IsIntegral<T>::value || IsFloatingPoint<T>::value)
    > {};

    /* is pointer to member */

    template<typename  > struct IsMemberPointerBase: false_t {};
    template<typename T, typename U>
    struct IsMemberPointerBase<T U::*>:  true_t {};

    template<typename T>
    struct IsMemberPointer: IsMemberPointerBase<
        typename RemoveConstVolatile<T>::type
    > {};

    /* is POD: currently wrong */

    template<typename T> struct IsPOD: IntegralConstant<bool,
        (IsIntegral<T>::value || IsFloatingPoint<T>::value || IsPointer<T>::value)
    > {};

    /* type equality */

    template<typename, typename> struct IsEqual      : false_t {};
    template<typename T        > struct IsEqual<T, T>:  true_t {};

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

    /* cppreference.com used for reference */
    template<typename T> struct ResultOf: internal::ResultOf<T> {};

    /* enable_if */

    template<bool B, typename T = void> struct enable_if {};

    template<typename T> struct enable_if<true, T> { typedef T type; };

    /* decay */

#if 0
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
#endif
}

#endif