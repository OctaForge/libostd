/* Ranges for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_RANGE_H
#define OCTA_RANGE_H

#include <stddef.h>

#include "octa/types.h"
#include "octa/utility.h"
#include "octa/type_traits.h"

namespace octa {
    struct InputRangeTag {};
    struct OutputRangeTag {};
    struct ForwardRangeTag: InputRangeTag {};
    struct BidirectionalRangeTag: ForwardRangeTag {};
    struct RandomAccessRangeTag: BidirectionalRangeTag {};
    struct FiniteRandomAccessRangeTag: RandomAccessRangeTag {};

    template<typename T> struct RangeHalf;

#define __OCTA_RANGE_TRAIT(Name, TypeName) \
    template<typename T> \
    struct __OctaRange##Name { \
        typedef typename T::TypeName Type; \
    }; \
    template<typename T> \
    struct __OctaRange##Name<RangeHalf<T>> { \
        typedef typename T::TypeName Type; \
    }; \
    template<typename T> \
    using Range##Name = typename __OctaRange##Name<T>::Type;

    __OCTA_RANGE_TRAIT(Category, Category)
    __OCTA_RANGE_TRAIT(Size, SizeType)
    __OCTA_RANGE_TRAIT(Value, ValType)
    __OCTA_RANGE_TRAIT(Reference, RefType)
    __OCTA_RANGE_TRAIT(Difference, DiffType)

#undef __OCTA_RANGE_TRAIT

    // is input range

    template<typename T, bool = IsConvertible<
        RangeCategory<T>, InputRangeTag
    >::value> struct IsInputRange: False {};

    template<typename T>
    struct IsInputRange<T, true>: True {};

    // is forward range

    template<typename T, bool = IsConvertible<
        RangeCategory<T>, ForwardRangeTag
    >::value> struct IsForwardRange: False {};

    template<typename T>
    struct IsForwardRange<T, true>: True {};

    // is bidirectional range

    template<typename T, bool = IsConvertible<
        RangeCategory<T>, BidirectionalRangeTag
    >::value> struct IsBidirectionalRange: False {};

    template<typename T>
    struct IsBidirectionalRange<T, true>: True {};

    // is random access range

    template<typename T, bool = IsConvertible<
        RangeCategory<T>, RandomAccessRangeTag
    >::value> struct IsRandomAccessRange: False {};

    template<typename T>
    struct IsRandomAccessRange<T, true>: True {};

    // is finite random access range

    template<typename T, bool = IsConvertible<
        RangeCategory<T>, FiniteRandomAccessRangeTag
    >::value> struct IsFiniteRandomAccessRange: False {};

    template<typename T>
    struct IsFiniteRandomAccessRange<T, true>: True {};

    // is infinite random access range

    template<typename T>
    struct IsInfiniteRandomAccessRange: IntegralConstant<bool,
        (IsRandomAccessRange<T>::value && !IsFiniteRandomAccessRange<T>::value)
    > {};

    // is output range

    template<typename T, typename P>
    struct __OctaOutputRangeTest {
        template<typename U, void (U::*)(P)> struct __OctaTest {};
        template<typename U> static char __octa_test(__OctaTest<U, &U::put> *);
        template<typename U> static  int __octa_test(...);
        static constexpr bool value = (sizeof(__octa_test<T>(0)) == sizeof(char));
    };

    template<typename T, bool = (IsConvertible<
        RangeCategory<T>, OutputRangeTag
    >::value || (IsInputRange<T>::value &&
        (__OctaOutputRangeTest<T, const RangeValue<T>  &>::value ||
         __OctaOutputRangeTest<T,       RangeValue<T> &&>::value)
    ))> struct IsOutputRange: False {};

    template<typename T>
    struct IsOutputRange<T, true>: True {};

    // range iterator

    template<typename T>
    struct __OctaRangeIterator {
        __OctaRangeIterator(): p_range() {}
        explicit __OctaRangeIterator(const T &range): p_range(range) {}
        __OctaRangeIterator &operator++() {
            p_range.pop_first();
            return *this;
        }
        RangeReference<T> operator*() const {
            return p_range.first();
        }
        bool operator!=(__OctaRangeIterator) const { return !p_range.empty(); }
    private:
        T p_range;
    };

    // range half

    template<typename T>
    struct RangeHalf {
    private:
        T p_range;
    public:
        typedef T RangeType;

        RangeHalf(): p_range() {}
        RangeHalf(const T &range): p_range(range) {}
        RangeHalf(const RangeHalf &half): p_range(half.p_range) {}
        RangeHalf(RangeHalf &&half): p_range(move(half.p_range)) {}

        RangeHalf &operator=(const RangeHalf &half) {
            p_range = half.p_range;
            return *this;
        }

        RangeHalf &operator=(RangeHalf &&half) {
            p_range = move(half.p_range);
            return *this;
        }

        T range() const { return p_range; }

        bool next() { return p_range.pop_first(); }
        bool prev() { return p_range.push_first(); }

        RangeSize<T> next_n(RangeSize<T> n) {
            return p_range.pop_first_n(n);
        }
        RangeSize<T> prev_n(RangeSize<T> n) {
            return p_range.push_first_n(n);
        }

        RangeReference<T> get() const {
            return p_range.first();
        }

        RangeDifference<T> distance(const RangeHalf &half) const {
            return p_range.distance_first(half.p_range);
        }

        bool equals(const RangeHalf &half) const {
            return p_range.equals_first(half.p_range);
        }

        bool operator==(const RangeHalf &half) const {
            return equals(half);
        }
        bool operator!=(const RangeHalf &half) const {
            return !equals(half);
        }

        /* iterator like interface */

        RangeReference<T> operator*() const {
            return get();
        }

        RangeReference<T> operator[](RangeSize<T> idx) const {
            return p_range[idx];
        }

        RangeHalf &operator++() {
            next();
            return *this;
        }
        RangeHalf operator++(int) {
            RangeHalf tmp(*this);
            next();
            return move(tmp);
        }

        RangeHalf &operator--() {
            prev();
            return *this;
        }
        RangeHalf operator--(int) {
            RangeHalf tmp(*this);
            prev();
            return move(tmp);
        }

        RangeHalf operator+(RangeDifference<T> n) {
            RangeHalf tmp(*this);
            if (n < 0) tmp.prev_n(-n);
            else tmp.next_n(n);
            return move(tmp);
        }
        RangeHalf operator-(RangeDifference<T> n) {
            RangeHalf tmp(*this);
            if (n < 0) tmp.next_n(-n);
            else tmp.prev_n(n);
            return move(tmp);
        }

        RangeHalf &operator+=(RangeDifference<T> n) {
            if (n < 0) prev_n(-n);
            else next_n(n);
            return *this;
        }
        RangeHalf &operator-=(RangeDifference<T> n) {
            if (n < 0) next_n(-n);
            else prev_n(n);
            return *this;
        }
    };

    template<typename R>
    RangeSize<R> __octa_pop_first_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i)
            if (!range.pop_first()) return i;
        return n;
    }

    template<typename R>
    RangeSize<R> __octa_pop_last_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i)
            if (!range.pop_last()) return i;
        return n;
    }

    template<typename R>
    RangeSize<R> __octa_push_first_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i)
            if (!range.push_first()) return i;
        return n;
    }

    template<typename R>
    RangeSize<R> __octa_push_last_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i)
            if (!range.push_last()) return i;
        return n;
    }

    template<typename B, typename C, typename V, typename R = V &,
             typename S = size_t, typename D = ptrdiff_t
    > struct InputRange {
        typedef C Category;
        typedef S SizeType;
        typedef D DiffType;
        typedef V ValType;
        typedef R RefType;

        __OctaRangeIterator<B> begin() const {
            return __OctaRangeIterator<B>((const B &)*this);
        }
        __OctaRangeIterator<B> end() const {
            return __OctaRangeIterator<B>();
        }

        SizeType pop_first_n(SizeType n) {
            return __octa_pop_first_n<B>(*((B *)this), n);
        }

        SizeType pop_last_n(SizeType n) {
            return __octa_pop_last_n<B>(*((B *)this), n);
        }

        SizeType push_first_n(SizeType n) {
            return __octa_push_first_n<B>(*((B *)this), n);
        }

        SizeType push_last_n(SizeType n) {
            return __octa_push_last_n<B>(*((B *)this), n);
        }

        B each() const {
            return B(*((B *)this));
        }

        RangeHalf<B> half() const {
            return RangeHalf<B>(*((B *)this));
        }
    };

    template<typename V, typename R = V &, typename S = size_t,
             typename D = ptrdiff_t
    > struct OutputRange {
        typedef OutputRangeTag Category;
        typedef S SizeType;
        typedef D DiffType;
        typedef V ValType;
        typedef R RefType;
    };

    template<typename T>
    struct ReverseRange: InputRange<ReverseRange<T>,
        RangeCategory<T>, RangeValue<T>, RangeReference<T>, RangeSize<T>,
        RangeDifference<T>
    > {
    private:
        typedef RangeReference<T> r_ref;
        typedef RangeSize<T> r_size;

        T p_range;

    public:
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
            p_range = move(v);
            return *this;
        }

        bool empty() const { return p_range.empty(); }
        r_size size() const { return p_range.size(); }

        bool equals_first(const ReverseRange &range) const {
        return p_range.equals_last(range.p_range);
        }
        bool equals_last(const ReverseRange &range) const {
            return p_range.equals_first(range.p_range);
        }

        RangeDifference<T> distance_first(const ReverseRange &range) const {
            return -p_range.distance_last(range.p_range);
        }
        RangeDifference<T> distance_last(const ReverseRange &range) const {
            return -p_range.distance_first(range.p_range);
        }

        bool pop_first() { return p_range.pop_last(); }
        bool pop_last() { return p_range.pop_first(); }

        bool push_first() { return p_range.push_last(); }
        bool push_last() { return p_range.push_first(); }

        r_size pop_first_n(r_size n) { return p_range.pop_first_n(n); }
        r_size pop_last_n(r_size n) { return p_range.pop_last_n(n); }

        r_size push_first_n(r_size n) { return p_range.push_first_n(n); }
        r_size push_last_n(r_size n) { return p_range.push_last_n(n); }

        r_ref first() const { return p_range.last(); }
        r_ref last() const { return p_range.first(); }

        r_ref operator[](r_size i) const { return p_range[size() - i - 1]; }

        ReverseRange<T> slice(r_size start, r_size end) const {
            r_size len = p_range.size();
            return ReverseRange<T>(p_range.slice(len - end, len - start));
        }
    };

    template<typename T>
    ReverseRange<T> make_reverse_range(const T &it) {
        return ReverseRange<T>(it);
    }

    template<typename T>
    struct MoveRange: InputRange<MoveRange<T>,
        RangeCategory<T>, RangeValue<T>, RangeValue<T> &&, RangeSize<T>,
        RangeDifference<T>
    > {
    private:
        typedef RangeValue<T>   r_val;
        typedef RangeValue<T> &&r_ref;
        typedef RangeSize<T>    r_size;

        T p_range;

    public:
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
            p_range = move(v);
            return *this;
        }

        bool empty() const { return p_range.empty(); }
        r_size size() const { return p_range.size(); }

        bool equals_first(const MoveRange &range)  const {
            return p_range.equals_first(range.p_range);
        }
        bool equals_last(const MoveRange &range) const {
            return p_range.equals_last(range.p_range);
        }

        RangeDifference<T> distance_first(const MoveRange &range) const {
            return p_range.distance_first(range.p_range);
        }
        RangeDifference<T> distance_last(const MoveRange &range) const {
            return p_range.distance_last(range.p_range);
        }

        bool pop_first() { return p_range.pop_first(); }
        bool pop_last() { return p_range.pop_last(); }

        bool push_first() { return p_range.push_first(); }
        bool push_last() { return p_range.push_last(); }

        r_size pop_first_n(r_size n) { return p_range.pop_first_n(n); }
        r_size pop_last_n(r_size n) { return p_range.pop_last_n(n); }

        r_size push_first_n(r_size n) { return p_range.push_first_n(n); }
        r_size push_last_n(r_size n) { return p_range.push_last_n(n); }

        r_ref first() const { return move(p_range.first()); }
        r_ref last() const { return move(p_range.last()); }

        r_ref operator[](r_size i) const { return move(p_range[i]); }

        MoveRange<T> slice(r_size start, r_size end) const {
            return MoveRange<T>(p_range.slice(start, end));
        }

        void put(const r_val &v) { p_range.put(v); }
        void put(r_val &&v) { p_range.put(move(v)); }
    };

    template<typename T>
    MoveRange<T> make_move_range(const T &it) {
        return MoveRange<T>(it);
    }

    template<typename T>
    struct NumberRange: InputRange<NumberRange<T>, ForwardRangeTag, T, T> {
        NumberRange(): p_a(0), p_b(0), p_step(0) {}
        NumberRange(const NumberRange &it): p_a(it.p_a), p_b(it.p_b),
            p_step(it.p_step) {}
        NumberRange(T a, T b, T step = T(1)): p_a(a), p_b(b),
            p_step(step) {}
        NumberRange(T v): p_a(0), p_b(v), p_step(1) {}

        bool empty() const { return p_a * p_step >= p_b * p_step; }

        bool equals_first(const NumberRange &range) const {
            return p_a == range.p_a;
        }

        bool pop_first() { p_a += p_step; return true; }
        bool push_first() { p_a -= p_step; return true; }
        T first() const { return p_a; }

    private:
        T p_a, p_b, p_step;
    };

    template<typename T>
    NumberRange<T> range(T a, T b, T step = T(1)) {
        return NumberRange<T>(a, b, step);
    }

    template<typename T>
    NumberRange<T> range(T v) {
        return NumberRange<T>(v);
    }

    template<typename T>
    struct PointerRange: InputRange<PointerRange<T>, FiniteRandomAccessRangeTag, T> {
        PointerRange(): p_beg(nullptr), p_end(nullptr) {}
        PointerRange(const PointerRange &v): p_beg(v.p_beg),
            p_end(v.p_end) {}
        PointerRange(T *beg, T *end): p_beg(beg), p_end(end) {}
        PointerRange(T *beg, size_t n): p_beg(beg), p_end(beg + n) {}

        PointerRange &operator=(const PointerRange &v) {
            p_beg = v.p_beg;
            p_end = v.p_end;
            return *this;
        }

        /* satisfy InputRange / ForwardRange */
        bool empty() const { return p_beg == p_end; }

        bool pop_first() {
            if (p_beg == p_end) return false;
            ++p_beg;
            return true;
        }
        bool push_first() {
            --p_beg; return true;
        }

        size_t pop_first_n(size_t n) {
            T *obeg = p_beg;
            size_t olen = p_end - p_beg;
            p_beg += n;
            if (p_beg > p_end) {
                p_beg = p_end;
                return olen;
            }
            return n;
        }

        size_t push_first_n(size_t n) {
            p_beg -= n; return true;
        }

        T &first() const { return *p_beg; }

        bool equals_first(const PointerRange &range)  const {
            return p_beg == range.p_beg;
        }

        ptrdiff_t distance_first(const PointerRange &range) const {
            return range.p_beg;
        }

        /* satisfy BidirectionalRange */
        bool pop_last() {
            if (p_end == p_beg) return false;
            --p_end;
            return true;
        }
        bool push_last() {
            ++p_end; return true;
        }

        size_t pop_last_n(size_t n) {
            T *oend = p_end;
            size_t olen = p_end - p_beg;
            p_end -= n;
            if (p_end < p_beg) {
                p_end = p_beg;
                return olen;
            }
            return n;
        }

        size_t push_last_n(size_t n) {
            p_end += n; return true;
        }

        T &last() const { return *(p_end - 1); }

        bool equals_last(const PointerRange &range) const {
            return p_end == range.p_end;
        }

        ptrdiff_t distance_last(const PointerRange &range) const {
            return range.p_end;
        }

        /* satisfy FiniteRandomAccessRange */
        size_t size() const { return p_end - p_beg; }

        PointerRange slice(size_t start, size_t end) const {
            return PointerRange(p_beg + start, p_beg + end);
        }

        T &operator[](size_t i) const { return p_beg[i]; }

        /* satisfy OutputRange */
        void put(const T &v) {
            *(p_beg++) = v;
        }
        void put(T &&v) {
            *(p_beg++) = move(v);
        }

    private:
        T *p_beg, *p_end;
    };

    template<typename T, typename S>
    struct EnumeratedValue {
        S index;
        T value;
    };

    template<typename T>
    struct EnumeratedRange: InputRange<EnumeratedRange<T>,
        CommonType<RangeCategory<T>, ForwardRangeTag>, RangeValue<T>,
        EnumeratedValue<RangeReference<T>, RangeSize<T>>,
        RangeSize<T>
    > {
    private:
        typedef RangeReference<T> r_ref;
        typedef RangeSize<T> r_size;

        T p_range;
        r_size p_index;

    public:
        EnumeratedRange(): p_range(), p_index(0) {}

        EnumeratedRange(const T &range): p_range(range), p_index(0) {}

        EnumeratedRange(const EnumeratedRange &it):
            p_range(it.p_range), p_index(it.p_index) {}

        EnumeratedRange(EnumeratedRange &&it):
            p_range(move(it.p_range)), p_index(it.p_index) {}

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
            p_range = move(v);
            p_index = 0;
            return *this;
        }

        bool empty() const { return p_range.empty(); }

        bool equals_first(const EnumeratedRange &range) const {
            return p_range.equals_first(range.p_range);
        }

        bool pop_first() {
            if (p_range.pop_first()) {
                ++p_index;
                return true;
            }
            return false;
        }

        r_size pop_first_n(r_size n) {
            r_size ret = p_range.pop_first_n(n);
            p_index += ret;
            return ret;
        }

        EnumeratedValue<r_ref, r_size> first() const {
            return EnumeratedValue<r_ref, r_size> { p_index, p_range.first() };
        }
    };

    template<typename T>
    EnumeratedRange<T> enumerate(const T &it) {
        return EnumeratedRange<T>(it);
    }

    template<typename T>
    struct TakeRange: InputRange<TakeRange<T>,
        CommonType<RangeCategory<T>, ForwardRangeTag>,
        RangeValue<T>, RangeReference<T>, RangeSize<T>
    > {
    private:
        T p_range;
        RangeSize<T> p_remaining;
    public:
        TakeRange(): p_range(), p_remaining(0) {}
        TakeRange(const T &range, RangeSize<T> rem): p_range(range),
            p_remaining(rem) {}
        TakeRange(const TakeRange &it): p_range(it.p_range),
            p_remaining(it.p_remaining) {}
        TakeRange(TakeRange &&it): p_range(move(it.p_range)),
            p_remaining(it.p_remaining) {}

        TakeRange &operator=(const TakeRange &v) {
            p_range = v.p_range; p_remaining = v.p_remaining; return *this;
        }
        TakeRange &operator=(TakeRange &&v) {
            p_range = move(v.p_range); p_remaining = v.p_remaining; return *this;
        }

        bool empty() const { return (p_remaining <= 0) || p_range.empty(); }

        bool pop_first() {
            if (p_range.pop_first()) {
                --p_remaining;
                return true;
            }
            return false;
        }
        bool push_first() {
            if (p_range.push_first()) {
                ++p_remaining;
                return true;
            }
            return false;
        }

        RangeSize<T> pop_first_n(RangeSize<T> n) {
            RangeSize<T> ret = p_range.pop_first_n(n);
            p_remaining -= ret;
            return ret;
        }
        RangeSize<T> push_first_n(RangeSize<T> n) {
            RangeSize<T> ret = p_range.push_first_n(n);
            p_remaining += ret;
            return ret;
        }

        RangeReference<T> first() const { return p_range.first(); }

        bool equals_first(const TakeRange &range) const {
            return p_range.equals_first(range.p_range);
        }

        RangeDifference<T> distance_first(const TakeRange &range) const {
            return p_range.distance_first(range.p_range);
        }
    };

    template<typename T>
    TakeRange<T> take(const T &it, RangeSize<T> n) {
        return TakeRange<T>(it, n);
    }

    template<typename T>
    struct ChunksRange: InputRange<ChunksRange<T>,
        CommonType<RangeCategory<T>, ForwardRangeTag>,
        TakeRange<T>, TakeRange<T>, RangeSize<T>
    > {
    private:
        T p_range;
        RangeSize<T> p_chunksize;
    public:
        ChunksRange(): p_range(), p_chunksize(0) {}
        ChunksRange(const T &range, RangeSize<T> chs): p_range(range),
            p_chunksize(chs) {}
        ChunksRange(const ChunksRange &it): p_range(it.p_range),
            p_chunksize(it.p_chunksize) {}
        ChunksRange(ChunksRange &&it): p_range(move(it.p_range)),
            p_chunksize(it.p_chunksize) {}

        ChunksRange &operator=(const ChunksRange &v) {
            p_range = v.p_range; p_chunksize = v.p_chunksize; return *this;
        }
        ChunksRange &operator=(ChunksRange &&v) {
            p_range = move(v.p_range); p_chunksize = v.p_chunksize; return *this;
        }

        bool empty() const { return p_range.empty(); }

        bool equals_first(const ChunksRange &range) const {
            return p_range.equals_first(range.p_range);
        }

        bool pop_first() { return p_range.pop_first_n(p_chunksize) > 0; }
        bool push_first() {
            T tmp = p_range;
            RangeSize<T> an = tmp.push_first_n(p_chunksize);
            if (an != p_chunksize) return false;
            p_range = tmp;
            return true;
        }
        RangeSize<T> pop_first_n(RangeSize<T> n) {
            return p_range.pop_first_n(p_chunksize * n) / p_chunksize;
        }
        RangeSize<T> push_first_n(RangeSize<T> n) {
            T tmp = p_range;
            RangeSize<T> an = tmp.push_first_n(p_chunksize * n);
            RangeSize<T> pn = an / p_chunksize;
            if (!pn) return 0;
            if (pn == n) {
                p_range = tmp;
                return pn;
            }
            return p_range.push_first_n(p_chunksize * an) / p_chunksize;
        }

        TakeRange<T> first() const { return take(p_range, p_chunksize); }
    };

    template<typename T>
    ChunksRange<T> chunks(const T &it, RangeSize<T> chs) {
        return ChunksRange<T>(it, chs);
    }

    template<typename T>
    auto each(T &r) -> decltype(r.each()) {
        return r.each();
    }

    template<typename T>
    auto each(const T &r) -> decltype(r.each()) {
        return r.each();
    }

    template<typename T, size_t N>
    PointerRange<T> each(T (&array)[N]) {
        return PointerRange<T>(array, N);
    }

    template<typename T>
    struct HalfRange: InputRange<HalfRange<T>,
        RangeCategory<T>, RangeValue<T>, RangeReference<T>, RangeSize<T>,
        RangeDifference<T>
    > {
    private:
        T p_beg;
        T p_end;
    public:
        HalfRange(): p_beg(), p_end() {}
        HalfRange(const HalfRange &range): p_beg(range.p_beg),
            p_end(range.p_end) {}
        HalfRange(HalfRange &&range): p_beg(move(range.p_beg)),
            p_end(move(range.p_end)) {}
        HalfRange(const T &beg, const T &end): p_beg(beg), p_end(end) {}
        HalfRange(T &&beg, T &&end): p_beg(move(beg)), p_end(move(end)) {}

        HalfRange &operator=(const HalfRange &range) {
            p_beg = range.p_beg;
            p_end = range.p_end;
            return *this;
        }

        HalfRange &operator=(HalfRange &&range) {
            p_beg = move(range.p_beg);
            p_end = move(range.p_end);
            return *this;
        }

        bool empty() const { return p_beg == p_end; }

        bool pop_first() {
            if (empty()) return false;
            return p_beg.next();
        }
        bool push_first() {
            return p_beg.prev();
        }
        bool pop_last() {
            if (empty()) return false;
            return p_end.prev();
        }
        bool push_last() {
            return p_end.next();
        }

        RangeReference<T> first() const {
            return p_beg.get();
        }
        RangeReference<T> last() const {
            auto copy = p_end;
            copy.prev();
            return copy.get();
        }

        bool equals_first(const HalfRange &range) const {
            return p_beg == range.p_beg;
        }
        bool equals_last(const HalfRange &range) const {
            return p_end == range.p_end;
        }

        RangeDifference<T> distance_first(const HalfRange &range) const {
            return p_beg.distance(range.p_beg);
        }
        RangeDifference<T> distance_last(const HalfRange &range) const {
            return p_end.distance(range.p_end);
        }

        RangeSize<T> size() const {
            return p_end - p_beg;
        }

        HalfRange<T> slice(RangeSize<T> start, RangeSize<T> end) const {
            return HalfRange<T>(p_beg + start, p_beg + end);
        }

        RangeReference<T> operator[](RangeSize<T> idx) const {
            return p_beg[idx];
        }

        void put(const RangeValue<T> &v) {
            p_beg.range().put(v);
        }
        void put(RangeValue<T> &&v) {
            p_beg.range().put(move(v));
        }
    };

    template<typename T>
    HalfRange<RangeHalf<T>>
    make_half_range(const RangeHalf<T> &a, const RangeHalf<T> &b) {
        return HalfRange<RangeHalf<T>>(a, b);
    }
}

#endif