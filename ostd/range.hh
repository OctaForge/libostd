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
#include <initializer_list>

#include "ostd/types.hh"

namespace ostd {

struct input_range_tag {};
struct output_range_tag {};
struct forward_range_tag: input_range_tag {};
struct bidirectional_range_tag: forward_range_tag {};
struct random_access_range_tag: bidirectional_range_tag {};
struct finite_random_access_range_tag: random_access_range_tag {};
struct contiguous_range_tag: finite_random_access_range_tag {};

template<typename T>
struct range_half;

#define OSTD_RANGE_TRAIT(Name, Member) \
namespace detail { \
    template<typename T> \
    struct range_##Name##_test { \
        template<typename U> \
        static char test(std::remove_reference_t<typename U::Member> *); \
        template<typename U> \
        static int test(...); \
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char)); \
    }; \
    template<typename T, bool = range_##Name##_test<T>::value> \
    struct range_##Name##_base {}; \
    template<typename T> \
    struct range_##Name##_base<T, true> { \
        using type = typename T::Member; \
    }; \
} \
template<typename T> \
using range_##Name##_t = typename detail::range_##Name##_base<T>::type;

OSTD_RANGE_TRAIT(category, range_category)
OSTD_RANGE_TRAIT(size, size_type)
OSTD_RANGE_TRAIT(value, value_type)
OSTD_RANGE_TRAIT(reference, reference)
OSTD_RANGE_TRAIT(difference, difference_type)

#undef OSTD_RANGE_TRAIT

namespace detail {
    template<typename U>
    static char is_range_test_f(
        typename U::range_category *, typename U::size_type *,
        typename U::difference_type *, typename U::value_type *,
        std::remove_reference_t<typename U::reference> *
    );
    template<typename U>
    static int is_range_test_f(...);

    template<typename T> constexpr bool is_range_test =
        (sizeof(is_range_test_f<T>(0, 0, 0, 0, 0)) == sizeof(char));
}

// is input range

namespace detail {
    template<typename T>
    constexpr bool is_input_range_core =
        std::is_convertible_v<range_category_t<T>, input_range_tag>;

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_input_range_base = false;

    template<typename T>
    constexpr bool is_input_range_base<T, true> = detail::is_input_range_core<T>;
}

template<typename T>
constexpr bool is_input_range = detail::is_input_range_base<T>;

// is forward range

namespace detail {
    template<typename T>
    constexpr bool is_forward_range_core =
        std::is_convertible_v<range_category_t<T>, forward_range_tag>;

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_forward_range_base = false;

    template<typename T>
    constexpr bool is_forward_range_base<T, true> = detail::is_forward_range_core<T>;
}

template<typename T>
constexpr bool is_forward_range = detail::is_forward_range_base<T>;

// is bidirectional range

namespace detail {
    template<typename T>
    constexpr bool is_bidirectional_range_core =
        std::is_convertible_v<range_category_t<T>, bidirectional_range_tag>;

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_bidirectional_range_base = false;

    template<typename T>
    constexpr bool is_bidirectional_range_base<T, true> =
        detail::is_bidirectional_range_core<T>;
}

template<typename T> constexpr bool is_bidirectional_range =
    detail::is_bidirectional_range_base<T>;

// is random access range

namespace detail {
    template<typename T>
    constexpr bool is_random_access_range_core =
        std::is_convertible_v<range_category_t<T>, random_access_range_tag>;

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_random_access_range_base = false;

    template<typename T>
    constexpr bool is_random_access_range_base<T, true> =
        detail::is_random_access_range_core<T>;
}

template<typename T> constexpr bool is_random_access_range =
    detail::is_random_access_range_base<T>;

// is finite random access range

namespace detail {
    template<typename T>
    constexpr bool is_finite_random_access_range_core =
        std::is_convertible_v<range_category_t<T>, finite_random_access_range_tag>;

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_finite_random_access_range_base = false;

    template<typename T>
    constexpr bool is_finite_random_access_range_base<T, true> =
        detail::is_finite_random_access_range_core<T>;
}

template<typename T> constexpr bool is_finite_random_access_range =
    detail::is_finite_random_access_range_base<T>;

// is infinite random access range

template<typename T> constexpr bool is_infinite_random_access_range =
    is_random_access_range<T> && !is_finite_random_access_range<T>;

// is contiguous range

namespace detail {
    template<typename T>
    constexpr bool is_contiguous_range_core =
        std::is_convertible_v<range_category_t<T>, contiguous_range_tag>;

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_contiguous_range_base = false;

    template<typename T>
    constexpr bool is_contiguous_range_base<T, true> =
        detail::is_contiguous_range_core<T>;
}

template<typename T> constexpr bool is_contiguous_range =
    detail::is_contiguous_range_base<T>;

// is output range

namespace detail {
    template<typename T, typename P>
    struct output_range_test {
        template<typename U, bool (U::*)(P)>
        struct test_t {};
        template<typename U>
        static char test(test_t<U, &U::put> *);
        template<typename U>
        static int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char));
    };

    template<typename T>
    constexpr bool is_output_range_core =
        std::is_convertible_v<range_category_t<T>, output_range_tag> || (
            is_input_range<T> && (
                detail::output_range_test<T, range_value_t<T> const &>::value ||
                detail::output_range_test<T, range_value_t<T>      &&>::value ||
                detail::output_range_test<T, range_value_t<T>        >::value
            )
        );

    template<typename T, bool = detail::is_range_test<T>>
    constexpr bool is_output_range_base = false;

    template<typename T>
    constexpr bool is_output_range_base<T, true> = detail::is_output_range_core<T>;
}

template<typename T>
constexpr bool is_output_range = detail::is_output_range_base<T>;

namespace detail {
    // range iterator

    template<typename T>
    struct range_iterator {
        range_iterator(): p_range(), p_init(false) {}
        explicit range_iterator(T const &range): p_range(), p_init(true) {
            ::new(&get_ref()) T(range);
        }
        explicit range_iterator(T &&range): p_range(), p_init(true) {
            ::new(&get_ref()) T(std::move(range));
        }
        range_iterator(const range_iterator &v): p_range(), p_init(true) {
            ::new(&get_ref()) T(v.get_ref());
        }
        range_iterator(range_iterator &&v): p_range(), p_init(true) {
            ::new(&get_ref()) T(std::move(v.get_ref()));
        }
        range_iterator &operator=(const range_iterator &v) {
            destroy();
            ::new(&get_ref()) T(v.get_ref());
            p_init = true;
            return *this;
        }
        range_iterator &operator=(range_iterator &&v) {
            destroy();
            swap(v);
            return *this;
        }
        ~range_iterator() {
            destroy();
        }
        range_iterator &operator++() {
            get_ref().pop_front();
            return *this;
        }
        range_reference_t<T> operator*() const {
            return get_ref().front();
        }
        bool operator!=(range_iterator) const { return !get_ref().empty(); }
        void swap(range_iterator &v) {
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
struct half_range;

namespace detail {
    template<typename R, bool = is_bidirectional_range<typename R::range>>
    struct range_add;

    template<typename R>
    struct range_add<R, true> {
        using diff_t = range_difference_t<typename R::range>;

        static diff_t add_n(R &half, diff_t n) {
            if (n < 0) {
                return -half.prev_n(n);
            }
            return half.next_n(n);
        }
        static diff_t sub_n(R &half, diff_t n) {
            if (n < 0) {
                return -half.next_n(n);
            }
            return half.prev_n(n);
        }
    };

    template<typename R>
    struct range_add<R, false> {
        using diff_t = range_difference_t<typename R::range>;

        static diff_t add_n(R &half, diff_t n) {
            if (n < 0) {
                return 0;
            }
            return half.next_n(n);
        }
        static diff_t sub_n(R &half, diff_t n) {
            if (n < 0) {
                return 0;
            }
            return half.prev_n(n);
        }
    };
}

namespace detail {
    template<typename>
    struct range_iterator_tag {
        /* better range types all become random access iterators */
        using type = std::random_access_iterator_tag;
    };

    template<>
    struct range_iterator_tag<input_range_tag> {
        using type = std::input_iterator_tag;
    };

    template<>
    struct range_iterator_tag<output_range_tag> {
        using type = std::output_iterator_tag;
    };

    template<>
    struct range_iterator_tag<forward_range_tag> {
        using type = std::forward_iterator_tag;
    };

    template<>
    struct range_iterator_tag<bidirectional_range_tag> {
        using type = std::bidirectional_iterator_tag;
    };
}

template<typename T>
struct range_half {
private:
    T p_range;
public:
    using range = T;

    using iterator_category = typename detail::range_iterator_tag<T>::type;
    using value_type        = range_value_t<T>;
    using difference_type   = range_difference_t<T>;
    using pointer           = range_value_t<T> *;
    using reference         = range_reference_t<T>;

    range_half() = delete;
    range_half(T const &range): p_range(range) {}

    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    range_half(range_half<U> const &half): p_range(half.p_range) {}

    range_half(range_half const &half): p_range(half.p_range) {}
    range_half(range_half &&half): p_range(std::move(half.p_range)) {}

    range_half &operator=(range_half const &half) {
        p_range = half.p_range;
        return *this;
    }

    range_half &operator=(range_half &&half) {
        p_range = std::move(half.p_range);
        return *this;
    }

    bool next() { return p_range.pop_front(); }
    bool prev() { return p_range.push_front(); }

    range_size_t<T> next_n(range_size_t<T> n) {
        return p_range.pop_front_n(n);
    }
    range_size_t<T> prev_n(range_size_t<T> n) {
        return p_range.push_front_n(n);
    }

    range_difference_t<T> add_n(range_difference_t<T> n) {
        return detail::range_add<range_half<T>>::add_n(*this, n);
    }
    range_difference_t<T> sub_n(range_difference_t<T> n) {
        return detail::range_add<range_half<T>>::sub_n(*this, n);
    }

    range_reference_t<T> get() const {
        return p_range.front();
    }

    range_difference_t<T> distance(range_half const &half) const {
        return p_range.distance_front(half.p_range);
    }

    bool equals(range_half const &half) const {
        return p_range.equals_front(half.p_range);
    }

    bool operator==(range_half const &half) const {
        return equals(half);
    }
    bool operator!=(range_half const &half) const {
        return !equals(half);
    }

    /* iterator like interface */

    range_reference_t<T> operator*() const {
        return p_range.front();
    }

    range_reference_t<T> operator[](range_size_t<T> idx) const {
        return p_range[idx];
    }

    range_half &operator++() {
        next();
        return *this;
    }
    range_half operator++(int) {
        range_half tmp(*this);
        next();
        return tmp;
    }

    range_half &operator--() {
        prev();
        return *this;
    }
    range_half operator--(int) {
        range_half tmp(*this);
        prev();
        return tmp;
    }

    range_half operator+(range_difference_t<T> n) const {
        range_half tmp(*this);
        tmp.add_n(n);
        return tmp;
    }
    range_half operator-(range_difference_t<T> n) const {
        range_half tmp(*this);
        tmp.sub_n(n);
        return tmp;
    }

    range_half &operator+=(range_difference_t<T> n) {
        add_n(n);
        return *this;
    }
    range_half &operator-=(range_difference_t<T> n) {
        sub_n(n);
        return *this;
    }

    T iter() const { return p_range; }

    half_range<range_half> iter(range_half const &other) const {
        return half_range<range_half>(*this, other);
    }

    range_value_t<T> *data() { return p_range.data(); }
    range_value_t<T> const *data() const { return p_range.data(); }
};

template<typename R>
inline range_difference_t<R> operator-(
    range_half<R> const &lhs, range_half<R> const &rhs
) {
    return rhs.distance(lhs);
}

namespace detail {
    template<typename R>
    range_size_t<R> pop_front_n(R &range, range_size_t<R> n) {
        for (range_size_t<R> i = 0; i < n; ++i) {
            if (!range.pop_front()) {
                return i;
            }
        }
        return n;
    }

    template<typename R>
    range_size_t<R> pop_back_n(R &range, range_size_t<R> n) {
        for (range_size_t<R> i = 0; i < n; ++i) {
            if (!range.pop_back()) {
                return i;
            }
        }
        return n;
    }

    template<typename R>
    range_size_t<R> push_front_n(R &range, range_size_t<R> n) {
        for (range_size_t<R> i = 0; i < n; ++i) {
            if (!range.push_front()) {
                return i;
            }
        }
        return n;
    }

    template<typename R>
    range_size_t<R> push_back_n(R &range, range_size_t<R> n) {
        for (range_size_t<R> i = 0; i < n; ++i) {
            if (!range.push_back()) {
                return i;
            }
        }
        return n;
    }
}

template<typename>
struct reverse_range;
template<typename>
struct move_range;
template<typename>
struct enumerated_range;
template<typename>
struct take_range;
template<typename>
struct chunks_range;
template<typename ...>
struct join_range;
template<typename ...>
struct zip_range;

template<typename B>
struct input_range {
    detail::range_iterator<B> begin() const {
        return detail::range_iterator<B>(*static_cast<B const *>(this));
    }
    detail::range_iterator<B> end() const {
        return detail::range_iterator<B>();
    }

    template<typename Size>
    Size pop_front_n(Size n) {
        return detail::pop_front_n<B>(*static_cast<B *>(this), n);
    }

    template<typename Size>
    Size pop_back_n(Size n) {
        return detail::pop_back_n<B>(*static_cast<B *>(this), n);
    }

    template<typename Size>
    Size push_front_n(Size n) {
        return detail::push_front_n<B>(*static_cast<B *>(this), n);
    }

    template<typename Size>
    Size push_back_n(Size n) {
        return detail::push_back_n<B>(*static_cast<B *>(this), n);
    }

    B iter() const {
        return B(*static_cast<B const *>(this));
    }

    reverse_range<B> reverse() const {
        return reverse_range<B>(iter());
    }

    move_range<B> movable() const {
        return move_range<B>(iter());
    }

    enumerated_range<B> enumerate() const {
        return enumerated_range<B>(iter());
    }

    template<typename Size>
    take_range<B> take(Size n) const {
        return take_range<B>(iter(), n);
    }

    template<typename Size>
    chunks_range<B> chunks(Size n) const {
        return chunks_range<B>(iter(), n);
    }

    template<typename R1, typename ...RR>
    join_range<B, R1, RR...> join(R1 r1, RR ...rr) const {
        return join_range<B, R1, RR...>(iter(), std::move(r1), std::move(rr)...);
    }

    template<typename R1, typename ...RR>
    zip_range<B, R1, RR...> zip(R1 r1, RR ...rr) const {
        return zip_range<B, R1, RR...>(iter(), std::move(r1), std::move(rr)...);
    }

    range_half<B> half() const {
        return range_half<B>(iter());
    }

    template<typename OR, typename Size>
    std::enable_if_t<is_output_range<OR>, Size> copy(OR &&orange, Size n = -1) {
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

    template<typename Value, typename Size>
    Size copy(Value *p, Size n = -1) {
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

    auto operator*() const {
        return std::forward<decltype(static_cast<B const *>(this)->front())>(
            static_cast<B const *>(this)->front()
        );
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

    template<typename Difference>
    B operator+(Difference n) const {
        B tmp(*static_cast<B const *>(this));
        tmp.pop_front_n(n);
        return tmp;
    }
    template<typename Difference>
    B operator-(Difference n) const {
        B tmp(*static_cast<B const *>(this));
        tmp.push_front_n(n);
        return tmp;
    }

    template<typename Difference>
    B &operator+=(Difference n) {
        static_cast<B *>(this)->pop_front_n(n);
        return *static_cast<B *>(this);
    }
    template<typename Difference>
    B &operator-=(Difference n) {
        static_cast<B *>(this)->push_front_n(n);
        return *static_cast<B *>(this);
    }

    /* pipe op, must be a member to work for user ranges automagically */

    template<typename F>
    auto operator|(F &&func) & {
        return func(*static_cast<B *>(this));
    }
    template<typename F>
    auto operator|(F &&func) const & {
        return func(*static_cast<B const *>(this));
    }
    template<typename F>
    auto operator|(F &&func) && {
        return func(std::move(*static_cast<B *>(this)));
    }
    template<typename F>
    auto operator|(F &&func) const && {
        return func(std::move(*static_cast<B const *>(this)));
    }

    /* universal bool operator */

    explicit operator bool() const {
        return !(static_cast<B const *>(this)->empty());
    }
};

template<typename B>
struct output_range {
    using range_category = output_range_tag;
};

template<typename R>
inline range_size_t<R> range_put_n(
    R &range, range_value_t<R> const *p, range_size_t<R> n
) {
    range_size_t<R> on = n;
    for (; n && range.put(*p++); --n);
    return (on - n);
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

namespace detail {
    template<typename C>
    static std::true_type test_direct_iter(decltype(std::declval<C &>().iter()) *);

    template<typename>
    static std::false_type test_direct_iter(...);

    template<typename C>
    constexpr bool direct_iter_test = decltype(test_direct_iter<C>(0))::value;

    template<typename C, typename = void>
    struct ranged_traits_core {};

    template<typename C>
    struct ranged_traits_core<C, std::enable_if_t<detail::direct_iter_test<C>>> {
        using range = decltype(std::declval<C &>().iter());

        static range iter(C &r) {
            return r.iter();
        }
    };
}

template<typename C>
struct ranged_traits: detail::ranged_traits_core<C> {};

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

template<typename T>
struct half_range: input_range<half_range<T>> {
    using range_category  = range_category_t  <typename T::range>;
    using value_type      = range_value_t     <typename T::range>;
    using reference       = range_reference_t <typename T::range>;
    using size_type       = range_size_t      <typename T::range>;
    using difference_type = range_difference_t<typename T::range>;

private:
    T p_beg;
    T p_end;

public:
    half_range() = delete;
    half_range(half_range const &range):
        p_beg(range.p_beg), p_end(range.p_end)
    {}
    half_range(half_range &&range):
        p_beg(std::move(range.p_beg)), p_end(std::move(range.p_end))
    {}
    half_range(T const &beg, T const &end):
        p_beg(beg),p_end(end)
    {}
    half_range(T &&beg, T &&end):
        p_beg(std::move(beg)), p_end(std::move(end))
    {}

    half_range &operator=(half_range const &range) {
        p_beg = range.p_beg;
        p_end = range.p_end;
        return *this;
    }

    half_range &operator=(half_range &&range) {
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

    reference front() const { return *p_beg; }
    reference back() const { return *(p_end - 1); }

    bool equals_front(half_range const &range) const {
        return p_beg == range.p_beg;
    }
    bool equals_back(half_range const &range) const {
        return p_end == range.p_end;
    }

    difference_type distance_front(half_range const &range) const {
        return range.p_beg - p_beg;
    }
    difference_type distance_back(half_range const &range) const {
        return range.p_end - p_end;
    }

    size_type size() const { return p_end - p_beg; }

    half_range slice(size_type start, size_type end) const {
        return half_range{p_beg + start, p_beg + end};
    }

    reference operator[](size_type idx) const {
        return p_beg[idx];
    }

    bool put(value_type const &v) {
        return p_beg.range().put(v);
    }
    bool put(value_type &&v) {
        return p_beg.range().put(std::move(v));
    }

    value_type *data() { return p_beg.data(); }
    value_type const *data() const { return p_beg.data(); }
};

template<typename T>
struct reverse_range: input_range<reverse_range<T>> {
    using range_category  = std::common_type_t<
        range_category_t<T>, finite_random_access_range_tag
    >;
    using value_type      = range_value_t     <T>;
    using reference       = range_reference_t <T>;
    using size_type       = range_size_t      <T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;

public:
    reverse_range() = delete;
    reverse_range(T const &range): p_range(range) {}
    reverse_range(reverse_range const &it): p_range(it.p_range) {}
    reverse_range(reverse_range &&it): p_range(std::move(it.p_range)) {}

    reverse_range &operator=(reverse_range const &v) {
        p_range = v.p_range;
        return *this;
    }
    reverse_range &operator=(reverse_range &&v) {
        p_range = std::move(v.p_range);
        return *this;
    }
    reverse_range &operator=(T const &v) {
        p_range = v;
        return *this;
    }
    reverse_range &operator=(T &&v) {
        p_range = std::move(v);
        return *this;
    }

    bool empty() const { return p_range.empty(); }
    size_type size() const { return p_range.size(); }

    bool equals_front(reverse_range const &r) const {
        return p_range.equals_back(r.p_range);
    }
    bool equals_back(reverse_range const &r) const {
        return p_range.equals_front(r.p_range);
    }

    difference_type distance_front(reverse_range const &r) const {
        return -p_range.distance_back(r.p_range);
    }
    difference_type distance_back(reverse_range const &r) const {
        return -p_range.distance_front(r.p_range);
    }

    bool pop_front() { return p_range.pop_back(); }
    bool pop_back() { return p_range.pop_front(); }

    bool push_front() { return p_range.push_back(); }
    bool push_back() { return p_range.push_front(); }

    size_type pop_front_n(size_type n) { return p_range.pop_front_n(n); }
    size_type pop_back_n(size_type n) { return p_range.pop_back_n(n); }

    size_type push_front_n(size_type n) { return p_range.push_front_n(n); }
    size_type push_back_n(size_type n) { return p_range.push_back_n(n); }

    reference front() const { return p_range.back(); }
    reference back() const { return p_range.front(); }

    reference operator[](size_type i) const { return p_range[size() - i - 1]; }

    reverse_range slice(size_type start, size_type end) const {
        size_type len = p_range.size();
        return reverse_range{p_range.slice(len - end, len - start)};
    }
};

template<typename T>
struct move_range: input_range<move_range<T>> {
    using range_category  = std::common_type_t<
        range_category_t<T>, finite_random_access_range_tag
    >;
    using value_type      = range_value_t     <T>;
    using reference       = range_value_t     <T> &&;
    using size_type       = range_size_t      <T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;

public:
    move_range() = delete;
    move_range(T const &range): p_range(range) {}
    move_range(move_range const &it): p_range(it.p_range) {}
    move_range(move_range &&it): p_range(std::move(it.p_range)) {}

    move_range &operator=(move_range const &v) {
        p_range = v.p_range;
        return *this;
    }
    move_range &operator=(move_range &&v) {
        p_range = std::move(v.p_range);
        return *this;
    }
    move_range &operator=(T const &v) {
        p_range = v;
        return *this;
    }
    move_range &operator=(T &&v) {
        p_range = std::move(v);
        return *this;
    }

    bool empty() const { return p_range.empty(); }
    size_type size() const { return p_range.size(); }

    bool equals_front(move_range const &r) const {
        return p_range.equals_front(r.p_range);
    }
    bool equals_back(move_range const &r) const {
        return p_range.equals_back(r.p_range);
    }

    difference_type distance_front(move_range const &r) const {
        return p_range.distance_front(r.p_range);
    }
    difference_type distance_back(move_range const &r) const {
        return p_range.distance_back(r.p_range);
    }

    bool pop_front() { return p_range.pop_front(); }
    bool pop_back() { return p_range.pop_back(); }

    bool push_front() { return p_range.push_front(); }
    bool push_back() { return p_range.push_back(); }

    size_type pop_front_n(size_type n) { return p_range.pop_front_n(n); }
    size_type pop_back_n(size_type n) { return p_range.pop_back_n(n); }

    size_type push_front_n(size_type n) { return p_range.push_front_n(n); }
    size_type push_back_n(size_type n) { return p_range.push_back_n(n); }

    reference front() const { return std::move(p_range.front()); }
    reference back() const { return std::move(p_range.back()); }

    reference operator[](size_type i) const { return std::move(p_range[i]); }

    move_range slice(size_type start, size_type end) const {
        return move_range{p_range.slice(start, end)};
    }

    bool put(value_type const &v) { return p_range.put(v); }
    bool put(value_type &&v) { return p_range.put(std::move(v)); }
};

template<typename T>
struct number_range: input_range<number_range<T>> {
    using range_category  = forward_range_tag;
    using value_type      = T;
    using reference       = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    number_range() = delete;
    number_range(T a, T b, T step = T(1)):
        p_a(a), p_b(b), p_step(step)
    {}
    number_range(T v): p_a(0), p_b(v), p_step(1) {}

    bool empty() const { return p_a * p_step >= p_b * p_step; }

    bool equals_front(number_range const &range) const {
        return p_a == range.p_a;
    }

    bool pop_front() { p_a += p_step; return true; }
    T front() const { return p_a; }

private:
    T p_a, p_b, p_step;
};

template<typename T>
inline number_range<T> range(T a, T b, T step = T(1)) {
    return number_range<T>(a, b, step);
}

template<typename T>
inline number_range<T> range(T v) {
    return number_range<T>(v);
}

template<typename T, typename S>
struct enumerated_value_t {
    S index;
    T value;
};

template<typename T>
struct enumerated_range: input_range<enumerated_range<T>> {
    using range_category  = std::common_type_t<
        range_category_t<T>, forward_range_tag
    >;
    using value_type      = range_value_t<T>;
    using reference       = enumerated_value_t<
        range_reference_t<T>, range_size_t<T>
    >;
    using size_type       = range_size_t      <T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;
    size_type p_index;

public:
    enumerated_range() = delete;

    enumerated_range(T const &range): p_range(range), p_index(0) {}

    enumerated_range(enumerated_range const &it):
        p_range(it.p_range), p_index(it.p_index)
    {}

    enumerated_range(enumerated_range &&it):
        p_range(std::move(it.p_range)), p_index(it.p_index)
    {}

    enumerated_range &operator=(enumerated_range const &v) {
        p_range = v.p_range;
        p_index = v.p_index;
        return *this;
    }
    enumerated_range &operator=(enumerated_range &&v) {
        p_range = std::move(v.p_range);
        p_index = v.p_index;
        return *this;
    }
    enumerated_range &operator=(T const &v) {
        p_range = v;
        p_index = 0;
        return *this;
    }
    enumerated_range &operator=(T &&v) {
        p_range = std::move(v);
        p_index = 0;
        return *this;
    }

    bool empty() const { return p_range.empty(); }

    bool equals_front(enumerated_range const &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() {
        if (p_range.pop_front()) {
            ++p_index;
            return true;
        }
        return false;
    }

    size_type pop_front_n(size_type n) {
        size_type ret = p_range.pop_front_n(n);
        p_index += ret;
        return ret;
    }

    reference front() const {
        return reference{p_index, p_range.front()};
    }
};

template<typename T>
struct take_range: input_range<take_range<T>> {
    using range_category  = std::common_type_t<
        range_category_t<T>, forward_range_tag
    >;
    using value_type      = range_value_t     <T>;
    using reference       = range_reference_t <T>;
    using size_type       = range_size_t      <T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;
    size_type p_remaining;

public:
    take_range() = delete;
    take_range(T const &range, range_size_t<T> rem):
        p_range(range), p_remaining(rem)
    {}
    take_range(take_range const &it):
        p_range(it.p_range), p_remaining(it.p_remaining)
    {}
    take_range(take_range &&it):
        p_range(std::move(it.p_range)), p_remaining(it.p_remaining)
    {}

    take_range &operator=(take_range const &v) {
        p_range = v.p_range; p_remaining = v.p_remaining; return *this;
    }
    take_range &operator=(take_range &&v) {
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

    size_type pop_front_n(size_type n) {
        size_type ret = p_range.pop_front_n(n);
        p_remaining -= ret;
        return ret;
    }

    reference front() const { return p_range.front(); }

    bool equals_front(take_range const &r) const {
        return p_range.equals_front(r.p_range);
    }
};

template<typename T>
struct chunks_range: input_range<chunks_range<T>> {
    using range_category   = std::common_type_t<
        range_category_t<T>, forward_range_tag
    >;
    using value_type      = take_range        <T>;
    using reference       = take_range        <T>;
    using size_type       = range_size_t      <T>;
    using difference_type = range_difference_t<T>;

private:
    T p_range;
    size_type p_chunksize;

public:
    chunks_range() = delete;
    chunks_range(T const &range, range_size_t<T> chs):
        p_range(range), p_chunksize(chs)
    {}
    chunks_range(chunks_range const &it):
        p_range(it.p_range), p_chunksize(it.p_chunksize)
    {}
    chunks_range(chunks_range &&it):
        p_range(std::move(it.p_range)), p_chunksize(it.p_chunksize)
    {}

    chunks_range &operator=(chunks_range const &v) {
        p_range = v.p_range; p_chunksize = v.p_chunksize; return *this;
    }
    chunks_range &operator=(chunks_range &&v) {
        p_range = std::move(v.p_range);
        p_chunksize = v.p_chunksize;
        return *this;
    }

    bool empty() const { return p_range.empty(); }

    bool equals_front(chunks_range const &r) const {
        return p_range.equals_front(r.p_range);
    }

    bool pop_front() { return p_range.pop_front_n(p_chunksize) > 0; }
    size_type pop_front_n(size_type n) {
        return p_range.pop_front_n(p_chunksize * n) / p_chunksize;
    }

    reference front() const { return p_range.take(p_chunksize); }
};

namespace detail {
    template<size_t I, size_t N, typename T>
    inline bool join_range_pop(T &tup) {
        if constexpr(I != N) {
            if (!std::get<I>(tup).empty()) {
                return std::get<I>(tup).pop_front();
            }
            return join_range_pop<I + 1, N>(tup);
        }
        return false;
    }

    template<size_t I, size_t N, typename T>
    inline auto join_range_front(T &tup) {
        if constexpr(I != N) {
            if (!std::get<I>(tup).empty()) {
                return std::get<I>(tup).front();
            }
            return join_range_front<I + 1, N>(tup);
        }
        return std::get<0>(tup).front();
    }
}

template<typename ...R>
struct join_range: input_range<join_range<R...>> {
    using range_category  = std::common_type_t<
        forward_range_tag, range_category_t<R>...
    >;
    using value_type      = std::common_type_t<range_value_t<R>...>;
    using reference       = std::common_type_t<range_reference_t<R>...>;
    using size_type       = std::common_type_t<range_size_t<R>...>;
    using difference_type = std::common_type_t<range_difference_t<R>...>;

private:
    std::tuple<R...> p_ranges;

public:
    join_range() = delete;
    join_range(R const &...ranges): p_ranges(ranges...) {}
    join_range(R &&...ranges): p_ranges(std::forward<R>(ranges)...) {}
    join_range(join_range const &v): p_ranges(v.p_ranges) {}
    join_range(join_range &&v): p_ranges(std::move(v.p_ranges)) {}

    join_range &operator=(join_range const &v) {
        p_ranges = v.p_ranges;
        return *this;
    }

    join_range &operator=(join_range &&v) {
        p_ranges = std::move(v.p_ranges);
        return *this;
    }

    bool empty() const {
        return std::apply([](auto const &...args) {
            return (... && args.empty());
        }, p_ranges);
    }

    bool equals_front(join_range const &r) const {
        return std::apply([&r](auto const &...r1) {
            return std::apply([&](auto const &...r2) {
                return (... && r1.equals_front(r2));
            }, r);
        }, p_ranges);
    }

    bool pop_front() {
        return detail::join_range_pop<0, sizeof...(R)>(p_ranges);
    }

    reference front() const {
        return detail::join_range_front<0, sizeof...(R)>(p_ranges);
    }
};

namespace detail {
    template<typename ...T>
    struct zip_value_type {
        using type = std::tuple<T...>;
    };

    template<typename T, typename U>
    struct zip_value_type<T, U> {
        using type = std::pair<T, U>;
    };

    template<typename ...T>
    using zip_value_t = typename detail::zip_value_type<T...>::type;
}

template<typename ...R>
struct zip_range: input_range<zip_range<R...>> {
    using range_category  = std::common_type_t<
        forward_range_tag, range_category_t<R>...
    >;
    using value_type      = detail::zip_value_t<range_value_t<R>...>;
    using reference       = detail::zip_value_t<range_reference_t<R>...>;
    using size_type       = std::common_type_t<range_size_t<R>...>;
    using difference_type = std::common_type_t<range_difference_t<R>...>;

private:
    std::tuple<R...> p_ranges;

public:
    zip_range() = delete;
    zip_range(R const &...ranges): p_ranges(ranges...) {}
    zip_range(R &&...ranges): p_ranges(std::forward<R>(ranges)...) {}
    zip_range(zip_range const &v): p_ranges(v.p_ranges) {}
    zip_range(zip_range &&v): p_ranges(std::move(v.p_ranges)) {}

    zip_range &operator=(zip_range const &v) {
        p_ranges = v.p_ranges;
        return *this;
    }

    zip_range &operator=(zip_range &&v) {
        p_ranges = std::move(v.p_ranges);
        return *this;
    }

    bool empty() const {
        return std::apply([](auto const &...args) {
            return (... || args.empty());
        }, p_ranges);
    }

    bool equals_front(zip_range const &r) const {
        return std::apply([&r](auto const &...r1) {
            return std::apply([&](auto const &...r2) {
                return (... && r1.equals_front(r2));
            }, r);
        }, p_ranges);
    }

    bool pop_front() {
        return std::apply([](auto &...args) {
            return (... && args.pop_front());
        }, p_ranges);
    }

    reference front() const {
        return std::apply([](auto &&...args) {
            return reference{args.front()...};
        }, p_ranges);
    }
};

template<typename T>
struct appender_range: output_range<appender_range<T>> {
    using value_type      = typename T::value_type;
    using reference       = typename T::reference;
    using size_type       = typename T::size_type;
    using difference_type = typename T::difference_type;

    appender_range(): p_data() {}
    appender_range(T const &v): p_data(v) {}
    appender_range(T &&v): p_data(std::move(v)) {}
    appender_range(appender_range const &v): p_data(v.p_data) {}
    appender_range(appender_range &&v): p_data(std::move(v.p_data)) {}

    appender_range &operator=(appender_range const &v) {
        p_data = v.p_data;
        return *this;
    }

    appender_range &operator=(appender_range &&v) {
        p_data = std::move(v.p_data);
        return *this;
    }

    appender_range &operator=(T const &v) {
        p_data = v;
        return *this;
    }

    appender_range &operator=(T &&v) {
        p_data = std::move(v);
        return *this;
    }

    void clear() { p_data.clear(); }

    void reserve(typename T::size_type cap) { p_data.reserve(cap); }
    void resize(typename T::size_type len) { p_data.resize(len); }

    size_type size() const { return p_data.size(); }
    size_type capacity() const { return p_data.capacity(); }

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
inline appender_range<T> appender() {
    return appender_range<T>();
}

template<typename T>
inline appender_range<T> appender(T &&v) {
    return appender_range<T>(std::forward<T>(v));
}

namespace detail {
    template<typename>
    struct iterator_range_tag_base {
        /* fallback, the most basic range */
        using type = input_range_tag;
    };

    template<>
    struct iterator_range_tag_base<std::output_iterator_tag> {
        using type = output_range_tag;
    };

    template<>
    struct iterator_range_tag_base<std::forward_iterator_tag> {
        using type = forward_range_tag;
    };

    template<>
    struct iterator_range_tag_base<std::bidirectional_iterator_tag> {
        using type = bidirectional_range_tag;
    };

    template<>
    struct iterator_range_tag_base<std::random_access_iterator_tag> {
        using type = finite_random_access_range_tag;
    };
}

template<typename T>
using iterator_range_tag = typename detail::iterator_range_tag_base<T>::type;

template<typename T>
struct iterator_range: input_range<iterator_range<T>> {
    using range_category  = std::conditional_t<
        std::is_pointer_v<T>,
        contiguous_range_tag,
        iterator_range_tag<typename std::iterator_traits<T>::iterator_category>
    >;
    using value_type      = typename std::iterator_traits<T>::value_type;
    using reference       = typename std::iterator_traits<T>::reference;
    using size_type       = std::make_unsigned_t<
        typename std::iterator_traits<T>::difference_type
    >;
    using difference_type = typename std::iterator_traits<T>::difference_type;

    iterator_range(T beg = T{}, T end = T{}): p_beg(beg), p_end(end) {}

    template<typename U, typename = std::enable_if_t<
        std::is_pointer_v<T> && std::is_pointer_v<U> &&
        std::is_convertible_v<U, T>
    >>
    iterator_range(iterator_range<U> const &v): p_beg(&v[0]), p_end(&v[v.size()]) {}

    iterator_range(iterator_range const &v): p_beg(v.p_beg), p_end(v.p_end) {}
    iterator_range(iterator_range &&v):
        p_beg(std::move(v.p_beg)), p_end(std::move(v.p_end))
    {}

    iterator_range &operator=(iterator_range const &v) {
        p_beg = v.p_beg;
        p_end = v.p_end;
        return *this;
    }

    iterator_range &operator=(iterator_range &&v) {
        p_beg = std::move(v.p_beg);
        p_end = std::move(v.p_end);
        return *this;
    }

    /* satisfy input_range / forward_range */
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

    size_type pop_front_n(size_type n) {
        using IC = typename std::iterator_traits<T>::iterator_category;
        if constexpr(std::is_convertible_v<IC, std::random_access_iterator_tag>) {
            size_type olen = size_type(p_end - p_beg);
            p_beg += n;
            if (p_beg > p_end) {
                p_beg = p_end;
                return olen;
            }
            return n;
        } else {
            return detail::pop_front_n(*this, n);
        }
    }

    size_type push_front_n(size_type n) {
        using IC = typename std::iterator_traits<T>::iterator_category;
        if constexpr(std::is_convertible_v<IC, std::random_access_iterator_tag>) {
            p_beg -= n;
            return true;
        } else {
            return detail::push_front_n(*this, n);
        }
    }

    reference front() const { return *p_beg; }

    bool equals_front(iterator_range const &range) const {
        return p_beg == range.p_beg;
    }

    difference_type distance_front(iterator_range const &range) const {
        return range.p_beg - p_beg;
    }

    /* satisfy bidirectional_range */
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

    size_type pop_back_n(size_type n) {
        using IC = typename std::iterator_traits<T>::iterator_category;
        if constexpr(std::is_convertible_v<IC, std::random_access_iterator_tag>) {
            size_type olen = size_type(p_end - p_beg);
            p_end -= n;
            if (p_end < p_beg) {
                p_end = p_beg;
                return olen;
            }
            return n;
        } else {
            return detail::pop_back_n(*this, n);
        }
    }

    size_type push_back_n(size_type n) {
        using IC = typename std::iterator_traits<T>::iterator_category;
        if constexpr(std::is_convertible_v<IC, std::random_access_iterator_tag>) {
            p_end += n;
            return true;
        } else {
            return detail::push_back_n(*this, n);
        }
    }

    reference back() const { return *(p_end - 1); }

    bool equals_back(iterator_range const &range) const {
        return p_end == range.p_end;
    }

    ptrdiff_t distance_back(iterator_range const &range) const {
        return range.p_end - p_end;
    }

    /* satisfy finite_random_access_range */
    size_type size() const { return size_type(p_end - p_beg); }

    iterator_range slice(size_type start, size_type end) const {
        return iterator_range(p_beg + start, p_beg + end);
    }

    reference operator[](size_type i) const { return p_beg[i]; }

    /* satisfy output_range */
    bool put(value_type const &v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = v;
        return true;
    }
    bool put(value_type &&v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = std::move(v);
        return true;
    }

    template<typename R>
    std::enable_if_t<is_output_range<R>, size_type> copy(
        R &&orange, size_type n = -1
    ) {
        if constexpr(std::is_pointer_v<T>) {
            size_type c = size();
            if (n < c) {
                c = n;
            }
            return range_put_n(orange, p_beg, c);
        } else {
            size_type on = n;
            for (; n && !empty(); --n) {
                if (!orange.put(front())) {
                    break;
                }
                pop_front();
            }
            return (on - n);
        }
    }

    size_type copy(std::remove_cv_t<value_type> *p, size_type n = -1) {
        using IC = typename std::iterator_traits<T>::iterator_category;
        if constexpr(std::is_convertible_v<IC, std::random_access_iterator_tag>) {
            size_type c = size();
            if (n < c) {
                c = n;
            }
            if constexpr(std::is_pointer_v<T> && std::is_pod_v<value_type>) {
                memcpy(p, p_beg, c * sizeof(value_type));
                return c;
            } else {
                return copy(iterator_range<
                    std::remove_cv_t<value_type> *
                >(p, p + c), c);
            }
        } else {
            size_type on = n;
            for (; n && !empty(); --n) {
                *p++ = front();
                pop_front();
            }
            return (on - n);
        }
    }
private:
    T p_beg, p_end;
};

template<typename T>
inline auto range_put_n(
    iterator_range<T> &range, range_value_t<iterator_range<T>> const *p,
    range_size_t<iterator_range<T>> n
) {
    using IC = typename std::iterator_traits<T>::iterator_category;
    if constexpr(std::is_convertible_v<IC, std::random_access_iterator_tag>) {
        using value_type = range_value_t<iterator_range<T>>;
        auto ret = range.size();
        if (n < ret) {
            ret = n;
        }
        if constexpr(std::is_pointer_v<T> && std::is_pod_v<value_type>) {
            memcpy(&range.front(), p, ret * sizeof(value_type));
            range.pop_front_n(ret);
        } else {
            for (auto i = ret; i; --i) {
                range.front() = *p++;
                range.pop_front();
            }
        }
        return ret;
    } else {
        auto on = n;
        for (; n && range.put(*p++); --n);
        return (on - n);
    }
}

template<typename T>
iterator_range<T> make_range(T beg, T end) {
    return iterator_range<T>{beg, end};
}

template<typename T>
struct ranged_traits<std::initializer_list<T>> {
    using range = iterator_range<T const *>;

    static range iter(std::initializer_list<T> il) {
        return range{il.begin(), il.end()};
    }
};

/* ranged_traits for initializer lists is not enough; we need to be able to
 * call ostd::iter({initializer list}) and that won't match against a generic
 * template, so we also need to define that here explicitly...
 */
template<typename T>
iterator_range<T const *> iter(std::initializer_list<T> init) noexcept {
    return iterator_range<T const *>(init.begin(), init.end());
}

template<typename T>
iterator_range<T const *> citer(std::initializer_list<T> init) noexcept {
    return iterator_range<T const *>(init.begin(), init.end());
}

template<typename T, size_t N>
struct ranged_traits<T[N]> {
    using range = iterator_range<T *>;

    static range iter(T (&array)[N]) {
        return range{array, array + N};
    }
};

template<typename T>
inline iterator_range<T *> iter(T *a, T *b) {
    return iterator_range<T *>(a, b);
}

/* iter on standard containers */

namespace detail {
    template<typename C>
    static std::true_type test_std_iter(
        decltype(std::begin(std::declval<C &>())) *,
        decltype(std::end(std::declval<C &>())) *
    );

    template<typename>
    static std::false_type test_std_iter(...);

    template<typename C>
    constexpr bool std_iter_test = decltype(test_std_iter<C>(0, 0))::value;

    template<typename C>
    struct ranged_traits_core<C, std::enable_if_t<
        detail::std_iter_test<C> && !detail::direct_iter_test<C>
    >> {
        using range = iterator_range<decltype(std::begin(std::declval<C &>()))>;

        static range iter(C &r) {
            return range{r.begin(), r.end()};
        }
    };
}

} /* namespace ostd */

#endif
