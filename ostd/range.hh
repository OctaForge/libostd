/* Ranges for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_RANGE_HH
#define OSTD_RANGE_HH

#include <stddef.h>
#include <string.h>

#include "ostd/new.hh"
#include "ostd/types.hh"
#include "ostd/utility.hh"
#include "ostd/type_traits.hh"
#include "ostd/tuple.hh"

namespace ostd {

struct InputRangeTag {};
struct OutputRangeTag {};
struct ForwardRangeTag: InputRangeTag {};
struct BidirectionalRangeTag: ForwardRangeTag {};
struct RandomAccessRangeTag: BidirectionalRangeTag {};
struct FiniteRandomAccessRangeTag: RandomAccessRangeTag {};
struct ContiguousRangeTag: FiniteRandomAccessRangeTag {};

template<typename T>
struct RangeHalf;

#define OSTD_RANGE_TRAIT(Name) \
namespace detail { \
    template<typename T> \
    struct Range##Name##Test { \
        template<typename U> \
        static char test(RemoveReference<typename U::Name> *); \
        template<typename U> \
        static int test(...); \
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char)); \
    }; \
    template<typename T, bool = Range##Name##Test<T>::value> \
    struct Range##Name##Base {}; \
    template<typename T> \
    struct Range##Name##Base<T, true> { \
        using Type = typename T::Name; \
    }; \
} \
template<typename T> \
using Range##Name = typename detail::Range##Name##Base<T>::Type;

OSTD_RANGE_TRAIT(Category)
OSTD_RANGE_TRAIT(Size)
OSTD_RANGE_TRAIT(Value)
OSTD_RANGE_TRAIT(Reference)
OSTD_RANGE_TRAIT(Difference)

#undef OSTD_RANGE_TRAIT

namespace detail {
    template<typename U>
    static char is_range_test(
        typename U::Category *, typename U::Size *,
        typename U::Difference *, typename U::Value *,
        RemoveReference<typename U::Reference> *
    );
    template<typename U>
    static int is_range_test(...);

    template<typename T> constexpr bool IsRangeTest =
        (sizeof(is_range_test<T>(0, 0, 0, 0, 0)) == sizeof(char));
}

// is input range

namespace detail {
    template<typename T>
    constexpr bool IsInputRangeCore =
        IsConvertible<RangeCategory<T>, InputRangeTag>;

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsInputRangeBase = false;

    template<typename T>
    constexpr bool IsInputRangeBase<T, true> = detail::IsInputRangeCore<T>;
}

template<typename T>
constexpr bool IsInputRange = detail::IsInputRangeBase<T>;

// is forward range

namespace detail {
    template<typename T>
    constexpr bool IsForwardRangeCore =
        IsConvertible<RangeCategory<T>, ForwardRangeTag>;

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsForwardRangeBase = false;

    template<typename T>
    constexpr bool IsForwardRangeBase<T, true> = detail::IsForwardRangeCore<T>;
}

template<typename T>
constexpr bool IsForwardRange = detail::IsForwardRangeBase<T>;

// is bidirectional range

namespace detail {
    template<typename T>
    constexpr bool IsBidirectionalRangeCore =
        IsConvertible<RangeCategory<T>, BidirectionalRangeTag>;

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsBidirectionalRangeBase = false;

    template<typename T>
    constexpr bool IsBidirectionalRangeBase<T, true> =
        detail::IsBidirectionalRangeCore<T>;
}

template<typename T> constexpr bool IsBidirectionalRange =
    detail::IsBidirectionalRangeBase<T>;

// is random access range

namespace detail {
    template<typename T>
    constexpr bool IsRandomAccessRangeCore =
        IsConvertible<RangeCategory<T>, RandomAccessRangeTag>;

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsRandomAccessRangeBase = false;

    template<typename T>
    constexpr bool IsRandomAccessRangeBase<T, true> =
        detail::IsRandomAccessRangeCore<T>;
}

template<typename T> constexpr bool IsRandomAccessRange =
    detail::IsRandomAccessRangeBase<T>;

// is finite random access range

namespace detail {
    template<typename T>
    constexpr bool IsFiniteRandomAccessRangeCore =
        IsConvertible<RangeCategory<T>, FiniteRandomAccessRangeTag>;

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsFiniteRandomAccessRangeBase = false;

    template<typename T>
    constexpr bool IsFiniteRandomAccessRangeBase<T, true> =
        detail::IsFiniteRandomAccessRangeCore<T>;
}

template<typename T> constexpr bool IsFiniteRandomAccessRange =
    detail::IsFiniteRandomAccessRangeBase<T>;

// is infinite random access range

template<typename T> constexpr bool IsInfiniteRandomAccessRange =
    IsRandomAccessRange<T> && !IsFiniteRandomAccessRange<T>;

// is contiguous range

namespace detail {
    template<typename T>
    constexpr bool IsContiguousRangeCore =
        IsConvertible<RangeCategory<T>, ContiguousRangeTag>;

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsContiguousRangeBase = false;

    template<typename T>
    constexpr bool IsContiguousRangeBase<T, true> =
        detail::IsContiguousRangeCore<T>;
}

template<typename T> constexpr bool IsContiguousRange =
    detail::IsContiguousRangeBase<T>;

// is output range

namespace detail {
    template<typename T, typename P>
    struct OutputRangeTest {
        template<typename U, bool (U::*)(P)>
        struct Test {};
        template<typename U>
        static char test(Test<U, &U::put> *);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char));
    };

    template<typename T>
    constexpr bool IsOutputRangeCore =
        IsConvertible<RangeCategory<T>, OutputRangeTag> || (
            IsInputRange<T> && (
                detail::OutputRangeTest<T, RangeValue<T> const &>::value ||
                detail::OutputRangeTest<T, RangeValue<T>      &&>::value ||
                detail::OutputRangeTest<T, RangeValue<T>        >::value
            )
        );

    template<typename T, bool = detail::IsRangeTest<T>>
    constexpr bool IsOutputRangeBase = false;

    template<typename T>
    constexpr bool IsOutputRangeBase<T, true> = detail::IsOutputRangeCore<T>;
}

template<typename T> constexpr bool IsOutputRange = detail::IsOutputRangeBase<T>;

namespace detail {
    // range iterator

    template<typename T>
    struct RangeIterator {
        RangeIterator(): p_range(), p_init(false) {}
        explicit RangeIterator(T const &range): p_range(), p_init(true) {
            ::new(&get_ref()) T(range);
        }
        explicit RangeIterator(T &&range): p_range(), p_init(true) {
            ::new(&get_ref()) T(move(range));
        }
        RangeIterator(const RangeIterator &v): p_range(), p_init(true) {
            ::new(&get_ref()) T(v.get_ref());
        }
        RangeIterator(RangeIterator &&v): p_range(), p_init(true) {
            ::new(&get_ref()) T(move(v.get_ref()));
        }
        RangeIterator &operator=(const RangeIterator &v) {
            destroy();
            ::new(&get_ref()) T(v.get_ref());
            p_init = true;
            return *this;
        }
        RangeIterator &operator=(RangeIterator &&v) {
            destroy();
            swap(v);
            return *this;
        }
        ~RangeIterator() {
            destroy();
        }
        RangeIterator &operator++() {
            get_ref().pop_front();
            return *this;
        }
        RangeReference<T> operator*() const {
            return get_ref().front();
        }
        bool operator!=(RangeIterator) const { return !get_ref().empty(); }
        void swap(RangeIterator &v) {
            detail::swap_adl(get_ref(). v.get_ref());
            detail::swap_adl(p_init, v.p_init);
        }
    private:
        T &get_ref() { return *reinterpret_cast<T *>(&p_range); }
        T const &get_ref() const { return *reinterpret_cast<T const *>(&p_range); }
        void destroy() {
            if (p_init) {
                get_ref().~T();
                p_init = false;
            }
        }
        AlignedStorage<sizeof(T), alignof(T)> p_range;
        bool p_init;
    };
}

// range half

template<typename T>
struct HalfRange;

namespace detail {
    template<typename R, bool = IsBidirectionalRange<typename R::Range>>
    struct RangeAdd;

    template<typename R>
    struct RangeAdd<R, true> {
        using Diff = RangeDifference<typename R::Range>;

        static Diff add_n(R &half, Diff n) {
            if (n < 0) {
                return -half.prev_n(n);
            }
            return half.next_n(n);
        }
        static Diff sub_n(R &half, Diff n) {
            if (n < 0) {
                return -half.next_n(n);
            }
            return half.prev_n(n);
        }
    };

    template<typename R>
    struct RangeAdd<R, false> {
        using Diff = RangeDifference<typename R::Range>;

        static Diff add_n(R &half, Diff n) {
            if (n < 0) {
                return 0;
            }
            return half.next_n(n);
        }
        static Diff sub_n(R &half, Diff n) {
            if (n < 0) {
                return 0;
            }
            return half.prev_n(n);
        }
    };
}

template<typename T>
struct RangeHalf {
private:
    T p_range;
public:
    using Range = T;

    RangeHalf() = delete;
    RangeHalf(T const &range): p_range(range) {}

    template<typename U, typename = EnableIf<IsConvertible<U, T>>>
    RangeHalf(RangeHalf<U> const &half): p_range(half.p_range) {}

    RangeHalf(RangeHalf const &half): p_range(half.p_range) {}
    RangeHalf(RangeHalf &&half): p_range(move(half.p_range)) {}

    RangeHalf &operator=(RangeHalf const &half) {
        p_range = half.p_range;
        return *this;
    }

    RangeHalf &operator=(RangeHalf &&half) {
        p_range = move(half.p_range);
        return *this;
    }

    bool next() { return p_range.pop_front(); }
    bool prev() { return p_range.push_front(); }

    RangeSize<T> next_n(RangeSize<T> n) {
        return p_range.pop_front_n(n);
    }
    RangeSize<T> prev_n(RangeSize<T> n) {
        return p_range.push_front_n(n);
    }

    RangeDifference<T> add_n(RangeDifference<T> n) {
        return detail::RangeAdd<RangeHalf<T>>::add_n(*this, n);
    }
    RangeDifference<T> sub_n(RangeDifference<T> n) {
        return detail::RangeAdd<RangeHalf<T>>::sub_n(*this, n);
    }

    RangeReference<T> get() const {
        return p_range.front();
    }

    RangeDifference<T> distance(RangeHalf const &half) const {
        return p_range.distance_front(half.p_range);
    }

    bool equals(RangeHalf const &half) const {
        return p_range.equals_front(half.p_range);
    }

    bool operator==(RangeHalf const &half) const {
        return equals(half);
    }
    bool operator!=(RangeHalf const &half) const {
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
        return tmp;
    }

    RangeHalf &operator--() {
        prev();
        return *this;
    }
    RangeHalf operator--(int) {
        RangeHalf tmp(*this);
        prev();
        return tmp;
    }

    RangeHalf operator+(RangeDifference<T> n) const {
        RangeHalf tmp(*this);
        tmp.add_n(n);
        return tmp;
    }
    RangeHalf operator-(RangeDifference<T> n) const {
        RangeHalf tmp(*this);
        tmp.sub_n(n);
        return tmp;
    }

    RangeHalf &operator+=(RangeDifference<T> n) {
        add_n(n);
        return *this;
    }
    RangeHalf &operator-=(RangeDifference<T> n) {
        sub_n(n);
        return *this;
    }

    T iter() const { return p_range; }

    HalfRange<RangeHalf> iter(RangeHalf const &other) const {
        return HalfRange<RangeHalf>(*this, other);
    }

    RangeValue<T> *data() { return p_range.data(); }
    RangeValue<T> const *data() const { return p_range.data(); }
};

template<typename R>
inline RangeDifference<R> operator-(R const &lhs, R const &rhs) {
    return rhs.distance(lhs);
}

namespace detail {
    template<typename R>
    RangeSize<R> pop_front_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i) {
            if (!range.pop_front()) {
                return i;
            }
        }
        return n;
    }

    template<typename R>
    RangeSize<R> pop_back_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i) {
            if (!range.pop_back()) {
                return i;
            }
        }
        return n;
    }

    template<typename R>
    RangeSize<R> push_front_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i) {
            if (!range.push_front()) {
                return i;
            }
        }
        return n;
    }

    template<typename R>
    RangeSize<R> push_back_n(R &range, RangeSize<R> n) {
        for (RangeSize<R> i = 0; i < n; ++i) {
            if (!range.push_back()) {
                return i;
            }
        }
        return n;
    }
}

template<typename>
struct ReverseRange;
template<typename>
struct MoveRange;
template<typename>
struct EnumeratedRange;
template<typename>
struct TakeRange;
template<typename>
struct ChunksRange;
template<typename ...>
struct JoinRange;
template<typename ...>
struct ZipRange;

template<
    typename B, typename C, typename V, typename R = V &,
    typename S = Size, typename D = Ptrdiff
>
struct InputRange {
    using Category = C;
    using Size = S;
    using Difference = D;
    using Value = V;
    using Reference = R;

    detail::RangeIterator<B> begin() const {
        return detail::RangeIterator<B>(*static_cast<B const *>(this));
    }
    detail::RangeIterator<B> end() const {
        return detail::RangeIterator<B>();
    }

    Size pop_front_n(Size n) {
        return detail::pop_front_n<B>(*static_cast<B *>(this), n);
    }

    Size pop_back_n(Size n) {
        return detail::pop_back_n<B>(*static_cast<B *>(this), n);
    }

    Size push_front_n(Size n) {
        return detail::push_front_n<B>(*static_cast<B *>(this), n);
    }

    Size push_back_n(Size n) {
        return detail::push_back_n<B>(*static_cast<B *>(this), n);
    }

    B iter() const {
        return B(*static_cast<B const *>(this));
    }

    ReverseRange<B> reverse() const {
        return ReverseRange<B>(iter());
    }

    MoveRange<B> movable() const {
        return MoveRange<B>(iter());
    }

    EnumeratedRange<B> enumerate() const {
        return EnumeratedRange<B>(iter());
    }

    TakeRange<B> take(Size n) const {
        return TakeRange<B>(iter(), n);
    }

    ChunksRange<B> chunks(Size n) const {
        return ChunksRange<B>(iter(), n);
    }

    template<typename R1, typename ...RR>
    JoinRange<B, R1, RR...> join(R1 r1, RR ...rr) const {
        return JoinRange<B, R1, RR...>(iter(), move(r1), move(rr)...);
    }

    template<typename R1, typename ...RR>
    ZipRange<B, R1, RR...> zip(R1 r1, RR ...rr) const {
        return ZipRange<B, R1, RR...>(iter(), move(r1), move(rr)...);
    }

    RangeHalf<B> half() const {
        return RangeHalf<B>(iter());
    }

    Size put_n(Value const *p, Size n) {
        B &r = *static_cast<B *>(this);
        Size on = n;
        for (; n && r.put(*p++); --n);
        return (on - n);
    }

    template<typename OR>
    EnableIf<IsOutputRange<OR>, Size> copy(OR &&orange, Size n = -1) {
        B r(*static_cast<B const *>(this));
        Size on = n;
        for (; n && !r.empty(); --n) {
            if (!orange.put(r.front())) {
                break;
            }
            r.pop_front();
        }
        return (on - n);
    }

    Size copy(RemoveCv<Value> *p, Size n = -1) {
        B r(*static_cast<B const *>(this));
        Size on = n;
        for (; n && !r.empty(); --n) {
            *p++ = r.front();
            r.pop_front();
        }
        return (on - n);
    }

    /* iterator like interface operating on the front part of the range
     * this is sometimes convenient as it can be used within expressions */

    Reference operator*() const {
        return static_cast<B const *>(this)->front();
    }

    B &operator++() {
        static_cast<B *>(this)->pop_front();
        return *static_cast<B *>(this);
    }
    B operator++(int) {
        B tmp(*static_cast<B const *>(this));
        static_cast<B *>(this)->pop_front();
        return tmp;
    }

    B &operator--() {
        static_cast<B *>(this)->push_front();
        return *static_cast<B *>(this);
    }
    B operator--(int) {
        B tmp(*static_cast<B const *>(this));
        static_cast<B *>(this)->push_front();
        return tmp;
    }

    B operator+(Difference n) const {
        B tmp(*static_cast<B const *>(this));
        tmp.pop_front_n(n);
        return tmp;
    }
    B operator-(Difference n) const {
        B tmp(*static_cast<B const *>(this));
        tmp.push_front_n(n);
        return tmp;
    }

    B &operator+=(Difference n) {
        static_cast<B *>(this)->pop_front_n(n);
        return *static_cast<B *>(this);
    }
    B &operator-=(Difference n) {
        static_cast<B *>(this)->push_front_n(n);
        return *static_cast<B *>(this);
    }

    /* universal bool operator */

    explicit operator bool() const {
        return !(static_cast<B const *>(this)->empty());
    }
};

template<typename R, typename F, typename = EnableIf<IsInputRange<R>>>
inline auto operator|(R &&range, F &&func) {
    return func(forward<R>(range));
}

inline auto reverse() {
    return [](auto &&obj) { return obj.reverse(); };
}

inline auto movable() {
    return [](auto &&obj) { return obj.movable(); };
}

inline auto enumerate() {
    return [](auto &&obj) { return obj.enumerate(); };
}

template<typename T>
inline auto take(T n) {
    return [n](auto &&obj) { return obj.take(n); };
}

template<typename T>
inline auto chunks(T n) {
    return [n](auto &&obj) { return obj.chunks(n); };
}

namespace detail {
    template<typename T, typename ...R, Size ...I>
    inline auto join_proxy(T &&obj, Tuple<R &&...> &&tup, TupleIndices<I...>) {
        return obj.join(forward<R>(get<I>(forward<Tuple<R &&...>>(tup)))...);
    }

    template<typename T, typename ...R, Size ...I>
    inline auto zip_proxy(T &&obj, Tuple<R &&...> &&tup, TupleIndices<I...>) {
        return obj.zip(forward<R>(get<I>(forward<Tuple<R &&...>>(tup)))...);
    }
}

template<typename R>
inline auto join(R &&range) {
    return [range = forward<R>(range)](auto &&obj) mutable {
        return obj.join(forward<R>(range));
    };
}

template<typename R1, typename ...R>
inline auto join(R1 &&r1, R &&...rr) {
    return [
        ranges = forward_as_tuple(forward<R1>(r1), forward<R>(rr)...)
    ] (auto &&obj) mutable {
        using Index = detail::MakeTupleIndices<sizeof...(R) + 1>;
        return detail::join_proxy(
            forward<decltype(obj)>(obj),
            forward<decltype(ranges)>(ranges), Index()
        );
    };
}

template<typename R>
inline auto zip(R &&range) {
    return [range = forward<R>(range)](auto &&obj) mutable {
        return obj.zip(forward<R>(range));
    };
}

template<typename R1, typename ...R>
inline auto zip(R1 &&r1, R &&...rr) {
    return [
        ranges = forward_as_tuple(forward<R1>(r1), forward<R>(rr)...)
    ] (auto &&obj) mutable {
        using Index = detail::MakeTupleIndices<sizeof...(R) + 1>;
        return detail::zip_proxy(
            forward<decltype(obj)>(obj),
            forward<decltype(ranges)>(ranges), Index()
        );
    };
}

template<typename T>
inline auto iter(T &r) -> decltype(r.iter()) {
    return r.iter();
}

template<typename T>
inline auto iter(T const &r) -> decltype(r.iter()) {
    return r.iter();
}

template<typename T>
inline auto citer(T const &r) -> decltype(r.iter()) {
    return r.iter();
}

template<
    typename B, typename V, typename R = V &,
    typename S = Size, typename D = Ptrdiff
>
struct OutputRange {
    using Category = OutputRangeTag;
    using Size = S;
    using Difference = D;
    using Value = V;
    using Reference = R;

    Size put_n(Value const *p, Size n) {
        B &r = *static_cast<B *>(this);
        Size on = n;
        for (; n && r.put(*p++); --n);
        return (on - n);
    }
};

template<typename T>
struct HalfRange: InputRange<HalfRange<T>,
    RangeCategory<typename T::Range>,
    RangeValue<typename T::Range>,
    RangeReference<typename T::Range>,
    RangeSize<typename T::Range>,
    RangeDifference<typename T::Range>
> {
private:
    using Rtype = typename T::Range;
    T p_beg;
    T p_end;
public:
    HalfRange() = delete;
    HalfRange(HalfRange const &range):
        p_beg(range.p_beg), p_end(range.p_end)
    {}
    HalfRange(HalfRange &&range):
        p_beg(move(range.p_beg)), p_end(move(range.p_end))
    {}
    HalfRange(T const &beg, T const &end):
        p_beg(beg),p_end(end)
    {}
    HalfRange(T &&beg, T &&end):
        p_beg(move(beg)), p_end(move(end))
    {}

    HalfRange &operator=(HalfRange const &range) {
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

    bool pop_front() {
        if (empty()) {
            return false;
        }
        return p_beg.next();
    }
    bool push_front() {
        return p_beg.prev();
    }
    bool pop_back() {
        if (empty()) {
            return false;
        }
        return p_end.prev();
    }
    bool push_back() {
        return p_end.next();
    }

    RangeReference<Rtype> front() const { return *p_beg; }
    RangeReference<Rtype> back() const { return *(p_end - 1); }

    bool equals_front(HalfRange const &range) const {
        return p_beg == range.p_beg;
    }
    bool equals_back(HalfRange const &range) const {
        return p_end == range.p_end;
    }

    RangeDifference<Rtype> distance_front(HalfRange const &range) const {
        return range.p_beg - p_beg;
    }
    RangeDifference<Rtype> distance_back(HalfRange const &range) const {
        return range.p_end - p_end;
    }

    RangeSize<Rtype> size() const { return p_end - p_beg; }

    HalfRange<Rtype> slice(RangeSize<Rtype> start, RangeSize<Rtype> end) const {
        return HalfRange<Rtype>(p_beg + start, p_beg + end);
    }

    RangeReference<Rtype> operator[](RangeSize<Rtype> idx) const {
        return p_beg[idx];
    }

    bool put(RangeValue<Rtype> const &v) {
        return p_beg.range().put(v);
    }
    bool put(RangeValue<Rtype> &&v) {
        return p_beg.range().put(move(v));
    }

    RangeValue<Rtype> *data() { return p_beg.data(); }
    RangeValue<Rtype> const *data() const { return p_beg.data(); }
};

template<typename T>
struct ReverseRange: InputRange<ReverseRange<T>,
    CommonType<RangeCategory<T>, FiniteRandomAccessRangeTag>,
    RangeValue<T>, RangeReference<T>, RangeSize<T>, RangeDifference<T>
> {
private:
    using Rref = RangeReference<T>;
    using Rsize = RangeSize<T>;

    T p_range;

public:
    ReverseRange() = delete;
    ReverseRange(T const &range): p_range(range) {}
    ReverseRange(ReverseRange const &it): p_range(it.p_range) {}
    ReverseRange(ReverseRange &&it): p_range(move(it.p_range)) {}

    ReverseRange &operator=(ReverseRange const &v) {
        p_range = v.p_range;
        return *this;
    }
    ReverseRange &operator=(ReverseRange &&v) {
        p_range = move(v.p_range);
        return *this;
    }
    ReverseRange &operator=(T const &v) {
        p_range = v;
        return *this;
    }
    ReverseRange &operator=(T &&v) {
        p_range = move(v);
        return *this;
    }

    bool empty() const { return p_range.empty(); }
    Rsize size() const { return p_range.size(); }

    bool equals_front(ReverseRange const &r) const {
        return p_range.equals_back(r.p_range);
    }
    bool equals_back(ReverseRange const &r) const {
        return p_range.equals_front(r.p_range);
    }

    RangeDifference<T> distance_front(ReverseRange const &r) const {
        return -p_range.distance_back(r.p_range);
    }
    RangeDifference<T> distance_back(ReverseRange const &r) const {
        return -p_range.distance_front(r.p_range);
    }

    bool pop_front() { return p_range.pop_back(); }
    bool pop_back() { return p_range.pop_front(); }

    bool push_front() { return p_range.push_back(); }
    bool push_back() { return p_range.push_front(); }

    Rsize pop_front_n(Rsize n) { return p_range.pop_front_n(n); }
    Rsize pop_back_n(Rsize n) { return p_range.pop_back_n(n); }

    Rsize push_front_n(Rsize n) { return p_range.push_front_n(n); }
    Rsize push_back_n(Rsize n) { return p_range.push_back_n(n); }

    Rref front() const { return p_range.back(); }
    Rref back() const { return p_range.front(); }

    Rref operator[](Rsize i) const { return p_range[size() - i - 1]; }

    ReverseRange<T> slice(Rsize start, Rsize end) const {
        Rsize len = p_range.size();
        return ReverseRange<T>(p_range.slice(len - end, len - start));
    }
};

template<typename T>
struct MoveRange: InputRange<MoveRange<T>,
    CommonType<RangeCategory<T>, FiniteRandomAccessRangeTag>,
    RangeValue<T>, RangeValue<T> &&, RangeSize<T>, RangeDifference<T>
> {
private:
    using Rval = RangeValue<T>;
    using Rref = RangeValue<T> &&;
    using Rsize = RangeSize<T>;

    T p_range;

public:
    MoveRange() = delete;
    MoveRange(T const &range): p_range(range) {}
    MoveRange(MoveRange const &it): p_range(it.p_range) {}
    MoveRange(MoveRange &&it): p_range(move(it.p_range)) {}

    MoveRange &operator=(MoveRange const &v) {
        p_range = v.p_range;
        return *this;
    }
    MoveRange &operator=(MoveRange &&v) {
        p_range = move(v.p_range);
        return *this;
    }
    MoveRange &operator=(T const &v) {
        p_range = v;
        return *this;
    }
    MoveRange &operator=(T &&v) {
        p_range = move(v);
        return *this;
    }

    bool empty() const { return p_range.empty(); }
    Rsize size() const { return p_range.size(); }

    bool equals_front(MoveRange const &r) const {
        return p_range.equals_front(r.p_range);
    }
    bool equals_back(MoveRange const &r) const {
        return p_range.equals_back(r.p_range);
    }

    RangeDifference<T> distance_front(MoveRange const &r) const {
        return p_range.distance_front(r.p_range);
    }
    RangeDifference<T> distance_back(MoveRange const &r) const {
        return p_range.distance_back(r.p_range);
    }

    bool pop_front() { return p_range.pop_front(); }
    bool pop_back() { return p_range.pop_back(); }

    bool push_front() { return p_range.push_front(); }
    bool push_back() { return p_range.push_back(); }

    Rsize pop_front_n(Rsize n) { return p_range.pop_front_n(n); }
    Rsize pop_back_n(Rsize n) { return p_range.pop_back_n(n); }

    Rsize push_front_n(Rsize n) { return p_range.push_front_n(n); }
    Rsize push_back_n(Rsize n) { return p_range.push_back_n(n); }

    Rref front() const { return move(p_range.front()); }
    Rref back() const { return move(p_range.back()); }

    Rref operator[](Rsize i) const { return move(p_range[i]); }

    MoveRange<T> slice(Rsize start, Rsize end) const {
        return MoveRange<T>(p_range.slice(start, end));
    }

    bool put(Rval const &v) { return p_range.put(v); }
    bool put(Rval &&v) { return p_range.put(move(v)); }
};

template<typename T>
struct NumberRange: InputRange<NumberRange<T>, ForwardRangeTag, T, T> {
    NumberRange() = delete;
    NumberRange(NumberRange const &it):
        p_a(it.p_a), p_b(it.p_b), p_step(it.p_step)
    {}
    NumberRange(T a, T b, T step = T(1)):
        p_a(a), p_b(b), p_step(step)
    {}
    NumberRange(T v): p_a(0), p_b(v), p_step(1) {}

    bool empty() const { return p_a * p_step >= p_b * p_step; }

    bool equals_front(NumberRange const &range) const {
        return p_a == range.p_a;
    }

    bool pop_front() { p_a += p_step; return true; }
    T front() const { return p_a; }

private:
    T p_a, p_b, p_step;
};

template<typename T>
inline NumberRange<T> range(T a, T b, T step = T(1)) {
    return NumberRange<T>(a, b, step);
}

template<typename T>
inline NumberRange<T> range(T v) {
    return NumberRange<T>(v);
}

template<typename T>
struct PointerRange: InputRange<PointerRange<T>, ContiguousRangeTag, T> {
private:
    struct Nat {};

public:
    PointerRange(): p_beg(nullptr), p_end(nullptr) {}

    template<typename U>
    PointerRange(T *beg, U end, EnableIf<
        (IsPointer<U> || IsNullPointer<U>) && IsConvertible<U, T *>, Nat
    > = Nat()): p_beg(beg), p_end(end) {}

    PointerRange(T *beg, Size n): p_beg(beg), p_end(beg + n) {}

    template<typename U, typename = EnableIf<IsConvertible<U *, T *>>>
    PointerRange(PointerRange<U> const &v): p_beg(&v[0]), p_end(&v[v.size()]) {}

    PointerRange &operator=(PointerRange const &v) {
        p_beg = v.p_beg;
        p_end = v.p_end;
        return *this;
    }

    /* satisfy InputRange / ForwardRange */
    bool empty() const { return p_beg == p_end; }

    bool pop_front() {
        if (p_beg == p_end) {
            return false;
        }
        ++p_beg;
        return true;
    }
    bool push_front() {
        --p_beg; return true;
    }

    Size pop_front_n(Size n) {
        Size olen = p_end - p_beg;
        p_beg += n;
        if (p_beg > p_end) {
            p_beg = p_end;
            return olen;
        }
        return n;
    }

    Size push_front_n(Size n) {
        p_beg -= n; return true;
    }

    T &front() const { return *p_beg; }

    bool equals_front(PointerRange const &range) const {
        return p_beg == range.p_beg;
    }

    Ptrdiff distance_front(PointerRange const &range) const {
        return range.p_beg - p_beg;
    }

    /* satisfy BidirectionalRange */
    bool pop_back() {
        if (p_end == p_beg) {
            return false;
        }
        --p_end;
        return true;
    }
    bool push_back() {
        ++p_end; return true;
    }

    Size pop_back_n(Size n) {
        Size olen = p_end - p_beg;
        p_end -= n;
        if (p_end < p_beg) {
            p_end = p_beg;
            return olen;
        }
        return n;
    }

    Size push_back_n(Size n) {
        p_end += n; return true;
    }

    T &back() const { return *(p_end - 1); }

    bool equals_back(PointerRange const &range) const {
        return p_end == range.p_end;
    }

    Ptrdiff distance_back(PointerRange const &range) const {
        return range.p_end - p_end;
    }

    /* satisfy FiniteRandomAccessRange */
    Size size() const { return p_end - p_beg; }

    PointerRange slice(Size start, Size end) const {
        return PointerRange(p_beg + start, p_beg + end);
    }

    T &operator[](Size i) const { return p_beg[i]; }

    /* satisfy OutputRange */
    bool put(T const &v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = v;
        return true;
    }
    bool put(T &&v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = move(v);
        return true;
    }

    Size put_n(T const *p, Size n) {
        Size ret = size();
        if (n < ret) {
            ret = n;
        }
        if (IsPod<T>) {
            memcpy(p_beg, p, ret * sizeof(T));
            p_beg += ret;
            return ret;
        }
        for (Size i = ret; i; --i) {
            *p_beg++ = *p++;
        }
        return ret;
    }

    template<typename R>
    EnableIf<IsOutputRange<R>, Size> copy(R &&orange, Size n = -1) {
        Size c = size();
        if (n < c) {
            c = n;
        }
        return orange.put_n(p_beg, c);
    }

    Size copy(RemoveCv<T> *p, Size n = -1) {
        Size c = size();
        if (n < c) {
            c = n;
        }
        if (IsPod<T>) {
            memcpy(p_beg, data(), c * sizeof(T));
            return c;
        }
        return copy(PointerRange(p, c), c);
    }

    T *data() { return p_beg; }
    T const *data() const { return p_beg; }

private:
    T *p_beg, *p_end;
};

template<typename T, Size N>
inline PointerRange<T> iter(T (&array)[N]) {
    return PointerRange<T>(array, N);
}

template<typename T, Size N>
inline PointerRange<T const> iter(T const (&array)[N]) {
    return PointerRange<T const>(array, N);
}

namespace detail {
    struct PtrNat {};
}

template<typename T, typename U>
inline PointerRange<T> iter(T *a, U b, EnableIf<
    (IsPointer<U> || IsNullPointer<U>) && IsConvertible<U, T *>, detail::PtrNat
> = detail::PtrNat()) {
    return PointerRange<T>(a, b);
}

template<typename T>
inline PointerRange<T> iter(T *a, ostd::Size b) {
    return PointerRange<T>(a, b);
}

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
    using Rref = RangeReference<T>;
    using Rsize = RangeSize<T>;

    T p_range;
    Rsize p_index;

public:
    EnumeratedRange() = delete;

    EnumeratedRange(T const &range): p_range(range), p_index(0) {}

    EnumeratedRange(EnumeratedRange const &it):
        p_range(it.p_range), p_index(it.p_index)
    {}

    EnumeratedRange(EnumeratedRange &&it):
        p_range(move(it.p_range)), p_index(it.p_index)
    {}

    EnumeratedRange &operator=(EnumeratedRange const &v) {
        p_range = v.p_range;
        p_index = v.p_index;
        return *this;
    }
    EnumeratedRange &operator=(EnumeratedRange &&v) {
        p_range = move(v.p_range);
        p_index = v.p_index;
        return *this;
    }
    EnumeratedRange &operator=(T const &v) {
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

    bool equals_front(EnumeratedRange const &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() {
        if (p_range.pop_front()) {
            ++p_index;
            return true;
        }
        return false;
    }

    Rsize pop_front_n(Rsize n) {
        Rsize ret = p_range.pop_front_n(n);
        p_index += ret;
        return ret;
    }

    EnumeratedValue<Rref, Rsize> front() const {
        return EnumeratedValue<Rref, Rsize> { p_index, p_range.front() };
    }
};

template<typename T>
struct TakeRange: InputRange<TakeRange<T>,
    CommonType<RangeCategory<T>, ForwardRangeTag>,
    RangeValue<T>, RangeReference<T>, RangeSize<T>
> {
private:
    T p_range;
    RangeSize<T> p_remaining;
public:
    TakeRange() = delete;
    TakeRange(T const &range, RangeSize<T> rem):
        p_range(range), p_remaining(rem)
    {}
    TakeRange(TakeRange const &it):
        p_range(it.p_range), p_remaining(it.p_remaining)
    {}
    TakeRange(TakeRange &&it):
        p_range(move(it.p_range)), p_remaining(it.p_remaining)
    {}

    TakeRange &operator=(TakeRange const &v) {
        p_range = v.p_range; p_remaining = v.p_remaining; return *this;
    }
    TakeRange &operator=(TakeRange &&v) {
        p_range = move(v.p_range);
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

    RangeSize<T> pop_front_n(RangeSize<T> n) {
        RangeSize<T> ret = p_range.pop_front_n(n);
        p_remaining -= ret;
        return ret;
    }

    RangeReference<T> front() const { return p_range.front(); }

    bool equals_front(TakeRange const &r) const {
        return p_range.equals_front(r.p_range);
    }
};

template<typename T>
struct ChunksRange: InputRange<ChunksRange<T>,
    CommonType<RangeCategory<T>, ForwardRangeTag>,
    TakeRange<T>, TakeRange<T>, RangeSize<T>
> {
private:
    T p_range;
    RangeSize<T> p_chunksize;
public:
    ChunksRange() = delete;
    ChunksRange(T const &range, RangeSize<T> chs):
        p_range(range), p_chunksize(chs)
    {}
    ChunksRange(ChunksRange const &it):
        p_range(it.p_range), p_chunksize(it.p_chunksize)
    {}
    ChunksRange(ChunksRange &&it):
        p_range(move(it.p_range)), p_chunksize(it.p_chunksize)
    {}

    ChunksRange &operator=(ChunksRange const &v) {
        p_range = v.p_range; p_chunksize = v.p_chunksize; return *this;
    }
    ChunksRange &operator=(ChunksRange &&v) {
        p_range = move(v.p_range);
        p_chunksize = v.p_chunksize;
        return *this;
    }

    bool empty() const { return p_range.empty(); }

    bool equals_front(ChunksRange const &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() { return p_range.pop_front_n(p_chunksize) > 0; }
    RangeSize<T> pop_front_n(RangeSize<T> n) {
        return p_range.pop_front_n(p_chunksize * n) / p_chunksize;
    }

    TakeRange<T> front() const { return p_range.take(p_chunksize); }
};

namespace detail {
    template<Size I, Size N>
    struct JoinRangeEmpty {
        template<typename T>
        static bool empty(T const &tup) {
            if (!ostd::get<I>(tup).empty()) {
                return false;
            }
            return JoinRangeEmpty<I + 1, N>::empty(tup);
        }
    };

    template<Size N>
    struct JoinRangeEmpty<N, N> {
        template<typename T>
        static bool empty(T const &) {
            return true;
        }
    };

    template<Size I, Size N>
    struct TupleRangeEqual {
        template<typename T>
        static bool equal(T const &tup1, T const &tup2) {
            if (!ostd::get<I>(tup1).equals_front(ostd::get<I>(tup2))) {
                return false;
            }
            return TupleRangeEqual<I + 1, N>::equal(tup1, tup2);
        }
    };

    template<Size N>
    struct TupleRangeEqual<N, N> {
        template<typename T>
        static bool equal(T const &, T const &) {
            return true;
        }
    };

    template<Size I, Size N>
    struct JoinRangePop {
        template<typename T>
        static bool pop(T &tup) {
            if (!ostd::get<I>(tup).empty()) {
                return ostd::get<I>(tup).pop_front();
            }
            return JoinRangePop<I + 1, N>::pop(tup);
        }
    };

    template<Size N>
    struct JoinRangePop<N, N> {
        template<typename T>
        static bool pop(T &) {
            return false;
        }
    };

    template<Size I, Size N, typename T>
    struct JoinRangeFront {
        template<typename U>
        static T front(U const &tup) {
            if (!ostd::get<I>(tup).empty()) {
                return ostd::get<I>(tup).front();
            }
            return JoinRangeFront<I + 1, N, T>::front(tup);
        }
    };

    template<Size N, typename T>
    struct JoinRangeFront<N, N, T> {
        template<typename U>
        static T front(U const &tup) {
            return ostd::get<0>(tup).front();
        }
    };
}

template<typename ...R>
struct JoinRange: InputRange<JoinRange<R...>,
    CommonType<ForwardRangeTag, RangeCategory<R>...>,
    CommonType<RangeValue<R>...>, CommonType<RangeReference<R>...>,
    CommonType<RangeSize<R>...>, CommonType<RangeDifference<R>...>> {
private:
    Tuple<R...> p_ranges;
public:
    JoinRange() = delete;
    JoinRange(R const &...ranges): p_ranges(ranges...) {}
    JoinRange(R &&...ranges): p_ranges(forward<R>(ranges)...) {}
    JoinRange(JoinRange const &v): p_ranges(v.p_ranges) {}
    JoinRange(JoinRange &&v): p_ranges(move(v.p_ranges)) {}

    JoinRange &operator=(JoinRange const &v) {
        p_ranges = v.p_ranges;
        return *this;
    }

    JoinRange &operator=(JoinRange &&v) {
        p_ranges = move(v.p_ranges);
        return *this;
    }

    bool empty() const {
        return detail::JoinRangeEmpty<0, sizeof...(R)>::empty(p_ranges);
    }

    bool equals_front(JoinRange const &r) const {
        return detail::TupleRangeEqual<0, sizeof...(R)>::equal(
            p_ranges, r.p_ranges
        );
    }

    bool pop_front() {
        return detail::JoinRangePop<0, sizeof...(R)>::pop(p_ranges);
    }

    CommonType<RangeReference<R>...> front() const {
        return detail::JoinRangeFront<
            0, sizeof...(R), CommonType<RangeReference<R>...>
        >::front(p_ranges);
    }
};

namespace detail {
    template<typename ...T>
    struct ZipValueType {
        using Type = Tuple<T...>;
    };

    template<typename T, typename U>
    struct ZipValueType<T, U> {
        using Type = Pair<T, U>;
    };

    template<typename ...T>
    using ZipValue = typename detail::ZipValueType<T...>::Type;

    template<Size I, Size N>
    struct ZipRangeEmpty {
        template<typename T>
        static bool empty(T const &tup) {
            if (ostd::get<I>(tup).empty()) {
                return true;
            }
            return ZipRangeEmpty<I + 1, N>::empty(tup);
        }
    };

    template<Size N>
    struct ZipRangeEmpty<N, N> {
        template<typename T>
        static bool empty(T const &) {
            return false;
        }
    };

    template<Size I, Size N>
    struct ZipRangePop {
        template<typename T>
        static bool pop(T &tup) {
            return (
                ostd::get<I>(tup).pop_front() && ZipRangePop<I + 1, N>::pop(tup)
            );
        }
    };

    template<Size N>
    struct ZipRangePop<N, N> {
        template<typename T>
        static bool pop(T &) {
            return true;
        }
    };

    template<typename ...T>
    struct ZipRangeFront {
        template<typename U, Size ...I>
        static ZipValue<T...> tup_get(U const &tup, detail::TupleIndices<I...>) {
            return ZipValue<T...>(ostd::get<I>(tup).front()...);
        }

        template<typename U>
        static ZipValue<T...> front(U const &tup) {
            using Index = detail::MakeTupleIndices<sizeof...(T)>;
            return ZipRangeFront<T...>::tup_get(tup, Index());
        }
    };
}

template<typename ...R>
struct ZipRange: InputRange<ZipRange<R...>,
    CommonType<ForwardRangeTag, RangeCategory<R>...>,
    detail::ZipValue<RangeValue<R>...>,
    detail::ZipValue<RangeReference<R>...>,
    CommonType<RangeSize<R>...>, CommonType<RangeDifference<R>...>> {
private:
    Tuple<R...> p_ranges;
public:
    ZipRange() = delete;
    ZipRange(R const &...ranges): p_ranges(ranges...) {}
    ZipRange(R &&...ranges): p_ranges(forward<R>(ranges)...) {}
    ZipRange(ZipRange const &v): p_ranges(v.p_ranges) {}
    ZipRange(ZipRange &&v): p_ranges(move(v.p_ranges)) {}

    ZipRange &operator=(ZipRange const &v) {
        p_ranges = v.p_ranges;
        return *this;
    }

    ZipRange &operator=(ZipRange &&v) {
        p_ranges = move(v.p_ranges);
        return *this;
    }

    bool empty() const {
        return detail::ZipRangeEmpty<0, sizeof...(R)>::empty(p_ranges);
    }

    bool equals_front(ZipRange const &r) const {
        return detail::TupleRangeEqual<0, sizeof...(R)>::equal(
            p_ranges, r.p_ranges
        );
    }

    bool pop_front() {
        return detail::ZipRangePop<0, sizeof...(R)>::pop(p_ranges);
    }

    detail::ZipValue<RangeReference<R>...> front() const {
        return detail::ZipRangeFront<RangeReference<R>...>::front(p_ranges);
    }
};

template<typename T>
struct AppenderRange: OutputRange<AppenderRange<T>, typename T::Value,
    typename T::Reference, typename T::Size, typename T::Difference> {
    AppenderRange(): p_data() {}
    AppenderRange(T const &v): p_data(v) {}
    AppenderRange(T &&v): p_data(move(v)) {}
    AppenderRange(AppenderRange const &v): p_data(v.p_data) {}
    AppenderRange(AppenderRange &&v): p_data(move(v.p_data)) {}

    AppenderRange &operator=(AppenderRange const &v) {
        p_data = v.p_data;
        return *this;
    }

    AppenderRange &operator=(AppenderRange &&v) {
        p_data = move(v.p_data);
        return *this;
    }

    AppenderRange &operator=(T const &v) {
        p_data = v;
        return *this;
    }

    AppenderRange &operator=(T &&v) {
        p_data = move(v);
        return *this;
    }

    void clear() { p_data.clear(); }

    void reserve(typename T::Size cap) { p_data.reserve(cap); }
    void resize(typename T::Size len) { p_data.resize(len); }

    typename T::Size size() const { return p_data.size(); }
    typename T::Size capacity() const { return p_data.capacity(); }

    bool put(typename T::ConstReference v) {
        p_data.push(v);
        return true;
    }

    bool put(typename T::Value &&v) {
        p_data.push(move(v));
        return true;
    }

    T &get() { return p_data; }
private:
    T p_data;
};

template<typename T>
inline AppenderRange<T> appender() {
    return AppenderRange<T>();
}

template<typename T>
inline AppenderRange<T> appender(T &&v) {
    return AppenderRange<T>(forward<T>(v));
}

// range of
template<typename T> using RangeOf = decltype(iter(declval<T>()));

} /* namespace ostd */

#endif
