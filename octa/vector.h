/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_VECTOR_H
#define OCTA_VECTOR_H

#include <string.h>

#include "octa/new.h"
#include "octa/traits.h"

namespace octa {
    template<typename T>
    class Vector {
        T *p_buf;
        size_t p_len, p_cap;

    public:
        enum { MIN_SIZE = 8 };

        explicit Vector(): p_buf(NULL), p_len(0), p_cap(0) {}

        Vector(const Vector &v): p_buf(NULL), p_len(0), p_cap(0) {
            *this = v;
        }

        Vector(size_t n, const T &val = T()): Vector() {
            p_buf = new uchar[n * sizeof(T)];
            p_len = p_cap = n;
            T *cur = p_buf, *last = p_buf + n;
            while (cur != last) new (cur++) T(val);
        }

        ~Vector() {
            clear();
        }

        void clear() {
            if (p_cap > 0) {
                if (octa::IsClass<T>::value) {
                    T *cur = p_buf, *last = p_buf + p_len;
                }
                delete[] (uchar *)p_buf;
                p_buf = NULL;
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
                        new (tcur++) T(*cur);
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

        void pop() {
            if (octa::IsClass<T>::value) {
                p_buf[--p_len].~T();
            } else {
                --p_len;
            }
        }

        T *get() { return p_buf; }

        size_t length() const { return p_len; }
        size_t capacity() const { return p_cap; }

        bool empty() const { return (p_len == 0); }
    };
}

#endif