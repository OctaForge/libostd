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

#define OCTA_RANGE_TRAIT(_Name, _TypeName) \
namespace detail { \
    template<typename _T> \
    struct Range##_Name##Base { \
        typedef typename _T::_TypeName Type; \
    }; \
    template<typename _T> \
    struct Range##_Name##Base<RangeHalf<_T>> { \
        typedef typename _T::_TypeName Type; \
    }; \
} \
template<typename _T> \
using Range##_Name = typename octa::detail::Range##_Name##Base<_T>::Type;

OCTA_RANGE_TRAIT(Category, Category)
OCTA_RANGE_TRAIT(Size, Size)
OCTA_RANGE_TRAIT(Value, Value)
OCTA_RANGE_TRAIT(Reference, Reference)
OCTA_RANGE_TRAIT(Difference, Difference)

#undef OCTA_RANGE_TRAIT

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

namespace detail {
    template<typename _T, typename _P>
    struct OutputRangeTest {
        template<typename _U, void (_U::*)(_P)> struct Test {};
        template<typename _U> static char test(Test<_U, &_U::put> *);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == sizeof(char));
    };
}

template<typename _T, bool = (octa::IsConvertible<
    RangeCategory<_T>, OutputRangeTag
>::value || (IsInputRange<_T>::value &&
    (octa::detail::OutputRangeTest<_T, const RangeValue<_T>  &>::value ||
     octa::detail::OutputRangeTest<_T,       RangeValue<_T> &&>::value)
))> struct IsOutputRange: False {};

template<typename _T>
struct IsOutputRange<_T, true>: True {};

namespace detail {
    // range iterator

    template<typename _T>
    struct RangeIterator {
        RangeIterator(): p_range() {}
        explicit RangeIterator(const _T &range): p_range(range) {}
        RangeIterator &operator++() {
            p_range.pop_front();
            return *this;
        }
        RangeReference<_T> operator*() const {
            return p_range.front();
        }
        bool operator!=(RangeIterator) const { return !p_range.empty(); }
    private:
        _T p_range;
    };
}

// range half

template<typename _T>
struct RangeHalf {
private:
    _T p_range;
public:
    typedef _T Range;

    RangeHalf(): p_range() {}
    RangeHalf(const _T &range): p_range(range) {}
    RangeHalf(const RangeHalf &half): p_range(half.p_range) {}
    RangeHalf(RangeHalf &&half): p_range(octa::move(half.p_range)) {}

    RangeHalf &operator=(const RangeHalf &half) {
        p_range = half.p_range;
        return *this;
    }

    RangeHalf &operator=(RangeHalf &&half) {
        p_range = octa::move(half.p_range);
        return *this;
    }

    _T range() const { return p_range; }

    bool next() { return p_range.pop_front(); }
    bool prev() { return p_range.push_front(); }

    RangeSize<_T> next_n(RangeSize<_T> n) {
        return p_range.pop_front_n(n);
    }
    RangeSize<_T> prev_n(RangeSize<_T> n) {
        return p_range.push_front_n(n);
    }

    RangeReference<_T> get() const {
        return p_range.front();
    }

    RangeDifference<_T> distance(const RangeHalf &half) const {
        return p_range.distance_front(half.p_range);
    }

    bool equals(const RangeHalf &half) const {
        return p_range.equals_front(half.p_range);
    }

    bool operator==(const RangeHalf &half) const {
        return equals(half);
    }
    bool operator!=(const RangeHalf &half) const {
        return !equals(half);
    }

    /* iterator like interface */

    RangeReference<_T> operator*() const {
        return get();
    }

    RangeReference<_T> operator[](RangeSize<_T> idx) const {
        return p_range[idx];
    }

    RangeHalf &operator++() {
        next();
        return *this;
    }
    RangeHalf operator++(int) {
        RangeHalf tmp(*this);
        next();
        return octa::move(tmp);
    }

    RangeHalf &operator--() {
        prev();
        return *this;
    }
    RangeHalf operator--(int) {
        RangeHalf tmp(*this);
        prev();
        return octa::move(tmp);
    }

    RangeHalf operator+(RangeDifference<_T> n) {
        RangeHalf tmp(*this);
        if (n < 0) tmp.prev_n(-n);
        else tmp.next_n(n);
        return octa::move(tmp);
    }
    RangeHalf operator-(RangeDifference<_T> n) {
        RangeHalf tmp(*this);
        if (n < 0) tmp.next_n(-n);
        else tmp.prev_n(n);
        return octa::move(tmp);
    }

    RangeHalf &operator+=(RangeDifference<_T> n) {
        if (n < 0) prev_n(-n);
        else next_n(n);
        return *this;
    }
    RangeHalf &operator-=(RangeDifference<_T> n) {
        if (n < 0) next_n(-n);
        else prev_n(n);
        return *this;
    }
};

template<typename _R>
RangeDifference<_R> operator-(const _R &lhs, const _R &rhs) {
    return rhs.distance(lhs);
}

namespace detail {
    template<typename _R>
    RangeSize<_R> pop_front_n(_R &range, RangeSize<_R> n) {
        for (RangeSize<_R> i = 0; i < n; ++i)
            if (!range.pop_front()) return i;
        return n;
    }

    template<typename _R>
    RangeSize<_R> pop_back_n(_R &range, RangeSize<_R> n) {
        for (RangeSize<_R> i = 0; i < n; ++i)
            if (!range.pop_back()) return i;
        return n;
    }

    template<typename _R>
    RangeSize<_R> push_front_n(_R &range, RangeSize<_R> n) {
        for (RangeSize<_R> i = 0; i < n; ++i)
            if (!range.push_front()) return i;
        return n;
    }

    template<typename _R>
    RangeSize<_R> push_back_n(_R &range, RangeSize<_R> n) {
        for (RangeSize<_R> i = 0; i < n; ++i)
            if (!range.push_back()) return i;
        return n;
    }
}

template<typename _B, typename _C, typename _V, typename _R = _V &,
         typename _S = size_t, typename _D = ptrdiff_t
> struct InputRange {
    typedef _C Category;
    typedef _S Size;
    typedef _D Difference;
    typedef _V Value;
    typedef _R Reference;

    octa::detail::RangeIterator<_B> begin() const {
        return octa::detail::RangeIterator<_B>((const _B &)*this);
    }
    octa::detail::RangeIterator<_B> end() const {
        return octa::detail::RangeIterator<_B>();
    }

    Size pop_front_n(Size n) {
        return octa::detail::pop_front_n<_B>(*((_B *)this), n);
    }

    Size pop_back_n(Size n) {
        return octa::detail::pop_back_n<_B>(*((_B *)this), n);
    }

    Size push_front_n(Size n) {
        return octa::detail::push_front_n<_B>(*((_B *)this), n);
    }

    Size push_back_n(Size n) {
        return octa::detail::push_back_n<_B>(*((_B *)this), n);
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

    _T p_range;

public:
    ReverseRange(): p_range() {}

    ReverseRange(const _T &range): p_range(range) {}

    ReverseRange(const ReverseRange &it): p_range(it.p_range) {}

    ReverseRange(ReverseRange &&it): p_range(octa::move(it.p_range)) {}

    ReverseRange &operator=(const ReverseRange &v) {
        p_range = v.p_range;
        return *this;
    }
    ReverseRange &operator=(ReverseRange &&v) {
        p_range = octa::move(v.p_range);
        return *this;
    }
    ReverseRange &operator=(const _T &v) {
        p_range = v;
        return *this;
    }
    ReverseRange &operator=(_T &&v) {
        p_range = octa::move(v);
        return *this;
    }

    bool empty() const { return p_range.empty(); }
    _r_size size() const { return p_range.size(); }

    bool equals_front(const ReverseRange &r) const {
    return p_range.equals_back(r.p_range);
    }
    bool equals_back(const ReverseRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    RangeDifference<_T> distance_front(const ReverseRange &r) const {
        return -p_range.distance_back(r.p_range);
    }
    RangeDifference<_T> distance_back(const ReverseRange &r) const {
        return -p_range.distance_front(r.p_range);
    }

    bool pop_front() { return p_range.pop_back(); }
    bool pop_back() { return p_range.pop_front(); }

    bool push_front() { return p_range.push_back(); }
    bool push_back() { return p_range.push_front(); }

    _r_size pop_front_n(_r_size n) { return p_range.pop_front_n(n); }
    _r_size pop_back_n(_r_size n) { return p_range.pop_back_n(n); }

    _r_size push_front_n(_r_size n) { return p_range.push_front_n(n); }
    _r_size push_back_n(_r_size n) { return p_range.push_back_n(n); }

    _r_ref front() const { return p_range.back(); }
    _r_ref back() const { return p_range.front(); }

    _r_ref operator[](_r_size i) const { return p_range[size() - i - 1]; }

    ReverseRange<_T> slice(_r_size start, _r_size end) const {
        _r_size len = p_range.size();
        return ReverseRange<_T>(p_range.slice(len - end, len - start));
    }
};

template<typename _T>
ReverseRange<_T> make_reverse_range(const _T &it) {
    return ReverseRange<_T>(it);
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

    _T p_range;

public:
    MoveRange(): p_range() {}

    MoveRange(const _T &range): p_range(range) {}

    MoveRange(const MoveRange &it): p_range(it.p_range) {}

    MoveRange(MoveRange &&it): p_range(octa::move(it.p_range)) {}

    MoveRange &operator=(const MoveRange &v) {
        p_range = v.p_range;
        return *this;
    }
    MoveRange &operator=(MoveRange &&v) {
        p_range = octa::move(v.p_range);
        return *this;
    }
    MoveRange &operator=(const _T &v) {
        p_range = v;
        return *this;
    }
    MoveRange &operator=(_T &&v) {
        p_range = octa::move(v);
        return *this;
    }

    bool empty() const { return p_range.empty(); }
    _r_size size() const { return p_range.size(); }

    bool equals_front(const MoveRange &r) const {
        return p_range.equals_front(r.p_range);
    }
    bool equals_back(const MoveRange &r) const {
        return p_range.equals_back(r.p_range);
    }

    RangeDifference<_T> distance_front(const MoveRange &r) const {
        return p_range.distance_front(r.p_range);
    }
    RangeDifference<_T> distance_back(const MoveRange &r) const {
        return p_range.distance_back(r.p_range);
    }

    bool pop_front() { return p_range.pop_front(); }
    bool pop_back() { return p_range.pop_back(); }

    bool push_front() { return p_range.push_front(); }
    bool push_back() { return p_range.push_back(); }

    _r_size pop_front_n(_r_size n) { return p_range.pop_front_n(n); }
    _r_size pop_back_n(_r_size n) { return p_range.pop_back_n(n); }

    _r_size push_front_n(_r_size n) { return p_range.push_front_n(n); }
    _r_size push_back_n(_r_size n) { return p_range.push_back_n(n); }

    _r_ref front() const { return octa::move(p_range.front()); }
    _r_ref back() const { return octa::move(p_range.back()); }

    _r_ref operator[](_r_size i) const { return octa::move(p_range[i]); }

    MoveRange<_T> slice(_r_size start, _r_size end) const {
        return MoveRange<_T>(p_range.slice(start, end));
    }

    void put(const _r_val &v) { p_range.put(v); }
    void put(_r_val &&v) { p_range.put(octa::move(v)); }
};

template<typename _T>
MoveRange<_T> make_move_range(const _T &it) {
    return MoveRange<_T>(it);
}

template<typename _T>
struct NumberRange: InputRange<NumberRange<_T>, ForwardRangeTag, _T, _T> {
    NumberRange(): p_a(0), p_b(0), p_step(0) {}
    NumberRange(const NumberRange &it): p_a(it.p_a), p_b(it.p_b),
        p_step(it.p_step) {}
    NumberRange(_T a, _T b, _T step = _T(1)): p_a(a), p_b(b),
        p_step(step) {}
    NumberRange(_T v): p_a(0), p_b(v), p_step(1) {}

    bool empty() const { return p_a * p_step >= p_b * p_step; }

    bool equals_front(const NumberRange &range) const {
        return p_a == range.p_a;
    }

    bool pop_front() { p_a += p_step; return true; }
    bool push_front() { p_a -= p_step; return true; }
    _T front() const { return p_a; }

private:
    _T p_a, p_b, p_step;
};

template<typename _T>
NumberRange<_T> range(_T a, _T b, _T step = _T(1)) {
    return NumberRange<_T>(a, b, step);
}

template<typename _T>
NumberRange<_T> range(_T v) {
    return NumberRange<_T>(v);
}

template<typename _T>
struct PointerRange: InputRange<PointerRange<_T>, FiniteRandomAccessRangeTag, _T> {
    PointerRange(): p_beg(nullptr), p_end(nullptr) {}
    PointerRange(const PointerRange &v): p_beg(v.p_beg),
        p_end(v.p_end) {}
    PointerRange(_T *beg, _T *end): p_beg(beg), p_end(end) {}
    PointerRange(_T *beg, size_t n): p_beg(beg), p_end(beg + n) {}

    PointerRange &operator=(const PointerRange &v) {
        p_beg = v.p_beg;
        p_end = v.p_end;
        return *this;
    }

    /* satisfy InputRange / ForwardRange */
    bool empty() const { return p_beg == p_end; }

    bool pop_front() {
        if (p_beg == p_end) return false;
        ++p_beg;
        return true;
    }
    bool push_front() {
        --p_beg; return true;
    }

    size_t pop_front_n(size_t n) {
        size_t olen = p_end - p_beg;
        p_beg += n;
        if (p_beg > p_end) {
            p_beg = p_end;
            return olen;
        }
        return n;
    }

    size_t push_front_n(size_t n) {
        p_beg -= n; return true;
    }

    _T &front() const { return *p_beg; }

    bool equals_front(const PointerRange &range) const {
        return p_beg == range.p_beg;
    }

    ptrdiff_t distance_front(const PointerRange &range) const {
        return range.p_beg - p_beg;
    }

    /* satisfy BidirectionalRange */
    bool pop_back() {
        if (p_end == p_beg) return false;
        --p_end;
        return true;
    }
    bool push_back() {
        ++p_end; return true;
    }

    size_t pop_back_n(size_t n) {
        size_t olen = p_end - p_beg;
        p_end -= n;
        if (p_end < p_beg) {
            p_end = p_beg;
            return olen;
        }
        return n;
    }

    size_t push_back_n(size_t n) {
        p_end += n; return true;
    }

    _T &back() const { return *(p_end - 1); }

    bool equals_back(const PointerRange &range) const {
        return p_end == range.p_end;
    }

    ptrdiff_t distance_back(const PointerRange &range) const {
        return range.p_end - p_end;
    }

    /* satisfy FiniteRandomAccessRange */
    size_t size() const { return p_end - p_beg; }

    PointerRange slice(size_t start, size_t end) const {
        return PointerRange(p_beg + start, p_beg + end);
    }

    _T &operator[](size_t i) const { return p_beg[i]; }

    /* satisfy OutputRange */
    void put(const _T &v) {
        *(p_beg++) = v;
    }
    void put(_T &&v) {
        *(p_beg++) = octa::move(v);
    }

private:
    _T *p_beg, *p_end;
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

    _T p_range;
    _r_size p_index;

public:
    EnumeratedRange(): p_range(), p_index(0) {}

    EnumeratedRange(const _T &range): p_range(range), p_index(0) {}

    EnumeratedRange(const EnumeratedRange &it):
        p_range(it.p_range), p_index(it.p_index) {}

    EnumeratedRange(EnumeratedRange &&it):
        p_range(octa::move(it.p_range)), p_index(it.p_index) {}

    EnumeratedRange &operator=(const EnumeratedRange &v) {
        p_range = v.p_range;
        p_index = v.p_index;
        return *this;
    }
    EnumeratedRange &operator=(EnumeratedRange &&v) {
        p_range = octa::move(v.p_range);
        p_index = v.p_index;
        return *this;
    }
    EnumeratedRange &operator=(const _T &v) {
        p_range = v;
        p_index = 0;
        return *this;
    }
    EnumeratedRange &operator=(_T &&v) {
        p_range = octa::move(v);
        p_index = 0;
        return *this;
    }

    bool empty() const { return p_range.empty(); }

    bool equals_front(const EnumeratedRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() {
        if (p_range.pop_front()) {
            ++p_index;
            return true;
        }
        return false;
    }

    _r_size pop_front_n(_r_size n) {
        _r_size ret = p_range.pop_front_n(n);
        p_index += ret;
        return ret;
    }

    EnumeratedValue<_r_ref, _r_size> front() const {
        return EnumeratedValue<_r_ref, _r_size> { p_index, p_range.front() };
    }
};

template<typename _T>
EnumeratedRange<_T> enumerate(const _T &it) {
    return EnumeratedRange<_T>(it);
}

template<typename _T>
struct TakeRange: InputRange<TakeRange<_T>,
    CommonType<RangeCategory<_T>, ForwardRangeTag>,
    RangeValue<_T>, RangeReference<_T>, RangeSize<_T>
> {
private:
    _T p_range;
    RangeSize<_T> p_remaining;
public:
    TakeRange(): p_range(), p_remaining(0) {}
    TakeRange(const _T &range, RangeSize<_T> rem): p_range(range),
        p_remaining(rem) {}
    TakeRange(const TakeRange &it): p_range(it.p_range),
        p_remaining(it.p_remaining) {}
    TakeRange(TakeRange &&it): p_range(octa::move(it.p_range)),
        p_remaining(it.p_remaining) {}

    TakeRange &operator=(const TakeRange &v) {
        p_range = v.p_range; p_remaining = v.p_remaining; return *this;
    }
    TakeRange &operator=(TakeRange &&v) {
        p_range = octa::move(v.p_range);
        p_remaining = v.p_remaining;
        return *this;
    }

    bool empty() const { return (p_remaining <= 0) || p_range.empty(); }

    bool pop_front() {
        if (p_range.pop_front()) {
            --p_remaining;
            return true;
        }
        return false;
    }
    bool push_front() {
        if (p_range.push_front()) {
            ++p_remaining;
            return true;
        }
        return false;
    }

    RangeSize<_T> pop_front_n(RangeSize<_T> n) {
        RangeSize<_T> ret = p_range.pop_front_n(n);
        p_remaining -= ret;
        return ret;
    }
    RangeSize<_T> push_front_n(RangeSize<_T> n) {
        RangeSize<_T> ret = p_range.push_front_n(n);
        p_remaining += ret;
        return ret;
    }

    RangeReference<_T> front() const { return p_range.front(); }

    bool equals_front(const TakeRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    RangeDifference<_T> distance_front(const TakeRange &r) const {
        return p_range.distance_front(r.p_range);
    }
};

template<typename _T>
TakeRange<_T> take(const _T &it, RangeSize<_T> n) {
    return TakeRange<_T>(it, n);
}

template<typename _T>
struct ChunksRange: InputRange<ChunksRange<_T>,
    CommonType<RangeCategory<_T>, ForwardRangeTag>,
    TakeRange<_T>, TakeRange<_T>, RangeSize<_T>
> {
private:
    _T p_range;
    RangeSize<_T> p_chunksize;
public:
    ChunksRange(): p_range(), p_chunksize(0) {}
    ChunksRange(const _T &range, RangeSize<_T> chs): p_range(range),
        p_chunksize(chs) {}
    ChunksRange(const ChunksRange &it): p_range(it.p_range),
        p_chunksize(it.p_chunksize) {}
    ChunksRange(ChunksRange &&it): p_range(octa::move(it.p_range)),
        p_chunksize(it.p_chunksize) {}

    ChunksRange &operator=(const ChunksRange &v) {
        p_range = v.p_range; p_chunksize = v.p_chunksize; return *this;
    }
    ChunksRange &operator=(ChunksRange &&v) {
        p_range = octa::move(v.p_range);
        p_chunksize = v.p_chunksize;
        return *this;
    }

    bool empty() const { return p_range.empty(); }

    bool equals_front(const ChunksRange &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() { return p_range.pop_front_n(p_chunksize) > 0; }
    bool push_front() {
        _T tmp = p_range;
        RangeSize<_T> an = tmp.push_front_n(p_chunksize);
        if (an != p_chunksize) return false;
        p_range = tmp;
        return true;
    }
    RangeSize<_T> pop_front_n(RangeSize<_T> n) {
        return p_range.pop_front_n(p_chunksize * n) / p_chunksize;
    }
    RangeSize<_T> push_front_n(RangeSize<_T> n) {
        _T tmp = p_range;
        RangeSize<_T> an = tmp.push_front_n(p_chunksize * n);
        RangeSize<_T> pn = an / p_chunksize;
        if (!pn) return 0;
        if (pn == n) {
            p_range = tmp;
            return pn;
        }
        return p_range.push_front_n(p_chunksize * an) / p_chunksize;
    }

    TakeRange<_T> front() const { return take(p_range, p_chunksize); }
};

template<typename _T>
ChunksRange<_T> chunks(const _T &it, RangeSize<_T> chs) {
    return ChunksRange<_T>(it, chs);
}

template<typename _T>
auto each(_T &r) -> decltype(r.each()) {
    return r.each();
}

template<typename _T>
auto each(const _T &r) -> decltype(r.each()) {
    return r.each();
}

template<typename _T, size_t _N>
PointerRange<_T> each(_T (&array)[_N]) {
    return PointerRange<_T>(array, _N);
}

// range of
template<typename _T> using RangeOf = decltype(octa::each(octa::declval<_T>()));

template<typename _T>
struct HalfRange: InputRange<HalfRange<_T>,
    RangeCategory<_T>, RangeValue<_T>, RangeReference<_T>, RangeSize<_T>,
    RangeDifference<_T>
> {
private:
    _T p_beg;
    _T p_end;
public:
    HalfRange(): p_beg(), p_end() {}
    HalfRange(const HalfRange &range): p_beg(range.p_beg),
        p_end(range.p_end) {}
    HalfRange(HalfRange &&range): p_beg(octa::move(range.p_beg)),
        p_end(octa::move(range.p_end)) {}
    HalfRange(const _T &beg, const _T &end): p_beg(beg),
        p_end(end) {}
    HalfRange(_T &&beg, _T &&end): p_beg(octa::move(beg)),
        p_end(octa::move(end)) {}

    HalfRange &operator=(const HalfRange &range) {
        p_beg = range.p_beg;
        p_end = range.p_end;
        return *this;
    }

    HalfRange &operator=(HalfRange &&range) {
        p_beg = octa::move(range.p_beg);
        p_end = octa::move(range.p_end);
        return *this;
    }

    bool empty() const { return p_beg == p_end; }

    bool pop_front() {
        if (empty()) return false;
        return p_beg.next();
    }
    bool push_front() {
        return p_beg.prev();
    }
    bool pop_back() {
        if (empty()) return false;
        return p_end.prev();
    }
    bool push_back() {
        return p_end.next();
    }

    RangeReference<_T> front() const { return *p_beg; }
    RangeReference<_T> back() const { return *(p_end - 1); }

    bool equals_front(const HalfRange &range) const {
        return p_beg == range.p_beg;
    }
    bool equals_back(const HalfRange &range) const {
        return p_end == range.p_end;
    }

    RangeDifference<_T> distance_front(const HalfRange &range) const {
        return range.p_beg - p_beg;
    }
    RangeDifference<_T> distance_back(const HalfRange &range) const {
        return range.p_end - p_end;
    }

    RangeSize<_T> size() const { return p_end - p_beg; }

    HalfRange<_T> slice(RangeSize<_T> start, RangeSize<_T> p_end) const {
        return HalfRange<_T>(p_beg + start, p_beg + p_end);
    }

    RangeReference<_T> operator[](RangeSize<_T> idx) const {
        return p_beg[idx];
    }

    void put(const RangeValue<_T> &v) {
        p_beg.range().put(v);
    }
    void put(RangeValue<_T> &&v) {
        p_beg.range().put(octa::move(v));
    }
};

template<typename _T>
HalfRange<RangeHalf<_T>>
make_half_range(const RangeHalf<_T> &a, const RangeHalf<_T> &b) {
    return HalfRange<RangeHalf<_T>>(a, b);
}

} /* namespace octa */

#endif