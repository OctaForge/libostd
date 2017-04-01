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
#include <algorithm>

#include "ostd/types.hh"

namespace ostd {

struct input_range_tag {};
struct output_range_tag {};
struct forward_range_tag: input_range_tag {};
struct bidirectional_range_tag: forward_range_tag {};
struct random_access_range_tag: bidirectional_range_tag {};
struct finite_random_access_range_tag: random_access_range_tag {};
struct contiguous_range_tag: finite_random_access_range_tag {};

namespace detail {
    template<typename R>
    struct range_category_test {
        template<typename RR>
        static char test(typename RR::range_category *);
        template<typename>
        static int test(...);
        static constexpr bool value = (sizeof(test<R>(0)) == sizeof(char));
    };

    template<typename R>
    constexpr bool test_range_category = range_category_test<R>::value;

    template<typename R, bool>
    struct range_traits_base {};

    template<typename R>
    struct range_traits_base<R, true> {
        using range_category  = typename R::range_category;
        using size_type       = typename R::size_type;
        using value_type      = typename R::value_type;
        using reference       = typename R::reference;
        using difference_type = typename R::difference_type;
    };

    template<typename R, bool>
    struct range_traits_impl {};

    template<typename R>
    struct range_traits_impl<R, true>: range_traits_base<
        R,
        std::is_convertible_v<typename R::range_category, input_range_tag> ||
        std::is_convertible_v<typename R::range_category, output_range_tag>
    > {};
}

template<typename R>
struct range_traits: detail::range_traits_impl<R, detail::test_range_category<R>> {};

template<typename R>
using range_category_t = typename range_traits<R>::range_category;

template<typename R>
using range_size_t = typename range_traits<R>::size_type;

template<typename R>
using range_value_t = typename range_traits<R>::value_type;

template<typename R>
using range_reference_t = typename range_traits<R>::reference;

template<typename R>
using range_difference_t = typename range_traits<R>::difference_type;

// is input range

namespace detail {
    template<typename T>
    constexpr bool is_input_range_core =
        std::is_convertible_v<range_category_t<T>, input_range_tag>;

    template<typename T, bool = detail::test_range_category<T>>
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

    template<typename T, bool = detail::test_range_category<T>>
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

    template<typename T, bool = detail::test_range_category<T>>
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

    template<typename T, bool = detail::test_range_category<T>>
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

    template<typename T, bool = detail::test_range_category<T>>
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

    template<typename T, bool = detail::test_range_category<T>>
    constexpr bool is_contiguous_range_base = false;

    template<typename T>
    constexpr bool is_contiguous_range_base<T, true> =
        detail::is_contiguous_range_core<T>;
}

template<typename T> constexpr bool is_contiguous_range =
    detail::is_contiguous_range_base<T>;

// is output range

namespace detail {
    template<typename R, typename T>
    static std::true_type test_outrange(typename std::is_same<
        decltype(std::declval<R &>().put(std::declval<T>())), void
    >::type *);

    template<typename, typename>
    static std::false_type test_outrange(...);

    template<typename R, typename T>
    constexpr bool output_range_test = decltype(test_outrange<R, T>())::value;

    template<typename T>
    constexpr bool is_output_range_core =
        std::is_convertible_v<range_category_t<T>, output_range_tag> || (
            is_input_range<T> && (
                output_range_test<T, range_value_t<T> const &> ||
                output_range_test<T, range_value_t<T>      &&> ||
                output_range_test<T, range_value_t<T>        >
            )
        );

    template<typename T, bool = detail::test_range_category<T>>
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
        range_iterator() = delete;
        explicit range_iterator(T const &range): p_range(range) {}
        explicit range_iterator(T &&range): p_range(std::move(range)) {}
        range_iterator &operator++() {
            p_range.pop_front();
            return *this;
        }
        range_reference_t<T> operator*() const {
            return p_range.front();
        }
        bool operator!=(std::nullptr_t) const {
            return !p_range.empty();
        }
    private:
        std::decay_t<T> p_range;
    };
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

template<typename IR>
inline range_size_t<IR> range_pop_front_n(IR &range, range_size_t<IR> n) {
    if constexpr(std::is_convertible_v<
        range_category_t<IR>, finite_random_access_range_tag
    >) {
        auto rs = range.size();
        n = std::min(n, rs);
        range = range.slice(n, rs);
        return n;
    } else {
        size_t r = 0;
        while (n-- && !range.empty()) {
            range.pop_front();
            ++r;
        }
        return r;
    }
}

template<typename IR>
inline range_size_t<IR> range_pop_back_n(IR &range, range_size_t<IR> n) {
    if constexpr(std::is_convertible_v<
        range_category_t<IR>, finite_random_access_range_tag
    >) {
        auto rs = range.size();
        n = std::min(n, rs);
        range = range.slice(0, rs - n);
        return n;
    } else {
        size_t r = 0;
        while (n-- && !range.empty()) {
            range.pop_back();
            ++r;
        }
        return r;
    }
}

template<typename B>
struct input_range {
    detail::range_iterator<B> begin() const {
        return detail::range_iterator<B>(*static_cast<B const *>(this));
    }
    std::nullptr_t end() const {
        return nullptr;
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

template<typename OR, typename IR>
inline void range_put_all(OR &orange, IR range) {
    for (; !range.empty(); range.pop_front()) {
        orange.put(range.front());
    }
}

template<typename T>
struct noop_output_range: output_range<noop_output_range<T>> {
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    void put(T const &) {}
};

template<typename R>
struct counting_output_range: output_range<counting_output_range<R>> {
    using value_type      = range_value_t<R>;
    using reference       = range_reference_t<R>;
    using size_type       = range_size_t<R>;
    using difference_type = range_difference_t<R>;

private:
    R p_range;
    size_type p_written = 0;

public:
    counting_output_range() = delete;
    counting_output_range(R const &range): p_range(range) {}

    void put(value_type const &v) {
        p_range.put(v);
        ++p_written;
    }
    void put(value_type &&v) {
        p_range.put(std::move(v));
        ++p_written;
    }

    size_type get_written() const {
        return p_written;
    }
};

template<typename R>
inline counting_output_range<R> range_counter(R const &range) {
    return counting_output_range<R>{range};
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
        return std::apply([&obj](auto &&...args) mutable {
            return obj.join(std::forward<decltype(args)>(args)...);
        }, std::move(ranges));
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
        return std::apply([&obj](auto &&...args) mutable {
            return obj.zip(std::forward<decltype(args)>(args)...);
        }, std::move(ranges));
    };
}

namespace detail {
    template<typename C>
    static std::true_type test_direct_iter(decltype(std::declval<C &>().iter()) *);

    template<typename>
    static std::false_type test_direct_iter(...);

    template<typename C>
    constexpr bool direct_iter_test = decltype(test_direct_iter<C>(0))::value;

    template<typename C>
    static std::true_type test_std_iter(
        decltype(std::begin(std::declval<C &>())) *,
        decltype(std::end(std::declval<C &>())) *
    );

    template<typename>
    static std::false_type test_std_iter(...);

    template<typename C>
    constexpr bool std_iter_test = decltype(test_std_iter<C>(0, 0))::value;

    /* direct iter and std iter; the case for std iter is
     * specialized after iterator_range is actually defined
     */
    template<typename C, bool, bool>
    struct ranged_traits_core {};

    /* direct iter is available, regardless of std iter being available */
    template<typename C, bool B>
    struct ranged_traits_core<C, true, B> {
        using range = decltype(std::declval<C &>().iter());

        static range iter(C &r) {
            return r.iter();
        }
    };

    template<typename C>
    struct ranged_traits_impl: ranged_traits_core<
        C, direct_iter_test<C>, std_iter_test<C>
    > {};
}

template<typename C>
struct ranged_traits: detail::ranged_traits_impl<C> {};

template<typename T>
inline auto iter(T &&r) ->
    decltype(ranged_traits<std::remove_reference_t<T>>::iter(r))
{
    return ranged_traits<std::remove_reference_t<T>>::iter(r);
}

template<typename T>
inline auto citer(T const &r) -> decltype(ranged_traits<T const>::iter(r)) {
    return ranged_traits<T const>::iter(r);
}

namespace detail {
    template<typename T>
    static std::true_type test_iterable(decltype(ostd::iter(std::declval<T>())) *);
    template<typename>
    static std::false_type test_iterable(...);

    template<typename T>
    constexpr bool iterable_test = decltype(test_iterable<T>(0))::value;
}

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

    void pop_front() { p_range.pop_back(); }
    void pop_back() { p_range.pop_front(); }

    reference front() const { return p_range.back(); }
    reference back() const { return p_range.front(); }

    reference operator[](size_type i) const { return p_range[size() - i - 1]; }

    reverse_range slice(size_type start, size_type end) const {
        size_type len = p_range.size();
        return reverse_range{p_range.slice(len - end, len - start)};
    }
    reverse_range slice(size_type start) const {
        return slice(start, size());
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

    void pop_front() { p_range.pop_front(); }
    void pop_back() { p_range.pop_back(); }

    reference front() const { return std::move(p_range.front()); }
    reference back() const { return std::move(p_range.back()); }

    reference operator[](size_type i) const { return std::move(p_range[i]); }

    move_range slice(size_type start, size_type end) const {
        return move_range{p_range.slice(start, end)};
    }
    move_range slice(size_type start) const {
        return slice(start, size());
    }

    void put(value_type const &v) { p_range.put(v); }
    void put(value_type &&v) { p_range.put(std::move(v)); }
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

    void pop_front() { p_a += p_step; }
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

    void pop_front() {
        p_range.pop_front();
        ++p_index;
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

    void pop_front() {
        p_range.pop_front();
        if (p_remaining >= 1) {
            --p_remaining;
        }
    }

    reference front() const { return p_range.front(); }
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

    void pop_front() {
        range_pop_front_n(p_range, p_chunksize);
    }

    reference front() const { return p_range.take(p_chunksize); }
};

namespace detail {
    template<size_t I, size_t N, typename T>
    inline void join_range_pop(T &tup) {
        if constexpr(I != N) {
            if (!std::get<I>(tup).empty()) {
                std::get<I>(tup).pop_front();
                return;
            }
            join_range_pop<I + 1, N>(tup);
        }
    }

    template<size_t I, size_t N, typename T>
    inline auto join_range_front(T &tup) {
        if constexpr(I != N) {
            if (!std::get<I>(tup).empty()) {
                return std::get<I>(tup).front();
            }
            return join_range_front<I + 1, N>(tup);
        }
        /* fallback, will probably throw */
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

    void pop_front() {
        detail::join_range_pop<0, sizeof...(R)>(p_ranges);
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

    void pop_front() {
        std::apply([](auto &...args) {
            (args.pop_front(), ...);
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
    bool empty() const { return p_data.empty(); }

    void reserve(typename T::size_type cap) { p_data.reserve(cap); }
    void resize(typename T::size_type len) { p_data.resize(len); }

    size_type size() const { return p_data.size(); }
    size_type capacity() const { return p_data.capacity(); }

    void put(typename T::const_reference v) {
        p_data.push_back(v);
    }

    void put(typename T::value_type &&v) {
        p_data.push_back(std::move(v));
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

    void pop_front() {
        ++p_beg;
        /* rely on iterators to do their own checks */
        if constexpr(std::is_pointer_v<T>) {
            if (p_beg > p_end) {
                throw std::out_of_range{"pop_front on empty range"};
            }
        }
    }

    reference front() const { return *p_beg; }

    /* satisfy bidirectional_range */
    void pop_back() {
        /* rely on iterators to do their own checks */
        if constexpr(std::is_pointer_v<T>) {
            if (p_end == p_beg) {
                throw std::out_of_range{"pop_back on empty range"};
            }
        }
        --p_end;
    }

    reference back() const { return *(p_end - 1); }

    /* satisfy finite_random_access_range */
    size_type size() const { return size_type(p_end - p_beg); }

    iterator_range slice(size_type start, size_type end) const {
        return iterator_range(p_beg + start, p_beg + end);
    }
    iterator_range slice(size_type start) const {
        return slice(start, size());
    }

    reference operator[](size_type i) const { return p_beg[i]; }

    /* statisfy contiguous_range */
    value_type *data() { return p_beg; }
    value_type const *data() const { return p_beg; }

    /* satisfy output_range */
    void put(value_type const &v) {
        /* rely on iterators to do their own checks */
        if constexpr(std::is_pointer_v<T>) {
            if (p_beg == p_end) {
                throw std::out_of_range{"put into an empty range"};
            }
        }
        *(p_beg++) = v;
    }
    void put(value_type &&v) {
        if constexpr(std::is_pointer_v<T>) {
            if (p_beg == p_end) {
                throw std::out_of_range{"put into an empty range"};
            }
        }
        *(p_beg++) = std::move(v);
    }

private:
    T p_beg, p_end;
};

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
    /* std iter is available, but at the same time direct iter is not */
    template<typename C>
    struct ranged_traits_core<C, false, true> {
        using range = iterator_range<decltype(std::begin(std::declval<C &>()))>;

        static range iter(C &r) {
            return range{r.begin(), r.end()};
        }
    };
}

} /* namespace ostd */

#endif
