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

    template<typename _T> struct RangeHalf;

#define __OCTA_RANGE_TRAIT(_Name, _TypeName) \
    template<typename _T> \
    struct __OctaRange##_Name { \
        typedef typename _T::_TypeName Type; \
    }; \
    template<typename _T> \
    struct __OctaRange##_Name<RangeHalf<_T>> { \
        typedef typename _T::_TypeName Type; \
    }; \
    template<typename _T> \
    using Range##_Name = typename __OctaRange##_Name<_T>::Type;

    __OCTA_RANGE_TRAIT(Category, Category)
    __OCTA_RANGE_TRAIT(Size, Size)
    __OCTA_RANGE_TRAIT(Value, Value)
    __OCTA_RANGE_TRAIT(Reference, Reference)
    __OCTA_RANGE_TRAIT(Difference, Difference)

#undef __OCTA_RANGE_TRAIT

    // is input range

    template<typename _T, bool = octa::IsConvertible<
        RangeCategory<_T>, InputRangeTag
    >::value> struct IsInputRange: False {};

    template<typename _T>
    struct IsInputRange<_T, true>: True {};

    // is forward range

    template<typename _T, bool = octa::IsConvertible<
        RangeCategory<_T>, ForwardRangeTag
    >::value> struct IsForwardRange: False {};

    template<typename _T>
    struct IsForwardRange<_T, true>: True {};

    // is bidirectional range

    template<typename _T, bool = octa::IsConvertible<
        RangeCategory<_T>, BidirectionalRangeTag
    >::value> struct IsBidirectionalRange: False {};

    template<typename _T>
    struct IsBidirectionalRange<_T, true>: True {};

    // is random access range

    template<typename _T, bool = octa::IsConvertible<
        RangeCategory<_T>, RandomAccessRangeTag
    >::value> struct IsRandomAccessRange: False {};

    template<typename _T>
    struct IsRandomAccessRange<_T, true>: True {};

    // is finite random access range

    template<typename _T, bool = octa::IsConvertible<
        RangeCategory<_T>, FiniteRandomAccessRangeTag
    >::value> struct IsFiniteRandomAccessRange: False {};

    template<typename _T>
    struct IsFiniteRandomAccessRange<_T, true>: True {};

    // is infinite random access range

    template<typename _T>
    struct IsInfiniteRandomAccessRange: IntegralConstant<bool,
        (IsRandomAccessRange<_T>::value && !IsFiniteRandomAccessRange<_T>::value)
    > {};

    // is output range

    template<typename _T, typename _P>
    struct __OctaOutputRangeTest {
        template<typename _U, void (_U::*)(_P)> struct __Test {};
        template<typename _U> static char __test(__Test<_U, &_U::put> *);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == sizeof(char));
    };

    template<typename _T, bool = (octa::IsConvertible<
        RangeCategory<_T>, OutputRangeTag
    >::value || (IsInputRange<_T>::value &&
        (__OctaOutputRangeTest<_T, const RangeValue<_T>  &>::value ||
         __OctaOutputRangeTest<_T,       RangeValue<_T> &&>::value)
    ))> struct IsOutputRange: False {};

    template<typename _T>
    struct IsOutputRange<_T, true>: True {};

    // range iterator

    template<typename _T>
    struct __OctaRangeIterator {
        __OctaRangeIterator(): __range() {}
        explicit __OctaRangeIterator(const _T &__range): __range(__range) {}
        __OctaRangeIterator &operator++() {
            __range.pop_front();
            return *this;
        }
        RangeReference<_T> operator*() const {
            return __range.front();
        }
        bool operator!=(__OctaRangeIterator) const { return !__range.empty(); }
    private:
        _T __range;
    };

    // range half

    template<typename _T>
    struct RangeHalf {
    private:
        _T __range;
    public:
        typedef _T Range;

        RangeHalf(): __range() {}
        RangeHalf(const _T &__range): __range(__range) {}
        RangeHalf(const RangeHalf &__half): __range(__half.__range) {}
        RangeHalf(RangeHalf &&__half): __range(octa::move(__half.__range)) {}

        RangeHalf &operator=(const RangeHalf &__half) {
            __range = __half.__range;
            return *this;
        }

        RangeHalf &operator=(RangeHalf &&__half) {
            __range = octa::move(__half.__range);
            return *this;
        }

        _T range() const { return __range; }

        bool next() { return __range.pop_front(); }
        bool prev() { return __range.push_front(); }

        RangeSize<_T> next_n(RangeSize<_T> __n) {
            return __range.pop_front_n(__n);
        }
        RangeSize<_T> prev_n(RangeSize<_T> __n) {
            return __range.push_front_n(__n);
        }

        RangeReference<_T> get() const {
            return __range.front();
        }

        RangeDifference<_T> distance(const RangeHalf &__half) const {
            return __range.distance_front(__half.__range);
        }

        bool equals(const RangeHalf &__half) const {
            return __range.equals_front(__half.__range);
        }

        bool operator==(const RangeHalf &__half) const {
            return equals(__half);
        }
        bool operator!=(const RangeHalf &__half) const {
            return !equals(__half);
        }

        /* iterator like interface */

        RangeReference<_T> operator*() const {
            return get();
        }

        RangeReference<_T> operator[](RangeSize<_T> __idx) const {
            return __range[__idx];
        }

        RangeHalf &operator++() {
            next();
            return *this;
        }
        RangeHalf operator++(int) {
            RangeHalf __tmp(*this);
            next();
            return octa::move(__tmp);
        }

        RangeHalf &operator--() {
            prev();
            return *this;
        }
        RangeHalf operator--(int) {
            RangeHalf __tmp(*this);
            prev();
            return octa::move(__tmp);
        }

        RangeHalf operator+(RangeDifference<_T> __n) {
            RangeHalf __tmp(*this);
            if (__n < 0) __tmp.prev_n(-__n);
            else __tmp.next_n(__n);
            return octa::move(__tmp);
        }
        RangeHalf operator-(RangeDifference<_T> __n) {
            RangeHalf __tmp(*this);
            if (__n < 0) __tmp.next_n(-__n);
            else __tmp.prev_n(__n);
            return octa::move(__tmp);
        }

        RangeHalf &operator+=(RangeDifference<_T> __n) {
            if (__n < 0) prev_n(-__n);
            else next_n(__n);
            return *this;
        }
        RangeHalf &operator-=(RangeDifference<_T> __n) {
            if (__n < 0) next_n(-__n);
            else prev_n(__n);
            return *this;
        }
    };

    template<typename _R>
    RangeDifference<_R> operator-(const _R &__lhs, const _R &__rhs) {
        return __rhs.distance(__lhs);
    }

    template<typename _R>
    RangeSize<_R> __octa_pop_front_n(_R &__range, RangeSize<_R> __n) {
        for (RangeSize<_R> __i = 0; __i < __n; ++__i)
            if (!__range.pop_front()) return __i;
        return __n;
    }

    template<typename _R>
    RangeSize<_R> __octa_pop_back_n(_R &__range, RangeSize<_R> __n) {
        for (RangeSize<_R> __i = 0; __i < __n; ++__i)
            if (!__range.pop_back()) return __i;
        return __n;
    }

    template<typename _R>
    RangeSize<_R> __octa_push_front_n(_R &__range, RangeSize<_R> __n) {
        for (RangeSize<_R> __i = 0; __i < __n; ++__i)
            if (!__range.push_front()) return __i;
        return __n;
    }

    template<typename _R>
    RangeSize<_R> __octa_push_back_n(_R &__range, RangeSize<_R> __n) {
        for (RangeSize<_R> __i = 0; __i < __n; ++__i)
            if (!__range.push_back()) return __i;
        return __n;
    }

    template<typename _B, typename _C, typename _V, typename _R = _V &,
             typename _S = size_t, typename _D = ptrdiff_t
    > struct InputRange {
        typedef _C Category;
        typedef _S Size;
        typedef _D Difference;
        typedef _V Value;
        typedef _R Reference;

        __OctaRangeIterator<_B> begin() const {
            return __OctaRangeIterator<_B>((const _B &)*this);
        }
        __OctaRangeIterator<_B> end() const {
            return __OctaRangeIterator<_B>();
        }

        Size pop_front_n(Size __n) {
            return __octa_pop_front_n<_B>(*((_B *)this), __n);
        }

        Size pop_back_n(Size __n) {
            return __octa_pop_back_n<_B>(*((_B *)this), __n);
        }

        Size push_front_n(Size __n) {
            return __octa_push_front_n<_B>(*((_B *)this), __n);
        }

        Size push_back_n(Size __n) {
            return __octa_push_back_n<_B>(*((_B *)this), __n);
        }

        _B each() const {
            return _B(*((_B *)this));
        }

        RangeHalf<_B> half() const {
            return RangeHalf<_B>(*((_B *)this));
        }
    };

    template<typename _V, typename _R = _V &, typename _S = size_t,
             typename _D = ptrdiff_t
    > struct OutputRange {
        typedef OutputRangeTag Category;
        typedef _S Size;
        typedef _D Difference;
        typedef _V Value;
        typedef _R Reference;
    };

    template<typename _T>
    struct ReverseRange: InputRange<ReverseRange<_T>,
        RangeCategory<_T>, RangeValue<_T>, RangeReference<_T>, RangeSize<_T>,
        RangeDifference<_T>
    > {
    private:
        typedef RangeReference<_T> _r_ref;
        typedef RangeSize<_T> _r_size;

        _T __range;

    public:
        ReverseRange(): __range() {}

        ReverseRange(const _T &__range): __range(__range) {}

        ReverseRange(const ReverseRange &__it): __range(__it.__range) {}

        ReverseRange(ReverseRange &&__it): __range(octa::move(__it.__range)) {}

        ReverseRange &operator=(const ReverseRange &__v) {
            __range = __v.__range;
            return *this;
        }
        ReverseRange &operator=(ReverseRange &&__v) {
            __range = octa::move(__v.__range);
            return *this;
        }
        ReverseRange &operator=(const _T &__v) {
            __range = __v;
            return *this;
        }
        ReverseRange &operator=(_T &&__v) {
            __range = octa::move(__v);
            return *this;
        }

        bool empty() const { return __range.empty(); }
        _r_size size() const { return __range.size(); }

        bool equals_front(const ReverseRange &__r) const {
        return __range.equals_back(__r.__range);
        }
        bool equals_back(const ReverseRange &__r) const {
            return __range.equals_front(__r.__range);
        }

        RangeDifference<_T> distance_front(const ReverseRange &__r) const {
            return -__range.distance_back(__r.__range);
        }
        RangeDifference<_T> distance_back(const ReverseRange &__r) const {
            return -__range.distance_front(__r.__range);
        }

        bool pop_front() { return __range.pop_back(); }
        bool pop_back() { return __range.pop_front(); }

        bool push_front() { return __range.push_back(); }
        bool push_back() { return __range.push_front(); }

        _r_size pop_front_n(_r_size __n) { return __range.pop_front_n(__n); }
        _r_size pop_back_n(_r_size __n) { return __range.pop_back_n(__n); }

        _r_size push_front_n(_r_size __n) { return __range.push_front_n(__n); }
        _r_size push_back_n(_r_size __n) { return __range.push_back_n(__n); }

        _r_ref front() const { return __range.back(); }
        _r_ref back() const { return __range.front(); }

        _r_ref operator[](_r_size __i) const { return __range[size() - __i - 1]; }

        ReverseRange<_T> slice(_r_size __start, _r_size __end) const {
            _r_size __len = __range.size();
            return ReverseRange<_T>(__range.slice(__len - __end, __len - __start));
        }
    };

    template<typename _T>
    ReverseRange<_T> make_reverse_range(const _T &__it) {
        return ReverseRange<_T>(__it);
    }

    template<typename _T>
    struct MoveRange: InputRange<MoveRange<_T>,
        RangeCategory<_T>, RangeValue<_T>, RangeValue<_T> &&, RangeSize<_T>,
        RangeDifference<_T>
    > {
    private:
        typedef RangeValue<_T>   _r_val;
        typedef RangeValue<_T> &&_r_ref;
        typedef RangeSize<_T>    _r_size;

        _T __range;

    public:
        MoveRange(): __range() {}

        MoveRange(const _T &__range): __range(__range) {}

        MoveRange(const MoveRange &__it): __range(__it.__range) {}

        MoveRange(MoveRange &&__it): __range(octa::move(__it.__range)) {}

        MoveRange &operator=(const MoveRange &__v) {
            __range = __v.__range;
            return *this;
        }
        MoveRange &operator=(MoveRange &&__v) {
            __range = octa::move(__v.__range);
            return *this;
        }
        MoveRange &operator=(const _T &__v) {
            __range = __v;
            return *this;
        }
        MoveRange &operator=(_T &&__v) {
            __range = octa::move(__v);
            return *this;
        }

        bool empty() const { return __range.empty(); }
        _r_size size() const { return __range.size(); }

        bool equals_front(const MoveRange &__r) const {
            return __range.equals_front(__r.__range);
        }
        bool equals_back(const MoveRange &__r) const {
            return __range.equals_back(__r.__range);
        }

        RangeDifference<_T> distance_front(const MoveRange &__r) const {
            return __range.distance_front(__r.__range);
        }
        RangeDifference<_T> distance_back(const MoveRange &__r) const {
            return __range.distance_back(__r.__range);
        }

        bool pop_front() { return __range.pop_front(); }
        bool pop_back() { return __range.pop_back(); }

        bool push_front() { return __range.push_front(); }
        bool push_back() { return __range.push_back(); }

        _r_size pop_front_n(_r_size __n) { return __range.pop_front_n(__n); }
        _r_size pop_back_n(_r_size __n) { return __range.pop_back_n(__n); }

        _r_size push_front_n(_r_size __n) { return __range.push_front_n(__n); }
        _r_size push_back_n(_r_size __n) { return __range.push_back_n(__n); }

        _r_ref front() const { return octa::move(__range.front()); }
        _r_ref back() const { return octa::move(__range.back()); }

        _r_ref operator[](_r_size __i) const { return octa::move(__range[__i]); }

        MoveRange<_T> slice(_r_size __start, _r_size __end) const {
            return MoveRange<_T>(__range.slice(__start, __end));
        }

        void put(const _r_val &__v) { __range.put(__v); }
        void put(_r_val &&__v) { __range.put(octa::move(__v)); }
    };

    template<typename _T>
    MoveRange<_T> make_move_range(const _T &__it) {
        return MoveRange<_T>(__it);
    }

    template<typename _T>
    struct NumberRange: InputRange<NumberRange<_T>, ForwardRangeTag, _T, _T> {
        NumberRange(): __a(0), __b(0), __step(0) {}
        NumberRange(const NumberRange &__it): __a(__it.__a), __b(__it.__b),
            __step(__it.__step) {}
        NumberRange(_T __a, _T __b, _T __step = _T(1)): __a(__a), __b(__b),
            __step(__step) {}
        NumberRange(_T __v): __a(0), __b(__v), __step(1) {}

        bool empty() const { return __a * __step >= __b * __step; }

        bool equals_front(const NumberRange &__range) const {
            return __a == __range.__a;
        }

        bool pop_front() { __a += __step; return true; }
        bool push_front() { __a -= __step; return true; }
        _T front() const { return __a; }

    private:
        _T __a, __b, __step;
    };

    template<typename _T>
    NumberRange<_T> range(_T __a, _T __b, _T __step = _T(1)) {
        return NumberRange<_T>(__a, __b, __step);
    }

    template<typename _T>
    NumberRange<_T> range(_T __v) {
        return NumberRange<_T>(__v);
    }

    template<typename _T>
    struct PointerRange: InputRange<PointerRange<_T>, FiniteRandomAccessRangeTag, _T> {
        PointerRange(): __beg(nullptr), __end(nullptr) {}
        PointerRange(const PointerRange &__v): __beg(__v.__beg),
            __end(__v.__end) {}
        PointerRange(_T *__beg, _T *__end): __beg(__beg), __end(__end) {}
        PointerRange(_T *__beg, size_t __n): __beg(__beg), __end(__beg + __n) {}

        PointerRange &operator=(const PointerRange &__v) {
            __beg = __v.__beg;
            __end = __v.__end;
            return *this;
        }

        /* satisfy InputRange / ForwardRange */
        bool empty() const { return __beg == __end; }

        bool pop_front() {
            if (__beg == __end) return false;
            ++__beg;
            return true;
        }
        bool push_front() {
            --__beg; return true;
        }

        size_t pop_front_n(size_t __n) {
            size_t __olen = __end - __beg;
            __beg += __n;
            if (__beg > __end) {
                __beg = __end;
                return __olen;
            }
            return __n;
        }

        size_t push_front_n(size_t __n) {
            __beg -= __n; return true;
        }

        _T &front() const { return *__beg; }

        bool equals_front(const PointerRange &__range) const {
            return __beg == __range.__beg;
        }

        ptrdiff_t distance_front(const PointerRange &__range) const {
            return __range.__beg - __beg;
        }

        /* satisfy BidirectionalRange */
        bool pop_back() {
            if (__end == __beg) return false;
            --__end;
            return true;
        }
        bool push_back() {
            ++__end; return true;
        }

        size_t pop_back_n(size_t __n) {
            size_t __olen = __end - __beg;
            __end -= __n;
            if (__end < __beg) {
                __end = __beg;
                return __olen;
            }
            return __n;
        }

        size_t push_back_n(size_t __n) {
            __end += __n; return true;
        }

        _T &back() const { return *(__end - 1); }

        bool equals_back(const PointerRange &__range) const {
            return __end == __range.__end;
        }

        ptrdiff_t distance_back(const PointerRange &__range) const {
            return __range.__end - __end;
        }

        /* satisfy FiniteRandomAccessRange */
        size_t size() const { return __end - __beg; }

        PointerRange slice(size_t __start, size_t __end) const {
            return PointerRange(__beg + __start, __beg + __end);
        }

        _T &operator[](size_t __i) const { return __beg[__i]; }

        /* satisfy OutputRange */
        void put(const _T &__v) {
            *(__beg++) = __v;
        }
        void put(_T &&__v) {
            *(__beg++) = octa::move(__v);
        }

    private:
        _T *__beg, *__end;
    };

    template<typename _T, typename _S>
    struct EnumeratedValue {
        _S index;
        _T value;
    };

    template<typename _T>
    struct EnumeratedRange: InputRange<EnumeratedRange<_T>,
        CommonType<RangeCategory<_T>, ForwardRangeTag>, RangeValue<_T>,
        EnumeratedValue<RangeReference<_T>, RangeSize<_T>>,
        RangeSize<_T>
    > {
    private:
        typedef RangeReference<_T> _r_ref;
        typedef RangeSize<_T> _r_size;

        _T __range;
        _r_size __index;

    public:
        EnumeratedRange(): __range(), __index(0) {}

        EnumeratedRange(const _T &__range): __range(__range), __index(0) {}

        EnumeratedRange(const EnumeratedRange &__it):
            __range(__it.__range), __index(__it.__index) {}

        EnumeratedRange(EnumeratedRange &&__it):
            __range(octa::move(__it.__range)), __index(__it.__index) {}

        EnumeratedRange &operator=(const EnumeratedRange &__v) {
            __range = __v.__range;
            __index = __v.__index;
            return *this;
        }
        EnumeratedRange &operator=(EnumeratedRange &&__v) {
            __range = octa::move(__v.__range);
            __index = __v.__index;
            return *this;
        }
        EnumeratedRange &operator=(const _T &__v) {
            __range = __v;
            __index = 0;
            return *this;
        }
        EnumeratedRange &operator=(_T &&__v) {
            __range = octa::move(__v);
            __index = 0;
            return *this;
        }

        bool empty() const { return __range.empty(); }

        bool equals_front(const EnumeratedRange &__r) const {
            return __range.equals_front(__r.__range);
        }

        bool pop_front() {
            if (__range.pop_front()) {
                ++__index;
                return true;
            }
            return false;
        }

        _r_size pop_front_n(_r_size __n) {
            _r_size __ret = __range.pop_front_n(__n);
            __index += __ret;
            return __ret;
        }

        EnumeratedValue<_r_ref, _r_size> front() const {
            return EnumeratedValue<_r_ref, _r_size> { __index, __range.front() };
        }
    };

    template<typename _T>
    EnumeratedRange<_T> enumerate(const _T &__it) {
        return EnumeratedRange<_T>(__it);
    }

    template<typename _T>
    struct TakeRange: InputRange<TakeRange<_T>,
        CommonType<RangeCategory<_T>, ForwardRangeTag>,
        RangeValue<_T>, RangeReference<_T>, RangeSize<_T>
    > {
    private:
        _T __range;
        RangeSize<_T> __remaining;
    public:
        TakeRange(): __range(), __remaining(0) {}
        TakeRange(const _T &__range, RangeSize<_T> __rem): __range(__range),
            __remaining(__rem) {}
        TakeRange(const TakeRange &__it): __range(__it.__range),
            __remaining(__it.__remaining) {}
        TakeRange(TakeRange &&__it): __range(octa::move(__it.__range)),
            __remaining(__it.__remaining) {}

        TakeRange &operator=(const TakeRange &__v) {
            __range = __v.__range; __remaining = __v.__remaining; return *this;
        }
        TakeRange &operator=(TakeRange &&__v) {
            __range = octa::move(__v.__range);
            __remaining = __v.__remaining;
            return *this;
        }

        bool empty() const { return (__remaining <= 0) || __range.empty(); }

        bool pop_front() {
            if (__range.pop_front()) {
                --__remaining;
                return true;
            }
            return false;
        }
        bool push_front() {
            if (__range.push_front()) {
                ++__remaining;
                return true;
            }
            return false;
        }

        RangeSize<_T> pop_front_n(RangeSize<_T> __n) {
            RangeSize<_T> __ret = __range.pop_front_n(__n);
            __remaining -= __ret;
            return __ret;
        }
        RangeSize<_T> push_front_n(RangeSize<_T> __n) {
            RangeSize<_T> __ret = __range.push_front_n(__n);
            __remaining += __ret;
            return __ret;
        }

        RangeReference<_T> front() const { return __range.front(); }

        bool equals_front(const TakeRange &__r) const {
            return __range.equals_front(__r.__range);
        }

        RangeDifference<_T> distance_front(const TakeRange &__r) const {
            return __range.distance_front(__r.__range);
        }
    };

    template<typename _T>
    TakeRange<_T> take(const _T &__it, RangeSize<_T> __n) {
        return TakeRange<_T>(__it, __n);
    }

    template<typename _T>
    struct ChunksRange: InputRange<ChunksRange<_T>,
        CommonType<RangeCategory<_T>, ForwardRangeTag>,
        TakeRange<_T>, TakeRange<_T>, RangeSize<_T>
    > {
    private:
        _T __range;
        RangeSize<_T> __chunksize;
    public:
        ChunksRange(): __range(), __chunksize(0) {}
        ChunksRange(const _T &__range, RangeSize<_T> __chs): __range(__range),
            __chunksize(__chs) {}
        ChunksRange(const ChunksRange &__it): __range(__it.__range),
            __chunksize(__it.__chunksize) {}
        ChunksRange(ChunksRange &&__it): __range(octa::move(__it.__range)),
            __chunksize(__it.__chunksize) {}

        ChunksRange &operator=(const ChunksRange &__v) {
            __range = __v.__range; __chunksize = __v.p_chunksize; return *this;
        }
        ChunksRange &operator=(ChunksRange &&__v) {
            __range = octa::move(__v.__range);
            __chunksize = __v.__chunksize;
            return *this;
        }

        bool empty() const { return __range.empty(); }

        bool equals_front(const ChunksRange &__r) const {
            return __range.equals_front(__r.__range);
        }

        bool pop_front() { return __range.pop_front_n(__chunksize) > 0; }
        bool push_front() {
            _T __tmp = __range;
            RangeSize<_T> __an = __tmp.push_front_n(__chunksize);
            if (__an != __chunksize) return false;
            __range = __tmp;
            return true;
        }
        RangeSize<_T> pop_front_n(RangeSize<_T> __n) {
            return __range.pop_front_n(__chunksize * __n) / __chunksize;
        }
        RangeSize<_T> push_front_n(RangeSize<_T> __n) {
            _T __tmp = __range;
            RangeSize<_T> __an = __tmp.push_front_n(__chunksize * __n);
            RangeSize<_T> __pn = __an / __chunksize;
            if (!__pn) return 0;
            if (__pn == __n) {
                __range = __tmp;
                return __pn;
            }
            return __range.push_front_n(__chunksize * __an) / __chunksize;
        }

        TakeRange<_T> front() const { return take(__range, __chunksize); }
    };

    template<typename _T>
    ChunksRange<_T> chunks(const _T &__it, RangeSize<_T> __chs) {
        return ChunksRange<_T>(__it, __chs);
    }

    template<typename _T>
    auto each(_T &__r) -> decltype(__r.each()) {
        return __r.each();
    }

    template<typename _T>
    auto each(const _T &__r) -> decltype(__r.each()) {
        return __r.each();
    }

    template<typename _T, size_t _N>
    PointerRange<_T> each(_T (&__array)[_N]) {
        return PointerRange<_T>(__array, _N);
    }

    // range of
    template<typename _T> using RangeOf = decltype(octa::each(octa::declval<_T>()));

    template<typename _T>
    struct HalfRange: InputRange<HalfRange<_T>,
        RangeCategory<_T>, RangeValue<_T>, RangeReference<_T>, RangeSize<_T>,
        RangeDifference<_T>
    > {
    private:
        _T __beg;
        _T __end;
    public:
        HalfRange(): __beg(), __end() {}
        HalfRange(const HalfRange &__range): __beg(__range.__beg),
            __end(__range.__end) {}
        HalfRange(HalfRange &&__range): __beg(octa::move(__range.__beg)),
            __end(octa::move(__range.__end)) {}
        HalfRange(const _T &__beg, const _T &__end): __beg(__beg),
            __end(__end) {}
        HalfRange(_T &&__beg, _T &&__end): __beg(octa::move(__beg)),
            __end(octa::move(__end)) {}

        HalfRange &operator=(const HalfRange &__range) {
            __beg = __range.p_beg;
            __end = __range.p_end;
            return *this;
        }

        HalfRange &operator=(HalfRange &&__range) {
            __beg = octa::move(__range.p_beg);
            __end = octa::move(__range.p_end);
            return *this;
        }

        bool empty() const { return __beg == __end; }

        bool pop_front() {
            if (empty()) return false;
            return __beg.next();
        }
        bool push_front() {
            return __beg.prev();
        }
        bool pop_back() {
            if (empty()) return false;
            return __end.prev();
        }
        bool push_back() {
            return __end.next();
        }

        RangeReference<_T> front() const { return *__beg; }
        RangeReference<_T> back() const { return *(__end - 1); }

        bool equals_front(const HalfRange &__range) const {
            return __beg == __range.__beg;
        }
        bool equals_back(const HalfRange &__range) const {
            return __end == __range.__end;
        }

        RangeDifference<_T> distance_front(const HalfRange &__range) const {
            return __range.__beg - __beg;
        }
        RangeDifference<_T> distance_back(const HalfRange &__range) const {
            return __range.__end - __end;
        }

        RangeSize<_T> size() const { return __end - __beg; }

        HalfRange<_T> slice(RangeSize<_T> __start, RangeSize<_T> __end) const {
            return HalfRange<_T>(__beg + __start, __beg + __end);
        }

        RangeReference<_T> operator[](RangeSize<_T> __idx) const {
            return __beg[__idx];
        }

        void put(const RangeValue<_T> &__v) {
            __beg.range().put(__v);
        }
        void put(RangeValue<_T> &&__v) {
            __beg.range().put(octa::move(__v));
        }
    };

    template<typename _T>
    HalfRange<RangeHalf<_T>>
    make_half_range(const RangeHalf<_T> &__a, const RangeHalf<_T> &__b) {
        return HalfRange<RangeHalf<_T>>(__a, __b);
    }
}

#endif