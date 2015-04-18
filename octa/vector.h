/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_VECTOR_H
#define OCTA_VECTOR_H

#include <string.h>
#include <stddef.h>

#include "octa/new.h"
#include "octa/traits.h"
#include "octa/utility.h"
#include "octa/range.h"

namespace octa {
    /* Implementation of vector ranges
     *
     * See the range specification as documented on OctaForge wiki.
     */
    template<typename T>
    struct VectorRange: InputRangeBase<VectorRange<T>, RandomAccessRange, T> {
        VectorRange(): p_beg(nullptr), p_end(nullptr) {}
        VectorRange(const VectorRange &v): p_beg(v.p_beg), p_end(v.p_end) {}
        VectorRange(T *beg, T *end): p_beg(beg), p_end(end) {}

        bool operator==(const VectorRange &v) const {
            return p_beg == v.p_beg && p_end == v.p_end;
        }
        bool operator!=(const VectorRange &v) const {
            return p_beg != v.p_beg || p_end != v.p_end;
        }

        /* satisfy InputRange / ForwardRange */
        bool empty() const { return p_beg == nullptr; }

        void pop_first() {
            if (p_beg == nullptr) return;
            if (++p_beg == p_end) p_beg = p_end = nullptr;
        }

              T &first()       { return *p_beg; }
        const T &first() const { return *p_beg; }

        /* satisfy BidirectionalRange */
        void pop_last() {
            if (p_end-- == p_beg) { p_end = nullptr; return; }
            if (p_end   == p_beg) p_beg = p_end = nullptr;
        }

              T &last()       { return *(p_end - 1); }
        const T &last() const { return *(p_end - 1); }

        /* satisfy RandomAccessRange */
        size_t length() const { return p_end - p_beg; }

        VectorRange slice(size_t start, size_t end) {
            return VectorRange(p_beg + start, p_beg + end);
        }

              T &operator[](size_t i)       { return p_beg[i]; }
        const T &operator[](size_t i) const { return p_beg[i]; }

    private:
        T *p_beg, *p_end;
    };

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

        struct type {
            typedef       T               value;
            typedef       T              &reference;
            typedef const T              &const_reference;
            typedef       T              *pointer;
            typedef const T              *const_pointer;
            typedef size_t                size;
            typedef ptrdiff_t             difference;
            typedef VectorRange<      T>  range;
            typedef VectorRange<const T>  const_range;
        };

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
        }

        void clear() {
            if (p_cap > 0) {
                if (octa::IsClass<T>::value) {
                    T *cur = p_buf, *last = p_buf + p_len;
                    while (cur != last) (*cur++).~T();
                }
                delete[] (uchar *)p_buf;
                p_buf = nullptr;
                p_len = p_cap = 0;
            }
        }

        Vector<T> &operator=(const Vector<T> &v) {
            if (this == &v) return *this;

            if (p_cap >= v.p_cap) {
                if (octa::IsClass<T>::value) {
                    T *cur = p_buf, *last = p_buf + p_len;
                    while (cur != last) (*cur++).~T();
                }
                p_len = v.p_len;
            } else {
                clear();
                p_len = v.p_len;
                p_cap = v.p_cap;
                p_buf = (T *)new uchar[p_cap * sizeof(T)];
            }

            if (!octa::IsClass<T>::value) {
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
            p_len = v.p_len;
            p_cap = v.p_cap;
            p_buf = v.disown();
            return *this;
        }

        void resize(size_t n, const T &v = T()) {
            size_t len = p_len;
            reserve(n);
            p_len = n;
            if (!octa::IsClass<T>::value) {
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
                p_cap = (n > MIN_SIZE) ? n : size_t(MIN_SIZE);
            } else {
                while (p_cap < n) p_cap *= 2;
            }
            T *tmp = (T *)new uchar[p_cap * sizeof(T)];
            if (oc > 0) {
                if (!octa::IsClass<T>::value) {
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
            if (octa::IsClass<T>::value) {
                p_buf[--p_len].~T();
            } else {
                --p_len;
            }
        }

        T &pop_ret() {
            return p_buf[--p_len];
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
            p_buf[idx] = forward<T>(v);
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
            return insert_range(idx, VectorRange<const T>(il.get(),
                il.get() + il.length()));
        }

        typename type::range each() {
            return VectorRange<T>(p_buf, p_buf + p_len);
        }
        typename type::const_range each() const {
            return VectorRange<const T>(p_buf, p_buf + p_len);
        }
    };
}

#endif