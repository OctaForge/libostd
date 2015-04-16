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
    struct VectorRange: Range<VectorRange, T> {
        struct type {
            typedef ptrdiff_t  difference;
            typedef T          value;
            typedef T         *pointer;
            typedef T         &reference;
        };

        VectorRange(): p_beg(nullptr), p_end(nullptr) {}
        VectorRange(const VectorRange &v): p_beg(v.p_beg), p_end(v.p_end) {}
        VectorRange(VectorRange &&v): p_beg(v.p_beg), p_end(v.p_end) {
            v.p_beg = v.p_end = nullptr;
        }
        VectorRange(T *beg, T *end): p_beg(beg), p_end(end) {}

        /* satisfy InputRange / ForwardRange */
        bool empty() const { return p_beg == nullptr; }

        void pop_first() {
            if (p_beg == nullptr) return;
            if (++p_beg == p_end) p_beg = p_end = nullptr;
        }

              T &first()       { return *p_beg; }
        const T &first() const { return *p_beg; }

        bool operator==(const VectorRange &v) const {
            return p_beg == v.p_beg && p_end == v.p_end;
        }
        bool operator!=(const VectorRange &v) const {
            return p_beg != v.p_beg || p_end != v.p_end;
        }

        /* satisfy BidirectionalRange */
        void pop_last() {
            if (p_end-- == p_beg) { p_end = nullptr; return; }
            if (p_end   == p_beg) p_beg = p_end = nullptr;
        }

              T &last()       { return *(p_end - 1); }
        const T &last() const { return *(p_end - 1); }

        /* satisfy RandomAccessRange */
        size_t length() const { return p_end - p_beg; }

              T &operator[](size_t i)       { return p_beg[i]; }
        const T &operator[](size_t i) const { return p_beg[i]; }

    private:
        T *p_beg, *p_end;
    };

    template<typename T>
    class Vector {
        T *p_buf;
        size_t p_len, p_cap;

    public:
        enum { MIN_SIZE = 8 };

        struct type {
            typedef       T   value;
            typedef       T  &reference;
            typedef const T  &const_reference;
            typedef       T  *pointer;
            typedef const T  *const_pointer;
            typedef size_t    size;
            typedef ptrdiff_t difference;
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

        Vector(initializer_list<T> v): Vector() {
            insert(0, v.length(), v.get());
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
                p_cap = (n > MIN_SIZE) ? n : MIN_SIZE;
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

        T &insert(size_t idx, const T &v) {
            push();
            for (size_t i = p_len - 1; i > idx; --i) p_buf[i] = p_buf[i - 1];
            p_buf[idx] = v;
        }

        T *insert(size_t idx, size_t n, const T *v) {
            if (p_len + n > p_cap) reserve(p_len + n);
            for (size_t i = 0; i < n; ++i) push();
            for (size_t i = p_len - 1; i > idx + n; --i)
                p_buf[i] = p_buf[i - n];
            for (size_t i = 0; i < n; ++i) p_buf[idx + i] = v[i];
            return &p_buf[idx];
        }

        VectorRange<T> each() {
            return VectorRange<T>(p_buf, p_buf + p_len);
        }
    };
}

#endif