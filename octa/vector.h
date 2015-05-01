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

        void insert_base(size_t idx, size_t n) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveAssignable<T>::value
        ) {
            if (p_len + n > p_cap) reserve(p_len + n);
            p_len += n;
            for (size_t i = p_len - 1; i > idx + n - 1; --i) {
                p_buf[i] = move(p_buf[i - n]);
            }
        }

    public:
        enum { MIN_SIZE = 8 };

        typedef size_t                 size_type;
        typedef ptrdiff_t              difference_type;
        typedef       T                value_type;
        typedef       T               &reference;
        typedef const T               &const_reference;
        typedef       T               *pointer;
        typedef const T               *const_pointer;
        typedef PointerRange<      T>  range;
        typedef PointerRange<const T>  const_range;

        Vector() noexcept: p_buf(nullptr), p_len(0), p_cap(0) {}

        explicit Vector(size_t n, const T &val = T()) noexcept(
            IsNothrowCopyConstructible<T>::value
        ): Vector() {
            p_buf = new uchar[n * sizeof(T)];
            p_len = p_cap = n;
            T *cur = p_buf, *last = p_buf + n;
            while (cur != last) new (cur++) T(val);
        }

        Vector(const Vector &v) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowCopyConstructible<T>::value
        ): Vector() {
            *this = v;
        }

        Vector(Vector &&v) noexcept: p_buf(v.p_buf), p_len(v.p_len),
        p_cap(v.p_cap) {
            v.p_buf = nullptr;
            v.p_len = v.p_cap = 0;
        }

        Vector(InitializerList<T> v) noexcept(
            IsNothrowCopyConstructible<T>::value
        ): Vector() {
            size_t len = v.length();
            const T *ptr = v.get();
            reserve(len);
            for (size_t i = 0; i < len; ++i)
                new (&p_buf[i]) T(ptr[i]);
            p_len = len;
        }

        ~Vector() noexcept(IsNothrowDestructible<T>::value) {
            clear();
        }

        void clear() noexcept(IsNothrowDestructible<T>::value) {
            if (p_cap > 0) {
                if (!octa::IsPod<T>()) {
                    T *cur = p_buf, *last = p_buf + p_len;
                    while (cur != last) (*cur++).~T();
                }
                delete[] (uchar *)p_buf;
                p_buf = nullptr;
                p_len = p_cap = 0;
            }
        }

        Vector<T> &operator=(const Vector<T> &v) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowCopyConstructible<T>::value
        ) {
            if (this == &v) return *this;

            if (p_cap >= v.p_cap) {
                if (!octa::IsPod<T>()) {
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

        Vector<T> &operator=(Vector<T> &&v) noexcept(
            IsNothrowDestructible<T>::value
        ) {
            clear();
            p_len = v.p_len;
            p_cap = v.p_cap;
            p_buf = v.disown();
            return *this;
        }

        void resize(size_t n, const T &v = T()) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
         && IsNothrowCopyConstructible<T>::value
        ) {
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

        void reserve(size_t n) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
        ) {
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

        T &operator[](size_t i) noexcept { return p_buf[i]; }
        const T &operator[](size_t i) const noexcept { return p_buf[i]; }

        T &at(size_t i) noexcept { return p_buf[i]; }
        const T &at(size_t i) const noexcept { return p_buf[i]; }

        T &push(const T &v) noexcept(
            noexcept(reserve(p_len + 1))
            && IsNothrowCopyConstructible<T>::value
        ) {
            if (p_len == p_cap) reserve(p_len + 1);
            new (&p_buf[p_len]) T(v);
            return p_buf[p_len++];
        }

        T &push() noexcept(
            noexcept(reserve(p_len + 1))
            && IsNothrowDefaultConstructible<T>::value
        ) {
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

        void pop() noexcept(IsNothrowDestructible<T>::value) {
            if (!octa::IsPod<T>()) {
                p_buf[--p_len].~T();
            } else {
                --p_len;
            }
        }

        T &pop_ret() noexcept {
            return p_buf[--p_len];
        }

        T &first() noexcept { return p_buf[0]; }
        const T &first() const noexcept { return p_buf[0]; };

        T &last() noexcept { return p_buf[p_len - 1]; }
        const T &last() const noexcept { return p_buf[p_len - 1]; }

        T *get() noexcept { return p_buf; }
        const T *get() const noexcept { return p_buf; }

        size_t length() const noexcept { return p_len; }
        size_t capacity() const noexcept { return p_cap; }

        bool empty() const noexcept { return (p_len == 0); }

        bool in_range(size_t idx) noexcept { return idx < p_len; }
        bool in_range(int idx) noexcept { return idx >= 0 && idx < p_len; }
        bool in_range(const T *ptr) noexcept {
            return ptr >= p_buf && ptr < &p_buf[p_len];
        }

        T *disown() noexcept {
            T *r = p_buf;
            p_buf = nullptr;
            p_len = p_cap = 0;
            return r;
        }

        T *insert(size_t idx, T &&v) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveAssignable<T>::value
        ) {
            insert_base(idx, 1);
            p_buf[idx] = move(v);
            return &p_buf[idx];
        }

        T *insert(size_t idx, const T &v) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveAssignable<T>::value
         && IsNothrowCopyAssignable<T>::value
        ) {
            insert_base(idx, 1);
            p_buf[idx] = v;
            return &p_buf[idx];
        }

        T *insert(size_t idx, size_t n, const T &v) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveAssignable<T>::value
         && IsNothrowCopyAssignable<T>::value
        ) {
            insert_base(idx, n);
            for (size_t i = 0; i < n; ++i) {
                p_buf[idx + i] = v;
            }
            return &p_buf[idx];
        }

        template<typename U>
        T *insert_range(size_t idx, U range) noexcept(
            IsNothrowDestructible<T>::value
         && IsNothrowMoveConstructible<T>::value
         && IsNothrowMoveAssignable<T>::value
         && noexcept(range.first())
         && noexcept(range.pop_first())
         && noexcept((*p_buf = range.first()))
        ) {
            size_t len = range.length();
            insert_base(idx, len);
            for (size_t i = 0; i < len; ++i) {
                p_buf[idx + i] = range.first();
                range.pop_first();
            }
            return &p_buf[idx];
        }

        T *insert(size_t idx, InitializerList<T> il) noexcept(
            noexcept(declval<Vector<T>>().insert_range(idx, il.range()))
        ) {
            return insert_range(idx, il.range());
        }

        range each() noexcept {
            return PointerRange<T>(p_buf, p_buf + p_len);
        }
        const_range each() const noexcept {
            return PointerRange<const T>(p_buf, p_buf + p_len);
        }

        void swap(Vector &v) noexcept {
            swap(p_len, v.p_len);
            swap(p_cap, v.p_cap);
            swap(p_buf, v.p_buf);
        }
    };

    template<typename T>
    void swap(Vector<T> &a, Vector<T> &b) noexcept {
        a.swap(b);
    }
}

#endif