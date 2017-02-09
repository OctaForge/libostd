/* Ranges for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_RANGE_HH
#define OSTD_RANGE_HH

#include <stddef.h>
#include <string.h>

#include <new>
#include <tuple>
#include <utility>
#include <iterator>
#include <type_traits>

#include "ostd/types.hh"
#include "ostd/utility.hh"

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
        static char test(std::remove_reference_t<typename U::Name> *); \
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
        std::remove_reference_t<typename U::Reference> *
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
        std::is_convertible_v<RangeCategory<T>, InputRangeTag>;

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
        std::is_convertible_v<RangeCategory<T>, ForwardRangeTag>;

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
        std::is_convertible_v<RangeCategory<T>, BidirectionalRangeTag>;

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
        std::is_convertible_v<RangeCategory<T>, RandomAccessRangeTag>;

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
        std::is_convertible_v<RangeCategory<T>, FiniteRandomAccessRangeTag>;

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
        std::is_convertible_v<RangeCategory<T>, ContiguousRangeTag>;

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
        std::is_convertible_v<RangeCategory<T>, OutputRangeTag> || (
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
            ::new(&get_ref()) T(std::move(range));
        }
        RangeIterator(const RangeIterator &v): p_range(), p_init(true) {
            ::new(&get_ref()) T(v.get_ref());
        }
        RangeIterator(RangeIterator &&v): p_range(), p_init(true) {
            ::new(&get_ref()) T(std::move(v.get_ref()));
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
            using std::swap;
            swap(get_ref(). v.get_ref());
            swap(p_init, v.p_init);
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
        std::aligned_storage_t<sizeof(T), alignof(T)> p_range;
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

namespace detail {
    template<typename>
    struct RangeIteratorTag {
        /* better range types all become random access iterators */
        using Type = std::random_access_iterator_tag;
    };

    template<>
    struct RangeIteratorTag<InputRangeTag> {
        using Type = std::input_iterator_tag;
    };

    template<>
    struct RangeIteratorTag<OutputRangeTag> {
        using Type = std::output_iterator_tag;
    };

    template<>
    struct RangeIteratorTag<ForwardRangeTag> {
        using Type = std::forward_iterator_tag;
    };

    template<>
    struct RangeIteratorTag<BidirectionalRangeTag> {
        using Type = std::bidirectional_iterator_tag;
    };
}

template<typename T>
struct RangeHalf {
private:
    T p_range;
public:
    using Range = T;

    using iterator_category = typename detail::RangeIteratorTag<T>::Type;
    using value_type        = RangeValue<T>;
    using difference_type   = RangeDifference<T>;
    using pointer           = RangeValue<T> *;
    using reference         = RangeReference<T>;

    RangeHalf() = delete;
    RangeHalf(T const &range): p_range(range) {}

    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    RangeHalf(RangeHalf<U> const &half): p_range(half.p_range) {}

    RangeHalf(RangeHalf const &half): p_range(half.p_range) {}
    RangeHalf(RangeHalf &&half): p_range(std::move(half.p_range)) {}

    RangeHalf &operator=(RangeHalf const &half) {
        p_range = half.p_range;
        return *this;
    }

    RangeHalf &operator=(RangeHalf &&half) {
        p_range = std::move(half.p_range);
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
        return p_range.front();
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
inline RangeDifference<R> operator-(
    RangeHalf<R> const &lhs, RangeHalf<R> const &rhs
) {
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
    typename S = size_t, typename D = ptrdiff_t
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
        return JoinRange<B, R1, RR...>(iter(), std::move(r1), std::move(rr)...);
    }

    template<typename R1, typename ...RR>
    ZipRange<B, R1, RR...> zip(R1 r1, RR ...rr) const {
        return ZipRange<B, R1, RR...>(iter(), std::move(r1), std::move(rr)...);
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
    std::enable_if_t<IsOutputRange<OR>, Size> copy(OR &&orange, Size n = -1) {
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

    Size copy(std::remove_cv_t<Value> *p, Size n = -1) {
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
        return std::forward<Reference>(static_cast<B const *>(this)->front());
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

template<typename R, typename F, typename = std::enable_if_t<IsInputRange<R>>>
inline auto operator|(R &&range, F &&func) {
    return func(std::forward<R>(range));
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
    template<typename T, typename ...R, size_t ...I>
    inline auto join_proxy(
        T &&obj, std::tuple<R &&...> &&tup, std::index_sequence<I...>
    ) {
        return obj.join(std::forward<R>(
            std::get<I>(std::forward<std::tuple<R &&...>>(tup))
        )...);
    }

    template<typename T, typename ...R, size_t ...I>
    inline auto zip_proxy(
        T &&obj, std::tuple<R &&...> &&tup, std::index_sequence<I...>
    ) {
        return obj.zip(std::forward<R>(
            std::get<I>(std::forward<std::tuple<R &&...>>(tup))
        )...);
    }
}

template<typename R>
inline auto join(R &&range) {
    return [range = std::forward<R>(range)](auto &&obj) mutable {
        return obj.join(std::forward<R>(range));
    };
}

template<typename R1, typename ...R>
inline auto join(R1 &&r1, R &&...rr) {
    return [
        ranges = std::forward_as_tuple(
            std::forward<R1>(r1), std::forward<R>(rr)...
        )
    ] (auto &&obj) mutable {
        return detail::join_proxy(
            std::forward<decltype(obj)>(obj),
            std::forward<decltype(ranges)>(ranges),
            std::make_index_sequence<sizeof...(R) + 1>()
        );
    };
}

template<typename R>
inline auto zip(R &&range) {
    return [range = std::forward<R>(range)](auto &&obj) mutable {
        return obj.zip(std::forward<R>(range));
    };
}

template<typename R1, typename ...R>
inline auto zip(R1 &&r1, R &&...rr) {
    return [
        ranges = std::forward_as_tuple(
            std::forward<R1>(r1), std::forward<R>(rr)...
        )
    ] (auto &&obj) mutable {
        return detail::zip_proxy(
            std::forward<decltype(obj)>(obj),
            std::forward<decltype(ranges)>(ranges),
            std::make_index_sequence<sizeof...(R) + 1>()
        );
    };
}

template<typename C, typename = void>
struct ranged_traits;

namespace detail {
    template<typename C>
    static std::true_type test_direct_iter(decltype(std::declval<C>().iter()) *);

    template<typename>
    static std::false_type test_direct_iter(...);

    template<typename C>
    constexpr bool direct_iter_test = decltype(test_direct_iter<C>(0))::value;
}

template<typename C>
struct ranged_traits<C, std::enable_if_t<detail::direct_iter_test<C>>> {
    static auto iter(C &r) -> decltype(r.iter()) {
        return r.iter();
    }
};

template<typename T>
inline auto iter(T &r) -> decltype(ranged_traits<T>::iter(r)) {
    return ranged_traits<T>::iter(r);
}

template<typename T>
inline auto iter(T const &r) -> decltype(ranged_traits<T const>::iter(r)) {
    return ranged_traits<T const>::iter(r);
}

template<typename T>
inline auto citer(T const &r) -> decltype(ranged_traits<T const>::iter(r)) {
    return ranged_traits<T const>::iter(r);
}

template<
    typename B, typename V, typename R = V &,
    typename S = size_t, typename D = ptrdiff_t
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
        p_beg(std::move(range.p_beg)), p_end(std::move(range.p_end))
    {}
    HalfRange(T const &beg, T const &end):
        p_beg(beg),p_end(end)
    {}
    HalfRange(T &&beg, T &&end):
        p_beg(std::move(beg)), p_end(std::move(end))
    {}

    HalfRange &operator=(HalfRange const &range) {
        p_beg = range.p_beg;
        p_end = range.p_end;
        return *this;
    }

    HalfRange &operator=(HalfRange &&range) {
        p_beg = std::move(range.p_beg);
        p_end = std::move(range.p_end);
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
        return p_beg.range().put(std::move(v));
    }

    RangeValue<Rtype> *data() { return p_beg.data(); }
    RangeValue<Rtype> const *data() const { return p_beg.data(); }
};

template<typename T>
struct ReverseRange: InputRange<ReverseRange<T>,
    std::common_type_t<RangeCategory<T>, FiniteRandomAccessRangeTag>,
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
    ReverseRange(ReverseRange &&it): p_range(std::move(it.p_range)) {}

    ReverseRange &operator=(ReverseRange const &v) {
        p_range = v.p_range;
        return *this;
    }
    ReverseRange &operator=(ReverseRange &&v) {
        p_range = std::move(v.p_range);
        return *this;
    }
    ReverseRange &operator=(T const &v) {
        p_range = v;
        return *this;
    }
    ReverseRange &operator=(T &&v) {
        p_range = std::move(v);
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
    std::common_type_t<RangeCategory<T>, FiniteRandomAccessRangeTag>,
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
    MoveRange(MoveRange &&it): p_range(std::move(it.p_range)) {}

    MoveRange &operator=(MoveRange const &v) {
        p_range = v.p_range;
        return *this;
    }
    MoveRange &operator=(MoveRange &&v) {
        p_range = std::move(v.p_range);
        return *this;
    }
    MoveRange &operator=(T const &v) {
        p_range = v;
        return *this;
    }
    MoveRange &operator=(T &&v) {
        p_range = std::move(v);
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

    Rref front() const { return std::move(p_range.front()); }
    Rref back() const { return std::move(p_range.back()); }

    Rref operator[](Rsize i) const { return std::move(p_range[i]); }

    MoveRange<T> slice(Rsize start, Rsize end) const {
        return MoveRange<T>(p_range.slice(start, end));
    }

    bool put(Rval const &v) { return p_range.put(v); }
    bool put(Rval &&v) { return p_range.put(std::move(v)); }
};

template<typename T>
struct NumberRange: InputRange<NumberRange<T>, ForwardRangeTag, T, T> {
    NumberRange() = delete;
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
    PointerRange(T *beg, U end, std::enable_if_t<
        (std::is_pointer_v<U> || std::is_null_pointer_v<U>) &&
         std::is_convertible_v<U, T *>, Nat
    > = Nat()): p_beg(beg), p_end(end) {}

    PointerRange(T *beg, size_t n): p_beg(beg), p_end(beg + n) {}

    template<typename U, typename = std::enable_if_t<
        std::is_convertible_v<U *, T *>
    >>
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

    T &front() const { return *p_beg; }

    bool equals_front(PointerRange const &range) const {
        return p_beg == range.p_beg;
    }

    ptrdiff_t distance_front(PointerRange const &range) const {
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

    T &back() const { return *(p_end - 1); }

    bool equals_back(PointerRange const &range) const {
        return p_end == range.p_end;
    }

    ptrdiff_t distance_back(PointerRange const &range) const {
        return range.p_end - p_end;
    }

    /* satisfy FiniteRandomAccessRange */
    size_t size() const { return p_end - p_beg; }

    PointerRange slice(size_t start, size_t end) const {
        return PointerRange(p_beg + start, p_beg + end);
    }

    T &operator[](size_t i) const { return p_beg[i]; }

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
        *(p_beg++) = std::move(v);
        return true;
    }

    size_t put_n(T const *p, size_t n) {
        size_t ret = size();
        if (n < ret) {
            ret = n;
        }
        if constexpr(std::is_pod_v<T>) {
            memcpy(p_beg, p, ret * sizeof(T));
            p_beg += ret;
            return ret;
        }
        for (size_t i = ret; i; --i) {
            *p_beg++ = *p++;
        }
        return ret;
    }

    template<typename R>
    std::enable_if_t<IsOutputRange<R>, size_t> copy(R &&orange, size_t n = -1) {
        size_t c = size();
        if (n < c) {
            c = n;
        }
        return orange.put_n(p_beg, c);
    }

    size_t copy(std::remove_cv_t<T> *p, size_t n = -1) {
        size_t c = size();
        if (n < c) {
            c = n;
        }
        if constexpr(std::is_pod_v<T>) {
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

template<typename T, size_t N>
struct ranged_traits<T[N]> {
    static PointerRange<T> iter(T (&array)[N]) {
        return PointerRange<T>(array, N);
    }
};

namespace detail {
    struct PtrNat {};
}

template<typename T, typename U>
inline PointerRange<T> iter(T *a, U b, std::enable_if_t<
    (std::is_pointer_v<U> || std::is_null_pointer_v<U>) &&
     std::is_convertible_v<U, T *>, detail::PtrNat
> = detail::PtrNat()) {
    return PointerRange<T>(a, b);
}

template<typename T>
inline PointerRange<T> iter(T *a, size_t b) {
    return PointerRange<T>(a, b);
}

template<typename T, typename S>
struct EnumeratedValue {
    S index;
    T value;
};

template<typename T>
struct EnumeratedRange: InputRange<EnumeratedRange<T>,
    std::common_type_t<RangeCategory<T>, ForwardRangeTag>, RangeValue<T>,
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
        p_range(std::move(it.p_range)), p_index(it.p_index)
    {}

    EnumeratedRange &operator=(EnumeratedRange const &v) {
        p_range = v.p_range;
        p_index = v.p_index;
        return *this;
    }
    EnumeratedRange &operator=(EnumeratedRange &&v) {
        p_range = std::move(v.p_range);
        p_index = v.p_index;
        return *this;
    }
    EnumeratedRange &operator=(T const &v) {
        p_range = v;
        p_index = 0;
        return *this;
    }
    EnumeratedRange &operator=(T &&v) {
        p_range = std::move(v);
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
    std::common_type_t<RangeCategory<T>, ForwardRangeTag>,
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
        p_range(std::move(it.p_range)), p_remaining(it.p_remaining)
    {}

    TakeRange &operator=(TakeRange const &v) {
        p_range = v.p_range; p_remaining = v.p_remaining; return *this;
    }
    TakeRange &operator=(TakeRange &&v) {
        p_range = std::move(v.p_range);
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
    std::common_type_t<RangeCategory<T>, ForwardRangeTag>,
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
        p_range(std::move(it.p_range)), p_chunksize(it.p_chunksize)
    {}

    ChunksRange &operator=(ChunksRange const &v) {
        p_range = v.p_range; p_chunksize = v.p_chunksize; return *this;
    }
    ChunksRange &operator=(ChunksRange &&v) {
        p_range = std::move(v.p_range);
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
    template<size_t I, size_t N>
    struct JoinRangeEmpty {
        template<typename T>
        static bool empty(T const &tup) {
            if (!std::get<I>(tup).empty()) {
                return false;
            }
            return JoinRangeEmpty<I + 1, N>::empty(tup);
        }
    };

    template<size_t N>
    struct JoinRangeEmpty<N, N> {
        template<typename T>
        static bool empty(T const &) {
            return true;
        }
    };

    template<size_t I, size_t N>
    struct TupleRangeEqual {
        template<typename T>
        static bool equal(T const &tup1, T const &tup2) {
            if (!std::get<I>(tup1).equals_front(std::get<I>(tup2))) {
                return false;
            }
            return TupleRangeEqual<I + 1, N>::equal(tup1, tup2);
        }
    };

    template<size_t N>
    struct TupleRangeEqual<N, N> {
        template<typename T>
        static bool equal(T const &, T const &) {
            return true;
        }
    };

    template<size_t I, size_t N>
    struct JoinRangePop {
        template<typename T>
        static bool pop(T &tup) {
            if (!std::get<I>(tup).empty()) {
                return std::get<I>(tup).pop_front();
            }
            return JoinRangePop<I + 1, N>::pop(tup);
        }
    };

    template<size_t N>
    struct JoinRangePop<N, N> {
        template<typename T>
        static bool pop(T &) {
            return false;
        }
    };

    template<size_t I, size_t N, typename T>
    struct JoinRangeFront {
        template<typename U>
        static T front(U const &tup) {
            if (!std::get<I>(tup).empty()) {
                return std::get<I>(tup).front();
            }
            return JoinRangeFront<I + 1, N, T>::front(tup);
        }
    };

    template<size_t N, typename T>
    struct JoinRangeFront<N, N, T> {
        template<typename U>
        static T front(U const &tup) {
            return std::get<0>(tup).front();
        }
    };
}

template<typename ...R>
struct JoinRange: InputRange<JoinRange<R...>,
    std::common_type_t<ForwardRangeTag, RangeCategory<R>...>,
    std::common_type_t<RangeValue<R>...>, std::common_type_t<RangeReference<R>...>,
    std::common_type_t<RangeSize<R>...>, std::common_type_t<RangeDifference<R>...>> {
private:
    std::tuple<R...> p_ranges;
public:
    JoinRange() = delete;
    JoinRange(R const &...ranges): p_ranges(ranges...) {}
    JoinRange(R &&...ranges): p_ranges(std::forward<R>(ranges)...) {}
    JoinRange(JoinRange const &v): p_ranges(v.p_ranges) {}
    JoinRange(JoinRange &&v): p_ranges(std::move(v.p_ranges)) {}

    JoinRange &operator=(JoinRange const &v) {
        p_ranges = v.p_ranges;
        return *this;
    }

    JoinRange &operator=(JoinRange &&v) {
        p_ranges = std::move(v.p_ranges);
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

    std::common_type_t<RangeReference<R>...> front() const {
        return detail::JoinRangeFront<
            0, sizeof...(R), std::common_type_t<RangeReference<R>...>
        >::front(p_ranges);
    }
};

namespace detail {
    template<typename ...T>
    struct ZipValueType {
        using Type = std::tuple<T...>;
    };

    template<typename T, typename U>
    struct ZipValueType<T, U> {
        using Type = std::pair<T, U>;
    };

    template<typename ...T>
    using ZipValue = typename detail::ZipValueType<T...>::Type;

    template<size_t I, size_t N>
    struct ZipRangeEmpty {
        template<typename T>
        static bool empty(T const &tup) {
            if (std::get<I>(tup).empty()) {
                return true;
            }
            return ZipRangeEmpty<I + 1, N>::empty(tup);
        }
    };

    template<size_t N>
    struct ZipRangeEmpty<N, N> {
        template<typename T>
        static bool empty(T const &) {
            return false;
        }
    };

    template<size_t I, size_t N>
    struct ZipRangePop {
        template<typename T>
        static bool pop(T &tup) {
            return (
                std::get<I>(tup).pop_front() && ZipRangePop<I + 1, N>::pop(tup)
            );
        }
    };

    template<size_t N>
    struct ZipRangePop<N, N> {
        template<typename T>
        static bool pop(T &) {
            return true;
        }
    };

    template<typename ...T>
    struct ZipRangeFront {
        template<typename U, size_t ...I>
        static ZipValue<T...> tup_get(U const &tup, std::index_sequence<I...>) {
            return ZipValue<T...>(std::get<I>(tup).front()...);
        }

        template<typename U>
        static ZipValue<T...> front(U const &tup) {
            return ZipRangeFront<T...>::tup_get(
                tup, std::make_index_sequence<sizeof...(T)>()
            );
        }
    };
}

template<typename ...R>
struct ZipRange: InputRange<ZipRange<R...>,
    std::common_type_t<ForwardRangeTag, RangeCategory<R>...>,
    detail::ZipValue<RangeValue<R>...>,
    detail::ZipValue<RangeReference<R>...>,
    std::common_type_t<RangeSize<R>...>,
    std::common_type_t<RangeDifference<R>...>> {
private:
    std::tuple<R...> p_ranges;
public:
    ZipRange() = delete;
    ZipRange(R const &...ranges): p_ranges(ranges...) {}
    ZipRange(R &&...ranges): p_ranges(std::forward<R>(ranges)...) {}
    ZipRange(ZipRange const &v): p_ranges(v.p_ranges) {}
    ZipRange(ZipRange &&v): p_ranges(std::move(v.p_ranges)) {}

    ZipRange &operator=(ZipRange const &v) {
        p_ranges = v.p_ranges;
        return *this;
    }

    ZipRange &operator=(ZipRange &&v) {
        p_ranges = std::move(v.p_ranges);
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
struct AppenderRange: OutputRange<AppenderRange<T>, typename T::value_type,
    typename T::reference, typename T::size_type, typename T::difference_type> {
    AppenderRange(): p_data() {}
    AppenderRange(T const &v): p_data(v) {}
    AppenderRange(T &&v): p_data(std::move(v)) {}
    AppenderRange(AppenderRange const &v): p_data(v.p_data) {}
    AppenderRange(AppenderRange &&v): p_data(std::move(v.p_data)) {}

    AppenderRange &operator=(AppenderRange const &v) {
        p_data = v.p_data;
        return *this;
    }

    AppenderRange &operator=(AppenderRange &&v) {
        p_data = std::move(v.p_data);
        return *this;
    }

    AppenderRange &operator=(T const &v) {
        p_data = v;
        return *this;
    }

    AppenderRange &operator=(T &&v) {
        p_data = std::move(v);
        return *this;
    }

    void clear() { p_data.clear(); }

    void reserve(typename T::size_type cap) { p_data.reserve(cap); }
    void resize(typename T::size_type len) { p_data.resize(len); }

    typename T::size_type size() const { return p_data.size(); }
    typename T::size_type capacity() const { return p_data.capacity(); }

    bool put(typename T::const_reference v) {
        p_data.push_back(v);
        return true;
    }

    bool put(typename T::value_type &&v) {
        p_data.push_back(std::move(v));
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
    return AppenderRange<T>(std::forward<T>(v));
}

namespace detail {
    template<typename>
    struct IteratorRangeTagBase {
        /* fallback, the most basic range */
        using Type = InputRangeTag;
    };

    template<>
    struct IteratorRangeTagBase<std::output_iterator_tag> {
        using Type = OutputRangeTag;
    };

    template<>
    struct IteratorRangeTagBase<std::forward_iterator_tag> {
        using Type = ForwardRangeTag;
    };

    template<>
    struct IteratorRangeTagBase<std::bidirectional_iterator_tag> {
        using Type = BidirectionalRangeTag;
    };

    template<>
    struct IteratorRangeTagBase<std::random_access_iterator_tag> {
        using Type = FiniteRandomAccessRangeTag;
    };
}

template<typename T>
using IteratorRangeTag = typename detail::IteratorRangeTagBase<T>::Type;

template<typename T>
struct IteratorRange: InputRange<
    IteratorRange<T>,
    IteratorRangeTag<typename std::iterator_traits<T>::iterator_category>,
    typename std::iterator_traits<T>::value_type,
    typename std::iterator_traits<T>::reference,
    std::make_unsigned_t<typename std::iterator_traits<T>::difference_type>,
    typename std::iterator_traits<T>::difference_type
> {
private:
    using RefT = typename std::iterator_traits<T>::reference;
    using DiffT = typename std::iterator_traits<T>::difference_type;

public:
    IteratorRange(T beg = T{}, T end = T{}): p_beg(beg), p_end(end) {}

    IteratorRange(IteratorRange const &v): p_beg(v.p_beg), p_end(v.p_end) {}
    IteratorRange(IteratorRange &&v):
        p_beg(std::move(v.p_beg)), p_end(std::move(v.p_end))
    {}

    IteratorRange &operator=(IteratorRange const &v) {
        p_beg = v.p_beg;
        p_end = v.p_end;
        return *this;
    }

    IteratorRange &operator=(IteratorRange &&v) {
        p_beg = std::move(v.p_beg);
        p_end = std::move(v.p_end);
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

    RefT front() const { return *p_beg; }

    bool equals_front(IteratorRange const &range) const {
        return p_beg == range.p_beg;
    }

    DiffT distance_front(IteratorRange const &range) const {
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

    RefT back() const { return *(p_end - 1); }

    bool equals_back(IteratorRange const &range) const {
        return p_end == range.p_end;
    }

    ptrdiff_t distance_back(IteratorRange const &range) const {
        return range.p_end - p_end;
    }

    /* satisfy FiniteRandomAccessRange */
    size_t size() const { return p_end - p_beg; }

    IteratorRange slice(size_t start, size_t end) const {
        return IteratorRange(p_beg + start, p_beg + end);
    }

    RefT operator[](size_t i) const { return p_beg[i]; }

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
        *(p_beg++) = std::move(v);
        return true;
    }

private:
    T p_beg, p_end;
};

template<typename T>
IteratorRange<T> make_range(T beg, T end) {
    return IteratorRange<T>{beg, end};
}

template<typename T>
IteratorRange<T> make_range(T beg, size_t n) {
    return IteratorRange<T>{beg, beg + n};
}

} /* namespace ostd */

#endif
