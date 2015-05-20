/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_VECTOR_H
#define OCTA_VECTOR_H

#include <string.h>
#include <stddef.h>

#include "octa/new.h"
#include "octa/type_traits.h"
#include "octa/utility.h"
#include "octa/range.h"
#include "octa/algorithm.h"
#include "octa/initializer_list.h"

namespace octa {
    template<typename T>
    class Vector {
        T *p_buf;
        size_t p_len, p_cap;

        void insert_base(size_t idx, size_t n) {
            if (p_len + n > p_cap) reserve(p_len + n);
            p_len += n;
            for (size_t i = p_len - 1; i > idx + n - 1; --i) {
                p_buf[i] = move(p_buf[i - n]);
            }
        }

    public:
        enum { MIN_SIZE = 8 };

        typedef size_t                 SizeType;
        typedef ptrdiff_t              DiffType;
        typedef       T                ValType;
        typedef       T               &RefType;
        typedef const T               &ConstRefType;
        typedef       T               *PtrType;
        typedef const T               *ConstPtrType;
        typedef PointerRange<      T>  RangeType;
        typedef PointerRange<const T>  ConstRangeType;

        Vector(): p_buf(nullptr), p_len(0), p_cap(0) {}

        explicit Vector(size_t n, const T &val = T()): Vector() {
            p_buf = new uchar[n * sizeof(T)];
            p_len = p_cap = n;
            T *cur = p_buf, *last = p_buf + n;
            while (cur != last) new (cur++) T(val);
        }

        Vector(const Vector &v): Vector() {
            *this = v;
        }

        Vector(Vector &&v): p_buf(v.p_buf), p_len(v.p_len), p_cap(v.p_cap) {
            v.p_buf = nullptr;
            v.p_len = v.p_cap = 0;
        }

        Vector(InitializerList<T> v): Vector() {
            size_t len = v.length();
            const T *ptr = v.get();
            reserve(len);
            for (size_t i = 0; i < len; ++i)
                new (&p_buf[i]) T(ptr[i]);
            p_len = len;
        }

        ~Vector() {
            clear();
            delete[] (uchar *)p_buf;
            p_buf = nullptr;
            p_cap = 0;
        }

        void clear() {
            if (p_len > 0 && !octa::IsPod<T>()) {
                T *cur = p_buf, *last = p_buf + p_len;
                while (cur != last) (*cur++).~T();
            }
            p_len = 0;
        }

        Vector<T> &operator=(const Vector<T> &v) {
            if (this == &v) return *this;
            clear();
            reserve(v.p_cap);
            p_len = v.p_len;
            if (octa::IsPod<T>()) {
                memcpy(p_buf, v.p_buf, p_len * sizeof(T));
            } else {
                T *cur = p_buf, *last = p_buf + p_len;
                T *vbuf = v.p_buf;
                while (cur != last) {
                    new (cur++) T(*vbuf++);
                }
            }
            return *this;
        }

        Vector<T> &operator=(Vector<T> &&v) {
            clear();
            delete[] (uchar *)p_buf;
            p_len = v.p_len;
            p_cap = v.p_cap;
            p_buf = v.disown();
            return *this;
        }

        Vector<T> &operator=(InitializerList<T> il) {
            clear();
            size_t ilen = il.length();
            reserve(ilen);
            if (octa::IsPod<T>()) {
                memcpy(p_buf, il.get(), ilen);
            } else {
                T *buf = p_buf, *ibuf = il.get(), *last = il.get() + ilen;
                while (ibuf != last) {
                    new (buf++) T(*ibuf++);
                }
            }
            p_len = ilen;
            return *this;
        }

        void resize(size_t n, const T &v = T()) {
            size_t len = p_len;
            reserve(n);
            p_len = n;
            if (octa::IsPod<T>()) {
                for (size_t i = len; i < p_len; ++i) {
                    p_buf[i] = T(v);
                }
            } else {
                T *first = p_buf + len;
                T *last  = p_buf + p_len;
                while (first != last) new (first++) T(v);
            }
        }

        void reserve(size_t n) {
            if (n <= p_len) {
                if (n == p_len) return;
                while (p_len > n) pop();
                return;
            }
            size_t oc = p_cap;
            if (!oc) {
                p_cap = max(n, size_t(MIN_SIZE));
            } else {
                while (p_cap < n) p_cap *= 2;
            }
            T *tmp = (T *)new uchar[p_cap * sizeof(T)];
            if (oc > 0) {
                if (octa::IsPod<T>()) {
                    memcpy(tmp, p_buf, p_len * sizeof(T));
                } else {
                    T *cur = p_buf, *tcur = tmp, *last = tmp + p_len;
                    while (tcur != last) {
                        new (tcur++) T(move(*cur));
                        (*cur).~T();
                        ++cur;
                    }
                }
                delete[] (uchar *)p_buf;
            }
            p_buf = tmp;
        }

        T &operator[](size_t i) { return p_buf[i]; }
        const T &operator[](size_t i) const { return p_buf[i]; }

        T &at(size_t i) { return p_buf[i]; }
        const T &at(size_t i) const { return p_buf[i]; }

        T &push(const T &v) {
            if (p_len == p_cap) reserve(p_len + 1);
            new (&p_buf[p_len]) T(v);
            return p_buf[p_len++];
        }

        T &push() {
            if (p_len == p_cap) reserve(p_len + 1);
            new (&p_buf[p_len]) T;
            return p_buf[p_len++];
        }

        template<typename ...U>
        T &emplace_back(U &&...args) {
            if (p_len == p_cap) reserve(p_len + 1);
            new (&p_buf[p_len]) T(forward<U>(args)...);
            return p_buf[p_len++];
        }

        void pop() {
            if (!octa::IsPod<T>()) {
                p_buf[--p_len].~T();
            } else {
                --p_len;
            }
        }

        T &first() { return p_buf[0]; }
        const T &first() const { return p_buf[0]; };

        T &last() { return p_buf[p_len - 1]; }
        const T &last() const { return p_buf[p_len - 1]; }

        T *get() { return p_buf; }
        const T *get() const { return p_buf; }

        size_t length() const { return p_len; }
        size_t capacity() const { return p_cap; }

        bool empty() const { return (p_len == 0); }

        bool in_range(size_t idx) { return idx < p_len; }
        bool in_range(int idx) { return idx >= 0 && idx < p_len; }
        bool in_range(const T *ptr) {
            return ptr >= p_buf && ptr < &p_buf[p_len];
        }

        T *disown() {
            T *r = p_buf;
            p_buf = nullptr;
            p_len = p_cap = 0;
            return r;
        }

        T *insert(size_t idx, T &&v) {
            insert_base(idx, 1);
            p_buf[idx] = move(v);
            return &p_buf[idx];
        }

        T *insert(size_t idx, const T &v) {
            insert_base(idx, 1);
            p_buf[idx] = v;
            return &p_buf[idx];
        }

        T *insert(size_t idx, size_t n, const T &v) {
            insert_base(idx, n);
            for (size_t i = 0; i < n; ++i) {
                p_buf[idx + i] = v;
            }
            return &p_buf[idx];
        }

        template<typename U>
        T *insert_range(size_t idx, U range) {
            size_t len = range.length();
            insert_base(idx, len);
            for (size_t i = 0; i < len; ++i) {
                p_buf[idx + i] = range.first();
                range.pop_first();
            }
            return &p_buf[idx];
        }

        T *insert(size_t idx, InitializerList<T> il) {
            return insert_range(idx, il.range());
        }

        RangeType each() {
            return PointerRange<T>(p_buf, p_buf + p_len);
        }
        ConstRangeType each() const {
            return PointerRange<const T>(p_buf, p_buf + p_len);
        }

        void swap(Vector &v) {
            swap(p_len, v.p_len);
            swap(p_cap, v.p_cap);
            swap(p_buf, v.p_buf);
        }
    };

    template<typename T>
    void swap(Vector<T> &a, Vector<T> &b) {
        a.swap(b);
    }
}

#endif