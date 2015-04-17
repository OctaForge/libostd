/* Ranges for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_RANGE_H
#define OCTA_RANGE_H

#include <stddef.h>

#include "octa/types.h"
#include "octa/traits.h"

namespace octa {
    struct InputRange {};
    struct ForwardRange {};
    struct BidirectionalRange {};
    struct RandomAccessRange {};

    template<typename T>
    struct RangeTraits {
        typedef typename T::type::category  category;
        typedef typename T::type::value     value;
        typedef typename T::type::pointer   pointer;
        typedef typename T::type::reference reference;
    };

    template<typename T>
    struct RangeTraits<T *> {
        typedef RandomAccessRange category;
        typedef T  value;
        typedef T *pointer;
        typedef T &reference;
    };

    template<typename T>
    struct RangeTraits<const T *> {
        typedef RandomAccessRange category;
        typedef T        value;
        typedef const T *pointer;
        typedef const T &reference;
    };

    template<>
    struct RangeTraits<void *> {
        typedef RandomAccessRange category;
        typedef uchar  value;
        typedef void  *pointer;
        typedef uchar &reference;
    };

    template<>
    struct RangeTraits<const void *> {
        typedef RandomAccessRange category;
        typedef uchar        value;
        typedef const void  *pointer;
        typedef const uchar &reference;
    };

    namespace internal {
        template<typename T>
        struct RangeIterator {
            RangeIterator(): p_range() {}
            explicit RangeIterator(const T &range): p_range(range) {}
            RangeIterator &operator++() {
                p_range.pop_first();
                return *this;
            }
            typename RangeTraits<T>::reference operator*() {
                return p_range.first();
            }
            typename RangeTraits<T>::reference operator*() const {
                return p_range.first();
            }
            bool operator!=(RangeIterator) const { return !p_range.empty(); }
        private:
            T p_range;
        };
    }

    template<typename B, typename C, typename V, typename P = V *, typename R = V &>
    struct Range {
        struct type {
            typedef C category;
            typedef V value;
            typedef P pointer;
            typedef R reference;
        };

        internal::RangeIterator<B> begin() {
            return internal::RangeIterator<B>((const B &)*this);
        }
        internal::RangeIterator<B> end() {
            return internal::RangeIterator<B>();
        }
    };

    template<typename T>
    struct ReverseRange: Range<ReverseRange<T>,
        typename RangeTraits<T>::category,
        typename RangeTraits<T>::value,
        typename RangeTraits<T>::pointer,
        typename RangeTraits<T>::reference
    > {
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

        typename RangeTraits<T>::reference first()       { return p_range.last(); }
        typename RangeTraits<T>::reference first() const { return p_range.last(); }

        typename RangeTraits<T>::reference last()       { return p_range.first(); }
        typename RangeTraits<T>::reference last() const { return p_range.first(); }

        typename RangeTraits<T>::reference operator[](size_t i) {
            return p_range[length() - i - 1];
        }
        typename RangeTraits<T>::reference operator[](size_t i) const {
            return p_range[length() - i - 1];
        }

    private:
        T p_range;
    };

    template<typename T>
    ReverseRange<T> make_reverse_range(const T &it) {
        return ReverseRange<T>(it);
    }

    template<typename T>
    struct MoveRange: Range<MoveRange<T>,
        typename RangeTraits<T>::category,
        typename RangeTraits<T>::value,
        typename RangeTraits<T>::pointer,
        typename RangeTraits<T>::value &&
    > {
        MoveRange(): p_range() {}
        MoveRange(const T &range): p_range(range) {}
        MoveRange(const MoveRange &it): p_range(it.p_range) {}
        MoveRange(MoveRange &&it): p_range(move(it.p_range)) {}

        bool   empty () const { return p_range.empty (); }
        size_t length() const { return p_range.length(); }

        void pop_first() { p_range.pop_first(); }
        void pop_last () { p_range.pop_last (); }

        bool operator==(const MoveRange &v) const {
            return p_range == v.p_range;
        }
        bool operator!=(const MoveRange &v) const {
            return p_range != v.p_range;
        }

        typename RangeTraits<T>::value &&first() { return move(p_range.first()); }
        typename RangeTraits<T>::value &&last () { return move(p_range.last ()); }

        typename RangeTraits<T>::value &&operator[](size_t i) {
            return move(p_range[i]);
        }

    private:
        T p_range;
    };

    template<typename T>
    MoveRange<T> make_move_range(const T &it) {
        return MoveRange<T>(it);
    }

    template<typename T>
    struct NumberRange: Range<NumberRange<T>, ForwardRange, T> {
        NumberRange(): p_a(0), p_b(0), p_step(0) {}
        NumberRange(const NumberRange &it): p_a(it.p_a), p_b(it.p_b),
            p_step(it.p_step) {}
        NumberRange(T a, T b, T step = 1): p_a(a), p_b(b), p_step(step) {}

        bool operator==(const NumberRange &v) const {
            return p_a == v.p_a && p_b == v.p_b && p_step == v.p_step;
        }
        bool operator!=(const NumberRange &v) const {
            return p_a != v.p_a || p_b != v.p_b || p_step != v.p_step;
        }

        bool empty() const { return p_a * p_step >= p_b * p_step; }
        void pop_first() { p_a += p_step; }
        T &first() { return p_a; }

    private:
        T p_a, p_b, p_step;
    };

    template<typename T>
    NumberRange<T> range(T a, T b, T step = 1) {
        return NumberRange<T>(a, b, step);
    }
}

#endif