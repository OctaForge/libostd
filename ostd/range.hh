/** @defgroup Ranges
 *
 * @brief Ranges are the backbone of libostd iterable objects and algorithms.
 *
 * Ranges are simple objects that provide access to a sequence of data. They
 * have a similar role to standard C++ iterators, but represent an entire
 * bounded sequence, while iterators represent positions (you need two
 * iterators to represent what a range is).
 *
 * There are input-type and output-type ranges, the former being for reading
 * from and the latter for writing into. There are also hybrid ranges called
 * mutable ranges, which are input-type ranges that at the same time have an
 * output range interface. Input-type ranges are further divided into several
 * categories, each of which extends the former. These include the basic input
 * range (access to front element), forward (access to front element plus
 * state copy guarantee), bidirectional (access to front and back), infinite
 * random access (access to random elements), finite random access (access
 * to random elements plus size and slicing) and contiguous (access to random
 * elements, size, slicing, contiguous memory guarantee).
 *
 * There is a whole article dedicated to ranges [here](@ref ranges). You can
 * also take a look at the many examples in the project tree.
 *
 * @{
 */

/** @file range.hh
 *
 * @brief The range system implementation.
 *
 * This file provides the actual range system implementation,
 * some basic range types, iteration utilities and range traits.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_RANGE_HH
#define OSTD_RANGE_HH

#include <cstddef>
#include <new>
#include <tuple>
#include <utility>
#include <iterator>
#include <type_traits>
#include <initializer_list>
#include <algorithm>

namespace ostd {

/** @addtogroup Ranges
 * @{
 */

/** @brief The input range tag.
 *
 * Every range type is identified by its tag, which essentially defines
 * the capabilities of the range. Input range is the very basic one, it
 * only has front element access and it doesn't guarantee that copies of
 * the range won't alter other copies' state. On the other hand, as it
 * provides almost no guarantees, it can be used for basically any
 * object, for example I/O streams, where the current state is defined
 * by the stream itself and therefore all ranges to it point to the same
 * current state.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::output_range_tag, ostd::forward_range_tag
 */
struct input_range_tag {};

/** @brief The output range tag.
 *
 * This tag is used for ranges that can be written into and nothing else.
 * Such ranges only have the ability to put an item inside of them, with
 * no further specified semantics. When an input-type range implements
 * output functionality, it doesn't use this tag, see ostd::input_range_tag.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::input_range_tag
 */
struct output_range_tag {};

/** @brief The forward range tag.
 *
 * This one is just like ostd::input_range_tag, it doesn't even add anything
 * to the interface, but it has an extra guarantee - you can copy forward
 * ranges and the copies won't affect each other's state. For example, with
 * I/O streams the ranges point to a stream with some shared state, so the
 * current position/element is defined by the stream itself, but with most
 * other objects you can represent the current position/element within the
 * range itself; for example a singly-linked list range can be forward,
 * because you can have one range pointing to one node, then copy it,
 * pop out the front element in the new copy and still have the previous
 * range point to the old element.
 *
 * Any forward range is at the same time input. That's why this tag derives
 * from it.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::input_range_tag, ostd::bidirectional_range_tag
 */
struct forward_range_tag: input_range_tag {};

/** @brief The bidirectional range tag.
 *
 * Bidirectional ranges are forward ranges plus access to the element on
 * the other side of the range. For example doubly linked lists would allow
 * this kind of access. You can't directly access elements in the middle or
 * count how many there are without linear complexity.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::forward_range_tag, ostd::random_access_range_tag
 */
struct bidirectional_range_tag: forward_range_tag {};

/** @brief The infinite random access range tag.
 *
 * Infinite random access ranges are bidirectional ranges plus access to
 * elements in the middle via an arbitrary index. They don't allow access
 * to range size or slicing though, they represent a potentially infinite
 * sequence.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::bidirectional_range_tag, ostd::finite_random_access_range_tag
 */
struct random_access_range_tag: bidirectional_range_tag {};

/** @brief The finite random access range tag.
 *
 * Finite random access are like infinite, but they're bounded, so you can
 * retrieve their size as well as make slices (think making a substring from
 * a string). They do not guarantee their memory is contiguous.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::contiguous_range_tag, ostd::random_access_range_tag
 */
struct finite_random_access_range_tag: random_access_range_tag {};

/** @brief The contiguous range tag.
 *
 * Contiguous ranges are the most featureful range category. They provide
 * the capabilities of all previous types and they also guarantee that the
 * backing memory is contiguous. A pair of pointers is a contiguous range.
 *
 * You can learn more about the characteristics [here](@ref ranges).
 *
 * @see ostd::finite_random_access_range_tag
 */
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
    struct range_traits_base {
        static constexpr bool is_element_swappable = false;
    };

    template<typename R>
    struct range_traits_base<R, true> {
        using range_category  = typename R::range_category;
        using size_type       = typename R::size_type;
        using value_type      = typename R::value_type;
        using reference       = typename R::reference;
        using difference_type = typename R::difference_type;

        static constexpr bool is_element_swappable =
            std::is_convertible_v<range_category, input_range_tag> &&
            std::is_lvalue_reference_v<reference> &&
            std::is_same_v<value_type, std::remove_reference_t<reference>> &&
            std::is_swappable_v<value_type>;
    };

    template<typename R, bool>
    struct range_traits_impl {
        static constexpr bool is_element_swappable = false;
    };

    template<typename R>
    struct range_traits_impl<R, true>: range_traits_base<
        R,
        std::is_convertible_v<typename R::range_category, input_range_tag> ||
        std::is_convertible_v<typename R::range_category, output_range_tag>
    > {};
}

/** @brief The traits (properties) of a range type.
 *
 * Using range traits, you can for check various properties of the range.
 * If the provided `R` type is not a range type, the traits struct will
 * be empty. Otherwise, it will contain the following member types:
 *
 * * `range_category` - one of the category tags (see ostd::input_range_tag)
 * * `size_type` - can contain the range's length (typically `size_t`)
 * * `value_type` - the type of the range's elements
 * * `reference` - the type returned from value accessors
 * * `difference_type` - the type used for distances (typically `ptrdiff_t`)
 *
 * It will always contain the following member as well:
 *
 * ~~~{.cc}
 * static constexpr bool is_element_swappable = ...;
 * ~~~
 *
 * This member will be false for non-range types and otherwise true when
 * the following conditions are met:
 *
 * * The range is an ostd::input_range_tag or better (not an output range).
 * * The `reference` member type is an lvalue reference.
 * * The `value_type` is the same as `std::remove_reference_t<reference>`.
 * * The `value_type` member is swappable (`std::is_swappable_v<value_type>`).
 *
 * If any of these four is not met, the member will also be false.
 *
 * You can read about more details [here](@ref ranges).
 *
 * @see ostd::range_category_t, ostd::range_size_t, ostd::range_value_t,
 *      ostd::range_reference_t, ostd::range_difference_t
 */
template<typename R>
struct range_traits: detail::range_traits_impl<R, detail::test_range_category<R>> {};

/** @brief The category tag of a range type.
 *
 * It's the same as doing
 *
 * ~~~{.cc}
 * typename ostd::range_traits<R>::range_category
 * ~~~
 *
 * @see ostd::range_size_t, ostd::range_value_t, ostd::range_reference_t,
 *      ostd::range_difference_t
 */
template<typename R>
using range_category_t = typename range_traits<R>::range_category;

/** @brief The size type of a range type.
 *
 * It's the same as doing
 *
 * ~~~{.cc}
 * typename ostd::range_traits<R>::size_type
 * ~~~
 *
 * @see ostd::range_category_t, ostd::range_value_t, ostd::range_reference_t,
 *      ostd::range_difference_t
 */
template<typename R>
using range_size_t = typename range_traits<R>::size_type;

/** @brief The value type of range elements.
 *
 * It's the same as doing
 *
 * ~~~{.cc}
 * typename ostd::range_traits<R>::value_type
 * ~~~
 *
 * @see ostd::range_category_t, ostd::range_size_t, ostd::range_reference_t,
 *      ostd::range_difference_t
 */
template<typename R>
using range_value_t = typename range_traits<R>::value_type;

/** @brief The type returned by range element accessors.
 *
 * It's the same as doing
 *
 * ~~~{.cc}
 * typename ostd::range_traits<R>::reference
 * ~~~
 *
 * @see ostd::range_category_t, ostd::range_size_t, ostd::range_value_t,
 *      ostd::range_difference_t
 */
template<typename R>
using range_reference_t = typename range_traits<R>::reference;

/** @brief The difference type of a range type.
 *
 * It's the same as doing
 *
 * ~~~{.cc}
 * typename ostd::range_traits<R>::difference_type
 * ~~~
 *
 * @see ostd::range_category_t, ostd::range_size_t, ostd::range_value_t,
 *      ostd::range_reference_t
 */
template<typename R>
using range_difference_t = typename range_traits<R>::difference_type;

/** @brief Checks whether the range's element accessors allow swapping.
 *
 * It's the same as doing
 *
 * ~~~{.cc}
 * ostd::range_traits<R>::is_element_swappable
 * ~~~
 *
 */
template<typename R>
constexpr bool is_range_element_swappable = range_traits<R>::is_element_swappable;

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

/** @brief Checks if the given type is an input range or better.
 *
 * This check never fails, so it will return `false` for non-range types.
 * Range types must be input or better for this to return `true`.
 *
 * @see ostd::is_forward_range, ostd::is_output_range
 */
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

/** @brief Checks if the given type is a forward range or better.
 *
 * This check never fails, so it will return `false` for non-range types.
 * Range types must be forward or better for this to return `true`.
 *
 * @see ostd::is_input_range, ostd::is_bidirectional_range
 */
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

/** @brief Checks if the given type is a bidirectional range or better.
 *
 * This check never fails, so it will return `false` for non-range types.
 * Range types must be bidirectional or better for this to return `true`.
 *
 * @see ostd::is_forward_range, ostd::is_random_access_range
 */
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

/** @brief Checks if the given type is a random access range or better.
 *
 * This check never fails, so it will return `false` for non-range types.
 * Range types must be infinite random access or better for this to
 * return `true`.
 *
 * @see ostd::is_bidirectional_range, ostd::is_finite_random_access_range
 */
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

/** @brief Checks if the given type is a finite random access range or better.
 *
 * This check never fails, so it will return `false` for non-range types.
 * Range types must be finite random access or better for this to
 * return `true`.
 *
 * @see ostd::is_random_access_range, ostd::is_contiguous_range
 */
template<typename T> constexpr bool is_finite_random_access_range =
    detail::is_finite_random_access_range_base<T>;

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

/** @brief Checks if the given type is a contiguous range.
 *
 * This check never fails, so it will return `false` for non-range types.
 *
 * @see ostd::is_finite_random_access_range
 */
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

/** @brief Checks if the given type is an output range.
 *
 * This check never fails, so it will return `false` for non-range types.
 * Range types must either have ostd::output_range_tag or implement the
 * output range capabilities, this doesn't merely check the tag but it
 * also checks the interface if the tag is not present.
 *
 * @see ostd::is_input_range
 */
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

namespace detail {
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
}

/** @brief Pops out at most `n` elements from the front of the given range.
 *
 * For finite random access ranges (ostd::finite_random_access_range_tag)
 * or better, slicing is used. Otherwise, each element is popped out
 * separately. Therefore, the complexity either depends on the slciing
 * operation (frequently `O(1)`) or is linear (`O(n)`).
 *
 * If the range doesn't have the given number of elements, it will pop
 * out at least what's there.
 *
 * The range type must be an input range (ostd::input_range_tag) or better.
 *
 * @returns The number of elements popped out.
 *
 * @see ostd::range_pop_back_n()
 */
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
        range_size_t<IR> r = 0;
        while (n-- && !range.empty()) {
            range.pop_front();
            ++r;
        }
        return r;
    }
}

/** @brief Pops out at most `n` elements from the back of the given range.
 *
 * For finite random access ranges (ostd::finite_random_access_range_tag)
 * or better, slicing is used. Otherwise, each element is popped out
 * separately. Therefore, the complexity either depends on the slciing
 * operation (frequently `O(1)`) or is linear (`O(n)`).
 *
 * If the range doesn't have the given number of elements, it will pop
 * out at least what's there.
 *
 * The range type must be a bidirectionalrange (ostd::bidirectional_range_tag)
 * or better.
 *
 * @returns The number of elements popped out.
 *
 * @see ostd::range_pop_front_n()
 */
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
        range_size_t<IR> r = 0;
        while (n-- && !range.empty()) {
            range.pop_back();
            ++r;
        }
        return r;
    }
}

/** @brief A base type for all input-type ranges to derive from.
 *
 * Every input range type derives from this, see [Ranges](@ref ranges).
 * It implements functionality every range type is supposed to have.
 *
 * Pure output ranges will need to derive from ostd::output_range.
 *
 * The pipe operator overloads call the given function with this range.
 * This allows pipeable algorithms which essentially return a lambda taking
 * the range which then performs the needed algorithm.
 */
template<typename B>
struct input_range {
    /** @brief Creates a very simple iterator for range-based for loop.
     *
     * Thanks to this, you can use the range-based for loop with ranges
     * effortlessly. It's not a proper full iterator type, it only has
     * the necessary functionality for iteration and has no end iterator,
     * see end().
     */
    auto begin() const {
        return detail::range_iterator<B>(*static_cast<B const *>(this));
    }

    /** @brief Gets a sentinel for begin(). */
    std::nullptr_t end() const {
        return nullptr;
    }

    /** @brief Creates a copy of the range.
     *
     * This is just like copy-constructing the range.
     */
    B iter() const {
        return B(*static_cast<B const *>(this));
    }

    /** @brief Gets a reverse range to the current range.
     *
     * A reverse range is a wrapper range that lazily reveses the direction.
     * Only defined for ranges that are at least bidirectional. It re-routes
     * the access so that front accesses back and vice versa.
     *
     * If this range is at least ostd::finite_ranodm_access_range_tag, the
     * wrapped range will be always ostd::finite_random_access_range_tag.
     * Otherwise, it will be ostd::bidirectional_range_tag.
     *
     * The value, reference, size and difference types are the same.
     */
    auto reverse() const {
        static_assert(
            is_bidirectional_range<B>,
            "Wrapped range must be bidirectional or better"
        );
        return detail::reverse_range<B>(iter());
    }

    /** @brief Gets a wrapper range that moves all the elements.
     *
     * The range is an ostd::input_range_tag. The base range is required to
     * have real references for its reference type and those references must
     * be references to the range's value type, i.e.
     *
     * ~~~{.cc}
     * std::is_reference_v<ostd::range_reference_t<R>> == true
     * std::is_same_v<
     *     ostd::range_value_t<T>,
     *     std::remove_reference_t<ostd::range_reference_t<T>
     * > == true
     * ~~~
     *
     * The value, size and difference types remain the same. The new reference
     * type becomes `ostd::range_value_t<R> &&`.
     *
     * Accesses to the front member result in the element being moved.
     */
    auto movable() const {
        return detail::move_range<B>(iter());
    }

    /** @brief Gets an enumerated range for the range.
     *
     * An enumerated range wraps the original range in a way where the accesses
     * remain mostly the same, but an index counter is kept and incremented on
     * each pop.
     *
     * It's always at most ostd::forward_range_tag. The value, size and
     * difference types stay the same, the new reference type is like this:
     *
     * ~~~{.cc}
     * struct enumerated_value_t {
     *     range_size_t<R> index;
     *     range_reference_t<R> value;
     * };
     * ~~~
     *
     * This is useful when you need to iterate over a range using the range
     * based for loop but still need a numerical index. An example:
     *
     * ~~~{.cc}
     * auto il = { 5, 10, 15};
     * // prints "0: 5", "1: 10", "2: 15"
     * for (auto v: ostd::iter(il).enumerate()) {
     *     ostd::writefln("%s: %s", v.index, v.value);
     * }
     * ~~~
     */
    auto enumerate() const {
        return detail::enumerated_range<B>(iter());
    }

    /** @brief Gets a range representing several elements of the range.
     *
     * Wraps the range in a way where at most `n` elements are considered
     * when manipulating the range. The result is at always at most
     * ostd::forward_range_tag.
     *
     * It is undefined behavior to try to `pop_front()` past the internal
     * counter (i.e. `empty()` must not be true when calling it).
     */
    template<typename Size>
    auto take(Size n) const {
        return detail::take_range<B>(iter(), n);
    }

    /** @brief Splits the range into range of chunks.
     *
     * The resulting range is at most ostd::forward_range_tag. Each element
     * of it is the result of take() on the current stored range. Each call
     * to `pop_front()` pops out at most `n` elements from the wrapped range.
     *
     * If the wrapped range's length is not a multiple of `n`, the last chunk
     * will have fewer elements than `n`.
     */
    template<typename Size>
    auto chunks(Size n) const {
        return detail::chunks_range<B>(iter(), n);
    }

    /** @brief Joins multiple ranges together.
     *
     * The ranges don't have to be the same. The types of ostd::range_traits
     * will be std::common_type_t of all ranges' trait types. The range itself
     * is at most ostd::forward_range_tag, but can be ostd::input_range_tag
     * if any of the joined ranges are.
     *
     * The range is empty when all joined ranges are empty. Access to front
     * is undefined if all joined ranges are empty.
     */
    template<typename R1, typename ...RR>
    auto join(R1 r1, RR ...rr) const {
        return detail::join_range<B, R1, RR...>(
            iter(), std::move(r1), std::move(rr)...
        );
    }

    /** @brief Zips multiple ranges together.
     *
     * The ranges will all be iterated at the same time up until the shortest
     * range's length. The wrapper range is at most ostd::forward_range_tag.
     *
     * The value type can be a pair (for two ranges) or a tuple (for more) of
     * the value types. The reference type is also a pair or a tuple, but of
     * the reference types. The size and difference types are common types
     * between the zipped ranges.
     */
    template<typename R1, typename ...RR>
    auto zip(R1 r1, RR ...rr) const {
        return detail::zip_range<B, R1, RR...>(
            iter(), std::move(r1), std::move(rr)...
        );
    }

    /** @brief Simpler syntax for accessing the front element. */
    auto operator*() const {
        return std::forward<decltype(static_cast<B const *>(this)->front())>(
            static_cast<B const *>(this)->front()
        );
    }

    /** @brief Simpler syntax for popping out the front element. */
    B &operator++() {
        static_cast<B *>(this)->pop_front();
        return *static_cast<B *>(this);
    }

    /** @brief Simpler syntax for popping out the front element.
     *
     * Returns the range before the pop happened.
     */
    B operator++(int) {
        B tmp(*static_cast<B const *>(this));
        static_cast<B *>(this)->pop_front();
        return tmp;
    }

    /* pipe op, must be a member to work for user ranges automagically */

    /** @brief A necessary overload for pipeable algorithm support. */
    template<typename F>
    auto operator|(F &&func) {
        return func(*static_cast<B *>(this));
    }

    /** @brief A necessary overload for pipeable algorithm support. */
    template<typename F>
    auto operator|(F &&func) const {
        return func(*static_cast<B const *>(this));
    }

    /* universal bool operator */

    /** @brief Checks if the range is not empty. */
    explicit operator bool() const {
        return !(static_cast<B const *>(this)->empty());
    }
};

/** @brief The base type for all pure output ranges. */
template<typename B>
struct output_range {
    /** @brief All pure output ranges have the same tag. */
    using range_category = output_range_tag;
};

/** @brief Puts all of `range`'s elements into `orange`.
 *
 * The default implementation is equivalent to iterating `range` and then
 * calling `orange.put(range.front())` on each, but it can be overloaded
 * with more efficient implementations per type. Usages of this in generic
 * algortihms follow ADL, so the right function will always be resolved.
 */
template<typename OR, typename IR>
inline void range_put_all(OR &orange, IR range) {
    for (; !range.empty(); range.pop_front()) {
        orange.put(range.front());
    }
}

namespace detail {
    template<typename T>
    struct noop_output_range: output_range<noop_output_range<T>> {
        using value_type      = T;
        using reference       = T &;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;

        /** @brief Has no effect. */
        void put(T const &) {}
    };
}

/** @brief Creates an output range that does nothing.
 *
 * This is a complete output range, but it doesn't actually store the given
 * values, instead it simply does nothing. Useful in metaprogramming and
 * when you need to go over a portion of something that uses sinks.
 */
template<typename T>
inline auto noop_sink() {
    return detail::noop_output_range<T>{};
}

namespace detail {
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
}

/** @brief Creates an output range that counts items put into it.
 *
 * Takes an output range and creates a wrapper output range around it that
 * counts each item put into it. A copy of the range is stored. The types
 * are inherited from the wrapped range.
 *
 * This method is provided to retrieve the number of values put into the range:
 *
 * ~~~{.cc}
 * range_size_t<R> get_written() const;
 * ~~~
 */
template<typename R>
inline auto counting_sink(R const &range) {
    return detail::counting_output_range<R>{range};
}

/** @brief A pipeable version of ostd::input_range::reverse(). */
inline auto reverse() {
    return [](auto &obj) { return obj.reverse(); };
}

/** @brief A pipeable version of ostd::input_range::movable(). */
inline auto movable() {
    return [](auto &obj) { return obj.movable(); };
}

/** @brief A pipeable version of ostd::input_range::enumerate(). */
inline auto enumerate() {
    return [](auto &obj) { return obj.enumerate(); };
}

/** @brief A pipeable version of ostd::input_range::take(). */
template<typename T>
inline auto take(T n) {
    return [n](auto &obj) { return obj.take(n); };
}

/** @brief A pipeable version of ostd::input_range::chunks(). */
template<typename T>
inline auto chunks(T n) {
    return [n](auto &obj) { return obj.chunks(n); };
}

/** @brief A pipeable version of ostd::input_range::join(). */
template<typename R>
inline auto join(R &&range) {
    return [range = std::forward<R>(range)](auto &obj) mutable {
        return obj.join(std::forward<R>(range));
    };
}

/** @brief A pipeable version of ostd::input_range::join(). */
template<typename R1, typename ...R>
inline auto join(R1 &&r1, R &&...rr) {
    return [
        ranges = std::forward_as_tuple(
            std::forward<R1>(r1), std::forward<R>(rr)...
        )
    ] (auto &obj) mutable {
        return std::apply([&obj](auto &&...args) mutable {
            return obj.join(std::forward<decltype(args)>(args)...);
        }, std::move(ranges));
    };
}

/** @brief A pipeable version of ostd::input_range::zip(). */
template<typename R>
inline auto zip(R &&range) {
    return [range = std::forward<R>(range)](auto &obj) mutable {
        return obj.zip(std::forward<R>(range));
    };
}

/** @brief A pipeable version of ostd::input_range::zip(). */
template<typename R1, typename ...R>
inline auto zip(R1 &&r1, R &&...rr) {
    return [
        ranges = std::forward_as_tuple(
            std::forward<R1>(r1), std::forward<R>(rr)...
        )
    ] (auto &obj) mutable {
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

/** @brief A structure to implement iterability of any object.
 *
 * Custom types can specialize this. It's also specialized in multiple
 * places in libostd. It needs to implement one member alias which is
 * `using range = ...` representing the range type and one method which
 * is `static range iter(C &)`.
 *
 * This default implementation handles all objects which implement the
 * `iter()` method that returns a range, as well as all objects that
 * have the standard iterator interface, using ostd::iterator_range.
 *
 * Because of these two default generic cases, you frequently won't need to
 * specialize this at all.
 *
 * Const and non-const versions need separate specializations.
 */
template<typename C>
struct ranged_traits: detail::ranged_traits_impl<C> {};

/** @brief A generic function to iterate any object.
 *
 * This uses ostd::ranged_traits to create the range object, so it will
 * work for anything ostd::ranged_traits is properly specialized for.
 *
 * @see ostd::citer()
 */
template<typename T>
inline auto iter(T &&r) ->
    decltype(ranged_traits<std::remove_reference_t<T>>::iter(r))
{
    return ranged_traits<std::remove_reference_t<T>>::iter(r);
}

/** @brief A generic function to iterate an immutable version of any object.
 *
 * This uses ostd::ranged_traits to create the range object, so it will
 * work for anything ostd::ranged_traits is properly specialized for.
 * It forces a range for an immutable object, unlike ostd::iter(), where
 * it depends on the constness of the input.
 *
 * @see ostd::iter()
 */
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

namespace detail {
    template<typename T>
    struct number_range: input_range<number_range<T>> {
        using range_category  = forward_range_tag;
        using value_type      = T;
        using reference       = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;

        number_range() = delete;

        number_range(T a, T b, std::make_signed_t<T> step = 1):
            p_a(a), p_b(b), p_step(step)
        {}

        number_range(T v): p_a(0), p_b(v), p_step(1) {}

        bool empty() const { return p_a * p_step >= p_b * p_step; }

        void pop_front() { p_a += p_step; }

        reference front() const { return p_a; }

    private:
        T p_a, p_b;
        std::make_signed_t<T> p_step;
    };
} /* namespace detail */

/** @brief Creates an integer interval between `a` and `b`.
 *
 * The result is an ostd::forward_range_tag range with `T` for value
 * and reference types. If `T` is unsigned, `step` is a signed version of
 * it. If it's signed, `step` is the same type.
 *
 * The range goes from `a` until `b`, incrementing by `step` with each
 * pop. The `b` boudnary is not included in the range (it's half open).
 * It's considered empty once `(current * step) >= (b * step)`, with
 * `current` being `a` at first, increased by `step` on each pop.
 *
 * This allows writing nice code such as
 *
 * ~~~{.cc}
 * for (int i: ostd::range(5, 10)) { // from 5 to 10, not including 10
 *     ...
 * }
 * ~~~
 */
template<typename T>
inline auto range(T a, T b, std::make_signed_t<T> step = 1) {
    return detail::number_range<T>(a, b, step);
}

/** @brief Creates an integer interval between `0` and `v`.
 *
 * Equivalent to ostd::range() with 0 and `v` for arguments.
 * This allows writing nice code such as
 *
 * ~~~{.cc}
 * for (int i: ostd::range(10)) { // from 0 to 10, not including 10
 *     ...
 * }
 * ~~~
 */
template<typename T>
inline auto range(T v) {
    return detail::number_range<T>(v);
}

namespace detail {
    template<typename T>
    struct reverse_range: input_range<reverse_range<T>> {
        using range_category  = std::conditional_t<
            is_finite_random_access_range<T>,
            finite_random_access_range_tag,
            bidirectional_range_tag
        >;
        using value_type      = range_value_t     <T>;
        using reference       = range_reference_t <T>;
        using size_type       = range_size_t      <T>;
        using difference_type = range_difference_t<T>;

    private:
        T p_range;

    public:
        reverse_range() = delete;

        reverse_range(T const &v): p_range(v) {}

        bool empty() const { return p_range.empty(); }

        size_type size() const { return p_range.size(); }

        void pop_front() { p_range.pop_back(); }
        void pop_back() { p_range.pop_front(); }

        reference front() const { return p_range.back(); }
        reference back() const { return p_range.front(); }

        reference operator[](size_type i) const {
            return p_range[size() - i - 1];
        }

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
        static_assert(
            std::is_reference_v<range_reference_t<T>>,
            "Wrapped range must return proper references for accessors"
        );
        static_assert( 
            std::is_same_v<
                range_value_t<T>, std::remove_reference_t<range_reference_t<T>>
            >,
            "Wrapped range references must be proper references to the value type"
        );

        using range_category  = input_range_tag;
        using value_type      = range_value_t     <T>;
        using reference       = range_value_t     <T> &&;
        using size_type       = range_size_t      <T>;
        using difference_type = range_difference_t<T>;

    private:
        T p_range;

    public:
        move_range() = delete;

        move_range(T const &range): p_range(range) {}

        bool empty() const { return p_range.empty(); }

        void pop_front() { p_range.pop_front(); }

        reference front() const { return std::move(p_range.front()); }
    };

    template<typename T>
    struct enumerated_range: input_range<enumerated_range<T>> {
    private:
        struct enumerated_value_t {
            range_size_t<T> index;
            range_reference_t<T> value;
        };

    public:
        using range_category  = std::common_type_t<
            range_category_t<T>, forward_range_tag
        >;
        using value_type      = range_value_t<T>;
        using reference       = enumerated_value_t;
        using size_type       = range_size_t      <T>;
        using difference_type = range_difference_t<T>;

    private:
        T p_range;
        size_type p_index;

    public:
        enumerated_range() = delete;
        enumerated_range(T const &range): p_range(range), p_index(0) {}

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

        take_range(T const &range, size_type rem):
            p_range(range), p_remaining(rem)
        {}

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

        bool empty() const { return p_range.empty(); }

        void pop_front() {
            range_pop_front_n(p_range, p_chunksize);
        }

        reference front() const { return p_range.take(p_chunksize); }
    };

    template<std::size_t I, std::size_t N, typename T>
    inline void join_range_pop(T &tup) {
        if constexpr(I != N) {
            if (!std::get<I>(tup).empty()) {
                std::get<I>(tup).pop_front();
                return;
            }
            join_range_pop<I + 1, N>(tup);
        }
    }

    template<std::size_t I, std::size_t N, typename T>
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

        bool empty() const {
            return std::apply([](auto const &...args) {
                return (... && args.empty());
            }, p_ranges);
        }

        void pop_front() {
            join_range_pop<0, sizeof...(R)>(p_ranges);
        }

        reference front() const {
            return join_range_front<0, sizeof...(R)>(p_ranges);
        }
    };

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

    template<typename ...R>
    struct zip_range: input_range<zip_range<R...>> {
        using range_category  = std::common_type_t<
            forward_range_tag, range_category_t<R>...
        >;
        using value_type      = zip_value_t<range_value_t<R>...>;
        using reference       = zip_value_t<range_reference_t<R>...>;
        using size_type       = std::common_type_t<range_size_t<R>...>;
        using difference_type = std::common_type_t<range_difference_t<R>...>;

    private:
        std::tuple<R...> p_ranges;

    public:
        zip_range() = delete;

        zip_range(R const &...ranges): p_ranges(ranges...) {}

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
} /* namespace detail */

/** @brief An appender range is an output range over a standard container.
 *
 * It will effectively `push_back(v)` into the container every time a `put(v)`
 * is called on the appender. It works with any container that implements the
 * `push_back(v)` method.
 *
 * It also implements some methods to manipulate the container itself which
 * are not part of the output range interface. Not all of them might work,
 * depending on the container stored.
 */
template<typename T>
struct appender_range: output_range<appender_range<T>> {
    using value_type      = typename T::value_type;
    using reference       = typename T::reference;
    using size_type       = typename T::size_type;
    using difference_type = typename T::difference_type;

    /** @brief Default constructs the container inside. */
    appender_range(): p_data() {}

    /** @brief Constructs from a copy of a container. */
    appender_range(T const &v): p_data(v) {}

    /** @brief Constructs an appender by moving a container into it. */
    appender_range(T &&v): p_data(std::move(v)) {}

    /** @brief Assigns a copy of a container to the appender. */
    appender_range &operator=(T const &v) {
        p_data = v;
        return *this;
    }

    /** @brief Assigns a container to the appender by move. */
    appender_range &operator=(T &&v) {
        p_data = std::move(v);
        return *this;
    }

    /** @brief Clears the underlying container's contents. */
    void clear() { p_data.clear(); }

    /** @brief Checks if the underlying container is empty. */
    bool empty() const { return p_data.empty(); }

    /** @brief Reserves some capacity in the container. */
    void reserve(size_type cap) { p_data.reserve(cap); }

    /** @brief Resizes the container. */
    void resize(size_type len) { p_data.resize(len); }

    /** @brief Gets the container size. */
    size_type size() const { return p_data.size(); }

    /** @brief Gets the container capacity. */
    size_type capacity() const { return p_data.capacity(); }

    /** @brief Appends a copy of a value to the end of the container. */
    void put(typename T::const_reference v) {
        p_data.push_back(v);
    }

    /** @brief Appends a value to the end of the container by move */
    void put(typename T::value_type &&v) {
        p_data.push_back(std::move(v));
    }

    /** @brief Gets a reference to the underlying container. */
    T &get() { return p_data; }
private:
    T p_data;
};

/** @brief A convenience method to create an appender. */
template<typename T>
inline appender_range<T> appender() {
    return appender_range<T>();
}

/** @brief A convenience method to create an appender. */
template<typename T>
inline appender_range<T> appender(T const &v) {
    return appender_range<T>(v);
}

/** @brief A convenience method to create an appender. */
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

/** @brief Gets the best range category that can be created from the given
  *        iterator category.
  *
  * They match up to bidirectional. Random access iterators create finite
  * random access ranges. Contiguous ranges can not be created from generic
  * iterator types.
  */
template<typename T>
using iterator_range_tag = typename detail::iterator_range_tag_base<T>::type;

/** @brief A range type made up of two iterators.
 *
 * Any iterator type is allowed. If the iterator type is a pointer, which
 * is also a perfectly valid iterator type, the range is contiguous.
 * Otherwise the range category is determined with ostd::iterator_range_tag.
 */
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

    /** @brief Creates an iterator range.
     *
     * The behavior is undefined if the iterators aren't a valid pair.
     */
    iterator_range(T beg = T{}, T end = T{}): p_beg(beg), p_end(end) {}

    /** @brief Converts a compatible pointer iterator range into another.
     *
     * This constructor is only expanded if this iterator range is over
     * a pointer, the other iterator range is also over a pointer and the
     * other's poiner is convertible to this one's pointer.
     */
    template<typename U, typename = std::enable_if_t<
        std::is_pointer_v<T> && std::is_pointer_v<U> &&
        std::is_convertible_v<U, T>
    >>
    iterator_range(iterator_range<U> const &v):
        p_beg(&v[0]), p_end(&v[v.size()])
    {}

    /** @brief Effectively true if the iterators are equal. */
    bool empty() const { return p_beg == p_end; }

    /** @brief Increments the front iterator.
     *
     * If the wrapped iterator is a pointer and the range is empty,
     * std::out_of_range is thrown.
     */
    void pop_front() {
        /* rely on iterators to do their own checks */
        if constexpr(std::is_pointer_v<T>) {
            if (p_beg == p_end) {
                throw std::out_of_range{"pop_front on empty range"};
            }
        }
        ++p_beg;
    }

    /** @brief Dereferences the beginning iterator. */
    reference front() const { return *p_beg; }

    /** @brief Decrements the back iterator.
     *
     * If the wrapped iterator is a pointer and the range is empty,
     * std::out_of_range is thrown. Only valid if the range is at
     * least bidirectional.
     */
    void pop_back() {
        /* rely on iterators to do their own checks */
        if constexpr(std::is_pointer_v<T>) {
            if (p_end == p_beg) {
                throw std::out_of_range{"pop_back on empty range"};
            }
        }
        --p_end;
    }

    /** @brief Dereferences the decremented end iterator.
     *
     * Effectively `*(end - 1)`. Only valid if the range is at least
     * bidirectional.
     */
    reference back() const { return *(p_end - 1); }

    /** @brief Gets the size of the range.
     *
     * Only valid if the range is at least finite random access.
     */
    size_type size() const { return size_type(p_end - p_beg); }

    /** @brief Slices the range.
     *
     * Only valid if the range is at least finite random access.
     * If the indexes are not within bounds, the behavior is undefined.
     */
    iterator_range slice(size_type start, size_type end) const {
        return iterator_range(p_beg + start, p_beg + end);
    }

    /** @brief Slices the range with size() for the second argument.
     *
     * Only valid if the range is at least finite random access.
     * If the index is not within bounds, the behavior is undefined.
     */
    iterator_range slice(size_type start) const {
        return slice(start, size());
    }

    /** @brief Indexes the range (beginning iterator).
     *
     * Only valid if the range is at least finite random access.
     * If the index is not within bounds, the behavior is undefined.
     */
    reference operator[](size_type i) const { return p_beg[i]; }

    /** @brief Gets the pointer to the first element.
     *
     * Only valid/useful if the range is contiguous.
     */
    value_type *data() { return &front(); }

    /** @brief Gets the pointer to the first element.
     *
     * Only valid/useful if the range is contiguous.
     */
    value_type const *data() const { return &front(); }

    /** @brief Assigns a copy of `v` to front and pops it out.
     *
     * Only valid if the iterator is mutable. If the iterator is
     * a pointer, std::out_of_range is thrown if the range is empty.
     */
    void put(value_type const &v) {
        /* rely on iterators to do their own checks */
        if constexpr(std::is_pointer_v<T>) {
            if (p_beg == p_end) {
                throw std::out_of_range{"put into an empty range"};
            }
        }
        *(p_beg++) = v;
    }

    /** @brief Moves `v` to front and pops it out.
     *
     * Only valid if the iterator is mutable. If the iterator is
     * a pointer, std::out_of_range is thrown if the range is empty.
     */
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

/** @brief A specialization of ostd::ranged_traits for initializer list types.
 *
 * Sadly, this will not be picked up by the type system when you try to use
 * ostd::iter() directly, so that is also overloaded for initializer lists
 * below.
 */
template<typename T>
struct ranged_traits<std::initializer_list<T>> {
    /** @brief The range type. */
    using range = iterator_range<T const *>;

    /** @brief Creates the range. */
    static range iter(std::initializer_list<T> il) {
        return range{il.begin(), il.end()};
    }
};

/** @brief An overload of ostd::iter() for initializer lists.
 *
 * This must be done to allow the type system to pick up on expressions
 * such as `ostd::iter({ a, b, c, d, ... })`. It will not do so automatically
 * because it looks for std::initializer_list in the prototype.
 */
template<typename T>
iterator_range<T const *> iter(std::initializer_list<T> init) noexcept {
    return iterator_range<T const *>(init.begin(), init.end());
}

/** @brief An overload of ostd::citer() for initializer lists.
 *
 * This must be done to allow the type system to pick up on expressions
 * such as `ostd::iter({ a, b, c, d, ... })`. It will not do so automatically
 * because it looks for std::initializer_list in the prototype.
 */
template<typename T>
iterator_range<T const *> citer(std::initializer_list<T> init) noexcept {
    return iterator_range<T const *>(init.begin(), init.end());
}

/** @brief A specialization of ostd::ranged_traits for static arrays.
 *
 * This allows iteration of static arrays directly with ostd::iter().
 */
template<typename T, std::size_t N>
struct ranged_traits<T[N]> {
    /** @brief The range type. */
    using range = iterator_range<T *>;

    /** @brief Creates the range. */
    static range iter(T (&array)[N]) {
        return range{array, array + N};
    }
};

/** @brief Creates an ostd::iterator_range from two pointers.
 *
 * The resulting iterator range will be contiguous. The length of the
 * resulting range will be `b - a`.
 */
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

/** @} */

} /* namespace ostd */

#endif

/** @} */
