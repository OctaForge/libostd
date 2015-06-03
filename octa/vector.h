/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_VECTOR_H
#define OCTA_VECTOR_H

#include <string.h>
#include <stddef.h>

#include "octa/type_traits.h"
#include "octa/utility.h"
#include "octa/range.h"
#include "octa/algorithm.h"
#include "octa/initializer_list.h"
#include "octa/memory.h"

namespace octa {
    template<typename _T, typename _A, bool = octa::IsEmpty<_A>::value>
    struct __OctaVectorPair;

    template<typename _T, typename _A>
    struct __OctaVectorPair<_T, _A, false> { /* non-empty allocator */
        _T *__ptr;
        _A  __a;

        template<typename _U>
        __OctaVectorPair(_T *__ptr, _U &&__a): __ptr(__ptr),
            __a(octa::forward<_U>(__a)) {}

        _A &__get_alloc() { return __a; }
        const _A &__get_alloc() const { return __a; }

        void swap(__OctaVectorPair &__v) {
            octa::swap(__ptr, __v.__ptr);
            octa::swap(__a  , __v.__a  );
        }
    };

    template<typename _T, typename _A>
    struct __OctaVectorPair<_T, _A, true>: _A { /* empty allocator */
        _T *__ptr;

        template<typename _U>
        __OctaVectorPair(_T *__ptr, _U &&__a):
            _A(octa::forward<_U>(__a)), __ptr(__ptr) {}

        _A &__get_alloc() { return *this; }
        const _A &__get_alloc() const { return *this; }

        void swap(__OctaVectorPair &__v) {
            octa::swap(__ptr, __v.__ptr);
        }
    };

    template<typename _T, typename _A = octa::Allocator<_T>>
    class Vector {
        __OctaVectorPair<_T, _A> __buf;
        size_t __len, __cap;

        void __insert_base(size_t __idx, size_t __n) {
            if (__len + __n > __cap) reserve(__len + __n);
            __len += __n;
            for (size_t __i = __len - 1; __i > __idx + __n - 1; --__i) {
                __buf.__ptr[__i] = octa::move(__buf.__ptr[__i - __n]);
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
                octa::allocator_construct(__buf.__get_alloc(),
                    &__buf.__ptr[__i], __range.front());
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
                octa::allocator_construct(__buf.__get_alloc(),
                    &__buf.__ptr[__i], __range.front());
                ++__i;
                __len = __i;
            }
        }

        void __copy_contents(const Vector &__v) {
            if (octa::IsPod<_T>()) {
                memcpy(__buf.__ptr, __v.__buf.__ptr, __len * sizeof(_T));
            } else {
                _T *__cur = __buf.__ptr, *__last = __buf.__ptr + __len;
                _T *__vbuf = __v.__buf.__ptr;
                while (__cur != __last) {
                    octa::allocator_construct(__buf.__get_alloc(),
                        __cur++, *__vbuf++);
                }
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
        typedef _A                      Allocator;

        Vector(const _A &__a = _A()): __buf(nullptr, __a), __len(0), __cap(0) {}

        explicit Vector(size_t __n, const _T &__val = _T(),
        const _A &__al = _A()): Vector(__al) {
            __buf.__ptr = octa::allocator_allocate(__buf.__get_alloc(), __n);
            __len = __cap = __n;
            _T *__cur = __buf.__ptr, *__last = __buf.__ptr + __n;
            while (__cur != __last)
                octa::allocator_construct(__buf.__get_alloc(), __cur++, __val);
        }

        Vector(const Vector &__v): __buf(nullptr,
        octa::allocator_container_copy(__v.__buf.__get_alloc())), __len(0),
        __cap(0) {
            reserve(__v.__cap);
            __len = __v.__len;
            __copy_contents(__v);
        }

        Vector(const Vector &__v, const _A &__a): __buf(nullptr, __a),
        __len(0), __cap(0) {
            reserve(__v.__cap);
            __len = __v.__len;
            __copy_contents(__v);
        }

        Vector(Vector &&__v): __buf(__v.__buf.__ptr,
        octa::move(__v.__buf.__get_alloc())), __len(__v.__len), __cap(__v.__cap) {
            __v.__buf.__ptr = nullptr;
            __v.__len = __v.__cap = 0;
        }

        Vector(Vector &&__v, const _A &__a) {
            if (__a != __v.__a) {
                __buf.__get_alloc() = __a;
                reserve(__v.__cap);
                __len = __v.__len;
                if (octa::IsPod<_T>()) {
                    memcpy(__buf.__ptr, __v.__buf.__ptr, __len * sizeof(_T));
                } else {
                    _T *__cur = __buf.__ptr, *__last = __buf.__ptr + __len;
                    _T *__vbuf = __v.__buf.__ptr;
                    while (__cur != __last) {
                        octa::allocator_construct(__buf.__get_alloc(), __cur++,
                            octa::move(*__vbuf++));
                    }
                }
                return;
            }
            new (&__buf) __OctaVectorPair<_T, _A>(__v.__buf.__ptr,
                octa::move(__v.__buf.__get_alloc()));
            __len = __v.__len;
            __cap = __v.__cap;
            __v.__buf.__ptr = nullptr;
            __v.__len = __v.__cap = 0;
        }

        Vector(InitializerList<_T> __v, const _A &__a = _A()): Vector(__a) {
            size_t __l = __v.end() - __v.begin();
            const _T *__ptr = __v.begin();
            reserve(__l);
            for (size_t __i = 0; __i < __l; ++__i)
                octa::allocator_construct(__buf.__get_alloc(),
                    &__buf.__ptr[__i], __ptr[__i]);
            __len = __l;
        }

        template<typename _R> Vector(_R __range, const _A &__a = _A()):
        Vector(__a) {
            __ctor_from_range(__range);
        }

        ~Vector() {
            clear();
            octa::allocator_deallocate(__buf.__get_alloc(), __buf.__ptr, __cap);
        }

        void clear() {
            if (__len > 0 && !octa::IsPod<_T>()) {
                _T *__cur = __buf.__ptr, *__last = __buf.__ptr + __len;
                while (__cur != __last)
                    octa::allocator_destroy(__buf.__get_alloc(), __cur++);
            }
            __len = 0;
        }

        Vector &operator=(const Vector &__v) {
            if (this == &__v) return *this;
            clear();
            reserve(__v.__cap);
            __len = __v.__len;
            __copy_contents(__v);
            return *this;
        }

        Vector &operator=(Vector &&__v) {
            clear();
            octa::allocator_deallocate(__buf.__get_alloc(), __buf.__ptr, __cap);
            __len = __v.__len;
            __cap = __v.__cap;
            __buf.~__OctaVectorPair<_T, _A>();
            new (&__buf) __OctaVectorPair<_T, _A>(__v.disown(),
                octa::move(__v.__buf.__get_alloc()));
            return *this;
        }

        Vector &operator=(InitializerList<_T> __il) {
            clear();
            size_t __ilen = __il.end() - __il.begin();
            reserve(__ilen);
            if (octa::IsPod<_T>()) {
                memcpy(__buf.__ptr, __il.begin(), __ilen);
            } else {
                _T *__tbuf = __buf.__ptr, *__ibuf = __il.begin(),
                    *__last = __il.end();
                while (__ibuf != __last) {
                    octa::allocator_construct(__buf.__get_alloc(),
                        __tbuf++, *__ibuf++);
                }
            }
            __len = __ilen;
            return *this;
        }

        template<typename _R>
        Vector &operator=(_R __range) {
            clear();
            __ctor_from_range(__range);
        }

        void resize(size_t __n, const _T &__v = _T()) {
            size_t __l = __len;
            reserve(__n);
            __len = __n;
            if (octa::IsPod<_T>()) {
                for (size_t __i = __l; __i < __len; ++__i) {
                    __buf.__ptr[__i] = _T(__v);
                }
            } else {
                _T *__first = __buf.__ptr + __l;
                _T *__last  = __buf.__ptr + __len;
                while (__first != __last)
                    octa::allocator_construct(__buf.__get_alloc(),
                        __first++, __v);
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
            _T *__tmp = octa::allocator_allocate(__buf.__get_alloc(), __cap);
            if (__oc > 0) {
                if (octa::IsPod<_T>()) {
                    memcpy(__tmp, __buf.__ptr, __len * sizeof(_T));
                } else {
                    _T *__cur = __buf.__ptr, *__tcur = __tmp,
                        *__last = __tmp + __len;
                    while (__tcur != __last) {
                        octa::allocator_construct(__buf.__get_alloc(), __tcur++,
                            octa::move(*__cur));
                        octa::allocator_destroy(__buf.__get_alloc(), __cur);
                        ++__cur;
                    }
                }
                octa::allocator_deallocate(__buf.__get_alloc(), __buf.__ptr,
                    __oc);
            }
            __buf.__ptr = __tmp;
        }

        _T &operator[](size_t __i) { return __buf.__ptr[__i]; }
        const _T &operator[](size_t __i) const { return __buf.__ptr[__i]; }

        _T &at(size_t __i) { return __buf.__ptr[__i]; }
        const _T &at(size_t __i) const { return __buf.__ptr[__i]; }

        _T &push(const _T &__v) {
            if (__len == __cap) reserve(__len + 1);
            octa::allocator_construct(__buf.__get_alloc(),
                &__buf.__ptr[__len], __v);
            return __buf.__ptr[__len++];
        }

        _T &push() {
            if (__len == __cap) reserve(__len + 1);
            octa::allocator_construct(__buf.__get_alloc(), &__buf.__ptr[__len]);
            return __buf.__ptr[__len++];
        }

        template<typename ..._U>
        _T &emplace_back(_U &&...__args) {
            if (__len == __cap) reserve(__len + 1);
            octa::allocator_construct(__buf.__get_alloc(), &__buf.__ptr[__len],
                octa::forward<_U>(__args)...);
            return __buf.__ptr[__len++];
        }

        void pop() {
            if (!octa::IsPod<_T>()) {
                octa::allocator_destroy(__buf.__get_alloc(),
                    &__buf.__ptr[--__len]);
            } else {
                --__len;
            }
        }

        _T &front() { return __buf.__ptr[0]; }
        const _T &front() const { return __buf.__ptr[0]; }

        _T &back() { return __buf.__ptr[__len - 1]; }
        const _T &back() const { return __buf.__ptr[__len - 1]; }

        _T *data() { return __buf.__ptr; }
        const _T *data() const { return __buf.__ptr; }

        size_t size() const { return __len; }
        size_t capacity() const { return __cap; }

        bool empty() const { return (__len == 0); }

        bool in_range(size_t __idx) { return __idx < __len; }
        bool in_range(int __idx) { return __idx >= 0 && size_t(__idx) < __len; }
        bool in_range(const _T *__ptr) {
            return __ptr >= __buf.__ptr && __ptr < &__buf.__ptr[__len];
        }

        _T *disown() {
            _T *__r = __buf.__ptr;
            __buf.__ptr = nullptr;
            __len = __cap = 0;
            return __r;
        }

        _T *insert(size_t __idx, _T &&__v) {
            __insert_base(__idx, 1);
            __buf.__ptr[__idx] = octa::move(__v);
            return &__buf.__ptr[__idx];
        }

        _T *insert(size_t __idx, const _T &__v) {
            __insert_base(__idx, 1);
            __buf.__ptr[__idx] = __v;
            return &__buf.__ptr[__idx];
        }

        _T *insert(size_t __idx, size_t __n, const _T &__v) {
            __insert_base(__idx, __n);
            for (size_t __i = 0; __i < __n; ++__i) {
                __buf.__ptr[__idx + __i] = __v;
            }
            return &__buf.__ptr[__idx];
        }

        template<typename _U>
        _T *insert_range(size_t __idx, _U __range) {
            size_t __l = __range.size();
            __insert_base(__idx, __l);
            for (size_t __i = 0; __i < __l; ++__i) {
                __buf.__ptr[__idx + __i] = __range.front();
                __range.pop_front();
            }
            return &__buf.__ptr[__idx];
        }

        _T *insert(size_t __idx, InitializerList<_T> __il) {
            return insert_range(__idx, octa::each(__il));
        }

        Range each() {
            return Range(__buf.__ptr, __buf.__ptr + __len);
        }
        ConstRange each() const {
            return ConstRange(__buf.__ptr, __buf.__ptr + __len);
        }

        void swap(Vector &__v) {
            octa::swap(__len, __v.__len);
            octa::swap(__cap, __v.__cap);
            __buf.swap(__v.__buf);
        }
    };
}

#endif