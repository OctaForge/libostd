/** @defgroup Strings
 *
 * @brief Provides string processing extensions.
 *
 * As libostd provides a range system, it represents string slices as
 * contiguous ranges of characters. This has many advantages, such as
 * being able to use them with generic algorithms. The string slices are
 * not zero terminated, which means creating subslices is very fast, it's
 * basically just pointer arithmetic.
 *
 * Integration with existing string handling facilities is ensured, so you
 * can incorporate libostd into any existing project and still benefit from
 * the new features.
 *
 * A simple example:
 *
 * ~~~{.cc}
 * #include <ostd/string.hh>
 * #include <ostd/io.hh>
 *
 * int main() {
 *     ostd::string_range x = "hello world";
 *     auto p1 = x.slice(0, 5);
 *     auto p2 = x.slice(6);
 *     ostd::writeln(p1); // hello
 *     ostd::writeln(p2); // world
 * }
 * ~~~
 *
 * An example of using libostd string formatting:
 *
 * @include format.cc
 *
 * See the examples provided with the library for further information.
 *
 * @{
 */

/** @file string.hh
 *
 * @brief String slice implementation as well as other utilities.
 *
 * This file implements string slices, their comparisons, utilities,
 * standard C++ string range integration, range literals, std::hash
 * support for string slices and others.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_STRING_HH
#define OSTD_STRING_HH

#include <cstdint>
#include <cstddef>
#include <cctype>
#include <string>
#include <string_view>
#include <type_traits>
#include <functional>
#include <utility>
#include <vector>
#include <stdexcept>

#include <ostd/range.hh>
#include <ostd/algorithm.hh>

namespace ostd {

/** @addtogroup Strings
 * @{
 */

template<typename T> struct basic_char_range;

/** @brief A mutable slice over `char`. */
using char_range = basic_char_range<char>;

/** @brief An immutable slice over `char`.
 *
 * This is used in most libostd APIs that read strings. More or less
 * anything is convertible to it, including mutable slices, so it's
 * a perfect fit as long as modifications are not necessary.
 */
using string_range = basic_char_range<char const>;

namespace utf {
    /* @brief Get the Unicode code point for a multibyte sequence.
     *
     * The string is advanced past the UTF-8 character in the front.
     * If the decoding fails, `false` is returned, otherwise it's `true`.
     */
    bool codepoint(string_range &r, char32_t &ret);

    /* @brief Get the number of Unicode code points in a string.
     *
     * This function keeps reading Unicode code points while it can and
     * once it can't it returns the number of valid ones with the rest
     * of the input string range being in `cont`. That means if the entire
     * string is a valid UTF-8 string, `cont` will be empty, otherwise it
     * will begin at the first invalid UTF-8 code point.
     *
     * If you're sure the string is valid or you don't need to handle the
     * error, you can use the more convenient overload below.
     */
    std::size_t length(string_range r, string_range &cont);

    /* @brief Get the number of Unicode code points in a valid UTF-8 string.
     *
     * If an invalid UTF-8 sequence is encountered, it returns the length
     * until that sequence.
     *
     * If you need to get the continuation string, use the general
     * error-handling overload of the function.
     */
    std::size_t length(string_range r);
} /* namespace utf */

/** @brief A string slice type.
 *
 * This is a contiguous range over a character type. The character type
 * can be any of the standard character types, of any size - for example
 * you would use `char32_t` to represent UTF-32 slices. The std::char_traits
 * structure is used for the basic string operations where possible.
 *
 * The range is mutable, i.e. it implements the output range interface.
 */
template<typename T>
struct basic_char_range: input_range<basic_char_range<T>> {
    using range_category = contiguous_range_tag;
    using value_type     = T;
    using reference      = T &;
    using size_type      = std::size_t;

private:
    using TR = std::char_traits<std::remove_const_t<T>>;
    struct nat {};

public:
    /** @brief Constructs an empty slice. */
    basic_char_range() noexcept: p_beg(nullptr), p_end(nullptr) {};

    /** @brief Constructs a slice from two pointers.
     *
     * The first pointer is the beginning of the slice
     * and the second pointer is just past the end.
     */
    basic_char_range(value_type *beg, value_type *end) noexcept:
        p_beg(beg), p_end(end)
    {}

    /** @brief Constructs an empty slice. */
    basic_char_range(std::nullptr_t) noexcept:
        p_beg(nullptr), p_end(nullptr)
    {}

    /** @brief Constructs a slice from a pointer or a static array.
     *
     * This constructor handles two cases. The input must be convertible
     * to `T *`, if it's not, this constructor is not enabled. Effectively,
     * if the input is a static array of `T`, the entire array is used to
     * create the slice, minus the potential zero at the end. If there is
     * no zero at the end, nothing is removed and the array is used whole.
     * If the input is not an array, the size is checked at runtime.
     */
    template<typename U>
    basic_char_range(U &&beg, std::enable_if_t<
        std::is_convertible_v<U, value_type *>, nat
    > = nat{}) noexcept: p_beg(beg) {
        if constexpr(std::is_array_v<std::remove_reference_t<U>>) {
            std::size_t N = std::extent_v<std::remove_reference_t<U>>;
            p_end = beg + N - (beg[N - 1] == '\0');
        } else {
            p_end = beg + (beg ? TR::length(beg) : 0);
        }
    }

    /** @brief Constructs a slice from an std::basic_string.
     *
     * This uses the string's data to construct a matching slice.
     */
    template<typename STR, typename A>
    basic_char_range(
        std::basic_string<std::remove_const_t<value_type>, STR, A> const &s
    ) noexcept:
        p_beg(s.data()), p_end(s.data() + s.size())
    {}

    /** @brief Constructs a slice from a different but compatible slice.
     *
     * The other slice can use any traits type, but a pointer to the
     * other slice's value type must be convertible to a pointer to
     * the new slice's value type, otherwise the constructor will not
     * be enabled.
     */
    template<typename U, typename = std::enable_if_t<
        std::is_convertible_v<U *, value_type *>
    >>
    basic_char_range(basic_char_range<U> const &v) noexcept:
        p_beg(&v[0]), p_end(&v[v.size()])
    {}

    /** @brief Slices are arbitrarily copy constructible. */
    basic_char_range &operator=(basic_char_range const &v) noexcept {
        p_beg = v.p_beg; p_end = v.p_end; return *this;
    }

    /** @brief Assigns the slice's data from a matching std::basic_string.
     *
     * The string does not have to be using a matching traits type.
     */
    template<typename STR, typename A>
    basic_char_range &operator=(
        std::basic_string<value_type, STR, A> const &s
    ) noexcept {
        p_beg = s.data(); p_end = s.data() + s.size(); return *this;
    }

    /** @brief Assigns the slice's data from a pointer.
     *
     * The data pointed to by the argument must be zero terminated.
     */
    basic_char_range &operator=(value_type *s) noexcept {
        p_beg = s; p_end = s + (s ? TR::length(s) : 0); return *this;
    }

    /** @brief Checks if the slice is empty. */
    bool empty() const noexcept { return p_beg == p_end; }

    /** @brief Pops the first character out of the slice.
     *
     * This is bounds checked, std::out_of_range is thrown when
     * slice was already empty before popping out the character.
     * No changes are done to the slice if it throws.
     *
     * @throws std::out_of_range when empty.
     *
     * @see front(), pop_back()
     */
    void pop_front() {
        if (p_beg == p_end) {
            throw std::out_of_range{"pop_front on empty range"};
        }
        ++p_beg;
    }

    /** @brief Gets a reference to the first character.
     *
     * The behavior is undefined when the slice is empty.
     *
     * @see back(), pop_front()
     */
    reference front() const noexcept { return *p_beg; }

    /** @brief Pops the last character out of the slice.
     *
     * This is bounds checked, std::out_of_range is thrown when
     * slice was already empty before popping out the character.
     * No changes are done to the slice if it throws.
     *
     * @throws std::out_of_range when empty.
     *
     * @see back(), pop_front()
     */
    void pop_back() {
        if (p_beg == p_end) {
            throw std::out_of_range{"pop_back on empty range"};
        }
        --p_end;
    }

    /** @brief Gets a reference to the last character.
     *
     * The behavior is undefined when the slice is empty.
     *
     * @see front(), pop_back()
     */
    reference back() const noexcept { return *(p_end - 1); }

    /** @brief Gets the number of value_type in the slice. */
    size_type size() const noexcept { return p_end - p_beg; }

    /** @brief Creates a sub-slice of the slice.
     *
     * Behavior is undefined if `start` and `end` are not within the
     * slice's bounds. There is no bound checking done in this call.
     * It's also undefined if the first argument is larger than the
     * second argument.
     */
    basic_char_range slice(size_type start, size_type end) const noexcept {
        return basic_char_range(p_beg + start, p_beg + end);
    }

    /** @brief Creates a sub-slice of the slice until the end.
     *
     * Equivalent to slice(size_type, size_type) with `size()` as
     * the second argument. The first argument must be within the
     * slice's boundaries otherwis the behavior is undefined.
     */
    basic_char_range slice(size_type start) const noexcept {
        return slice(start, size());
    }

    /** @brief Gets a reference to a character within the slice.
     *
     * The behavior is undefined if the index is not within the bounds.
     */
    reference operator[](size_type i) const noexcept { return p_beg[i]; }

    /** @brief Writes a character at the beginning and pops it out.
     *
     * @throws std::out_of_range when empty.
     */
    void put(value_type v) {
        if (p_beg == p_end) {
            throw std::out_of_range{"put into an empty range"};
        }
        *(p_beg++) = v;
    }

    /** @brief Gets the pointer to the beginning. */
    value_type *data() noexcept { return p_beg; }

    /** @brief Gets the pointer to the beginning. */
    value_type const *data() const noexcept { return p_beg; }

    /** @brief Compares two slices.
     *
     * This works similarly to the C function `strcmp` or the `compare`
     * method of std::char_traits, but does not depend on the strings
     * to be terminated.
     *
     * If this slice is empty and the other is not, this method returns
     * -1. If it's the other way around, it returns 1. If both are empty,
     * 0 is returned. Otherwise, the `compare` method of std::char_traits
     * is used to compare the data, using the smaller of the lengths as the
     * count.
     *
     * It is not a part of the range interface, just the string slice
     * interface.
     *
     * @see case_compare()
     */
    int compare(basic_char_range<value_type const> s) const noexcept {
        size_type s1 = size(), s2 = s.size();
        int ret;
        if (!s1 || !s2) {
            goto diffsize;
        }
        if ((ret = TR::compare(data(), s.data(), std::min(s1, s2)))) {
            return ret;
        }
diffsize:
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    /** @brief Compares two slices in a case insensitive manner.
     *
     * Lexicographically compares the strings like compare(), but in
     * a case insensitive way. The std::toupper() function is used to
     * convert the characters to uppercase when comparing.
     *
     * Returns a negative value when this slice is less than the other
     * slice and a positive value when the other way around. Zero is
     * returned when they're equal.
     */
    int case_compare(basic_char_range<value_type const> s) const noexcept {
        size_type s1 = size(), s2 = s.size();
        for (size_type i = 0, ms = std::min(s1, s2); i < ms; ++i) {
            int d = std::toupper(p_beg[i]) - std::toupper(s[i]);
            if (d) {
                return d;
            }
        }
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    /** @brief Implicitly converts a string slice to std::basic_string_view.
     *
     * String views represent more or less the same thing but they're always
     * immutable. This simple conversion allows usage of string slices on
     * any API that uses either strings or string view, as well as construct
     * strings and string views out of slices.
     */
    operator std::basic_string_view<std::remove_cv_t<value_type>>()
        const noexcept
    {
        return std::basic_string_view<std::remove_cv_t<T>>{data(), size()};
    }

private:
    T *p_beg, *p_end;
};

/* comparisons between ranges */

/** @brief Like `!lhs.compare(rhs)`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator==(
    basic_char_range<T> lhs, basic_char_range<U> rhs
) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator!=(
    basic_char_range<T> lhs, basic_char_range<U> rhs
) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<(
    basic_char_range<T> lhs, basic_char_range<U> rhs
) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>(
    basic_char_range<T> lhs, basic_char_range<U> rhs
) noexcept {
    return lhs.compare(rhs) > 0;
}

/** @brief Like `lhs.compare(rhs) <= 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<=(
    basic_char_range<T> lhs, basic_char_range<U> rhs
) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>=(
    basic_char_range<T> lhs, basic_char_range<U> rhs
) noexcept {
    return lhs.compare(rhs) >= 0;
}

/* comparisons between ranges and char arrays */

/** @brief Like `!lhs.compare(rhs)`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator==(basic_char_range<T> lhs, U *rhs) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator!=(basic_char_range<T> lhs, U *rhs) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<(basic_char_range<T> lhs, U *rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>(basic_char_range<T> lhs, U *rhs) noexcept {
    return lhs.compare(rhs) > 0;
}

/** @brief Like `lhs.compare(rhs) <= 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<=(basic_char_range<T> lhs, U *rhs) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>=(basic_char_range<T> lhs, U *rhs) noexcept {
    return lhs.compare(rhs) >= 0;
}

/** @brief Like `!rhs.compare(lhs)`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator==(U *lhs, basic_char_range<T> rhs) noexcept {
    return !rhs.compare(lhs);
}

/** @brief Like `rhs.compare(lhs)`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator!=(U *lhs, basic_char_range<T> rhs) noexcept {
    return rhs.compare(lhs);
}

/** @brief Like `rhs.compare(lhs) > 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<(U *lhs, basic_char_range<T> rhs) noexcept {
    return rhs.compare(lhs) > 0;
}

/** @brief Like `rhs.compare(lhs) < 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>(U *lhs, basic_char_range<T> rhs) noexcept {
    return rhs.compare(lhs) < 0;
}

/** @brief Like `rhs.compare(lhs) >= 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<=(U *lhs, basic_char_range<T> rhs) noexcept {
    return rhs.compare(lhs) >= 0;
}

/** @brief Like `rhs.compare(lhs) <= 0`. */
template<typename T, typename U>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>=(U *lhs, basic_char_range<T> rhs) noexcept {
    return rhs.compare(lhs) <= 0;
}

/* comparisons between ranges and stdlib strings */

/** @brief Like `!lhs.compare(rhs)`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator==(
    basic_char_range<T> lhs, std::basic_string<U, TR, A> const &rhs
) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator!=(
    basic_char_range<T> lhs, std::basic_string<U, TR, A> const &rhs
) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<(
    basic_char_range<T> lhs, std::basic_string<U, TR, A> const &rhs
) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>(
    basic_char_range<T> lhs, std::basic_string<U, TR, A> const &rhs
) noexcept {
    return lhs.compare(rhs) > 0;
}

/** @brief Like `lhs.compare(rhs) <= 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<=(
    basic_char_range<T> lhs, std::basic_string<U, TR, A> const &rhs
) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>=(
    basic_char_range<T> lhs, std::basic_string<U, TR, A> const &rhs
) noexcept {
    return lhs.compare(rhs) >= 0;
}

/** @brief Like `!rhs.compare(lhs)`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator==(
    std::basic_string<U, TR, A> const &lhs, basic_char_range<T> rhs
) noexcept {
    return !rhs.compare(lhs);
}

/** @brief Like `rhs.compare(lhs)`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator!=(
    std::basic_string<U, TR, A> const &lhs, basic_char_range<T> rhs
) noexcept {
    return rhs.compare(lhs);
}

/** @brief Like `rhs.compare(lhs) > 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<(
    std::basic_string<U, TR, A> const &lhs, basic_char_range<T> rhs
) noexcept {
    return rhs.compare(lhs) > 0;
}

/** @brief Like `rhs.compare(lhs) < 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>(
    std::basic_string<U, TR, A> const &lhs, basic_char_range<T> rhs
) noexcept {
    return rhs.compare(lhs) < 0;
}

/** @brief Like `rhs.compare(lhs) >= 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator<=(
    std::basic_string<U, TR, A> const &lhs, basic_char_range<T> rhs
) noexcept {
    return rhs.compare(lhs) >= 0;
}

/** @brief Like `rhs.compare(lhs) <= 0`. */
template<typename T, typename TR, typename U, typename A>
inline std::enable_if_t<
    std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>, bool
> operator>=(
    std::basic_string<U, TR, A> const &lhs, basic_char_range<T> rhs
) noexcept {
    return rhs.compare(lhs) <= 0;
}

/** @brief Checks if a string slice starts with another slice. */
inline bool starts_with(string_range a, string_range b) noexcept {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

/** @brief Mutable range integration for std::basic_string.
 *
 * The range type used for mutable string references
 * is an ostd::basic_char_range with mutable values.
 */
template<typename T, typename TR, typename A>
struct ranged_traits<std::basic_string<T, TR, A>> {
    /** @brief The range type. */
    using range = basic_char_range<T>;

    /** @brief Creates a range. */
    static range iter(std::basic_string<T, TR, A> &v) noexcept {
        return range{v.data(), v.data() + v.size()};
    }
};

/** @brief Immutable range integration for std::basic_string.
 *
 * The range type used for immutable string references
 * is an ostd::basic_char_range with immutable values.
 */
template<typename T, typename TR, typename A>
struct ranged_traits<std::basic_string<T, TR, A> const> {
    /** @brief The range type. */
    using range = basic_char_range<T const>;

    /** @brief Creates a range. */
    static range iter(std::basic_string<T, TR, A> const &v) noexcept {
        return range{v.data(), v.data() + v.size()};
    }
};

/* more UTF utilities beyond basic API */

namespace utf {

/** @addtogroup Strings
 * @{
 */

    /** @brief Thrown on UTF-8 decoding failure. */
    struct utf_error: std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    namespace detail {
        struct codepoint_range: input_range<codepoint_range> {
            using range_category = forward_range_tag;
            using value_type = char32_t;
            using reference = char32_t;
            using size_type = std::size_t;

            codepoint_range() = delete;
            codepoint_range(string_range r): p_range(r) {
                if (r.empty()) {
                    p_current = -1;
                } else {
                    advance();
                }
            }

            bool empty() const { return (p_current < 0); }

            void pop_front() {
                if (p_current > 0 && p_range.empty()) {
                    p_current = -1;
                    return;
                }
                advance();
            }

            char32_t front() const {
                return p_current;
            }

        private:
            void advance() {
                if (char32_t ret; !codepoint(p_range, ret)) {
                    /* range is unchanged */
                    p_current = -1;
                    throw utf_error{"UTF-8 decoding failed"};
                } else {
                    p_current = ret;
                }
            }

            string_range p_range;
            std::int32_t p_current;
        };
    } /* namespace detail */

    /** @brief Iterate over the code points of a string.
     *
     * The resulting range is ostd::forward_range_tag. The range will
     * contain the code points of the given string. On error, which may
     * be during any string advancement (the constructor or `pop_front()`),
     * an ostd::utf_error is raised.
     */
    inline auto iter_codes(string_range r) {
        return detail::codepoint_range{r};
    }

/** @} */

} /* namespace utf */

/* string literals */

inline namespace literals {
inline namespace string_literals {

/** @addtogroup Strings
 * @{
 */

    /** @brief A custom literal for string ranges.
     *
     * You need to enable this explicitly by using this namespace. It's
     * not enabled by default to ensure compatibility with existing code.
     */
    inline string_range operator "" _sr(char const *str, std::size_t len)
        noexcept
    {
        return string_range(str, str + len);
    }

/** @} */

}
}

/** @} */

} /* namespace ostd */

namespace std {

/** @addtogroup Strings
 * @{
 */

/** @brief Standard std::hash integration for string slices.
 *
 * This integrates all possible slice types with standard hashing.
 * It uses the hashing used for matching std::basic_string_view,
 * so the algorithm (and thus result) will always match standard strings.
 */
template<typename T>
struct hash<ostd::basic_char_range<T>> {
    std::size_t operator()(ostd::basic_char_range<T> const &v)
        const noexcept
    {
        return hash<std::basic_string_view<std::remove_const_t<T>>>{}(v);
    }
};

/** @} */

}

#endif

/** @} */
