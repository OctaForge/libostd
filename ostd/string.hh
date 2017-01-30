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

#include "ostd/utility.hh"
#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/functional.hh"
#include "ostd/type_traits.hh"
#include "ostd/algorithm.hh"

namespace ostd {

template<typename T>
struct CharRangeBase: InputRange<
    CharRangeBase<T>, ContiguousRangeTag, T
> {
private:
    struct Nat {};

public:
    CharRangeBase(): p_beg(nullptr), p_end(nullptr) {};

    template<typename U>
    CharRangeBase(T *beg, U end, EnableIf<
        (IsPointer<U> || IsNullPointer<U>) && IsConvertible<U, T *>, Nat
    > = Nat()): p_beg(beg), p_end(end) {}

    CharRangeBase(T *beg, size_t n): p_beg(beg), p_end(beg + n) {}

    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    template<typename U>
    CharRangeBase(
        U beg, EnableIf<IsConvertible<U, T *> && !IsArray<U>, Nat> = Nat()
    ): p_beg(beg), p_end(static_cast<T *>(beg) + (beg ? strlen(beg) : 0)) {}

    CharRangeBase(Nullptr): p_beg(nullptr), p_end(nullptr) {}

    template<typename U, size_t N>
    CharRangeBase(U (&beg)[N], EnableIf<IsConvertible<U *, T *>, Nat> = Nat()):
        p_beg(beg), p_end(beg + N - (beg[N - 1] == '\0'))
    {}

    template<typename U, typename TR, typename A>
    CharRangeBase(std::basic_string<U, TR, A> const &s, EnableIf<
        IsConvertible<U *, T *>, Nat
    > = Nat()):
        p_beg(s.data()), p_end(s.data() + s.size())
    {}

    template<typename U, typename = EnableIf<IsConvertible<U *, T *>>>
    CharRangeBase(CharRangeBase<U> const &v):
        p_beg(&v[0]), p_end(&v[v.size()])
    {}

    CharRangeBase &operator=(CharRangeBase const &v) {
        p_beg = v.p_beg; p_end = v.p_end; return *this;
    }

    template<typename TR, typename A>
    CharRangeBase &operator=(std::basic_string<T, TR, A> const &s) {
        p_beg = s.data(); p_end = s.data() + s.size(); return *this;
    }
    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    CharRangeBase &operator=(T *s) {
        p_beg = s; p_end = s + (s ? strlen(s) : 0); return *this;
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

    bool equals_front(CharRangeBase const &range) const {
        return p_beg == range.p_beg;
    }

    ptrdiff_t distance_front(CharRangeBase const &range) const {
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

    bool equals_back(CharRangeBase const &range) const {
        return p_end == range.p_end;
    }

    ptrdiff_t distance_back(CharRangeBase const &range) const {
        return range.p_end - p_end;
    }

    size_t size() const { return p_end - p_beg; }

    CharRangeBase slice(size_t start, size_t end) const {
        return CharRangeBase(p_beg + start, p_beg + end);
    }

    T &operator[](size_t i) const { return p_beg[i]; }

    bool put(T v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = v;
        return true;
    }

    size_t put_n(T const *p, size_t n) {
        size_t an = ostd::min(n, size());
        memcpy(p_beg, p, an * sizeof(T));
        p_beg += an;
        return an;
    }

    T *data() { return p_beg; }
    T const *data() const { return p_beg; }

    /* non-range */
    int compare(CharRangeBase<T const> s) const {
        size_t s1 = size(), s2 = s.size();
        int ret;
        if (!s1 || !s2) {
            goto diffsize;
        }
        if ((ret = memcmp(data(), s.data(), ostd::min(s1, s2)))) {
            return ret;
        }
diffsize:
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    int case_compare(CharRangeBase<T const> s) const {
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
    EnableIf<IsOutputRange<R>, size_t> copy(R &&orange, size_t n = -1) {
        return orange.put_n(data(), ostd::min(n, size()));
    }

    size_t copy(RemoveCv<T> *p, size_t n = -1) {
        size_t c = ostd::min(n, size());
        memcpy(p, data(), c * sizeof(T));
        return c;
    }

    /* that way we can assign, append etc to std::string */
    operator std::basic_string_view<RemoveCv<T>>() const {
        return std::basic_string_view<RemoveCv<T>>{data(), size()};
    }

private:
    T *p_beg, *p_end;
};

using CharRange = CharRangeBase<char>;
using ConstCharRange = CharRangeBase<char const>;

inline bool operator==(ConstCharRange lhs, ConstCharRange rhs) {
    return !lhs.compare(rhs);
}

inline bool operator!=(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs);
}

inline bool operator<(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator>(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator<=(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator>=(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) >= 0;
}

inline bool starts_with(ConstCharRange a, ConstCharRange b) {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

template<typename T>
struct ranged_traits<std::basic_string<T>> {
    static CharRangeBase<T> iter(std::basic_string<T> &v) {
        return CharRangeBase<T>{v.data(), v.size()};
    }
};

template<typename T>
struct ranged_traits<std::basic_string<T> const> {
    static CharRangeBase<T const> iter(std::basic_string<T> const &v) {
        return CharRangeBase<T const>{v.data(), v.size()};
    }
};

template<typename T, typename R>
inline std::basic_string<T> make_string(R range) {
    /* TODO: specialize for contiguous ranges and matching value types */
    std::basic_string<T> ret;
    for (; !range.empty(); range.pop_front()) {
        ret.push_back(range.front());
    }
    return ret;
}

template<typename R>
inline std::basic_string<RemoveCv<RangeValue<R>>> make_string(R range) {
    return make_string<RemoveCv<RangeValue<R>>>(std::move(range));
}

/* string literals */

inline namespace literals {
inline namespace string_literals {
    inline ConstCharRange operator "" _sr(char const *str, size_t len) {
        return ConstCharRange(str, len);
    }
}
}

namespace detail {
    template<
        typename T, bool = IsConvertible<T, ConstCharRange>,
        bool = IsConvertible<T, char>
    >
    struct ConcatPut;

    template<typename T, bool B>
    struct ConcatPut<T, true, B> {
        template<typename R>
        static bool put(R &sink, ConstCharRange v) {
            return v.size() && (sink.put_n(&v[0], v.size()) == v.size());
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
bool concat(R &&sink, T const &v, ConstCharRange sep, F func) {
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
        sink.put_n(&sep[0], sep.size());
    }
    return true;
}

template<typename R, typename T>
bool concat(R &&sink, T const &v, ConstCharRange sep = " ") {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return true;
    }
    for (;;) {
        ConstCharRange ret = range.front();
        if (!ret.size() || (sink.put_n(&ret[0], ret.size()) != ret.size())) {
            return false;
        }
        range.pop_front();
        if (range.empty()) {
            break;
        }
        sink.put_n(&sep[0], sep.size());
    }
    return true;
}

template<typename R, typename T, typename F>
bool concat(R &&sink, std::initializer_list<T> v, ConstCharRange sep, F func) {
    return concat(sink, ostd::iter(v), sep, func);
}

template<typename R, typename T>
bool concat(R &&sink, std::initializer_list<T> v, ConstCharRange sep = " ") {
    return concat(sink, ostd::iter(v), sep);
}

namespace detail {
    template<typename R>
    struct TostrRange: OutputRange<TostrRange<R>, char> {
        TostrRange() = delete;
        TostrRange(R &out): p_out(out), p_written(0) {}
        bool put(char v) {
            bool ret = p_out.put(v);
            p_written += ret;
            return ret;
        }
        size_t put_n(char const *v, size_t n) {
            size_t ret = p_out.put_n(v, n);
            p_written += ret;
            return ret;
        }
        size_t put_string(ConstCharRange r) {
            return put_n(&r[0], r.size());
        }
        size_t get_written() const { return p_written; }
    private:
        R &p_out;
        size_t p_written;
    };

    template<typename T, typename R>
    static auto test_stringify(int) ->
        BoolConstant<IsSame<decltype(std::declval<T>().stringify()), std::string>>;

    template<typename T, typename R>
    static True test_stringify(decltype(std::declval<T const &>().to_string
        (std::declval<R &>())) *);

    template<typename, typename>
    static False test_stringify(...);

    template<typename T, typename R>
    constexpr bool StringifyTest = decltype(test_stringify<T, R>(0))::value;

    template<typename T>
    static True test_iterable(decltype(ostd::iter(std::declval<T>())) *);
    template<typename>
    static False test_iterable(...);

    template<typename T>
    constexpr bool IterableTest = decltype(test_iterable<T>(0))::value;
}

template<typename T, typename = void>
struct ToString;

template<typename T>
struct ToString<T, EnableIf<detail::IterableTest<T>>> {
    using Argument = RemoveCv<RemoveReference<T>>;
    using Result = std::string;

    std::string operator()(T const &v) const {
        std::string ret("{");
        auto x = appender<std::string>();
        if (concat(x, ostd::iter(v), ", ", ToString<
            RemoveConst<RemoveReference<
                RangeReference<decltype(ostd::iter(v))>
            >>
        >())) {
            ret += x.get();
        }
        ret += "}";
        return ret;
    }
};

template<typename T>
struct ToString<T, EnableIf<
    detail::StringifyTest<T, detail::TostrRange<AppenderRange<std::string>>>
>> {
    using Argument = RemoveCv<RemoveReference<T>>;
    using Result = std::string;

    std::string operator()(T const &v) const {
        auto app = appender<std::string>();
        detail::TostrRange<AppenderRange<std::string>> sink(app);
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
struct ToString<CharRange> {
    using Argument = CharRange;
    using Result = std::string;
    std::string operator()(Argument const &s) {
        return std::string{s};
    }
};

template<>
struct ToString<ConstCharRange> {
    using Argument = ConstCharRange;
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
        ret += ToString<RemoveCv<RemoveReference<T>>>()(v.first);
        ret += ", ";
        ret += ToString<RemoveCv<RemoveReference<U>>>()(v.second);
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
            ret += ToString<RemoveCv<RemoveReference<
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
            ret += ToString<RemoveCv<RemoveReference<
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
    return ToString<RemoveCv<RemoveReference<T>>>()(v);
}

template<typename T>
std::string to_string(std::initializer_list<T> init) {
    return to_string(iter(init));
}

template<typename R>
struct TempCString {
private:
    RemoveCv<RangeValue<R>> *p_buf;
    bool p_allocated;

public:
    TempCString() = delete;
    TempCString(TempCString const &) = delete;
    TempCString(TempCString &&s): p_buf(s.p_buf), p_allocated(s.p_allocated) {
        s.p_buf = nullptr;
        s.p_allocated = false;
    }
    TempCString(R input, RemoveCv<RangeValue<R>> *sbuf, size_t bufsize)
    : p_buf(nullptr), p_allocated(false) {
        if (input.empty()) {
            return;
        }
        if (input.size() >= bufsize) {
            p_buf = new RemoveCv<RangeValue<R>>[input.size() + 1];
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

    operator RemoveCv<RangeValue<R>> const *() const { return p_buf; }
    RemoveCv<RangeValue<R>> const *get() const { return p_buf; }

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
    R input, RemoveCv<RangeValue<R>> *buf, size_t bufsize
) {
    return TempCString<R>(input, buf, bufsize);
}

} /* namespace ostd */

namespace std {

template<>
struct hash<ostd::CharRange> {
    size_t operator()(ostd::CharRange const &v) {
        return hash<std::string_view>{}(v);
    }
};

template<>
struct hash<ostd::ConstCharRange> {
    size_t operator()(ostd::CharRange const &v) {
        return hash<std::string_view>{}(v);
    }
};

}

#endif
