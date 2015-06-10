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
    template<typename A, bool = octa::IsEmpty<A>::value>
    struct VectorPair;

    template<typename A>
    struct VectorPair<A, false> { /* non-empty allocator */
        octa::AllocatorPointer<A> p_ptr;
        A p_a;

        template<typename U>
        VectorPair(octa::AllocatorPointer<A> ptr, U &&a): p_ptr(ptr),
            p_a(octa::forward<U>(a)) {}

        A &get_alloc() { return p_a; }
        const A &get_alloc() const { return p_a; }

        void swap(VectorPair &v) {
            octa::swap(p_ptr, v.p_ptr);
            octa::swap(p_a  , v.p_a  );
        }
    };

    template<typename A>
    struct VectorPair<A, true>: A { /* empty allocator */
        octa::AllocatorPointer<A> p_ptr;

        template<typename U>
        VectorPair(octa::AllocatorPointer<A> ptr, U &&a):
            A(octa::forward<U>(a)), p_ptr(ptr) {}

        A &get_alloc() { return *this; }
        const A &get_alloc() const { return *this; }

        void swap(VectorPair &v) {
            octa::swap(p_ptr, v.p_ptr);
        }
    };
} /* namespace detail */

template<typename T, typename A = octa::Allocator<T>>
class Vector {
    using VecPair = octa::detail::VectorPair<A>;

    VecPair p_buf;
    octa::Size p_len, p_cap;

    void insert_base(octa::Size idx, octa::Size n) {
        if (p_len + n > p_cap) reserve(p_len + n);
        p_len += n;
        for (octa::Size i = p_len - 1; i > idx + n - 1; --i) {
            p_buf.p_ptr[i] = octa::move(p_buf.p_ptr[i - n]);
        }
    }

    template<typename R>
    void ctor_from_range(R &range, octa::EnableIf<
        octa::IsFiniteRandomAccessRange<R>::value, bool
    > = true) {
        octa::RangeSize<R> l = range.size();
        reserve(l);
        p_len = l;
        if (octa::IsPod<T>() && octa::IsSame<T, octa::RangeValue<R>>()) {
            memcpy(p_buf.p_ptr, &range.front(), range.size());
            return;
        }
        for (octa::Size i = 0; !range.empty(); range.pop_front()) {
            octa::allocator_construct(p_buf.get_alloc(),
                &p_buf.p_ptr[i], range.front());
            ++i;
        }
    }

    template<typename R>
    void ctor_from_range(R &range, EnableIf<
        !octa::IsFiniteRandomAccessRange<R>::value, bool
    > = true) {
        octa::Size i = 0;
        for (; !range.empty(); range.pop_front()) {
            reserve(i + 1);
            octa::allocator_construct(p_buf.get_alloc(),
                &p_buf.p_ptr[i], range.front());
            ++i;
            p_len = i;
        }
    }

    void copy_contents(const Vector &v) {
        if (octa::IsPod<T>()) {
            memcpy(p_buf.p_ptr, v.p_buf.p_ptr, p_len * sizeof(T));
        } else {
            Pointer cur = p_buf.p_ptr, last = p_buf.p_ptr + p_len;
            Pointer vbuf = v.p_buf.p_ptr;
            while (cur != last) {
                octa::allocator_construct(p_buf.get_alloc(),
                   cur++, *vbuf++);
            }
        }
    }

public:
    using Size = octa::Size;
    using Difference = octa::Ptrdiff;
    using Value = T;
    using Reference = T &;
    using ConstReference = const T &;
    using Pointer = octa::AllocatorPointer<A>;
    using ConstPointer = octa::AllocatorConstPointer<A>;
    using Range = octa::PointerRange<T>;
    using ConstRange = octa::PointerRange<const T>;
    using Allocator = A;

    Vector(const A &a = A()): p_buf(nullptr, a), p_len(0), p_cap(0) {}

    explicit Vector(Size n, const T &val = T(),
    const A &al = A()): Vector(al) {
        p_buf.p_ptr = octa::allocator_allocate(p_buf.get_alloc(), n);
        p_len = p_cap = n;
        Pointer cur = p_buf.p_ptr, last = p_buf.p_ptr + n;
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

    Vector(const Vector &v, const A &a): p_buf(nullptr, a),
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

    Vector(Vector &&v, const A &a) {
        if (a != v.a) {
            p_buf.get_alloc() = a;
            reserve(v.p_cap);
            p_len = v.p_len;
            if (octa::IsPod<T>()) {
                memcpy(p_buf.p_ptr, v.p_buf.p_ptr, p_len * sizeof(T));
            } else {
                Pointer cur = p_buf.p_ptr, last = p_buf.p_ptr + p_len;
                Pointer vbuf = v.p_buf.p_ptr;
                while (cur != last) {
                    octa::allocator_construct(p_buf.get_alloc(), cur++,
                        octa::move(*vbuf++));
                }
            }
            return;
        }
        new (&p_buf) VecPair(v.p_buf.p_ptr,
            octa::move(v.p_buf.get_alloc()));
        p_len = v.p_len;
        p_cap = v.p_cap;
        v.p_buf.p_ptr = nullptr;
        v.p_len = v.p_cap = 0;
    }

    Vector(ConstPointer buf, Size n, const A &a = A()): Vector(a) {
        reserve(n);
        if (octa::IsPod<T>()) {
            memcpy(p_buf.p_ptr, buf, n * sizeof(T));
        } else {
            for (Size i = 0; i < n; ++i)
                octa::allocator_construct(p_buf.get_alloc(),
                    &p_buf.p_ptr[i], buf[i]);
        }
        p_len = n;
    }

    Vector(InitializerList<T> v, const A &a = A()):
        Vector(v.begin(), v.size(), a) {}

    template<typename R> Vector(R range, const A &a = A()):
    Vector(a) {
        ctor_from_range(range);
    }

    ~Vector() {
        clear();
        octa::allocator_deallocate(p_buf.get_alloc(), p_buf.p_ptr, p_cap);
    }

    void clear() {
        if (p_len > 0 && !octa::IsPod<T>()) {
            Pointer cur = p_buf.p_ptr, last = p_buf.p_ptr + p_len;
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
        p_buf.~VecPair();
        new (&p_buf) VecPair(v.disown(), octa::move(v.p_buf.get_alloc()));
        return *this;
    }

    Vector &operator=(InitializerList<T> il) {
        clear();
        Size ilen = il.end() - il.begin();
        reserve(ilen);
        if (octa::IsPod<T>()) {
            memcpy(p_buf.p_ptr, il.begin(), ilen);
        } else {
            Pointer tbuf = p_buf.p_ptr, ibuf = il.begin(),
                last = il.end();
            while (ibuf != last) {
                octa::allocator_construct(p_buf.get_alloc(),
                    tbuf++, *ibuf++);
            }
        }
        p_len = ilen;
        return *this;
    }

    template<typename R>
    Vector &operator=(R range) {
        clear();
        ctor_from_range(range);
        return *this;
    }

    void resize(Size n, const T &v = T()) {
        Size l = p_len;
        reserve(n);
        p_len = n;
        if (octa::IsPod<T>()) {
            for (Size i = l; i < p_len; ++i) {
                p_buf.p_ptr[i] = T(v);
            }
        } else {
            Pointer first = p_buf.p_ptr + l;
            Pointer last  = p_buf.p_ptr + p_len;
            while (first != last)
                octa::allocator_construct(p_buf.get_alloc(), first++, v);
        }
    }

    void reserve(Size n) {
        if (n <= p_cap) return;
        Size oc = p_cap;
        if (!oc) {
            p_cap = octa::max(n, Size(8));
        } else {
            while (p_cap < n) p_cap *= 2;
        }
        Pointer tmp = octa::allocator_allocate(p_buf.get_alloc(), p_cap);
        if (oc > 0) {
            if (octa::IsPod<T>()) {
                memcpy(tmp, p_buf.p_ptr, p_len * sizeof(T));
            } else {
                Pointer cur = p_buf.p_ptr, tcur = tmp,
                    last = tmp + p_len;
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

    T &operator[](Size i) { return p_buf.p_ptr[i]; }
    const T &operator[](Size i) const { return p_buf.p_ptr[i]; }

    T &at(Size i) { return p_buf.p_ptr[i]; }
    const T &at(Size i) const { return p_buf.p_ptr[i]; }

    T &push(const T &v) {
        if (p_len == p_cap) reserve(p_len + 1);
        octa::allocator_construct(p_buf.get_alloc(),
            &p_buf.p_ptr[p_len], v);
        return p_buf.p_ptr[p_len++];
    }

    T &push() {
        if (p_len == p_cap) reserve(p_len + 1);
        octa::allocator_construct(p_buf.get_alloc(), &p_buf.p_ptr[p_len]);
        return p_buf.p_ptr[p_len++];
    }

    template<typename ...U>
    T &emplace_back(U &&...args) {
        if (p_len == p_cap) reserve(p_len + 1);
        octa::allocator_construct(p_buf.get_alloc(), &p_buf.p_ptr[p_len],
            octa::forward<U>(args)...);
        return p_buf.p_ptr[p_len++];
    }

    void pop() {
        if (!octa::IsPod<T>()) {
            octa::allocator_destroy(p_buf.get_alloc(),
                &p_buf.p_ptr[--p_len]);
        } else {
            --p_len;
        }
    }

    T &front() { return p_buf.p_ptr[0]; }
    const T &front() const { return p_buf.p_ptr[0]; }

    T &back() { return p_buf.p_ptr[p_len - 1]; }
    const T &back() const { return p_buf.p_ptr[p_len - 1]; }

    Pointer data() { return p_buf.p_ptr; }
    ConstPointer data() const { return p_buf.p_ptr; }

    Size size() const { return p_len; }
    Size capacity() const { return p_cap; }

    bool empty() const { return (p_len == 0); }

    bool in_range(Size idx) { return idx < p_len; }
    bool in_range(int idx) { return idx >= 0 && Size(idx) < p_len; }
    bool in_range(ConstPointer ptr) {
        return ptr >= p_buf.p_ptr && ptr < &p_buf.p_ptr[p_len];
    }

    Pointer disown() {
        Pointer r = p_buf.p_ptr;
        p_buf.p_ptr = nullptr;
        p_len = p_cap = 0;
        return r;
    }

    Range insert(Size idx, T &&v) {
        insert_base(idx, 1);
        p_buf.p_ptr[idx] = octa::move(v);
        return Range(&p_buf.p_ptr[idx], &p_buf.p_ptr[p_len]);
    }

    Range insert(Size idx, const T &v) {
        insert_base(idx, 1);
        p_buf.p_ptr[idx] = v;
        return Range(&p_buf.p_ptr[idx], &p_buf.p_ptr[p_len]);
    }

    Range insert(Size idx, Size n, const T &v) {
        insert_base(idx, n);
        for (Size i = 0; i < n; ++i) {
            p_buf.p_ptr[idx + i] = v;
        }
        return Range(&p_buf.p_ptr[idx], &p_buf.p_ptr[p_len]);
    }

    template<typename U>
    Range insert_range(Size idx, U range) {
        Size l = range.size();
        insert_base(idx, l);
        for (Size i = 0; i < l; ++i) {
            p_buf.p_ptr[idx + i] = range.front();
            range.pop_front();
        }
        return Range(&p_buf.p_ptr[idx], &p_buf.p_ptr[p_len]);
    }

    Range insert(Size idx, InitializerList<T> il) {
        return insert_range(idx, octa::each(il));
    }

    Range each() {
        return Range(p_buf.p_ptr, p_buf.p_ptr + p_len);
    }
    ConstRange each() const {
        return ConstRange(p_buf.p_ptr, p_buf.p_ptr + p_len);
    }
    ConstRange ceach() const {
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