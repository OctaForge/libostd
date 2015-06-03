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

template<typename _T, typename _A = octa::Allocator<_T>>
class StringBase {
    octa::Vector<_T> p_buf;

    void terminate() {
        if (p_buf.empty() || (p_buf.back() != '\0')) p_buf.push('\0');
    }

public:
    typedef size_t                  Size;
    typedef ptrdiff_t               Difference;
    typedef       _T                Value;
    typedef       _T               &Reference;
    typedef const _T               &ConstReference;
    typedef       _T               *Pointer;
    typedef const _T               *ConstPointer;
    typedef PointerRange<      _T>  Range;
    typedef PointerRange<const _T>  ConstRange;
    typedef _A                      Allocator;

    StringBase(const _A &a = _A()): p_buf(1, '\0', a) {}

    StringBase(const StringBase &s): p_buf(s.p_buf) {}
    StringBase(const StringBase &s, const _A &a):
        p_buf(s.p_buf, a) {}
    StringBase(StringBase &&s): p_buf(octa::move(s.p_buf)) {}
    StringBase(StringBase &&s, const _A &a):
        p_buf(octa::move(s.p_buf), a) {}

    StringBase(const StringBase &s, size_t pos, size_t len = npos,
    const _A &a = _A()):
        p_buf(s.p_buf.each().slice(pos,
            (len == npos) ? s.p_buf.size() : (pos + len)), a) {
        terminate();
    }

    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    StringBase(const _T *v, const _A &a = _A()):
        p_buf(ConstRange(v, strlen(v) + 1), a) {}

    template<typename _R> StringBase(_R range, const _A &a = _A()):
    p_buf(range, a) {
        terminate();
    }

    void clear() { p_buf.clear(); }

    StringBase<_T> &operator=(const StringBase &v) {
        p_buf.operator=(v);
        return *this;
    }
    StringBase<_T> &operator=(StringBase &&v) {
        p_buf.operator=(octa::move(v));
        return *this;
    }
    StringBase<_T> &operator=(const _T *v) {
        p_buf = ConstRange(v, strlen(v) + 1);
        return *this;
    }

    void resize(size_t n, _T v = _T()) {
        p_buf.pop();
        p_buf.resize(n, v);
        terminate();
    }

    void reserve(size_t n) {
        p_buf.reserve(n + 1);
    }

    _T &operator[](size_t i) { return p_buf[i]; }
    const _T &operator[](size_t i) const { return p_buf[i]; }

    _T &at(size_t i) { return p_buf[i]; }
    const _T &at(size_t i) const { return p_buf[i]; }

    _T &front() { return p_buf[0]; }
    const _T &front() const { return p_buf[0]; };

    _T &back() { return p_buf[size() - 1]; }
    const _T &back() const { return p_buf[size() - 1]; }

    _T *data() { return p_buf.data(); }
    const _T *data() const { return p_buf.data(); }

    size_t size() const {
        return p_buf.size() - 1;
    }

    size_t capacity() const {
        return p_buf.capacity() - 1;
    }

    bool empty() const { return (size() == 0); }

    void push(_T v) {
        p_buf.back() = v;
        p_buf.push('\0');
    }

    StringBase<_T> &append(const StringBase &s) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), s.p_buf.each());
        return *this;
    }

    StringBase<_T> &append(const StringBase &s, size_t idx, size_t len) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), octa::PointerRange<_T>(&s[idx],
            (len == npos) ? (s.size() - idx) : len));
        terminate();
        return *this;
    }

    StringBase<_T> &append(const _T *s) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), ConstRange(s,
            strlen(s) + 1));
        return *this;
    }

    StringBase<_T> &append(size_t n, _T c) {
        p_buf.pop();
        for (size_t i = 0; i < n; ++i) p_buf.push(c);
        p_buf.push('\0');
        return *this;
    }

    template<typename _R>
    StringBase<_T> &append_range(_R range) {
        p_buf.pop();
        p_buf.insert_range(p_buf.size(), range);
        terminate();
        return *this;
    }

    StringBase<_T> &operator+=(const StringBase &s) {
        return append(s);
    }
    StringBase<_T> &operator+=(const _T *s) {
        return append(s);
    }
    StringBase<_T> &operator+=(_T c) {
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

typedef StringBase<char> String;

template<typename _T, typename _F>
String concat(const _T v, const String &sep, _F func) {
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

template<typename _T>
String concat(const _T &v, const String &sep = " ") {
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

template<typename _T, typename _F>
String concat(std::initializer_list<_T> v, const String &sep, _F func) {
    return concat(octa::each(v), sep, func);
}

template<typename _T>
String concat(std::initializer_list<_T> v, const String &sep = " ") {
    return concat(octa::each(v), sep);
}

namespace detail {
    template<typename _T>
    struct ToStringTest {
        template<typename _U, String (_U::*)() const> struct Test {};
        template<typename _U> static char test(Test<_U, &_U::to_string> *);
        template<typename _U> static  int test(...);
        static constexpr bool value = (sizeof(test<_T>(0)) == sizeof(char));
    };
}

template<typename _T> struct ToString {
    typedef _T Argument;
    typedef String Result;

    template<typename _U>
    static String to_str(const _U &v,
        octa::EnableIf<octa::detail::ToStringTest<_U>::value, bool> = true
    ) {
        return v.to_string();
    }

    template<typename _U>
    static String to_str(const _U &v,
        octa::EnableIf<!octa::detail::ToStringTest<_U>::value &&
            !octa::IsScalar<_U>::value, bool> = true
    ) {
        String ret("{");
        ret += concat(octa::each(v), ", ", ToString<octa::RangeReference<
            decltype(octa::each(v))
        >>());
        ret += "}";
        return octa::move(ret);
    }

    template<typename _U>
    static String to_str(const _U &v,
        octa::EnableIf<!octa::detail::ToStringTest<_U>::value &&
            octa::IsScalar<_U>::value, bool> = true
    ) {
        return ToString<_U>()(v);
    }

    String operator()(const _T &v) {
        return octa::move(to_str<octa::RemoveCv<
            octa::RemoveReference<_T>
        >>(v));
    }
};

namespace detail {
    template<typename _T>
    void str_printf(octa::Vector<char> *s, const char *fmt, _T v) {
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
    typedef bool Argument;
    typedef String Result;
    String operator()(bool b) {
        return b ? "true" : "false";
    }
};

template<> struct ToString<char> {
    typedef char Argument;
    typedef String Result;
    String operator()(char c) {
        String ret;
        ret.push(c);
        return octa::move(ret);
    }
};

#define OCTA_TOSTR_NUM(_T, fmt) \
template<> struct ToString<_T> { \
    typedef _T Argument; \
    typedef String Result; \
    String operator()(_T v) { \
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

template<typename _T> struct ToString<_T *> {
    typedef _T *Argument;
    typedef String Result;
    String operator()(Argument v) {
        String ret;
        octa::detail::str_printf((octa::Vector<char> *)&ret, "%p", v);
        return octa::move(ret);
    }
};

template<> struct ToString<String> {
    typedef const String &Argument;
    typedef String Result;
    String operator()(Argument s) {
        return s;
    }
};

template<typename _T, typename _U> struct ToString<octa::Pair<_T, _U>> {
    typedef const octa::Pair<_T, _U> &Argument;
    typedef String Result;
    String operator()(Argument v) {
        String ret("{");
        ret += ToString<octa::RemoveCv<octa::RemoveReference<_T>>>()
            (v.first);
        ret += ", ";
        ret += ToString<octa::RemoveCv<octa::RemoveReference<_U>>>()
            (v.second);
        ret += "}";
        return octa::move(ret);
    }
};

template<typename _T>
String to_string(const _T &v) {
    return octa::move(ToString<octa::RemoveCv<octa::RemoveReference<_T>>>
        ()(v));
}

template<typename _T>
String to_string(std::initializer_list<_T> init) {
    return octa::move(ToString<std::initializer_list<_T>>()(init));
}

} /* namespace octa */

#endif