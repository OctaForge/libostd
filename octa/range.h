/* Ranges for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_RANGE_H
#define OCTA_RANGE_H

#include <stddef.h>

#include "octa/types.h"
#include "octa/utility.h"

namespace octa {
    struct InputRange {};
    struct OutputRange {};
    struct ForwardRange {};
    struct BidirectionalRange {};
    struct RandomAccessRange {};

    template<typename T>
    struct RangeTraits {
        typedef typename T::type::category  category;
        typedef typename T::type::value     value;
        typedef typename T::type::reference reference;
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

    template<typename B, typename C, typename V, typename R = V &>
    struct InputRangeBase {
        struct type {
            typedef C category;
            typedef V value;
            typedef R reference;
        };

        internal::RangeIterator<B> begin() {
            return internal::RangeIterator<B>((const B &)*this);
        }
        internal::RangeIterator<B> end() {
            return internal::RangeIterator<B>();
        }
    };

    template<typename V, typename R = V &>
    struct OutputRangeBase {
        struct type {
            typedef OutputRange category;
            typedef V value;
            typedef R reference;
        };
    };

    template<typename T>
    struct ReverseRange: InputRangeBase<ReverseRange<T>,
        typename RangeTraits<T>::category,
        typename RangeTraits<T>::value,
        typename RangeTraits<T>::reference
    > {
        ReverseRange(): p_range() {}
        ReverseRange(const T &range): p_range(range) {}
        ReverseRange(const ReverseRange &it): p_range(it.p_range) {}
        ReverseRange(ReverseRange &&it): p_range(move(it.p_range)) {}

        ReverseRange &operator=(const ReverseRange &v) {
            p_range = v.p_range;
            return *this;
        }
        ReverseRange &operator=(ReverseRange &&v) {
            p_range = move(v.p_range);
            return *this;
        }
        ReverseRange &operator=(const T &v) {
            p_range = v;
            return *this;
        }
        ReverseRange &operator=(T &&v) {
            p_range = forward<T>(v);
            return *this;
        }

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

        ReverseRange<T> slice(size_t start, size_t end) {
            size_t len = p_range.length();
            return ReverseRange<T>(p_range.slice(len - end, len - start));
        }

    private:
        T p_range;
    };

    template<typename T>
    ReverseRange<T> make_reverse_range(const T &it) {
        return ReverseRange<T>(it);
    }

    template<typename T>
    struct MoveRange: InputRangeBase<MoveRange<T>,
        typename RangeTraits<T>::category,
        typename RangeTraits<T>::value,
        typename RangeTraits<T>::value &&
    > {
        MoveRange(): p_range() {}
        MoveRange(const T &range): p_range(range) {}
        MoveRange(const MoveRange &it): p_range(it.p_range) {}
        MoveRange(MoveRange &&it): p_range(move(it.p_range)) {}

        MoveRange &operator=(const MoveRange &v) {
            p_range = v.p_range;
            return *this;
        }
        MoveRange &operator=(MoveRange &&v) {
            p_range = move(v.p_range);
            return *this;
        }
        MoveRange &operator=(const T &v) {
            p_range = v;
            return *this;
        }
        MoveRange &operator=(T &&v) {
            p_range = forward<T>(v);
            return *this;
        }

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

        MoveRange<T> slice(size_t start, size_t end) {
            return MoveRange<T>(p_range.slice(start, end));
        }

    private:
        T p_range;
    };

    template<typename T>
    MoveRange<T> make_move_range(const T &it) {
        return MoveRange<T>(it);
    }

    template<typename T>
    struct NumberRange: InputRangeBase<NumberRange<T>, ForwardRange, T> {
        NumberRange(): p_a(0), p_b(0), p_step(0) {}
        NumberRange(const NumberRange &it): p_a(it.p_a), p_b(it.p_b),
            p_step(it.p_step) {}
        NumberRange(T a, T b, T step = 1): p_a(a), p_b(b), p_step(step) {}
        NumberRange(T v): p_a(0), p_b(v), p_step(1) {}

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

    template<typename T>
    NumberRange<T> range(T v) {
        return NumberRange<T>(v);
    }
}

#endif