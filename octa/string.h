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
        octa::Vector<_T> __buf;

        void __terminate() {
            if (__buf.empty() || (__buf.back() != '\0')) __buf.push('\0');
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

        StringBase(const _A &__a = _A()): __buf(1, '\0', __a) {}

        StringBase(const StringBase &__s): __buf(__s.__buf) {}
        StringBase(const StringBase &__s, const _A &__a):
            __buf(__s.__buf, __a) {}
        StringBase(StringBase &&__s): __buf(octa::move(__s.__buf)) {}
        StringBase(StringBase &&__s, const _A &__a):
            __buf(octa::move(__s.__buf), __a) {}

        StringBase(const StringBase &__s, size_t __pos, size_t __len = npos,
        const _A &__a = _A()):
            __buf(__s.__buf.each().slice(__pos,
                (__len == npos) ? __s.__buf.size() : (__pos + __len)), __a) {
            __terminate();
        }

        /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
        StringBase(const _T *__v, const _A &__a = _A()):
            __buf(ConstRange(__v, strlen(__v) + 1), __a) {}

        template<typename _R> StringBase(_R __range, const _A &__a = _A()):
        __buf(__range, __a) {
            __terminate();
        }

        void clear() { __buf.clear(); }

        StringBase<_T> &operator=(const StringBase &__v) {
            __buf.operator=(__v);
            return *this;
        }
        StringBase<_T> &operator=(StringBase &&__v) {
            __buf.operator=(octa::move(__v));
            return *this;
        }
        StringBase<_T> &operator=(const _T *__v) {
            __buf = ConstRange(__v, strlen(__v) + 1);
            return *this;
        }

        void resize(size_t __n, _T __v = _T()) {
            __buf.pop();
            __buf.resize(__n, __v);
            __terminate();
        }

        void reserve(size_t __n) {
            __buf.reserve(__n + 1);
        }

        _T &operator[](size_t __i) { return __buf[__i]; }
        const _T &operator[](size_t __i) const { return __buf[__i]; }

        _T &at(size_t __i) { return __buf[__i]; }
        const _T &at(size_t __i) const { return __buf[__i]; }

        _T &front() { return __buf[0]; }
        const _T &front() const { return __buf[0]; };

        _T &back() { return __buf[size() - 1]; }
        const _T &back() const { return __buf[size() - 1]; }

        _T *data() { return __buf.data(); }
        const _T *data() const { return __buf.data(); }

        size_t size() const {
            return __buf.size() - 1;
        }

        size_t capacity() const {
            return __buf.capacity() - 1;
        }

        bool empty() const { return (size() == 0); }

        void push(_T __v) {
            __buf.back() = __v;
            __buf.push('\0');
        }

        StringBase<_T> &append(const StringBase &__s) {
            __buf.pop();
            __buf.insert_range(__buf.size(), __s.__buf.each());
            return *this;
        }

        StringBase<_T> &append(const StringBase &__s, size_t __idx, size_t __len) {
            __buf.pop();
            __buf.insert_range(__buf.size(), octa::PointerRange<_T>(&__s[__idx],
                (__len == npos) ? (__s.size() - __idx) : __len));
            __terminate();
            return *this;
        }

        StringBase<_T> &append(const _T *__s) {
            __buf.pop();
            __buf.insert_range(__buf.size(), ConstRange(__s,
                strlen(__s) + 1));
            return *this;
        }

        StringBase<_T> &append(size_t __n, _T __c) {
            __buf.pop();
            for (size_t __i = 0; __i < __n; ++__i) __buf.push(__c);
            __buf.push('\0');
            return *this;
        }

        template<typename _R>
        StringBase<_T> &append_range(_R __range) {
            __buf.pop();
            __buf.insert_range(__buf.size(), __range);
            __terminate();
            return *this;
        }

        StringBase<_T> &operator+=(const StringBase &__s) {
            return append(__s);
        }
        StringBase<_T> &operator+=(const _T *__s) {
            return append(__s);
        }
        StringBase<_T> &operator+=(_T __c) {
            __buf.pop();
            __buf.push(__c);
            __buf.push('\0');
            return *this;
        }

        Range each() {
            return Range(__buf.data(), size());
        }
        ConstRange each() const {
            return ConstRange(__buf.data(), size());
        }

        void swap(StringBase &__v) {
            __buf.swap(__v);
        }
    };

    typedef StringBase<char> String;

    template<typename _T>
    struct __OctaIsRangeTest {
        template<typename _U> static char __test(typename _U::Category *);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == sizeof(char));
    };

    template<typename _T, typename _F>
    String concat(const _T &__v, const String &__sep, _F __func) {
        String __ret;
        auto __range = octa::each(__v);
        if (__range.empty()) return octa::move(__ret);
        for (;;) {
            __ret += __func(__range.front());
            __range.pop_front();
            if (__range.empty()) break;
            __ret += __sep;
        }
        return octa::move(__ret);
    }

    template<typename _T>
    String concat(const _T &__v, const String &__sep = " ") {
        String __ret;
        auto __range = octa::each(__v);
        if (__range.empty()) return octa::move(__ret);
        for (;;) {
            __ret += __range.front();
            __range.pop_front();
            if (__range.empty()) break;
            __ret += __sep;
        }
        return octa::move(__ret);
    }

    template<typename _T, typename _F>
    String concat(std::initializer_list<_T> __v, const String &__sep, _F __func) {
        return concat(octa::each(__v), __sep, __func);
    }

    template<typename _T>
    String concat(std::initializer_list<_T> __v, const String &__sep = " ") {
        return concat(octa::each(__v), __sep);
    }

    template<typename _T>
    struct __OctaToStringTest {
        template<typename _U, String (_U::*)() const> struct __Test {};
        template<typename _U> static char __test(__Test<_U, &_U::to_string> *);
        template<typename _U> static  int __test(...);
        static constexpr bool value = (sizeof(__test<_T>(0)) == sizeof(char));
    };

    template<typename _T> struct ToString {
        typedef _T Argument;
        typedef String Result;

        template<typename _U>
        static String __octa_to_str(const _U &__v,
            octa::EnableIf<__OctaToStringTest<_U>::value, bool> = true
        ) {
            return __v.to_string();
        }

        template<typename _U>
        static String __octa_to_str(const _U &__v,
            octa::EnableIf<!__OctaToStringTest<_U>::value &&
                !octa::IsScalar<_U>::value, bool> = true
        ) {
            String __ret("{");
            __ret += concat(octa::each(__v), ", ", ToString<octa::RangeReference<
                decltype(octa::each(__v))
            >>());
            __ret += "}";
            return octa::move(__ret);
        }

        template<typename _U>
        static String __octa_to_str(const _U &__v,
            octa::EnableIf<!__OctaToStringTest<_U>::value &&
                octa::IsScalar<_U>::value, bool> = true
        ) {
            return ToString<_U>()(__v);
        }

        String operator()(const _T &__v) {
            return octa::move(__octa_to_str<octa::RemoveCv<
                octa::RemoveReference<_T>
            >>(__v));
        }
    };

    template<typename _T>
    void __octa_str_printf(octa::Vector<char> *__s, const char *__fmt, _T __v) {
        char __buf[256];
        int __n = snprintf(__buf, sizeof(__buf), __fmt, __v);
        __s->clear();
        __s->reserve(__n + 1);
        if (__n >= (int)sizeof(__buf))
            snprintf(__s->data(), __n + 1, __fmt, __v);
        else if (__n > 0)
            memcpy(__s->data(), __buf, __n + 1);
        else {
            __n = 0;
            *(__s->data()) = '\0';
        }
        *(((size_t *)__s) + 1) = __n + 1;
    }

    template<> struct ToString<bool> {
        typedef bool Argument;
        typedef String Result;
        String operator()(bool __b) {
            return __b ? "true" : "false";
        }
    };

    template<> struct ToString<char> {
        typedef char Argument;
        typedef String Result;
        String operator()(char __c) {
            String __ret;
            __ret.push(__c);
            return octa::move(__ret);
        }
    };

#define __OCTA_TOSTR_NUM(_T, __fmt) \
    template<> struct ToString<_T> { \
        typedef _T Argument; \
        typedef String Result; \
        String operator()(_T __v) { \
            String __ret; \
            __octa_str_printf((octa::Vector<char> *)&__ret, __fmt, __v); \
            return octa::move(__ret); \
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

    template<typename _T> struct ToString<_T *> {
        typedef _T *Argument;
        typedef String Result;
        String operator()(Argument __v) {
            String __ret;
            __octa_str_printf((octa::Vector<char> *)&__ret, "%p", __v);
            return octa::move(__ret);
        }
    };

    template<> struct ToString<String> {
        typedef const String &Argument;
        typedef String Result;
        String operator()(Argument __s) {
            return __s;
        }
    };

    template<typename _T, typename _U> struct ToString<octa::Pair<_T, _U>> {
        typedef const octa::Pair<_T, _U> &Argument;
        typedef String Result;
        String operator()(Argument __v) {
            String __ret("{");
            __ret += ToString<octa::RemoveCv<octa::RemoveReference<_T>>>()
                (__v.first);
            __ret += ", ";
            __ret += ToString<octa::RemoveCv<octa::RemoveReference<_U>>>()
                (__v.second);
            __ret += "}";
            return octa::move(__ret);
        }
    };

    template<typename _T>
    String to_string(const _T &__v) {
        return octa::move(ToString<octa::RemoveCv<octa::RemoveReference<_T>>>
            ()(__v));
    }

    template<typename _T>
    String to_string(std::initializer_list<_T> __init) {
        return octa::move(ToString<std::initializer_list<_T>>()(__init));
    }
}

#endif