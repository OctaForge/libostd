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

    template<typename T>
    internal::RangeIterator<T> begin(const T &range) {
        return internal::RangeIterator<T>(range);
    }
    template<typename T>
    internal::RangeIterator<T> end(T) {
        return internal::RangeIterator<T>();
    }

#define OCTA_RANGE_ITERATOR_SETUP \
    auto begin() -> decltype(octa::begin(*this)) { return octa::begin(*this); } \
    auto end  () -> decltype(octa::begin(*this)) { return octa::end  (*this); }

#define OCTA_RANGE_ITERATOR_GLOBAL_SETUP \
    template<typename T> \
    auto begin(const T &v) -> decltype(octa::begin(v)) { \
        return octa::begin(v); \
    } \
    template<typename T> \
    auto end(const T &v) -> decltype(octa::begin(v)) { \
        return octa::end(v); \
    }

    template<typename T>
    struct ReverseRange {
        struct type {
            typedef typename RangeTraits<T>::category  category;
            typedef typename RangeTraits<T>::value     value;
            typedef typename RangeTraits<T>::pointer   pointer;
            typedef typename RangeTraits<T>::reference reference;
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
    ReverseRange<T> make_reverse_range(const T &it) {
        return ReverseRange<T>(it);
    }

    template<typename T>
    struct MoveRange {
        struct type {
            typedef typename RangeTraits<T>::category   category;
            typedef typename RangeTraits<T>::value      value;
            typedef typename RangeTraits<T>::pointer    pointer;
            typedef typename RangeTraits<T>::value    &&reference;
        };

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

        typename type::reference first() { return move(p_range.first()); }
        typename type::reference last () { return move(p_range.last ()); }

        typename type::reference operator[](size_t i) {
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
    struct NumberRange {
        struct type {
            typedef octa::ForwardRange category;
            typedef T  value;
            typedef T *pointer;
            typedef T &reference;
        };

        NumberRange(): p_a(0), p_b(0), p_step(0) {}
        NumberRange(const NumberRange &it): p_a(it.p_a), p_b(it.p_b),
            p_step(it.p_step) {}
        NumberRange(T a, T b, T step = 1): p_a(a), p_b(b), p_step(step) {}

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