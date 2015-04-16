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
        RangeIterator(const RangeIterator &it): p_range(it.p_range) {}
        RangeIterator(RangeIterator &&it): p_range(move(it.p_range)) {}

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

    template<template<typename> class T, typename U>
    struct Range {
        RangeIterator<T<U>> begin() {
            return RangeIterator<T<U>>((const T<U> &)*this);
        }
        RangeIterator<T<U>> end() {
            return RangeIterator<T<U>>();
        }
    };

    template<typename T>
    struct ReverseRange: Range<ReverseRange, T> {
        struct type {
            typedef typename RangeTraits<T>::difference difference;
            typedef typename RangeTraits<T>::value      value;
            typedef typename RangeTraits<T>::pointer    pointer;
            typedef typename RangeTraits<T>::reference  reference;
        };

        ReverseRange(): p_range() {}
        ReverseRange(const T &range): p_range(range) {}
        ReverseRange(const ReverseRange &it): p_range(it.p_range) {}
        ReverseRange(ReverseRange &&it): p_range(move(it.p_range)) {}

        bool   empty () const { return p_range.empty (); }
        size_t length() const { return p_range.length(); }

        void pop_first() { p_range.pop_last (); }
        void pop_last () { p_range.pop_first(); }

        bool operator==(const ReverseRange &v) const {
            return p_range == v.p_range;
        }
        bool operator!=(const ReverseRange &v) const {
            return p_range != v.p_range;
        }

        typename type::reference first()       { return p_range.last(); }
        typename type::reference first() const { return p_range.last(); }

        typename type::reference last()       { return p_range.first(); }
        typename type::reference last() const { return p_range.first(); }

        typename type::reference operator[](size_t i) {
            return p_range[length() - i - 1];
        }
        typename type::reference operator[](size_t i) const {
            return p_range[length() - i - 1];
        }

    private:
        T p_range;
    };

    template<typename T>
    ReverseRange<T> reverse(const T &it) {
        return ReverseRange<T>(it);
    }
}

#endif