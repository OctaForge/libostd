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
#include <utility>

#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/algorithm.hh"

namespace ostd {

template<typename T, typename TR = std::char_traits<std::remove_const_t<T>>>
struct basic_char_range: input_range<basic_char_range<T>> {
    using range_category  = contiguous_range_tag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

private:
    struct nat {};

public:
    basic_char_range(): p_beg(nullptr), p_end(nullptr) {};
    basic_char_range(T *beg, T *end): p_beg(beg), p_end(end) {}
    basic_char_range(std::nullptr_t): p_beg(nullptr), p_end(nullptr) {}

    template<typename U>
    basic_char_range(U &&beg, std::enable_if_t<
        std::is_convertible_v<U, T *>, nat
    > = nat{}): p_beg(beg) {
        if constexpr(std::is_array_v<std::remove_reference_t<U>>) {
            size_t N = std::extent_v<std::remove_reference_t<U>>;
            p_end = beg + N - (beg[N - 1] == '\0');
        } else {
            p_end = beg + (beg ? TR::length(beg) : 0);
        }
    }

    template<typename STR, typename A>
    basic_char_range(std::basic_string<std::remove_const_t<T>, STR, A> const &s):
        p_beg(s.data()), p_end(s.data() + s.size())
    {}

    template<typename U, typename TTR, typename = std::enable_if_t<
        std::is_convertible_v<U *, T *>
    >>
    basic_char_range(basic_char_range<U, TTR> const &v):
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

    void pop_front() {
        ++p_beg;
        if (p_beg > p_end) {
            throw std::out_of_range{"pop_front on empty range"};
        }
    }
    void push_front() {
        --p_beg;
    }

    void pop_front_n(size_t n) {
        p_beg += n;
        if (p_beg > p_end) {
            throw std::out_of_range{"pop_front_n of too many elements"};
        }
    }

    void push_front_n(size_t n) {
        p_beg -= n;
    }

    T &front() const { return *p_beg; }

    bool equals_front(basic_char_range const &range) const {
        return p_beg == range.p_beg;
    }

    ptrdiff_t distance_front(basic_char_range const &range) const {
        return range.p_beg - p_beg;
    }

    void pop_back() {
        if (p_end == p_beg) {
            return;
        }
        --p_end;
    }
    void push_back() {
        ++p_end;
    }

    void pop_back_n(size_t n) {
        p_end -= n;
        if (p_end < p_beg) {
            throw std::out_of_range{"pop_back_n of too many elements"};
        }
    }

    void push_back_n(size_t n) {
        p_end += n;
    }

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

    void put(T v) {
        *(p_beg++) = v;
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
        if ((ret = TR::compare(data(), s.data(), std::min(s1, s2)))) {
            return ret;
        }
diffsize:
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    int case_compare(basic_char_range<T const> s) const {
        size_t s1 = size(), s2 = s.size();
        for (size_t i = 0, ms = std::min(s1, s2); i < ms; ++i) {
            int d = toupper(p_beg[i]) - toupper(s[i]);
            if (d) {
                return d;
            }
        }
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    /* that way we can assign, append etc to std::string */
    operator std::basic_string_view<std::remove_cv_t<T>>() const {
        return std::basic_string_view<std::remove_cv_t<T>>{data(), size()};
    }

private:
    T *p_beg, *p_end;
};

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
        static void put(R &sink, string_range v) {
            sink = ostd::copy(v, sink);
        }
    };

    template<typename T>
    struct ConcatPut<T, false, true> {
        template<typename R>
        static void put(R &sink, char v) {
            sink.put(v);
        }
    };
}

template<typename R, typename T, typename F>
R &&concat(R &&sink, T const &v, string_range sep, F func) {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return std::forward<R>(sink);
    }
    for (;;) {
        detail::ConcatPut<
            decltype(func(range.front()))
        >::put(sink, func(range.front()));
        range.pop_front();
        if (range.empty()) {
            break;
        }
        sink = ostd::copy(sep, sink);
    }
    return std::forward<R>(sink);
}

template<typename R, typename T>
R &&concat(R &&sink, T const &v, string_range sep = " ") {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return std::forward<R>(sink);
    }
    for (;;) {
        string_range ret = range.front();
        sink = ostd::copy(ret, sink);
        range.pop_front();
        if (range.empty()) {
            break;
        }
        sink = ostd::copy(sep, sink);
    }
    return std::forward<R>(sink);
}

template<typename R, typename T, typename F>
R &&concat(R &&sink, std::initializer_list<T> v, string_range sep, F func) {
    return concat(sink, ostd::iter(v), sep, func);
}

template<typename R, typename T>
R &&concat(R &&sink, std::initializer_list<T> v, string_range sep = " ") {
    return concat(sink, ostd::iter(v), sep);
}

namespace detail {
    template<typename R>
    struct tostr_range: output_range<tostr_range<R>> {
        using value_type      = char;
        using reference       = char &;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;

        tostr_range() = delete;
        tostr_range(R &out): p_out(out) {}
        void put(char v) {
            p_out.put(v);
        }
        void put_string(string_range r) {
            p_out = ostd::copy(r, p_out);
        }
    private:
        R &p_out;
    };

    template<typename T, typename R>
    static std::true_type test_stringify(
        decltype(std::declval<T const &>().to_string(std::declval<R &>())) *
    );

    template<typename, typename>
    static std::false_type test_stringify(...);

    template<typename T, typename R>
    constexpr bool stringify_test = decltype(test_stringify<T, R>(0))::value;

    template<typename T>
    static std::true_type test_iterable(decltype(ostd::iter(std::declval<T>())) *);
    template<typename>
    static std::false_type test_iterable(...);

    template<typename T>
    constexpr bool iterable_test = decltype(test_iterable<T>(0))::value;
}

template<typename T, typename = void>
struct to_string;

template<typename T>
struct to_string<T, std::enable_if_t<detail::iterable_test<T>>> {
    std::string operator()(T const &v) const {
        std::string ret("{");
        auto x = appender_range<std::string>{};
        concat(x, ostd::iter(v), ", ", to_string<
            std::remove_const_t<std::remove_reference_t<
                range_reference_t<decltype(ostd::iter(v))>
            >>
        >());
        ret += x.get();
        ret += "}";
        return ret;
    }
};

template<typename T>
struct to_string<T, std::enable_if_t<
    detail::stringify_test<T, detail::tostr_range<appender_range<std::string>>>
>> {
    std::string operator()(T const &v) const {
        auto app = appender_range<std::string>{};
        detail::tostr_range<appender_range<std::string>> sink(app);
        if (!v.to_string(sink)) {
            return std::string{};
        }
        return std::move(app.get());
    }
};

template<>
struct to_string<bool> {
    std::string operator()(bool b) {
        return b ? "true" : "false";
    }
};

template<>
struct to_string<char> {
    std::string operator()(char c) {
        std::string ret;
        ret += c;
        return ret;
    }
};

#define OSTD_TOSTR_NUM(T) \
template<> \
struct to_string<T> { \
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
struct to_string<T *> {
    std::string operator()(T *v) {
        char buf[16];
        sprintf(buf, "%p", v);
        return buf;
    }
};

template<>
struct to_string<char const *> {
    std::string operator()(char const *s) {
        return s;
    }
};

template<>
struct to_string<char *> {
    std::string operator()(char *s) {
        return s;
    }
};

template<>
struct to_string<std::string> {
    std::string operator()(std::string const &s) {
        return s;
    }
};

template<>
struct to_string<char_range> {
    std::string operator()(char_range const &s) {
        return std::string{s};
    }
};

template<>
struct to_string<string_range> {
    std::string operator()(string_range const &s) {
        return std::string{s};
    }
};

template<typename T, typename U>
struct to_string<std::pair<T, U>> {
    std::string operator()(std::pair<T, U> const &v) {
        std::string ret{"{"};
        ret += to_string<std::remove_cv_t<std::remove_reference_t<T>>>()(v.first);
        ret += ", ";
        ret += to_string<std::remove_cv_t<std::remove_reference_t<U>>>()(v.second);
        ret += "}";
        return ret;
    }
};

namespace detail {
    template<size_t I, size_t N>
    struct tuple_to_str {
        template<typename T>
        static void append(std::string &ret, T const &tup) {
            ret += ", ";
            ret += to_string<std::remove_cv_t<std::remove_reference_t<
                decltype(std::get<I>(tup))
            >>>()(std::get<I>(tup));
            tuple_to_str<I + 1, N>::append(ret, tup);
        }
    };

    template<size_t N>
    struct tuple_to_str<N, N> {
        template<typename T>
        static void append(std::string &, T const &) {}
    };

    template<size_t N>
    struct tuple_to_str<0, N> {
        template<typename T>
        static void append(std::string &ret, T const &tup) {
            ret += to_string<std::remove_cv_t<std::remove_reference_t<
                decltype(std::get<0>(tup))
            >>>()(std::get<0>(tup));
            tuple_to_str<1, N>::append(ret, tup);
        }
    };
}

template<typename ...T>
struct to_string<std::tuple<T...>> {
    std::string operator()(std::tuple<T...> const &v) {
        std::string ret("{");
        detail::tuple_to_str<0, sizeof...(T)>::append(ret, v);
        ret += "}";
        return ret;
    }
};

template<typename R>
struct temp_c_string {
private:
    std::remove_cv_t<range_value_t<R>> *p_buf;
    bool p_allocated;

public:
    temp_c_string() = delete;
    temp_c_string(temp_c_string const &) = delete;
    temp_c_string(temp_c_string &&s): p_buf(s.p_buf), p_allocated(s.p_allocated) {
        s.p_buf = nullptr;
        s.p_allocated = false;
    }
    temp_c_string(R input, std::remove_cv_t<range_value_t<R>> *sbuf, size_t bufsize)
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
        char_range bufr{p_buf, p_buf + input.size()};
        ostd::copy(input, bufr).put('\0');
    }
    ~temp_c_string() {
        if (p_allocated) {
            delete[] p_buf;
        }
    }

    temp_c_string &operator=(temp_c_string const &) = delete;
    temp_c_string &operator=(temp_c_string &&s) {
        swap(s);
        return *this;
    }

    operator std::remove_cv_t<range_value_t<R>> const *() const { return p_buf; }
    std::remove_cv_t<range_value_t<R>> const *get() const { return p_buf; }

    void swap(temp_c_string &s) {
        using std::swap;
        swap(p_buf, s.p_buf);
        swap(p_allocated, s.p_allocated);
    }
};

template<typename R>
inline void swap(temp_c_string<R> &a, temp_c_string<R> &b) {
    a.swap(b);
}

template<typename R>
inline temp_c_string<R> to_temp_cstr(
    R input, std::remove_cv_t<range_value_t<R>> *buf, size_t bufsize
) {
    return temp_c_string<R>(input, buf, bufsize);
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
