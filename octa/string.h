/* String for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_STRING_H
#define OCTA_STRING_H

#include <stdio.h>
#include <stddef.h>

#include "octa/utility.h"
#include "octa/range.h"
#include "octa/vector.h"

namespace octa {
static constexpr size_t npos = -1;

template<typename T, typename A = octa::Allocator<T>>
class StringBase {
    octa::Vector<T> p_buf;

    void terminate() {
        if (p_buf.empty() || (p_buf.back() != '\0')) p_buf.push('\0');
    }

public:
    using Size = size_t;
    using Difference = ptrdiff_t;
    using Value = T;
    using Reference = T &;
    using ConstReference = const T &;
    using Pointer = T *;
    using ConstPointer = const T *;
    using Range = PointerRange<T>;
    using ConstRange = PointerRange<const T>;
    using Allocator = A;

    StringBase(const A &a = A()): p_buf(1, '\0', a) {}

    StringBase(const StringBase &s): p_buf(s.p_buf) {}
    StringBase(const StringBase &s, const A &a):
        p_buf(s.p_buf, a) {}
    StringBase(StringBase &&s): p_buf(octa::move(s.p_buf)) {}
    StringBase(StringBase &&s, const A &a):
        p_buf(octa::move(s.p_buf), a) {}

    StringBase(const StringBase &s, size_t pos, size_t len = npos,
    const A &a = A()):
        p_buf(s.p_buf.each().slice(pos,
            (len == npos) ? s.p_buf.size() : (pos + len)), a) {
        terminate();
    }

    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    StringBase(const T *v, const A &a = A()):
        p_buf(ConstRange(v, strlen(v) + 1), a) {}

    template<typename R> StringBase(R range, const A &a = A()):
    p_buf(range, a) {
        terminate();
    }

    void clear() { p_buf.clear(); }

    StringBase<T> &operator=(const StringBase &v) {
        p_buf.operator=(v);
        return *this;
    }
    StringBase<T> &operator=(StringBase &&v) {
        p_buf.operator=(octa::move(v));
        return *this;
    }
    StringBase<T> &operator=(const T *v) {
        p_buf = ConstRange(v, strlen(v) + 1);
        return *this;
    }

    void resize(size_t n, T v = T()) {
        p_buf.pop();
        p_buf.resize(n, v);
        terminate();
    }

    void reserve(size_t n) {
        p_buf.reserve(n + 1);
    }

    T &operator[](size_t i) { return p_buf[i]; }
    const T &operator[](size_t i) const { return p_buf[i]; }

    T &at(size_t i) { return p_buf[i]; }
    const T &at(size_t i) const { return p_buf[i]; }

    T &front() { return p_buf[0]; }
    const T &front() const { return p_buf[0]; };

    T &back() { return p_buf[size() - 1]; }
    const T &back() const { return p_buf[size() - 1]; }

    T *data() { return p_buf.data(); }
    const T *data() const { return p_buf.data(); }

    size_t size() const {
        return p_buf.size() - 1;
    }

    size_t capacity() const {
        return p_buf.capacity() - 1;
    }

    bool empty() const { return (size() == 0); }

    void push(T v) {
        p_buf.back() = v;
        p_buf.push('\0');
    }

    StringBase<T> &append(const StringBase &s) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), s.p_buf.each());
        return *this;
    }

    StringBase<T> &append(const StringBase &s, size_t idx, size_t len) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), octa::PointerRange<T>(&s[idx],
            (len == npos) ? (s.size() - idx) : len));
        terminate();
        return *this;
    }

    StringBase<T> &append(const T *s) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), ConstRange(s,
            strlen(s) + 1));
        return *this;
    }

    StringBase<T> &append(size_t n, T c) {
        p_buf.pop();
        for (size_t i = 0; i < n; ++i) p_buf.push(c);
        p_buf.push('\0');
        return *this;
    }

    template<typename R>
    StringBase<T> &append_range(R range) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), range);
        terminate();
        return *this;
    }

    StringBase<T> &operator+=(const StringBase &s) {
        return append(s);
    }
    StringBase<T> &operator+=(const T *s) {
        return append(s);
    }
    StringBase<T> &operator+=(T c) {
        p_buf.pop();
        p_buf.push(c);
        p_buf.push('\0');
        return *this;
    }

    Range each() {
        return Range(p_buf.data(), size());
    }
    ConstRange each() const {
        return ConstRange(p_buf.data(), size());
    }

    void swap(StringBase &v) {
        p_buf.swap(v);
    }
};

using String = StringBase<char>;

template<typename T, typename F>
String concat(const T v, const String &sep, F func) {
    String ret;
    auto range = octa::each(v);
    if (range.empty()) return octa::move(ret);
    for (;;) {
        ret += func(range.front());
        range.pop_front();
        if (range.empty()) break;
        ret += sep;
    }
    return octa::move(ret);
}

template<typename T>
String concat(const T &v, const String &sep = " ") {
    String ret;
    auto range = octa::each(v);
    if (range.empty()) return octa::move(ret);
    for (;;) {
        ret += range.front();
        range.pop_front();
        if (range.empty()) break;
        ret += sep;
    }
    return octa::move(ret);
}

template<typename T, typename F>
String concat(std::initializer_list<T> v, const String &sep, F func) {
    return concat(octa::each(v), sep, func);
}

template<typename T>
String concat(std::initializer_list<T> v, const String &sep = " ") {
    return concat(octa::each(v), sep);
}

namespace detail {
    template<typename T>
    struct ToStringTest {
        template<typename U, String (U::*)() const> struct Test {};
        template<typename U> static char test(Test<U, &U::to_string> *);
        template<typename U> static  int test(...);
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(char));
    };
}

template<typename T> struct ToString {
    using Argument = T;
    using Result = String;

    template<typename U>
    static String to_str(const U &v,
        octa::EnableIf<octa::detail::ToStringTest<U>::value, bool> = true
    ) {
        return v.to_string();
    }

    template<typename U>
    static String to_str(const U &v,
        octa::EnableIf<!octa::detail::ToStringTest<U>::value &&
            !octa::IsScalar<U>::value, bool> = true
    ) {
        String ret("{");
        ret += concat(octa::each(v), ", ", ToString<octa::RangeReference<
            decltype(octa::each(v))
        >>());
        ret += "}";
        return octa::move(ret);
    }

    template<typename U>
    static String to_str(const U &v,
        octa::EnableIf<!octa::detail::ToStringTest<U>::value &&
            octa::IsScalar<U>::value, bool> = true
    ) {
        return ToString<U>()(v);
    }

    String operator()(const T &v) {
        return octa::move(to_str<octa::RemoveCv<
            octa::RemoveReference<T>
        >>(v));
    }
};

namespace detail {
    template<typename T>
    void str_printf(octa::Vector<char> *s, const char *fmt, T v) {
        char buf[256];
        int n = snprintf(buf, sizeof(buf), fmt, v);
        s->clear();
        s->reserve(n + 1);
        if (n >= (int)sizeof(buf))
            snprintf(s->data(), n + 1, fmt, v);
        else if (n > 0)
            memcpy(s->data(), buf, n + 1);
        else {
            n = 0;
            *(s->data()) = '\0';
        }
        *(((size_t *)s) + 1) = n + 1;
    }
}

template<> struct ToString<bool> {
    using Argument = bool;
    using Result = String;
    String operator()(bool b) {
        return b ? "true" : "false";
    }
};

template<> struct ToString<char> {
    using Argument = char;
    using Result = String;
    String operator()(char c) {
        String ret;
        ret.push(c);
        return octa::move(ret);
    }
};

#define OCTA_TOSTR_NUM(T, fmt) \
template<> struct ToString<T> { \
    using Argument = T; \
    using Result = String; \
    String operator()(T v) { \
        String ret; \
        octa::detail::str_printf((octa::Vector<char> *)&ret, fmt, v); \
        return octa::move(ret); \
    } \
};

OCTA_TOSTR_NUM(int, "%d")
OCTA_TOSTR_NUM(int &, "%d")
OCTA_TOSTR_NUM(uint, "%u")
OCTA_TOSTR_NUM(long, "%ld")
OCTA_TOSTR_NUM(ulong, "%lu")
OCTA_TOSTR_NUM(llong, "%lld")
OCTA_TOSTR_NUM(ullong, "%llu")
OCTA_TOSTR_NUM(float, "%f")
OCTA_TOSTR_NUM(double, "%f")
OCTA_TOSTR_NUM(ldouble, "%Lf")

#undef OCTA_TOSTR_NUM

template<typename T> struct ToString<T *> {
    using Argument = T *;
    using Result = String;
    String operator()(Argument v) {
        String ret;
        octa::detail::str_printf((octa::Vector<char> *)&ret, "%p", v);
        return octa::move(ret);
    }
};

template<> struct ToString<String> {
    using Argument = const String &;
    using Result = String;
    String operator()(Argument s) {
        return s;
    }
};

template<typename T, typename U> struct ToString<octa::Pair<T, U>> {
    using Argument = const octa::Pair<T, U> &;
    using Result = String;
    String operator()(Argument v) {
        String ret("{");
        ret += ToString<octa::RemoveCv<octa::RemoveReference<T>>>()
            (v.first);
        ret += ", ";
        ret += ToString<octa::RemoveCv<octa::RemoveReference<U>>>()
            (v.second);
        ret += "}";
        return octa::move(ret);
    }
};

template<typename T>
String to_string(const T &v) {
    return octa::move(ToString<octa::RemoveCv<octa::RemoveReference<T>>>
        ()(v));
}

template<typename T>
String to_string(std::initializer_list<T> init) {
    return octa::move(ToString<std::initializer_list<T>>()(init));
}

} /* namespace octa */

#endif