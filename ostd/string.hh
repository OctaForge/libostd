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
#include <cstring>
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

static_assert(
    (sizeof(wchar_t) == sizeof(char)) ||
    (sizeof(wchar_t) == sizeof(char16_t)) ||
    (sizeof(wchar_t) == sizeof(char32_t)),
    "wchar_t must correspond to either char, char16_t or char32_t"
);

namespace detail {
    std::size_t tstrlen(char const *p) noexcept;
    std::size_t tstrlen(char16_t const *p) noexcept;
    std::size_t tstrlen(char32_t const *p) noexcept;
    std::size_t tstrlen(wchar_t const *p) noexcept;
}

/** @addtogroup Strings
 * @{
 */

/** @brief A string slice type.
 *
 * This is a contiguous range over a character type. The character type
 * can be any of the standard character types, of any size - for example
 * you would use `char32_t` to represent UTF-32 slices.
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
    struct nat {};

public:
    /** @brief Constructs an empty slice. */
    basic_char_range() noexcept: p_beg(nullptr), p_end(nullptr) {}

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

    /** @brief Slices are arbitrarily copy constructible. */
    basic_char_range(basic_char_range const &v) noexcept:
        p_beg(v.p_beg), p_end(v.p_end)
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
            p_end = beg + (beg ? detail::tstrlen(beg) : 0);
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
     * A pointer to the other slice's value type must be convertible to
     * a pointer to the new slice's value type, otherwise the constructor
     * will not be enabled.
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

    /** @brief Assigns the slice's data from a matching std::basic_string. */
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
        p_beg = s; p_end = s + (s ? detail::tstrlen(s) : 0); return *this;
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
    size_type size() const noexcept { return size_type(p_end - p_beg); }

    /** @brief Gets the number of code points in the slice.
     *
     * Effectively the same as utf::length().
     */
    inline size_type length() const noexcept;

    /** @brief Gets the number of code points in the slice.
     *
     * Effectively the same as utf::length().
     */
    inline size_type length(basic_char_range &cont) const noexcept;

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
     * It performs an ordinary lexicographical comparison, the values
     * are compared and the first string to have a lesser value is
     * considered lexicographically less. If they are equal up to a
     * point but one of them terminates early, it's also less.
     *
     * If the `this` slice is the lesser one, a negative value is
     * returned. If they are equal (if they're both zero length,
     * it counts as equal) then `0` is returned. Otherwise, a
     * positive value is returned.
     *
     * This works with the slice's native unit values, i.e. bytes
     * for UTF-8, `char16_t` for UTF-16 and `char32_t` for UTF-32.
     * These units are compared by getting the difference between
     * them (i.e. `this[index] - other[index]`).
     *
     * It is not a part of the range interface, just the string slice
     * interface.
     *
     * @see case_compare()
     */
    int compare(basic_char_range<value_type const> s) const noexcept {
        size_type s1 = size(), s2 = s.size();
        for (size_type i = 0, ms = std::min(s1, s2); i < ms; ++i) {
            int d = int(p_beg[i]) - int(s[i]);
            if (d) {
                return d;
            }
        }
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    /** @brief Compares two slices in a case insensitive manner.
     *
     * Works exactly the same as compare(), but in a case insensitive
     * way, i.e. it lowercases the characters and compares them after
     * that.
     *
     * For UTF-8, it decodes the string on the fly, then lowercases the
     * decoded code points and uses their difference (without encoding
     * them back). If the decoding fails, the failing code unit is used
     * as-is, so this function never fails. Identical treatment is given
     * to UTF-16.
     */
    inline int case_compare(basic_char_range<value_type const> s) const noexcept;

    /** @brief Iterate over the Unicode units of the given type.
     *
     * Like utf::iter_u().
     */
    template<typename C>
    inline auto iter_u() const;

    /** @brief Iterate over the Unicode units of the size in bits.
     *
     * The type maps to `char` for 8, `char16_t` for 16 and `char32_t`
     * for 32, or UTF-8, UTF-16 and UTF-32.
     *
     * Like utf::iter_u().
     */
    template<std::size_t N>
    inline auto iter_u() const;

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

/** @brief A mutable slice over `char`. */
using char_range = basic_char_range<char>;

/** @brief A mutable slice over `wchar_t`. */
using wchar_range = basic_char_range<wchar_t>;

/** @brief A mutable slice over `char16_t`. */
using char16_range = basic_char_range<char16_t>;

/** @brief A mutable slice over `char32_t`. */
using char32_range = basic_char_range<char32_t>;

/** @brief An immutable slice over `char`.
 *
 * This is used in most libostd APIs that read strings. More or less
 * anything is convertible to it, including mutable slices, so it's
 * a perfect fit as long as modifications are not necessary.
 */
using string_range = basic_char_range<char const>;

/** @brief An immutable slice over `wchar_t`.
 *
 * Included primarily for compatibility with other APIs.
 */
using wstring_range = basic_char_range<wchar_t const>;

/** @brief An immutable slice over `char16_t`.
 *
 * Included for basic UTF-16 compatibility.
 */
using u16string_range = basic_char_range<char16_t const>;

/** @brief An immutable slice over `char32_t`.
 *
 * Can represent UTF-32 strings.
 */
using u32string_range = basic_char_range<char32_t const>;

/* comparisons between utf-8 ranges */

/** @brief Like `!lhs.compare(rhs)`. */
inline bool operator==(string_range lhs, string_range rhs) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
inline bool operator!=(string_range lhs, string_range rhs) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
inline bool operator<(string_range lhs, string_range rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
inline bool operator>(string_range lhs, string_range rhs) noexcept {
    return lhs.compare(rhs) > 0;
}
/** @brief Like `lhs.compare(rhs) <= 0`. */
inline bool operator<=(string_range lhs, string_range rhs) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
inline bool operator>=(string_range lhs, string_range rhs) noexcept {
    return lhs.compare(rhs) >= 0;
}

/* comparisons between utf-16 ranges */

/** @brief Like `!lhs.compare(rhs)`. */
inline bool operator==(u16string_range lhs, u16string_range rhs) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
inline bool operator!=(u16string_range lhs, u16string_range rhs) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
inline bool operator<(u16string_range lhs, u16string_range rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
inline bool operator>(u16string_range lhs, u16string_range rhs) noexcept {
    return lhs.compare(rhs) > 0;
}
/** @brief Like `lhs.compare(rhs) <= 0`. */
inline bool operator<=(u16string_range lhs, u16string_range rhs) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
inline bool operator>=(u16string_range lhs, u16string_range rhs) noexcept {
    return lhs.compare(rhs) >= 0;
}

/* comparisons between utf-32 ranges */

/** @brief Like `!lhs.compare(rhs)`. */
inline bool operator==(u32string_range lhs, u32string_range rhs) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
inline bool operator!=(u32string_range lhs, u32string_range rhs) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
inline bool operator<(u32string_range lhs, u32string_range rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
inline bool operator>(u32string_range lhs, u32string_range rhs) noexcept {
    return lhs.compare(rhs) > 0;
}
/** @brief Like `lhs.compare(rhs) <= 0`. */
inline bool operator<=(u32string_range lhs, u32string_range rhs) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
inline bool operator>=(u32string_range lhs, u32string_range rhs) noexcept {
    return lhs.compare(rhs) >= 0;
}

/* comparisons between wide ranges */

/** @brief Like `!lhs.compare(rhs)`. */
inline bool operator==(wstring_range lhs, wstring_range rhs) noexcept {
    return !lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs)`. */
inline bool operator!=(wstring_range lhs, wstring_range rhs) noexcept {
    return lhs.compare(rhs);
}

/** @brief Like `lhs.compare(rhs) < 0`. */
inline bool operator<(wstring_range lhs, wstring_range rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

/** @brief Like `lhs.compare(rhs) > 0`. */
inline bool operator>(wstring_range lhs, wstring_range rhs) noexcept {
    return lhs.compare(rhs) > 0;
}
/** @brief Like `lhs.compare(rhs) <= 0`. */
inline bool operator<=(wstring_range lhs, wstring_range rhs) noexcept {
    return lhs.compare(rhs) <= 0;
}

/** @brief Like `lhs.compare(rhs) >= 0`. */
inline bool operator>=(wstring_range lhs, wstring_range rhs) noexcept {
    return lhs.compare(rhs) >= 0;
}

/** @brief Checks if a string slice starts with another slice. */
inline bool starts_with(string_range a, string_range b) noexcept {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

/** @brief Checks if a string slice starts with another slice. */
inline bool starts_with(u16string_range a, u16string_range b) noexcept {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

/** @brief Checks if a string slice starts with another slice. */
inline bool starts_with(u32string_range a, u32string_range b) noexcept {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

/** @brief Checks if a string slice starts with another slice. */
inline bool starts_with(wstring_range a, wstring_range b) noexcept {
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

    /** @brief A Unicode type of the same size as `wchar_t`.
     *
     * This can be an alias to either `char32_t`, `char16_t` or `char`
     * representing UTF-32, UTF-16 or UTF-8 respectively. It represents
     * a Unicode character type that in a platform specific way represents
     * one of the 3 encodings. It follows the same order so if `wchar_t`
     * can fit UTF-32, it's UTF-32, otherwise it tries UTF-16 and UTF-8
     * as the next best thing.
     *
     * On most platforms and all platforms that we care about, this does
     * the right thing; UTF-32 on most Unix-like systems (with 32-bit
     * `wchar_t`), UTF-16 on Windows (with 16-bit `wchar_t`).
     */
    using wchar_fixed_t = std::conditional_t<
        sizeof(wchar_t) == sizeof(char32_t),
        char32_t,
        std::conditional_t<
            sizeof(wchar_t) == sizeof(char16_t),
            char16_t,
            char
        >
    >;

    namespace detail {
        template<typename C>
        struct max_units_base;

        template<>
        struct max_units_base<char32_t> {
            static constexpr std::size_t const value = 1;
            static constexpr std::size_t const bits  = 32;
        };

        template<>
        struct max_units_base<char16_t> {
            static constexpr std::size_t const value = 2;
            static constexpr std::size_t const bits  = 16;
        };

        template<>
        struct max_units_base<char> {
            static constexpr std::size_t const value = 4;
            static constexpr std::size_t const bits  = 8;
        };

        template<>
        struct max_units_base<wchar_t>: max_units_base<wchar_fixed_t> {};
    } /* namespace detail */

    /** @brief The maximum number of code units to represent a code point.
     *
     * The allowed input types are `char`, `char16_t`, `char32_t`, `wchar_t`.
     * The result is 4, 2, 1 and platform-dependent (usually 1 or 2). It
     * makes use of utf::wchar_fixed_t for `wchar_t`.
     */
    template<typename C>
    static inline constexpr std::size_t const max_units =
        detail::max_units_base<C>::value;

    /** @brief The number of bits for a code unit type.
     *
     * Keep in mind that this does not represent the *actual* number of bits
     * in the type, just the smallest number of bits for an integer type that
     * can hold all code units of the encoding. So for `char` it's 8, for
     * `char16_t` it's 16, for `char32_t` it's 32, for `wchar_t` it's
     * platform defined.
     */
    template<typename C>
    static inline constexpr std::size_t const unit_bits =
        detail::max_units_base<C>::bits;

    namespace detail {
        template<std::size_t N>
        struct unicode_t_base;

        template<>
        struct unicode_t_base<32> {
            using type = char32_t;
        };

        template<>
        struct unicode_t_base<16> {
            using type = char16_t;
        };

        template<>
        struct unicode_t_base<8> {
            using type = char;
        };
    }

    /** @brief The UTF unit type according to utf::unit_bits.
     *
     * For 8 this will be `char`, for 16 `char16_t`, for 32 `char32_t`.
     */
    template<std::size_t N>
    using unicode_t = typename detail::unicode_t_base<N>::type;

    /** @brief A normalized Unicode type for the given character type.
     *
     * This will map to itself for all character types but `wchar_t`,
     * which maps to utf::wchar_fixed_t.
     */
    template<typename T>
    using unicode_base_t = unicode_t<unit_bits<T>>;

    /** @brief A constant to tell if `wchar_t` is UTF-32.
     *
     * No actual checks are performed, utf::wchar_fixed_t is used to tell.
     */
    static inline constexpr bool const is_wchar_u32 =
        std::is_same_v<wchar_fixed_t, char32_t>;

    /** @brief A constant to tell if `wchar_t` is UTF-16.
     *
     * No actual checks are performed, utf::wchar_fixed_t is used to tell.
     */
    static inline constexpr bool const is_wchar_u16 =
        std::is_same_v<wchar_fixed_t, char16_t>;

    /** @brief A constant to tell if `wchar_t` is UTF-8.
     *
     * No actual checks are performed, utf::wchar_fixed_t is used to tell.
     */
    static inline constexpr bool const is_wchar_u8 =
        std::is_same_v<wchar_fixed_t, char>;

    namespace detail {
        template<typename C>
        struct is_character_base {
            static constexpr bool const value = false;
        };

        template<>
        struct is_character_base<char> {
            static constexpr bool const value = true;
        };

        template<>
        struct is_character_base<char16_t> {
            static constexpr bool const value = true;
        };

        template<>
        struct is_character_base<char32_t> {
            static constexpr bool const value = true;
        };

        template<>
        struct is_character_base<wchar_t> {
            static constexpr bool const value = true;
        };
    }

    /** @brief Check whether the input type is a character type.
     *
     * For `char`, `char16_t`, `char32_t` and `wchar_t` this is true,
     * otherwise it's false.
     */
    template<typename C>
    static inline constexpr bool const is_character =
        detail::is_character_base<C>::value;

    /** @brief The maximum value a Unicode code point can have. */
    static inline constexpr char32_t const max_unicode = 0x10FFFF;

    /** @brief Thrown on UTF-8 decoding failure. */
    struct utf_error: std::runtime_error {
        using std::runtime_error::runtime_error;
        /* empty, for vtable placement */
        virtual ~utf_error();
    };

    /* @brief Get the Unicode code point for a UTF-8 sequence.
     *
     * The string is advanced past the Unicode character in the front.
     * If the decoding fails, `false` is returned, otherwise it's `true`.
     */
    bool decode(string_range &r, char32_t &ret) noexcept;

    /* @brief Get the Unicode code point for a UTF-16 sequence.
     *
     * The string is advanced past the Unicode character in the front.
     * If the decoding fails, `false` is returned, otherwise it's `true`.
     */
    bool decode(u16string_range &r, char32_t &ret) noexcept;

    /* @brief Get the Unicode code point from a UTF-32 string.
     *
     * The string is advanced by one. It can also fail if utf::isvalid()
     * returns `false` for the front character, in which case the string
     * will not be advanced.
     */
     bool decode(u32string_range &r, char32_t &ret) noexcept;

    /* @brief Get the Unicode code point for a wide Unicode char/sequence.
     *
     * The input is treated as either UTF-8, UTF-16 or UTF-32 depending
     * on the size of the wide character. Typically, it will be UTF-16
     * on Windows and UTF-32 on Unix-like systems, with UTF-32 taking
     * priority (on systems where two or more of the types are the same
     * size).
     */
    bool decode(wstring_range &r, char32_t &ret) noexcept;

    namespace detail {
        std::size_t encode(
            char (&ret)[4], char32_t ch
        ) noexcept;
        std::size_t encode(
            char16_t (&ret)[2], char32_t ch
        ) noexcept;
    }

    /* @brief Encode a Unicode code point in the given encoding.
     *
     * The encoding is specified by the template parameter `C` which
     * can be one of the character types (utf::is_character), the
     * encoding used is picked based on utf::max_units.
     *
     * The return value is the number of values written into `sink`.
     * If none were written, the encoding failed.
     *
     * If your input is a string and you want to advance it, use the
     * utf::encode(R, basic_char_range) variant.
     */
    template<typename C, typename R>
    inline std::size_t encode(R &sink, char32_t ch) {
        std::size_t ret;
        if constexpr(max_units<C> == 1) {
            sink.put(C(ch));
            ret = 1;
        } else {
            unicode_base_t<C> buf[max_units<C>];
            ret = detail::encode(buf, ch);
            for (std::size_t i = 0; i < ret; ++i) {
                sink.put(C(buf[i]));
            }
        }
        return ret;
    }

    /* @brief Encode a Unicode code point from a string in the given encoding.
     *
     * The encoding is specified by the template parameter `C` which
     * can be one of the character types (utf::is_character), the
     * encoding used is picked based on utf::max_units.
     *
     * Unlike utf::encode(R, char32_t), this takes a string as a second
     * input and the string can be in any UTF encoding and use any of the
     * available character types. The function advances the string by one
     * code point, which may mean multiple values.
     *
     * The return value is the number of values written into `sink`.
     * If none were written, the encoding failed and the string is not
     * advanced.
     */
    template<typename C, typename OR, typename IR>
    inline std::size_t encode(OR &sink, IR &r) {
        using IC = std::remove_const_t<range_value_t<IR>>;
        if constexpr(max_units<IC> == 1) {
            std::size_t n = 0;
            if (!r.empty() && (n = utf::encode<C>(sink, char32_t(r.front())))) {
                r.pop_front();
            }
            return n;
        } else if constexpr(max_units<IC> == max_units<C>) {
            /* FIXME: advance by a whole character always */
            if (!r.empty()) {
                sink.put(C(r.front()));
                r.pop_front();
                return 1;
            }
        } else {
            auto rr = r;
            if (char32_t ch; utf::decode(rr, ch)) {
                if (std::size_t n; (n = utf::encode<C>(sink, ch))) {
                    r = rr;
                    return n;
                }
            }
        }
        return 0;
    }

    /** @brief Encode a Unicode code point from a string in the given encoding.
     *
     * This is the same as utf::encode() but takes the number of bits as in
     * utf::unit_bits as an input instead of a type, mapping 8 to `char`,
     * 16 to `char16_t` and 32 to `char32_t`.
     */
    template<std::size_t N, typename OR, typename IR>
    inline std::size_t encode(OR &sink, IR &r) {
        return encode<unicode_t<N>>(sink, r);
    }

    /** @brief Write a Unicode replacement character into the sink.
     *
     * Depending on the type input, this encodes the replacement character
     * into the sink as either UTF-8, UTF-16 or UTF-32. ALl 4 character types
     * are allowed.
     */
    template<typename C, typename R>
    inline std::size_t replace(R &sink) {
        if constexpr(max_units<C> > 2) {
            sink.put(range_value_t<R>(0xEF));
            sink.put(range_value_t<R>(0xBF));
            sink.put(range_value_t<R>(0xBD));
            return 3;
        } else {
            sink.put(range_value_t<R>(0xFFFD));
        }
        return 1;
    }

    /** @brief Write a Unicode replacement character into the sink.
     *
     * This is the same as utf::replace() but takes the number of bits as in
     * utf::unit_bits as an input instead of a type, mapping 8 to `char`,
     * 16 to `char16_t` and 32 to `char32_t`.
     */
    template<std::size_t N, typename R>
    inline std::size_t replace(R &sink) {
        return replace<unicode_t<N>>(sink);
    }

    /** @brief Get the number of Unicode code points in a string.
     *
     * This function keeps reading Unicode code points while it can and
     * once it can't it returns the number of valid ones with the rest
     * of the input string range being in `cont`. That means if the entire
     * string is a valid UTF-8 string, `cont` will be empty, otherwise it
     * will begin at the first invalid UTF-8 code unit.
     *
     * If you're sure the string is valid or you don't need to handle the
     * error, you can use the more convenient overload below.
     */
    std::size_t length(string_range r, string_range &cont) noexcept;

    /** @brief Get the number of Unicode code points in a string.
     *
     * This function keeps reading Unicode code points while it can and
     * once it can't it returns the number of valid ones with the rest
     * of the input string range being in `cont`. That means if the entire
     * string is a valid UTF-16 string, `cont` will be empty, otherwise it
     * will begin at the first invalid UTF-16 code unit.
     *
     * If you're sure the string is valid or you don't need to handle the
     * error, you can use the more convenient overload below.
     */
    std::size_t length(u16string_range r, u16string_range &cont) noexcept;

    /** @brief Get the number of Unicode code points in a string.
     *
     * This function keeps reading Unicode code points while it can and
     * once it can't it returns the number of valid ones with the rest
     * of the input string range being in `cont`. That means if the entire
     * string is a valid UTF-32 string, `cont` will be empty, otherwise it
     * will begin at the first invalid code point.
     *
     * If you're sure the string is valid or you don't need to handle the
     * error, you can use the more convenient overload below.
     */
    std::size_t length(u32string_range r, u32string_range &cont) noexcept;

    /** @brief Get the number of Unicode code points in a string.
     *
     * This function keeps reading Unicode code points while it can and
     * once it can't it returns the number of valid ones with the rest
     * of the input string range being in `cont`. That means if the entire
     * string is a valid Unicode wide string, `cont` will be empty,
     * otherwise it will begin at the first invalid code unit.
     *
     * If you're sure the string is valid or you don't need to handle the
     * error, you can use the more convenient overload below.
     *
     * The behavior of this function is platform dependent as wide
     * characters represent different things on different systems.
     */
    std::size_t length(wstring_range r, wstring_range &cont) noexcept;

    /** @brief Get the number of Unicode code points in a UTF-8 string.
     *
     * If an invalid UTF-8 sequence is encountered, it's considered
     * 1 character and therefore the resulting length will be the
     * number of valid code points plus the number of invalid
     * code units as if they were replaced with valid code points.
     *
     * If you need to stop at an invalid code unit and get the
     * continuation string, use the overload above.
     */
    std::size_t length(string_range r) noexcept;

    /** @brief Get the number of Unicode code points in a UTF-16 string.
     *
     * If an invalid UTF-16 sequence is encountered, it's considered
     * 1 character and therefore the resulting length will be the
     * number of valid code points plus the number of invalid
     * code units as if they were replaced with valid code points.
     *
     * If you need to stop at an invalid code unit and get the
     * continuation string, use the overload above.
     */
    std::size_t length(u16string_range r) noexcept;

    /** @brief Get the number of Unicode code points in a UTF-32 string.
     *
     * This, like the above overloads for multibyte encodings, treats
     * invalid values as code points, so this function effectively just
     * returns the size of the given range.
     */
    std::size_t length(u32string_range r) noexcept;

    /** @brief Get the number of Unicode code points in a wide string.
     *
     * If an invalid sequence is encountered, it's considered
     * 1 character and therefore the resulting length will be the
     * number of valid code points plus the number of invalid
     * code units as if they were replaced with valid code points.
     *
     * If you need to stop at an invalid code unit and get the
     * continuation string, use the overload above.
     *
     * The behavior of this function is platform dependent as wide
     * characters represent different things on different systems.
     */
    std::size_t length(wstring_range r) noexcept;

    namespace detail {
        template<typename IC, typename OC>
        struct unicode_range: input_range<unicode_range<IC, OC>> {
            using range_category = forward_range_tag;
            using value_type = OC;
            using reference = OC;
            using size_type = std::size_t;

            unicode_range() = delete;
            unicode_range(basic_char_range<IC const> r): p_range(r) {
                if (!r.empty()) {
                    advance();
                }
            }

            bool empty() const { return p_left.empty(); }

            void pop_front() {
                std::size_t n = p_left.size();
                if (n) {
                    bool done = false;
                    if constexpr(max_units<OC> > 1) {
                        done = (n > 1);
                    }
                    if (done || p_range.empty()) {
                        p_left.pop_front();
                        return;
                    }
                }
                advance();
            }

            OC front() const {
                return p_left.front();
            }

        private:
            void advance() {
                auto r = basic_char_range<OC>(p_buf, p_buf + max_units<OC>);
                if (std::size_t n; !(n = utf::encode<OC>(r, p_range))) {
                    /* range is unchanged */
                    p_left = basic_char_range<OC>{};
                    throw utf_error{"Unicode encoding failed"};
                } else {
                    p_left = basic_char_range<OC>{p_buf, p_buf + n};
                }
            }

            basic_char_range<IC const> p_range;
            basic_char_range<OC> p_left{};
            OC p_buf[max_units<OC>];
        };
    } /* namespace detail */

    /** @brief Iterate a Unicode string as a different encoding.
     *
     * This returns an ostd::forward_range_tag that will iterate over
     * the given Unicode character range as a different UTF encoding.
     * The UTF encoding is specified using the given type `C`, being
     * UTF-9 for `char`, UTF-16 for `char16_t`, UTF-32 for `char32_t`
     * and platform specific for `wchar_t`.
     */
    template<typename C, typename R>
    inline auto iter_u(R &&str) {
        return detail::unicode_range<
            std::remove_const_t<
                range_value_t<std::remove_cv_t<std::remove_reference_t<R>>>
            >, C
        >(std::forward<R>(str));
    }

    /** @brief Like ostd::iter_u but taking bits to specify the encoding.
     *
     * This uses utf::unicode_t with `N` to call utf::iter_u().
     */
    template<std::size_t N, typename R>
    inline auto iter_u(R &&str) {
        return iter_u<unicode_t<N>>(std::forward<R>(str));
    }

    /** @brief Check whether a code point is alphanumeric.
     *
     * This is true for either utf::isalpha() or utf::isdigit(). Also
     * equivalent to std::isalnum().
     */
    bool isalnum(char32_t c) noexcept;

    /** @brief Check whether a code point is alphabetic.
     *
     * This is like std::isalpha() but strictly Unicode and works on the
     * entire code point range. Returns true for alphabetic characters,
     * false for others.
     *
     * The categories considered alphabetic are `L*`.
     */
    bool isalpha(char32_t c) noexcept;

    /** @brief Check whether a code point is a blank.
     *
     * This is like std::isblank() but strictly Unicode and works on the
     * entire code point range. Returns true for blanks, false for others.
     *
     * The blank characters are only space (U+20) and tab (`U+9`).
     */
    bool isblank(char32_t c) noexcept;

    /** @brief Check whether a code point is a control character.
     *
     * This is like std::iscntrl() but strictly Unicode and works on the
     * entire code point range. Returns true for blanks, false for others.
     *
     * The category considered control characters is `Cc`.
     */
    bool iscntrl(char32_t c) noexcept;

    /** @brief Check whether a code point is a digit.
     *
     * This is like std::isdigit() but strictly Unicode and works on the
     * entire code point range. Returns true for digit characters,
     * false for others.
     *
     * The category considered a digit is `Nd`.
     */
    bool isdigit(char32_t c) noexcept;

    /** @brief Check whether a code point is graphic.
     *
     * This is true when the input is not utf::isspace() and is
     * utf::isprint(). Also equivalent to std::isgraph().
     */
    bool isgraph(char32_t c) noexcept;

    /** @brief Check whether a code point is lowercase.
     *
     * This is like std::islower() but strictly Unicode and works on the
     * entire code point range. Returns true for lowercase characters,
     * false for others.
     *
     * The category considered a lowercase is `Ll`.
     */
    bool islower(char32_t c) noexcept;

    /** @brief Check whether a code point is printable.
     *
     * Equivalent to std::isprint() but for Unicode. This is true for
     * all characters that are not utf::iscntrl() and that are not
     * U+2028, U+2029, U+FFF9, U+FFFA, U+FFFB.
     */
    bool isprint(char32_t c) noexcept;

    /** @brief Check whether a code point is punctuation.
     *
     * This is like std::ispunct() but strictly Unicode and works on the
     * entire code point range. Returns true for punctuation characters,
     * false for others. Punctuation characters are those that satisfy
     * utf::isgraph() but are not utf::isalnum().
     */
    bool ispunct(char32_t c) noexcept;

    /** @brief Check whether a code point is a whitespace.
     *
     * This is like std::isspace() but strictly Unicode and works on the
     * entire code point range. Returns true for whitespace, false for others.
     *
     * The categories considered blanks are `Z*` with the `B`, `S` and `WS`
     * bidirectional categories.
     */
    bool isspace(char32_t c) noexcept;

    /** @brief Check whether a code point is titlecase.
     *
     * This has no standard ctype equivalent. Returns true for
     * titlecase characters, false for others.
     *
     * The category considered a uppercase is `Lt`.
     */
    bool istitle(char32_t c) noexcept;

    /** @brief Check whether a code point is uppercase.
     *
     * This is like std::isipper but strictly Unicode and works on the
     * entire code point range. Returns true for ippercase characters,
     * false for others.
     *
     * The category considered a uppercase is `Lu`.
     */
    bool isupper(char32_t c) noexcept;

    /** @brief Check whether a code point is a valid character.
     *
     * This is all code points within the range (utf::max_unicode)
     * that are not surrogate code points (U+D800 to U+DFFF),
     * non-characters (U+FDD0 to U+FDEF) and end-of-plane
     * characters (U+FFFE and U+FFFF).
     *
     * This is Unicode specific and has no standard ctype equivalent.
     */
    bool isvalid(char32_t c) noexcept;

    /** @brief Check whether a code point is a hexadecimal digit.
     *
     * This only considers the ASCII character range, returning
     * true for digits (U+30 to U+39) as well as letters A to F
     * in lowercase and uppercase (U+41 to U+46, U+61 to U+66).
     *
     * Behaves exactly the same as std::isxdigit() in the C locale,
     * but unlike the former it never changes behavior, i.e. it cannot
     * support codepage extensions, being Unicode only.
     */
    bool isxdigit(char32_t c) noexcept;

    /** @brief Convert a Unicode code point to lowercase.
     *
     * Like std::tolower() but works with Unicode code points. If the
     * code point is already lowercase or has no lowercase equivalent,
     * this just returns the input unchanged, otherwise it returns the
     * matching lowercase variant.
     */
    char32_t tolower(char32_t c) noexcept;

    /** @brief Convert a Unicode code point to uppercase.
     *
     * Like std::toupper() but works with Unicode code points. If the
     * code point is already uppercase or has no uppercase equivalent,
     * this just returns the input unchanged, otherwise it returns the
     * matching uppercase variant.
     */
    char32_t toupper(char32_t c) noexcept;

    /** @brief Compare two UTF-8 strings.
     *
     * Basically returns `s1.compare(s2)`, so for detailed documentation
     * please refer to basic_char_range::compare().
     */
    inline int compare(string_range s1, string_range s2) noexcept {
        return s1.compare(s2);
    }

    /** @brief Compare two UTF-16 strings.
     *
     * Basically returns `s1.compare(s2)`, so for detailed documentation
     * please refer to basic_char_range::compare().
     */
    inline int compare(u16string_range s1, u16string_range s2) noexcept {
        return s1.compare(s2);
    }

    /** @brief Compare two UTF-32 strings.
     *
     * Basically returns `s1.compare(s2)`, so for detailed documentation
     * please refer to basic_char_range::compare().
     */
    inline int compare(u32string_range s1, u32string_range s2) noexcept {
        return s1.compare(s2);
    }

    /** @brief Compare two wide strings.
     *
     * Basically returns `s1.compare(s2)`, so for detailed documentation
     * please refer to basic_char_range::compare().
     */
    inline int compare(wstring_range s1, wstring_range s2) noexcept {
        return s1.compare(s2);
    }

    /** @brief Compare two UTF-8 strings as case insensitive.
     *
     * The case insensitive comparison is done by advancing by code points
     * and converting each code point to lowercase using utf::tolower()
     * before doing the comparison, with invalid code units being
     * compared as they are (so this function never fails).
     *
     * @see basic_char_range::case_compare()
     */
    int case_compare(string_range s1, string_range s2) noexcept;

    /** @brief Compare two UTF-16 strings as case insensitive.
     *
     * The case insensitive comparison is done by advancing by code points
     * and converting each code point to lowercase using utf::tolower()
     * before doing the comparison, with invalid code units being
     * compared as they are (so this function never fails).
     *
     * @see basic_char_range::case_compare()
     */
    int case_compare(u16string_range s1, u16string_range s2) noexcept;

    /** @brief Compare two UTF-32 strings as case insensitive.
     *
     * The case insensitive comparison is done by converting each code
     * point to lowercase using utf::tolower() before doing the comparison,
     * with invalid code points being compared as they are (so this function
     * never fails).
     *
     * @see basic_char_range::case_compare()
     */
    int case_compare(u32string_range s1, u32string_range s2) noexcept;

    /** @brief Compare two wide strings as case insensitive.
     *
     * The case insensitive comparison is done by advancing by code points
     * and converting each code point to lowercase using utf::tolower()
     * before doing the comparison, with invalid code units being
     * compared as they are (so this function never fails).
     *
     * The internal behavior of this function is platform specific
     * depending on the size of `wchar_t`.
     *
     * @see basic_char_range::case_compare()
     */
    int case_compare(wstring_range s1, wstring_range s2) noexcept;
/** @} */

} /* namespace utf */

template<typename T>
inline std::size_t basic_char_range<T>::length() const noexcept {
    return utf::length(*this);
}

template<typename T>
inline std::size_t basic_char_range<T>::length(
    basic_char_range<T> &cont
) const noexcept {
    return utf::length(*this, cont);
}

template<typename T>
template<typename C>
inline auto basic_char_range<T>::iter_u() const {
    return utf::iter_u<C>(*this);
}

template<typename T>
template<std::size_t N>
inline auto basic_char_range<T>::iter_u() const {
    return utf::iter_u<N>(*this);
}

template<typename T>
inline int basic_char_range<T>::case_compare(
    basic_char_range<T const> s
) const noexcept {
    return utf::case_compare(*this, s);
}

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
