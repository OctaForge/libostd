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
                (len == npos) ? s.p_buf.length() : (pos + len))) {
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

        T &operator[](size_t i) { return p_buf[i]; }
        const T &operator[](size_t i) const { return p_buf[i]; }

        T &at(size_t i) { return p_buf[i]; }
        const T &at(size_t i) const { return p_buf[i]; }

        T &first() { return p_buf[0]; }
        const T &first() const { return p_buf[0]; };

        T &last() { return p_buf[length() - 1]; }
        const T &last() const { return p_buf[length() - 1]; }

        T *data() { return p_buf.data(); }
        const T *data() const { return p_buf.data(); }

        size_t length() const {
            return p_buf.length() - 1;
        }

        size_t capacity() const {
            return p_buf.capacity() - 1;
        }

        bool empty() const { return (length() == 0); }

        void push(T v) {
            p_buf.last() = v;
            p_buf.push('\0');
        }

        StringBase<T> &operator+=(const StringBase &s) {
            p_buf.pop();
            p_buf.insert_range(p_buf.length(), s.p_buf.each());
            return *this;
        }
        StringBase<T> &operator+=(const T *s) {
            p_buf.pop();
            p_buf.insert_range(p_buf.length(), PointerRange<const T>(s,
                strlen(s) + 1));
            return *this;
        }
        StringBase<T> &operator+=(T c) {
            p_buf.pop();
            p_buf.push(c);
            p_buf.push('\0');
            return *this;
        }

        RangeType each() {
            return PointerRange<T>(p_buf.data(), length());
        }
        ConstRangeType each() const {
            return PointerRange<const T>(p_buf.data(), length());
        }

        void swap(StringBase &v) {
            p_buf.swap(v);
        }
    };

    typedef StringBase<char> String;

    template<typename R, typename F>
    String concat(R range, F func, String sep) {
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

    template<typename T> struct ToString {
        typedef T ArgType;
        typedef String ResultType;
        String operator()(const T &) { return ""; }
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

    template<> struct ToString<int> {
        typedef int ArgType;
        typedef String ResultType;
        String operator()(int v) {
            char buf[128];
            sprintf(buf, "%d", v);
            return String((const char *)buf);
        }
    };
}

#endif