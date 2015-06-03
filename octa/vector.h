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
    template<typename _T>
    class Vector {
        _T *__buf;
        size_t __len, __cap;

        void __insert_base(size_t __idx, size_t __n) {
            if (__len + __n > __cap) reserve(__len + __n);
            __len += __n;
            for (size_t __i = __len - 1; __i > __idx + __n - 1; --__i) {
                __buf[__i] = octa::move(__buf[__i - __n]);
            }
        }

        template<typename _R>
        void __ctor_from_range(_R &__range, octa::EnableIf<
            octa::IsFiniteRandomAccessRange<_R>::value, bool
        > = true) {
            octa::RangeSize<_R> __l = __range.size();
            reserve(__l);
            __len = __l;
            for (size_t __i = 0; !__range.empty(); __range.pop_front()) {
                new (&__buf[__i]) _T(__range.front());
                ++__i;
            }
        }

        template<typename _R>
        void __ctor_from_range(_R &__range, EnableIf<
            !octa::IsFiniteRandomAccessRange<_R>::value, bool
        > = true) {
            size_t __i = 0;
            for (; !__range.empty(); __range.pop_front()) {
                reserve(__i + 1);
                new (&__buf[__i]) _T(__range.front());
                ++__i;
                __len = __i;
            }
        }

    public:
        enum { MIN_SIZE = 8 };

        typedef size_t                  Size;
        typedef ptrdiff_t               Difference;
        typedef       _T                Value;
        typedef       _T               &Reference;
        typedef const _T               &ConstReference;
        typedef       _T               *Pointer;
        typedef const _T               *ConstPointer;
        typedef PointerRange<      _T>  Range;
        typedef PointerRange<const _T>  ConstRange;

        Vector(): __buf(nullptr), __len(0), __cap(0) {}

        explicit Vector(size_t __n, const _T &__val = _T()): Vector() {
            __buf = (_T *)new uchar[__n * sizeof(_T)];
            __len = __cap = __n;
            _T *__cur = __buf, *__last = __buf + __n;
            while (__cur != __last) new (__cur++) _T(__val);
        }

        Vector(const Vector &__v): Vector() {
            *this = __v;
        }

        Vector(Vector &&__v): __buf(__v.__buf), __len(__v.__len),
                              __cap(__v.__cap) {
            __v.__buf = nullptr;
            __v.__len = __v.__cap = 0;
        }

        Vector(InitializerList<_T> __v): Vector() {
            size_t __l = __v.end() - __v.begin();
            const _T *__ptr = __v.begin();
            reserve(__l);
            for (size_t __i = 0; __i < __l; ++__i)
                new (&__buf[__i]) _T(__ptr[__i]);
            __len = __l;
        }

        template<typename _R> Vector(_R __range): Vector() {
            __ctor_from_range(__range);
        }

        ~Vector() {
            clear();
            delete[] (uchar *)__buf;
        }

        void clear() {
            if (__len > 0 && !octa::IsPod<_T>()) {
                _T *__cur = __buf, *__last = __buf + __len;
                while (__cur != __last) (*__cur++).~_T();
            }
            __len = 0;
        }

        Vector<_T> &operator=(const Vector<_T> &__v) {
            if (this == &__v) return *this;
            clear();
            reserve(__v.__cap);
            __len = __v.__len;
            if (octa::IsPod<_T>()) {
                memcpy(__buf, __v.__buf, __len * sizeof(_T));
            } else {
                _T *__cur = __buf, *__last = __buf + __len;
                _T *__vbuf = __v.__buf;
                while (__cur != __last) {
                    new (__cur++) _T(*__vbuf++);
                }
            }
            return *this;
        }

        Vector<_T> &operator=(Vector<_T> &&__v) {
            clear();
            delete[] (uchar *)__buf;
            __len = __v.__len;
            __cap = __v.__cap;
            __buf = __v.disown();
            return *this;
        }

        Vector<_T> &operator=(InitializerList<_T> __il) {
            clear();
            size_t __ilen = __il.end() - __il.begin();
            reserve(__ilen);
            if (octa::IsPod<_T>()) {
                memcpy(__buf, __il.begin(), __ilen);
            } else {
                _T *__tbuf = __buf, *__ibuf = __il.begin(), *__last = __il.end();
                while (__ibuf != __last) {
                    new (__tbuf++) _T(*__ibuf++);
                }
            }
            __len = __ilen;
            return *this;
        }

        template<typename _R>
        Vector<_T> &operator=(_R __range) {
            clear();
            __ctor_from_range(__range);
        }

        void resize(size_t __n, const _T &__v = _T()) {
            size_t __l = __len;
            reserve(__n);
            __len = __n;
            if (octa::IsPod<_T>()) {
                for (size_t __i = __l; __i < __len; ++__i) {
                    __buf[__i] = _T(__v);
                }
            } else {
                _T *__first = __buf + __l;
                _T *__last  = __buf + __len;
                while (__first != __last) new (__first++) _T(__v);
            }
        }

        void reserve(size_t __n) {
            if (__n <= __cap) return;
            size_t __oc = __cap;
            if (!__oc) {
                __cap = octa::max(__n, size_t(MIN_SIZE));
            } else {
                while (__cap < __n) __cap *= 2;
            }
            _T *__tmp = (_T *)new uchar[__cap * sizeof(_T)];
            if (__oc > 0) {
                if (octa::IsPod<_T>()) {
                    memcpy(__tmp, __buf, __len * sizeof(_T));
                } else {
                    _T *__cur = __buf, *__tcur = __tmp, *__last = __tmp + __len;
                    while (__tcur != __last) {
                        new (__tcur++) _T(octa::move(*__cur));
                        (*__cur).~_T();
                        ++__cur;
                    }
                }
                delete[] (uchar *)__buf;
            }
            __buf = __tmp;
        }

        _T &operator[](size_t __i) { return __buf[__i]; }
        const _T &operator[](size_t __i) const { return __buf[__i]; }

        _T &at(size_t __i) { return __buf[__i]; }
        const _T &at(size_t __i) const { return __buf[__i]; }

        _T &push(const _T &__v) {
            if (__len == __cap) reserve(__len + 1);
            new (&__buf[__len]) _T(__v);
            return __buf[__len++];
        }

        _T &push() {
            if (__len == __cap) reserve(__len + 1);
            new (&__buf[__len]) _T;
            return __buf[__len++];
        }

        template<typename ..._U>
        _T &emplace_back(_U &&...__args) {
            if (__len == __cap) reserve(__len + 1);
            new (&__buf[__len]) _T(octa::forward<_U>(__args)...);
            return __buf[__len++];
        }

        void pop() {
            if (!octa::IsPod<_T>()) {
                __buf[--__len].~_T();
            } else {
                --__len;
            }
        }

        _T &front() { return __buf[0]; }
        const _T &front() const { return __buf[0]; };

        _T &back() { return __buf[__len - 1]; }
        const _T &back() const { return __buf[__len - 1]; }

        _T *data() { return __buf; }
        const _T *data() const { return __buf; }

        size_t size() const { return __len; }
        size_t capacity() const { return __cap; }

        bool empty() const { return (__len == 0); }

        bool in_range(size_t __idx) { return __idx < __len; }
        bool in_range(int __idx) { return __idx >= 0 && size_t(__idx) < __len; }
        bool in_range(const _T *__ptr) {
            return __ptr >= __buf && __ptr < &__buf[__len];
        }

        _T *disown() {
            _T *__r = __buf;
            __buf = nullptr;
            __len = __cap = 0;
            return __r;
        }

        _T *insert(size_t __idx, _T &&__v) {
            __insert_base(__idx, 1);
            __buf[__idx] = octa::move(__v);
            return &__buf[__idx];
        }

        _T *insert(size_t __idx, const _T &__v) {
            __insert_base(__idx, 1);
            __buf[__idx] = __v;
            return &__buf[__idx];
        }

        _T *insert(size_t __idx, size_t __n, const _T &__v) {
            __insert_base(__idx, __n);
            for (size_t __i = 0; __i < __n; ++__i) {
                __buf[__idx + __i] = __v;
            }
            return &__buf[__idx];
        }

        template<typename _U>
        _T *insert_range(size_t __idx, _U __range) {
            size_t __l = __range.size();
            __insert_base(__idx, __l);
            for (size_t __i = 0; __i < __l; ++__i) {
                __buf[__idx + __i] = __range.front();
                __range.pop_front();
            }
            return &__buf[__idx];
        }

        _T *insert(size_t __idx, InitializerList<_T> __il) {
            return insert_range(__idx, octa::each(__il));
        }

        Range each() {
            return Range(__buf, __buf + __len);
        }
        ConstRange each() const {
            return ConstRange(__buf, __buf + __len);
        }

        void swap(Vector &__v) {
            octa::swap(__len, __v.__len);
            octa::swap(__cap, __v.__cap);
            octa::swap(__buf, __v.__buf);
        }
    };
}

#endif