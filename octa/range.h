/* Ranges for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_RANGE_H
#define OCTA_RANGE_H

#include <stddef.h>

#include "octa/types.h"

namespace octa {
    template<typename T>
    struct RangeTraits {
        typedef typename T::type::difference difference;
        typedef typename T::type::value      value;
        typedef typename T::type::pointer    pointer;
        typedef typename T::type::reference  reference;
    };

    template<typename T>
    struct RangeTraits<T *> {
        typedef ptrdiff_t  difference;
        typedef T          value;
        typedef T         *pointer;
        typedef T         &reference;
    };

    template<typename T>
    struct RangeTraits<const T *> {
        typedef ptrdiff_t  difference;
        typedef T          value;
        typedef const T   *pointer;
        typedef const T   &reference;
    };

    template<>
    struct RangeTraits<void *> {
        typedef ptrdiff_t  difference;
        typedef uchar      value;
        typedef void      *pointer;
        typedef uchar     &reference;
    };

    template<>
    struct RangeTraits<const void *> {
        typedef ptrdiff_t    difference;
        typedef uchar        value;
        typedef const void  *pointer;
        typedef const uchar &reference;
    };

    template<typename T>
    class RangeIterator {
        T p_range;

    public:
        struct type {
            typedef typename RangeTraits<T>::difference difference;
            typedef typename RangeTraits<T>::value      value;
            typedef typename RangeTraits<T>::pointer    pointer;
            typedef typename RangeTraits<T>::reference  reference;
        };

        RangeIterator(): p_range() {}
        RangeIterator(const T &range): p_range(range) {}

        template<typename U>
        RangeIterator(const RangeIterator<U> &it): p_range(it.range()) {}

        T range() const { return p_range; }

        RangeIterator &operator++() {
            p_range.pop_first();
            return *this;
        }

        typename type::reference operator*() {
            return p_range.first();
        }

        template<typename U>
        friend bool operator!=(const RangeIterator &a, const RangeIterator<U> &b) {
            return a.range() != b.range();
        }
    };

    template<typename T>
    struct Range {
        RangeIterator<T> begin() { return RangeIterator<T>((const T &)*this); }
        RangeIterator<T> end() { return RangeIterator<T>(); }
    };
}

#endif