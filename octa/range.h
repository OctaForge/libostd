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

    template<typename T>
    struct __OctaRangeIterator {
        __OctaRangeIterator(): p_range() {}
        explicit __OctaRangeIterator(const T &range): p_range(range) {}
        __OctaRangeIterator &operator++() {
            p_range.pop_first();
            return *this;
        }
        typename RangeTraits<T>::reference operator*() {
            return p_range.first();
        }
        typename RangeTraits<T>::reference operator*() const {
            return p_range.first();
        }
        bool operator!=(__OctaRangeIterator) const { return !p_range.empty(); }
    private:
        T p_range;
    };

    template<typename B, typename C, typename V, typename R = V &>
    struct InputRangeBase {
        struct type {
            typedef C category;
            typedef V value;
            typedef R reference;
        };

        __OctaRangeIterator<B> begin() {
            return __OctaRangeIterator<B>((const B &)*this);
        }
        __OctaRangeIterator<B> end() {
            return __OctaRangeIterator<B>();
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

        void put(const typename RangeTraits<T>::value &v) { p_range.put(v); }

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

    template<typename T>
    struct PointerRange: InputRangeBase<PointerRange<T>, RandomAccessRange, T> {
        PointerRange(): p_beg(nullptr), p_end(nullptr) {}
        PointerRange(const PointerRange &v): p_beg(v.p_beg), p_end(v.p_end) {}
        PointerRange(T *beg, T *end): p_beg(beg), p_end(end) {}
        PointerRange(T *beg, size_t n): p_beg(beg), p_end(beg + n) {}

        bool operator==(const PointerRange &v) const {
            return p_beg == v.p_beg && p_end == v.p_end;
        }
        bool operator!=(const PointerRange &v) const {
            return p_beg != v.p_beg || p_end != v.p_end;
        }

        /* satisfy InputRange / ForwardRange */
        bool empty() const { return p_beg == nullptr; }

        void pop_first() {
            if (p_beg == nullptr) return;
            if (++p_beg == p_end) p_beg = p_end = nullptr;
        }

              T &first()       { return *p_beg; }
        const T &first() const { return *p_beg; }

        /* satisfy BidirectionalRange */
        void pop_last() {
            if (p_end-- == p_beg) { p_end = nullptr; return; }
            if (p_end   == p_beg) p_beg = p_end = nullptr;
        }

              T &last()       { return *(p_end - 1); }
        const T &last() const { return *(p_end - 1); }

        /* satisfy RandomAccessRange */
        size_t length() const { return p_end - p_beg; }

        PointerRange slice(size_t start, size_t end) {
            return PointerRange(p_beg + start, p_beg + end);
        }

              T &operator[](size_t i)       { return p_beg[i]; }
        const T &operator[](size_t i) const { return p_beg[i]; }

        /* satisfy OutputRange */
        void put(const T &v) { *(p_beg++) = v; }

    private:
        T *p_beg, *p_end;
    };

    template<typename T>
    struct EnumeratedValue {
        size_t index;
        T      value;
    };

    template<typename T>
    struct EnumeratedRange: InputRangeBase<EnumeratedRange<T>,
        InputRange,     typename RangeTraits<T>::value,
        EnumeratedValue<typename RangeTraits<T>::reference>
    > {
        EnumeratedRange(): p_range(), p_index(0) {}
        EnumeratedRange(const T &range): p_range(range), p_index(0) {}
        EnumeratedRange(const EnumeratedRange &it): p_range(it.p_range),
            p_index(it.p_index) {}
        EnumeratedRange(EnumeratedRange &&it): p_range(move(it.p_range)),
            p_index(it.p_index) {}

        EnumeratedRange &operator=(const EnumeratedRange &v) {
            p_range = v.p_range;
            p_index = v.p_index;
            return *this;
        }
        EnumeratedRange &operator=(EnumeratedRange &&v) {
            p_range = move(v.p_range);
            p_index = v.p_index;
            return *this;
        }
        EnumeratedRange &operator=(const T &v) {
            p_range = v;
            p_index = 0;
            return *this;
        }
        EnumeratedRange &operator=(T &&v) {
            p_range = forward<T>(v);
            p_index = 0;
            return *this;
        }

        bool empty() const { return p_range.empty(); }

        void pop_first() { ++p_index; p_range.pop_first(); }

        EnumeratedValue<typename RangeTraits<T>::reference> first() {
            return EnumeratedValue<typename RangeTraits<T>::reference> {
                p_index, p_range.first()
            };
        }
        EnumeratedValue<typename RangeTraits<T>::reference> first() const {
            return EnumeratedValue<typename RangeTraits<T>::reference> {
                p_index, p_range.first()
            };
        }

        bool operator==(const EnumeratedRange &v) const {
            return p_range == v.p_range;
        }
        bool operator!=(const EnumeratedRange &v) const {
            return p_range != v.p_range;
        }

    private:
        T p_range;
        size_t p_index;
    };

    template<typename T>
    EnumeratedRange<T> enumerate(const T &it) {
        return EnumeratedRange<T>(it);
    }
}

#endif