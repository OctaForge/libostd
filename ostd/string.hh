/* String utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_STRING_HH
#define OSTD_STRING_HH

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <functional>

#include "ostd/utility.hh"
#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/algorithm.hh"

namespace ostd {

template<typename T, typename TR = std::char_traits<std::remove_const_t<T>>>
struct basic_char_range: input_range<basic_char_range<T>> {
    using range_category  = ContiguousRangeTag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

private:
    struct Nat {};

public:
    basic_char_range(): p_beg(nullptr), p_end(nullptr) {};
    basic_char_range(T *beg, T *end): p_beg(beg), p_end(end) {}

    template<typename U>
    basic_char_range(
        U beg, std::enable_if_t<
            std::is_convertible_v<U, T *> && !std::is_array_v<U>, Nat
        > = Nat()
    ): p_beg(beg), p_end(static_cast<T *>(beg) + (beg ? TR::length(beg) : 0)) {}

    basic_char_range(std::nullptr_t): p_beg(nullptr), p_end(nullptr) {}

    template<typename U, size_t N>
    basic_char_range(U (&beg)[N], std::enable_if_t<
        std::is_convertible_v<U *, T *>, Nat
    > = Nat()):
        p_beg(beg), p_end(beg + N - (beg[N - 1] == '\0'))
    {}

    template<typename STR, typename A>
    basic_char_range(std::basic_string<std::remove_const_t<T>, STR, A> const &s):
        p_beg(s.data()), p_end(s.data() + s.size())
    {}

    template<typename U, typename = std::enable_if_t<
        std::is_convertible_v<U *, T *>
    >>
    basic_char_range(basic_char_range<U> const &v):
        p_beg(&v[0]), p_end(&v[v.size()])
    {}

    basic_char_range &operator=(basic_char_range const &v) {
        p_beg = v.p_beg; p_end = v.p_end; return *this;
    }

    template<typename STR, typename A>
    basic_char_range &operator=(std::basic_string<T, STR, A> const &s) {
        p_beg = s.data(); p_end = s.data() + s.size(); return *this;
    }

    basic_char_range &operator=(T *s) {
        p_beg = s; p_end = s + (s ? TR::length(s) : 0); return *this;
    }

    bool empty() const { return p_beg == p_end; }

    bool pop_front() {
        if (p_beg == p_end) {
            return false;
        }
        ++p_beg;
        return true;
    }
    bool push_front() { --p_beg; return true; }

    size_t pop_front_n(size_t n) {
        size_t olen = p_end - p_beg;
        p_beg += n;
        if (p_beg > p_end) {
            p_beg = p_end;
            return olen;
        }
        return n;
    }

    size_t push_front_n(size_t n) { p_beg -= n; return true; }

    T &front() const { return *p_beg; }

    bool equals_front(basic_char_range const &range) const {
        return p_beg == range.p_beg;
    }

    ptrdiff_t distance_front(basic_char_range const &range) const {
        return range.p_beg - p_beg;
    }

    bool pop_back() {
        if (p_end == p_beg) {
            return false;
        }
        --p_end;
        return true;
    }
    bool push_back() { ++p_end; return true; }

    size_t pop_back_n(size_t n) {
        size_t olen = p_end - p_beg;
        p_end -= n;
        if (p_end < p_beg) {
            p_end = p_beg;
            return olen;
        }
        return n;
    }

    size_t push_back_n(size_t n) { p_end += n; return true; }

    T &back() const { return *(p_end - 1); }

    bool equals_back(basic_char_range const &range) const {
        return p_end == range.p_end;
    }

    ptrdiff_t distance_back(basic_char_range const &range) const {
        return range.p_end - p_end;
    }

    size_t size() const { return p_end - p_beg; }

    basic_char_range slice(size_t start, size_t end) const {
        return basic_char_range(p_beg + start, p_beg + end);
    }

    T &operator[](size_t i) const { return p_beg[i]; }

    bool put(T v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = v;
        return true;
    }

    T *data() { return p_beg; }
    T const *data() const { return p_beg; }

    /* non-range */
    int compare(basic_char_range<T const> s) const {
        size_t s1 = size(), s2 = s.size();
        int ret;
        if (!s1 || !s2) {
            goto diffsize;
        }
        if ((ret = TR::compare(data(), s.data(), ostd::min(s1, s2)))) {
            return ret;
        }
diffsize:
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    int case_compare(basic_char_range<T const> s) const {
        size_t s1 = size(), s2 = s.size();
        for (size_t i = 0, ms = ostd::min(s1, s2); i < ms; ++i) {
            int d = toupper(p_beg[i]) - toupper(s[i]);
            if (d) {
                return d;
            }
        }
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    template<typename R>
    std::enable_if_t<is_output_range<R>, size_t> copy(R &&orange, size_t n = -1) {
        return range_put_n(orange, data(), ostd::min(n, size()));
    }

    size_t copy(std::remove_cv_t<T> *p, size_t n = -1) {
        size_t c = ostd::min(n, size());
        TR::copy(p, data(), c);
        return c;
    }

    /* that way we can assign, append etc to std::string */
    operator std::basic_string_view<std::remove_cv_t<T>>() const {
        return std::basic_string_view<std::remove_cv_t<T>>{data(), size()};
    }

private:
    T *p_beg, *p_end;
};

template<typename T, typename TR>
inline size_t range_put_n(basic_char_range<T, TR> &range, T const *p, size_t n) {
    size_t an = ostd::min(n, range.size());
    TR::copy(range.data(), p, an);
    range.pop_front_n(an);
    return an;
}

using char_range = basic_char_range<char>;
using string_range = basic_char_range<char const>;

/* comparisons between ranges */

template<typename T, typename TR>
inline bool operator==(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return !lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator!=(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator<(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) < 0;
}

template<typename T, typename TR>
inline bool operator>(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) > 0;
}

template<typename T, typename TR>
inline bool operator<=(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) <= 0;
}

template<typename T, typename TR>
inline bool operator>=(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) >= 0;
}

/* comparisons between mutable ranges and char arrays */

template<typename T, typename TR>
inline bool operator==(basic_char_range<T, TR> lhs, T const *rhs) {
    return !lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator!=(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator<(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) < 0;
}

template<typename T, typename TR>
inline bool operator>(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) > 0;
}

template<typename T, typename TR>
inline bool operator<=(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) <= 0;
}

template<typename T, typename TR>
inline bool operator>=(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) >= 0;
}

template<typename T, typename TR>
inline bool operator==(T const *lhs, basic_char_range<T, TR> rhs) {
    return !rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator!=(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator<(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) > 0;
}

template<typename T, typename TR>
inline bool operator>(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) < 0;
}

template<typename T, typename TR>
inline bool operator<=(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) >= 0;
}

template<typename T, typename TR>
inline bool operator>=(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) <= 0;
}

/* comparisons between immutable ranges and char arrays */

template<typename T, typename TR>
inline bool operator==(basic_char_range<T const, TR> lhs, T const *rhs) {
    return !lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator!=(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator<(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) < 0;
}

template<typename T, typename TR>
inline bool operator>(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) > 0;
}

template<typename T, typename TR>
inline bool operator<=(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) <= 0;
}

template<typename T, typename TR>
inline bool operator>=(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) >= 0;
}

template<typename T, typename TR>
inline bool operator==(T const *lhs, basic_char_range<T const, TR> rhs) {
    return !rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator!=(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator<(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) > 0;
}

template<typename T, typename TR>
inline bool operator>(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) < 0;
}

template<typename T, typename TR>
inline bool operator<=(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) >= 0;
}

template<typename T, typename TR>
inline bool operator>=(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) <= 0;
}

inline bool starts_with(string_range a, string_range b
) {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

template<typename T, typename TR, typename A>
struct ranged_traits<std::basic_string<T, TR, A>> {
    using range = basic_char_range<T, TR>;

    static range iter(std::basic_string<T, TR, A> &v) {
        return range{v.data(), v.data() + v.size()};
    }
};

template<typename T, typename TR, typename A>
struct ranged_traits<std::basic_string<T, TR, A> const> {
    using range = basic_char_range<T const, TR>;

    static range iter(std::basic_string<T, TR, A> const &v) {
        return range{v.data(), v.data() + v.size()};
    }
};

template<
    typename T, typename TR = std::char_traits<T>,
    typename A = std::allocator<T>, typename R
>
inline std::basic_string<T, TR, A> make_string(R range, A const &alloc = A{}) {
    std::basic_string<T, TR, A> ret{alloc};
    using C = range_category_t<R>;
    if constexpr(std::is_convertible_v<C, finite_random_access_range_tag>) {
        /* finite random access or contiguous */
        auto h = range.half();
        ret.reserve(range.size());
        ret.insert(ret.end(), h, h + range.size());
    } else {
        /* infinite random access and below */
        for (; !range.empty(); range.pop_front()) {
            ret.push_back(range.front());
        }
    }
    return ret;
}

template<
    typename R, typename TR = std::char_traits<std::remove_cv_t<range_value_t<R>>>,
    typename A = std::allocator<std::remove_cv_t<range_value_t<R>>>
>
inline std::basic_string<std::remove_cv_t<range_value_t<R>>, TR, A> make_string(
    R range, A const &alloc = A{}
) {
    return make_string<std::remove_cv_t<range_value_t<R>>, TR, A>(
        std::move(range), alloc
    );
}

/* string literals */

inline namespace literals {
inline namespace string_literals {
    inline string_range operator "" _sr(char const *str, size_t len) {
        return string_range(str, str + len);
    }
}
}

namespace detail {
    template<
        typename T, bool = std::is_convertible_v<T, string_range>,
        bool = std::is_convertible_v<T, char>
    >
    struct ConcatPut;

    template<typename T, bool B>
    struct ConcatPut<T, true, B> {
        template<typename R>
        static bool put(R &sink, string_range v) {
            return v.size() && (range_put_n(sink, &v[0], v.size()) == v.size());
        }
    };

    template<typename T>
    struct ConcatPut<T, false, true> {
        template<typename R>
        static bool put(R &sink, char v) {
            return sink.put(v);
        }
    };
}

template<typename R, typename T, typename F>
bool concat(R &&sink, T const &v, string_range sep, F func) {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return true;
    }
    for (;;) {
        if (!detail::ConcatPut<
            decltype(func(range.front()))
        >::put(sink, func(range.front()))) {
            return false;
        }
        range.pop_front();
        if (range.empty()) {
            break;
        }
        range_put_n(sink, &sep[0], sep.size());
    }
    return true;
}

template<typename R, typename T>
bool concat(R &&sink, T const &v, string_range sep = " ") {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return true;
    }
    for (;;) {
        string_range ret = range.front();
        if (!ret.size() || (range_put_n(sink, &ret[0], ret.size()) != ret.size())) {
            return false;
        }
        range.pop_front();
        if (range.empty()) {
            break;
        }
        range_put_n(sink, &sep[0], sep.size());
    }
    return true;
}

template<typename R, typename T, typename F>
bool concat(R &&sink, std::initializer_list<T> v, string_range sep, F func) {
    return concat(sink, ostd::iter(v), sep, func);
}

template<typename R, typename T>
bool concat(R &&sink, std::initializer_list<T> v, string_range sep = " ") {
    return concat(sink, ostd::iter(v), sep);
}

namespace detail {
    template<typename R>
    struct tostr_range: output_range<tostr_range<R>> {
        using value_type      = char;
        using reference       = char &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        template<typename RR>
        friend size_t range_put_n(tostr_range<RR> &range, char const *p, size_t n);

        tostr_range() = delete;
        tostr_range(R &out): p_out(out), p_written(0) {}
        bool put(char v) {
            bool ret = p_out.put(v);
            p_written += ret;
            return ret;
        }
        size_t put_string(string_range r) {
            size_t ret = range_put_n(p_out, r.data(), r.size());
            p_written += ret;
            return ret;
        }
        size_t get_written() const { return p_written; }
    private:
        R &p_out;
        size_t p_written;
    };

    template<typename R>
    inline size_t range_put_n(tostr_range<R> &range, char const *p, size_t n) {
        size_t ret = range_put_n(range.p_out, p, n);
        range.p_written += ret;
        return ret;
    }

    template<typename T, typename R>
    static auto test_stringify(int) -> std::integral_constant<
        bool, std::is_same_v<decltype(std::declval<T>().stringify()), std::string>
    >;

    template<typename T, typename R>
    static std::true_type test_stringify(
        decltype(std::declval<T const &>().to_string(std::declval<R &>())) *
    );

    template<typename, typename>
    static std::false_type test_stringify(...);

    template<typename T, typename R>
    constexpr bool StringifyTest = decltype(test_stringify<T, R>(0))::value;

    template<typename T>
    static std::true_type test_iterable(decltype(ostd::iter(std::declval<T>())) *);
    template<typename>
    static std::false_type test_iterable(...);

    template<typename T>
    constexpr bool IterableTest = decltype(test_iterable<T>(0))::value;
}

template<typename T, typename = void>
struct ToString;

template<typename T>
struct ToString<T, std::enable_if_t<detail::IterableTest<T>>> {
    using Argument = std::remove_cv_t<std::remove_reference_t<T>>;
    using Result = std::string;

    std::string operator()(T const &v) const {
        std::string ret("{");
        auto x = appender<std::string>();
        if (concat(x, ostd::iter(v), ", ", ToString<
            std::remove_const_t<std::remove_reference_t<
                range_reference_t<decltype(ostd::iter(v))>
            >>
        >())) {
            ret += x.get();
        }
        ret += "}";
        return ret;
    }
};

template<typename T>
struct ToString<T, std::enable_if_t<
    detail::StringifyTest<T, detail::tostr_range<appender_range<std::string>>>
>> {
    using Argument = std::remove_cv_t<std::remove_reference_t<T>>;
    using Result = std::string;

    std::string operator()(T const &v) const {
        auto app = appender<std::string>();
        detail::tostr_range<appender_range<std::string>> sink(app);
        if (!v.to_string(sink)) {
            return std::string{};
        }
        return std::move(app.get());
    }
};

template<>
struct ToString<bool> {
    using Argument = bool;
    using Result = std::string;
    std::string operator()(bool b) {
        return b ? "true" : "false";
    }
};

template<>
struct ToString<char> {
    using Argument = char;
    using Result = std::string;
    std::string operator()(char c) {
        std::string ret;
        ret += c;
        return ret;
    }
};

#define OSTD_TOSTR_NUM(T) \
template<> \
struct ToString<T> { \
    using Argument = T; \
    using Result = std::string; \
    std::string operator()(T v) { \
        return std::to_string(v); \
    } \
};

OSTD_TOSTR_NUM(sbyte)
OSTD_TOSTR_NUM(short)
OSTD_TOSTR_NUM(int)
OSTD_TOSTR_NUM(long)
OSTD_TOSTR_NUM(float)
OSTD_TOSTR_NUM(double)

OSTD_TOSTR_NUM(byte)
OSTD_TOSTR_NUM(ushort)
OSTD_TOSTR_NUM(uint)
OSTD_TOSTR_NUM(ulong)
OSTD_TOSTR_NUM(llong)
OSTD_TOSTR_NUM(ullong)
OSTD_TOSTR_NUM(ldouble)

#undef OSTD_TOSTR_NUM

template<typename T>
struct ToString<T *> {
    using Argument = T *;
    using Result = std::string;
    std::string operator()(Argument v) {
        char buf[16];
        sprintf(buf, "%p", v);
        return buf;
    }
};

template<>
struct ToString<char const *> {
    using Argument = char const *;
    using Result = std::string;
    std::string operator()(char const *s) {
        return s;
    }
};

template<>
struct ToString<char *> {
    using Argument = char *;
    using Result = std::string;
    std::string operator()(char *s) {
        return s;
    }
};

template<>
struct ToString<std::string> {
    using Argument = std::string;
    using Result = std::string;
    std::string operator()(Argument const &s) {
        return s;
    }
};

template<>
struct ToString<char_range> {
    using Argument = char_range;
    using Result = std::string;
    std::string operator()(Argument const &s) {
        return std::string{s};
    }
};

template<>
struct ToString<string_range> {
    using Argument = string_range;
    using Result = std::string;
    std::string operator()(Argument const &s) {
        return std::string{s};
    }
};

template<typename T, typename U>
struct ToString<std::pair<T, U>> {
    using Argument = std::pair<T, U>;
    using Result = std::string;
    std::string operator()(Argument const &v) {
        std::string ret{"{"};
        ret += ToString<std::remove_cv_t<std::remove_reference_t<T>>>()(v.first);
        ret += ", ";
        ret += ToString<std::remove_cv_t<std::remove_reference_t<U>>>()(v.second);
        ret += "}";
        return ret;
    }
};

namespace detail {
    template<size_t I, size_t N>
    struct TupleToString {
        template<typename T>
        static void append(std::string &ret, T const &tup) {
            ret += ", ";
            ret += ToString<std::remove_cv_t<std::remove_reference_t<
                decltype(std::get<I>(tup))
            >>>()(std::get<I>(tup));
            TupleToString<I + 1, N>::append(ret, tup);
        }
    };

    template<size_t N>
    struct TupleToString<N, N> {
        template<typename T>
        static void append(std::string &, T const &) {}
    };

    template<size_t N>
    struct TupleToString<0, N> {
        template<typename T>
        static void append(std::string &ret, T const &tup) {
            ret += ToString<std::remove_cv_t<std::remove_reference_t<
                decltype(std::get<0>(tup))
            >>>()(std::get<0>(tup));
            TupleToString<1, N>::append(ret, tup);
        }
    };
}

template<typename ...T>
struct ToString<std::tuple<T...>> {
    using Argument = std::tuple<T...>;
    using Result = std::string;
    std::string operator()(Argument const &v) {
        std::string ret("{");
        detail::TupleToString<0, sizeof...(T)>::append(ret, v);
        ret += "}";
        return ret;
    }
};

template<typename T>
typename ToString<T>::Result to_string(T const &v) {
    return ToString<std::remove_cv_t<std::remove_reference_t<T>>>()(v);
}

template<typename T>
std::string to_string(std::initializer_list<T> init) {
    return to_string(iter(init));
}

template<typename R>
struct TempCString {
private:
    std::remove_cv_t<range_value_t<R>> *p_buf;
    bool p_allocated;

public:
    TempCString() = delete;
    TempCString(TempCString const &) = delete;
    TempCString(TempCString &&s): p_buf(s.p_buf), p_allocated(s.p_allocated) {
        s.p_buf = nullptr;
        s.p_allocated = false;
    }
    TempCString(R input, std::remove_cv_t<range_value_t<R>> *sbuf, size_t bufsize)
    : p_buf(nullptr), p_allocated(false) {
        if (input.empty()) {
            return;
        }
        if (input.size() >= bufsize) {
            p_buf = new std::remove_cv_t<range_value_t<R>>[input.size() + 1];
            p_allocated = true;
        } else {
            p_buf = sbuf;
        }
        p_buf[input.copy(p_buf)] = '\0';
    }
    ~TempCString() {
        if (p_allocated) {
            delete[] p_buf;
        }
    }

    TempCString &operator=(TempCString const &) = delete;
    TempCString &operator=(TempCString &&s) {
        swap(s);
        return *this;
    }

    operator std::remove_cv_t<range_value_t<R>> const *() const { return p_buf; }
    std::remove_cv_t<range_value_t<R>> const *get() const { return p_buf; }

    void swap(TempCString &s) {
        using std::swap;
        swap(p_buf, s.p_buf);
        swap(p_allocated, s.p_allocated);
    }
};

template<typename R>
inline void swap(TempCString<R> &a, TempCString<R> &b) {
    a.swap(b);
}

template<typename R>
inline TempCString<R> to_temp_cstr(
    R input, std::remove_cv_t<range_value_t<R>> *buf, size_t bufsize
) {
    return TempCString<R>(input, buf, bufsize);
}

} /* namespace ostd */

namespace std {

template<typename T, typename TR>
struct hash<ostd::basic_char_range<T, TR>> {
    size_t operator()(ostd::basic_char_range<T, TR> const &v) const {
        return hash<std::basic_string_view<std::remove_const_t<T>, TR>>{}(v);
    }
};

}

#endif
