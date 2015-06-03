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

namespace detail {
    template<typename _T, typename _A, bool = octa::IsEmpty<_A>::value>
    struct VectorPair;

    template<typename _T, typename _A>
    struct VectorPair<_T, _A, false> { /* non-empty allocator */
        _T *p_ptr;
        _A  p_a;

        template<typename _U>
        VectorPair(_T *ptr, _U &&a): p_ptr(ptr),
            p_a(octa::forward<_U>(a)) {}

        _A &get_alloc() { return p_a; }
        const _A &get_alloc() const { return p_a; }

        void swap(VectorPair &v) {
            octa::swap(p_ptr, v.p_ptr);
            octa::swap(p_a  , v.p_a  );
        }
    };

    template<typename _T, typename _A>
    struct VectorPair<_T, _A, true>: _A { /* empty allocator */
        _T *p_ptr;

        template<typename _U>
        VectorPair(_T *ptr, _U &&a):
            _A(octa::forward<_U>(a)), p_ptr(ptr) {}

        _A &get_alloc() { return *this; }
        const _A &get_alloc() const { return *this; }

        void swap(VectorPair &v) {
            octa::swap(p_ptr, v.p_ptr);
        }
    };
} /* namespace detail */

template<typename _T, typename _A = octa::Allocator<_T>>
class Vector {
    typedef octa::detail::VectorPair<_T, _A> _vp_type;

    _vp_type p_buf;
    size_t p_len, p_cap;

    void insert_base(size_t idx, size_t n) {
        if (p_len + n > p_cap) reserve(p_len + n);
        p_len += n;
        for (size_t i = p_len - 1; i > idx + n - 1; --i) {
            p_buf.p_ptr[i] = octa::move(p_buf.p_ptr[i - n]);
        }
    }

    template<typename _R>
    void ctor_from_range(_R &range, octa::EnableIf<
        octa::IsFiniteRandomAccessRange<_R>::value, bool
    > = true) {
        octa::RangeSize<_R> l = range.size();
        reserve(l);
        p_len = l;
        for (size_t i = 0; !range.empty(); range.pop_front()) {
            octa::allocator_construct(p_buf.get_alloc(),
                &p_buf.p_ptr[i], range.front());
            ++i;
        }
    }

    template<typename _R>
    void ctor_from_range(_R &range, EnableIf<
        !octa::IsFiniteRandomAccessRange<_R>::value, bool
    > = true) {
        size_t i = 0;
        for (; !range.empty(); range.pop_front()) {
            reserve(i + 1);
            octa::allocator_construct(p_buf.get_alloc(),
                &p_buf.p_ptr[i], range.front());
            ++i;
            p_len = i;
        }
    }

    void copy_contents(const Vector &v) {
        if (octa::IsPod<_T>()) {
            memcpy(p_buf.p_ptr, v.p_buf.p_ptr, p_len * sizeof(_T));
        } else {
            _T *cur = p_buf.p_ptr, *last = p_buf.p_ptr + p_len;
            _T *vbuf = v.p_buf.p_ptr;
            while (cur != last) {
                octa::allocator_construct(p_buf.get_alloc(),
                   cur++, *vbuf++);
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

    Vector(const _A &a = _A()): p_buf(nullptr, a), p_len(0), p_cap(0) {}

    explicit Vector(size_t n, const _T &val = _T(),
    const _A &al = _A()): Vector(al) {
        p_buf.p_ptr = octa::allocator_allocate(p_buf.get_alloc(), n);
        p_len = p_cap = n;
        _T *cur = p_buf.p_ptr, *last = p_buf.p_ptr + n;
        while (cur != last)
            octa::allocator_construct(p_buf.get_alloc(), cur++, val);
    }

    Vector(const Vector &v): p_buf(nullptr,
    octa::allocator_container_copy(v.p_buf.get_alloc())), p_len(0),
    p_cap(0) {
        reserve(v.p_cap);
        p_len = v.p_len;
        copy_contents(v);
    }

    Vector(const Vector &v, const _A &a): p_buf(nullptr, a),
    p_len(0), p_cap(0) {
        reserve(v.p_cap);
        p_len = v.p_len;
        copy_contents(v);
    }

    Vector(Vector &&v): p_buf(v.p_buf.p_ptr,
    octa::move(v.p_buf.get_alloc())), p_len(v.p_len), p_cap(v.p_cap) {
        v.p_buf.p_ptr = nullptr;
        v.p_len = v.p_cap = 0;
    }

    Vector(Vector &&v, const _A &a) {
        if (a != v.a) {
            p_buf.get_alloc() = a;
            reserve(v.p_cap);
            p_len = v.p_len;
            if (octa::IsPod<_T>()) {
                memcpy(p_buf.p_ptr, v.p_buf.p_ptr, p_len * sizeof(_T));
            } else {
                _T *cur = p_buf.p_ptr, *last = p_buf.p_ptr + p_len;
                _T *vbuf = v.p_buf.p_ptr;
                while (cur != last) {
                    octa::allocator_construct(p_buf.get_alloc(), cur++,
                        octa::move(*vbuf++));
                }
            }
            return;
        }
        new (&p_buf) _vp_type(v.p_buf.p_ptr,
            octa::move(v.p_buf.get_alloc()));
        p_len = v.p_len;
        p_cap = v.p_cap;
        v.p_buf.p_ptr = nullptr;
        v.p_len = v.p_cap = 0;
    }

    Vector(InitializerList<_T> v, const _A &a = _A()): Vector(a) {
        size_t l = v.end() - v.begin();
        const _T *ptr = v.begin();
        reserve(l);
        for (size_t i = 0; i < l; ++i)
            octa::allocator_construct(p_buf.get_alloc(),
                &p_buf.p_ptr[i], ptr[i]);
        p_len = l;
    }

    template<typename _R> Vector(_R range, const _A &a = _A()):
    Vector(a) {
        ctor_from_range(range);
    }

    ~Vector() {
        clear();
        octa::allocator_deallocate(p_buf.get_alloc(), p_buf.p_ptr, p_cap);
    }

    void clear() {
        if (p_len > 0 && !octa::IsPod<_T>()) {
            _T *cur = p_buf.p_ptr, *last = p_buf.p_ptr + p_len;
            while (cur != last)
                octa::allocator_destroy(p_buf.get_alloc(), cur++);
        }
        p_len = 0;
    }

    Vector &operator=(const Vector &v) {
        if (this == &v) return *this;
        clear();
        reserve(v.p_cap);
        p_len = v.p_len;
        copy_contents(v);
        return *this;
    }

    Vector &operator=(Vector &&v) {
        clear();
        octa::allocator_deallocate(p_buf.get_alloc(), p_buf.p_ptr, p_cap);
        p_len = v.p_len;
        p_cap = v.p_cap;
        p_buf.~_vp_type();
        new (&p_buf) _vp_type(v.disown(), octa::move(v.p_buf.get_alloc()));
        return *this;
    }

    Vector &operator=(InitializerList<_T> il) {
        clear();
        size_t ilen = il.end() - il.begin();
        reserve(ilen);
        if (octa::IsPod<_T>()) {
            memcpy(p_buf.p_ptr, il.begin(), ilen);
        } else {
            _T *tbuf = p_buf.p_ptr, *ibuf = il.begin(),
                *last = il.end();
            while (ibuf != last) {
                octa::allocator_construct(p_buf.get_alloc(),
                    tbuf++, *ibuf++);
            }
        }
        p_len = ilen;
        return *this;
    }

    template<typename _R>
    Vector &operator=(_R range) {
        clear();
        ctor_from_range(range);
    }

    void resize(size_t n, const _T &v = _T()) {
        size_t l = p_len;
        reserve(n);
        p_len = n;
        if (octa::IsPod<_T>()) {
            for (size_t i = l; i < p_len; ++i) {
                p_buf.p_ptr[i] = _T(v);
            }
        } else {
            _T *first = p_buf.p_ptr + l;
            _T *last  = p_buf.p_ptr + p_len;
            while (first != last)
                octa::allocator_construct(p_buf.get_alloc(), first++, v);
        }
    }

    void reserve(size_t n) {
        if (n <= p_cap) return;
        size_t oc = p_cap;
        if (!oc) {
            p_cap = octa::max(n, size_t(MIN_SIZE));
        } else {
            while (p_cap < n) p_cap *= 2;
        }
        _T *tmp = octa::allocator_allocate(p_buf.get_alloc(), p_cap);
        if (oc > 0) {
            if (octa::IsPod<_T>()) {
                memcpy(tmp, p_buf.p_ptr, p_len * sizeof(_T));
            } else {
                _T *cur = p_buf.p_ptr, *tcur = tmp,
                    *last = tmp + p_len;
                while (tcur != last) {
                    octa::allocator_construct(p_buf.get_alloc(), tcur++,
                        octa::move(*cur));
                    octa::allocator_destroy(p_buf.get_alloc(), cur);
                    ++cur;
                }
            }
            octa::allocator_deallocate(p_buf.get_alloc(), p_buf.p_ptr, oc);
        }
        p_buf.p_ptr = tmp;
    }

    _T &operator[](size_t i) { return p_buf.p_ptr[i]; }
    const _T &operator[](size_t i) const { return p_buf.p_ptr[i]; }

    _T &at(size_t i) { return p_buf.p_ptr[i]; }
    const _T &at(size_t i) const { return p_buf.p_ptr[i]; }

    _T &push(const _T &v) {
        if (p_len == p_cap) reserve(p_len + 1);
        octa::allocator_construct(p_buf.get_alloc(),
            &p_buf.p_ptr[p_len], v);
        return p_buf.p_ptr[p_len++];
    }

    _T &push() {
        if (p_len == p_cap) reserve(p_len + 1);
        octa::allocator_construct(p_buf.get_alloc(), &p_buf.p_ptr[p_len]);
        return p_buf.p_ptr[p_len++];
    }

    template<typename ..._U>
    _T &emplace_back(_U &&...args) {
        if (p_len == p_cap) reserve(p_len + 1);
        octa::allocator_construct(p_buf.get_alloc(), &p_buf.p_ptr[p_len],
            octa::forward<_U>(args)...);
        return p_buf.p_ptr[p_len++];
    }

    void pop() {
        if (!octa::IsPod<_T>()) {
            octa::allocator_destroy(p_buf.get_alloc(),
                &p_buf.p_ptr[--p_len]);
        } else {
            --p_len;
        }
    }

    _T &front() { return p_buf.p_ptr[0]; }
    const _T &front() const { return p_buf.p_ptr[0]; }

    _T &back() { return p_buf.p_ptr[p_len - 1]; }
    const _T &back() const { return p_buf.p_ptr[p_len - 1]; }

    _T *data() { return p_buf.p_ptr; }
    const _T *data() const { return p_buf.p_ptr; }

    size_t size() const { return p_len; }
    size_t capacity() const { return p_cap; }

    bool empty() const { return (p_len == 0); }

    bool in_range(size_t idx) { return idx < p_len; }
    bool in_range(int idx) { return idx >= 0 && size_t(idx) < p_len; }
    bool in_range(const _T *ptr) {
        return ptr >= p_buf.p_ptr && ptr < &p_buf.p_ptr[p_len];
    }

    _T *disown() {
        _T *r = p_buf.p_ptr;
        p_buf.p_ptr = nullptr;
        p_len = p_cap = 0;
        return r;
    }

    _T *insert(size_t idx, _T &&v) {
        insert_base(idx, 1);
        p_buf.p_ptr[idx] = octa::move(v);
        return &p_buf.p_ptr[idx];
    }

    _T *insert(size_t idx, const _T &v) {
        insert_base(idx, 1);
        p_buf.p_ptr[idx] = v;
        return &p_buf.p_ptr[idx];
    }

    _T *insert(size_t idx, size_t n, const _T &v) {
        insert_base(idx, n);
        for (size_t i = 0; i < n; ++i) {
            p_buf.p_ptr[idx + i] = v;
        }
        return &p_buf.p_ptr[idx];
    }

    template<typename _U>
    _T *insert_range(size_t idx, _U range) {
        size_t l = range.size();
        insert_base(idx, l);
        for (size_t i = 0; i < l; ++i) {
            p_buf.p_ptr[idx + i] = range.front();
            range.pop_front();
        }
        return &p_buf.p_ptr[idx];
    }

    _T *insert(size_t idx, InitializerList<_T> il) {
        return insert_range(idx, octa::each(il));
    }

    Range each() {
        return Range(p_buf.p_ptr, p_buf.p_ptr + p_len);
    }
    ConstRange each() const {
        return ConstRange(p_buf.p_ptr, p_buf.p_ptr + p_len);
    }

    void swap(Vector &v) {
        octa::swap(p_len, v.p_len);
        octa::swap(p_cap, v.p_cap);
        p_buf.swap(v.p_buf);
    }
};

} /* namespace octa */

#endif