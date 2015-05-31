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
    using std::initializer_list;

    static constexpr size_t npos = -1;

    template<typename T>
    class StringBase {
        Vector<T> p_buf;

        void terminate() {
            if (p_buf.empty() || (p_buf.last() != '\0')) p_buf.push('\0');
        }

    public:
        typedef size_t                 SizeType;
        typedef ptrdiff_t              DiffType;
        typedef       T                ValType;
        typedef       T               &RefType;
        typedef const T               &ConstRefType;
        typedef       T               *PtrType;
        typedef const T               *ConstPtrType;
        typedef PointerRange<      T>  RangeType;
        typedef PointerRange<const T>  ConstRangeType;

        StringBase(): p_buf(1, '\0') {}

        StringBase(const StringBase &s): p_buf(s.p_buf) {}
        StringBase(StringBase &&s): p_buf(move(s.p_buf)) {}

        StringBase(const StringBase &s, size_t pos, size_t len = npos):
            p_buf(s.p_buf.each().slice(pos,
                (len == npos) ? s.p_buf.size() : (pos + len))) {
            terminate();
        }

        /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
        StringBase(const T *v): p_buf(PointerRange<const T>(v, strlen(v) + 1)) {}

        template<typename R> StringBase(R range): p_buf(range) {
            terminate();
        }

        void clear() { p_buf.clear(); }

        StringBase<T> &operator=(const StringBase &v) {
            p_buf.operator=(v);
            return *this;
        }
        StringBase<T> &operator=(StringBase &&v) {
            p_buf.operator=(move(v));
            return *this;
        }
        StringBase<T> &operator=(const T *v) {
            p_buf = PointerRange<const T>(v, strlen(v) + 1);
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

        T &first() { return p_buf[0]; }
        const T &first() const { return p_buf[0]; };

        T &last() { return p_buf[size() - 1]; }
        const T &last() const { return p_buf[size() - 1]; }

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
            p_buf.last() = v;
            p_buf.push('\0');
        }

        StringBase<T> &append(const StringBase &s) {
            p_buf.pop();
            p_buf.insert_range(p_buf.size(), s.p_buf.each());
            return *this;
        }

        StringBase<T> &append(const StringBase &s, size_t idx, size_t len) {
            p_buf.pop();
            p_buf.insert_range(p_buf.size(), PointerRange<T>(&s[idx],
                (len == npos) ? (s.size() - idx) : len));
            terminate();
            return *this;
        }

        StringBase<T> &append(const T *s) {
            p_buf.pop();
            p_buf.insert_range(p_buf.size(), PointerRange<const T>(s,
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

        RangeType each() {
            return PointerRange<T>(p_buf.data(), size());
        }
        ConstRangeType each() const {
            return PointerRange<const T>(p_buf.data(), size());
        }

        void swap(StringBase &v) {
            p_buf.swap(v);
        }
    };

    typedef StringBase<char> String;

    template<typename R, typename F>
    String concat(R range, String sep, F func) {
        String ret;
        if (range.empty()) return move(ret);
        for (;;) {
            ret += func(range.first());
            range.pop_first();
            if (range.empty()) break;
            ret += sep;
        }
        return move(ret);
    }

    template<typename R>
    String concat(R range, String sep = " ") {
        String ret;
        if (range.empty()) return move(ret);
        for (;;) {
            ret += range.first();
            range.pop_first();
            if (range.empty()) break;
            ret += sep;
        }
        return move(ret);
    }

    template<typename T, typename F>
    String concat(initializer_list<T> il, String sep, F func) {
        return concat(each(il), sep, func);
    }

    template<typename T>
    String concat(initializer_list<T> il, String sep = " ") {
        return concat(each(il), sep);
    }

    template<typename T>
    struct __OctaToStringTest {
        template<typename U, String (U::*)() const> struct __OctaTest {};
        template<typename U> static char __octa_test(__OctaTest<U, &U::to_string> *);
        template<typename U> static  int __octa_test(...);
        static constexpr bool value = (sizeof(__octa_test<T>(0)) == sizeof(char));
    };

    template<typename T> struct ToString {
        typedef T ArgType;
        typedef String ResultType;

        template<typename U>
        static String __octa_to_str(const U &v,
            EnableIf<__OctaToStringTest<U>::value, bool> = true
        ) {
            return v.to_string();
        }

        template<typename U>
        static String __octa_to_str(const U &v,
            EnableIf<!__OctaToStringTest<U>::value && !IsScalar<U>::value,
                bool> = true
        ) {
            String ret("{");
            ret += concat(each(v), ", ", ToString<RangeReference<
                decltype(each(v))
            >>());
            ret += "}";
            return move(ret);
        }

        template<typename U>
        static String __octa_to_str(const U &v,
            EnableIf<!__OctaToStringTest<U>::value && IsScalar<U>::value,
                bool> = true
        ) {
            return ToString<U>()(v);
        }

        String operator()(const T &v) {
            return move(__octa_to_str<RemoveCv<RemoveReference<T>>>(v));
        }
    };

    template<typename T>
    void __octa_str_printf(Vector<char> *s, const char *fmt, T v) {
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

    template<> struct ToString<bool> {
        typedef bool ArgType;
        typedef String ResultType;
        String operator()(bool b) {
            return b ? "true" : "false";
        }
    };

    template<> struct ToString<char> {
        typedef char ArgType;
        typedef String ResultType;
        String operator()(char c) {
            String ret;
            ret.push(c);
            return move(ret);
        }
    };

#define __OCTA_TOSTR_NUM(T, fmt) \
    template<> struct ToString<T> { \
        typedef T ArgType; \
        typedef String ResultType; \
        String operator()(T v) { \
            String ret; \
            __octa_str_printf((Vector<char> *)&ret, fmt, v); \
            return move(ret); \
        } \
    };

    __OCTA_TOSTR_NUM(int, "%d")
    __OCTA_TOSTR_NUM(int &, "%d")
    __OCTA_TOSTR_NUM(uint, "%u")
    __OCTA_TOSTR_NUM(long, "%ld")
    __OCTA_TOSTR_NUM(ulong, "%lu")
    __OCTA_TOSTR_NUM(llong, "%lld")
    __OCTA_TOSTR_NUM(ullong, "%llu")
    __OCTA_TOSTR_NUM(float, "%f")
    __OCTA_TOSTR_NUM(double, "%f")
    __OCTA_TOSTR_NUM(ldouble, "%Lf")

#undef __OCTA_TOSTR_NUM

    template<typename T> struct ToString<T *> {
        typedef T *ArgType;
        typedef String ResultType;
        String operator()(ArgType v) {
            String ret;
            __octa_str_printf((Vector<char> *)&ret, "%p", v);
            return move(ret);
        }
    };

    template<> struct ToString<String> {
        typedef const String &ArgType;
        typedef String ResultType;
        String operator()(ArgType s) {
            return s;
        }
    };

    template<typename T, typename U> struct ToString<Pair<T, U>> {
        typedef const Pair<T, U> &ArgType;
        typedef String ResultType;
        String operator()(ArgType v) {
            String ret("{");
            ret += ToString<RemoveCv<RemoveReference<T>>>()(v.first);
            ret += ", ";
            ret += ToString<RemoveCv<RemoveReference<U>>>()(v.second);
            ret += "}";
            return move(ret);
        }
    };

    template<typename T>
    String to_string(const T &v) {
        return move(ToString<RemoveCv<RemoveReference<T>>>()(v));
    }

    template<typename T>
    String to_string(initializer_list<T> init) {
        return move(ToString<initializer_list<T>>()(init));
    }
}

#endif