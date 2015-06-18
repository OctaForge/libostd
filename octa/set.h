/* A set container for OctaSTD. Implemented as a hash table.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_SET_H
#define OCTA_SET_H

#include "octa/types.h"
#include "octa/utility.h"
#include "octa/memory.h"
#include "octa/functional.h"
#include "octa/initializer_list.h"

#include "octa/internal/hashtable.h"

namespace octa {

namespace detail {
    template<typename T, typename A> struct SetBase {
        static inline const T &get_key(const T &e) {
            return e;
        }
        static inline T &get_data(T &e) {
            return e;
        }
        template<typename U>
        static inline void set_key(T &, const U &, A &) {}
        static inline void swap_elem(T &a, T &b) { octa::swap(a, b); }
    };
}

template<
    typename T,
    typename H = octa::ToHash<T>,
    typename C = octa::Equal<T>,
    typename A = octa::Allocator<T>
> struct Set {
private:
    using Base = octa::detail::Hashtable<
        octa::detail::SetBase<T, A>, T, T, T, H, C, A, false
    >;
    Base p_table;

public:
    using Key = T;
    using Size = octa::Size;
    using Difference = octa::Ptrdiff;
    using Hasher = H;
    using KeyEqual = C;
    using Value = T;
    using Reference = Value &;
    using Pointer = octa::AllocatorPointer<A>;
    using ConstPointer = octa::AllocatorConstPointer<A>;
    using Range = octa::HashRange<T>;
    using ConstRange = octa::HashRange<const T>;
    using LocalRange = octa::BucketRange<T>;
    using ConstLocalRange = octa::BucketRange<const T>;
    using Allocator = A;

    explicit Set(octa::Size size, const H &hf = H(),
        const C &eqf = C(), const A &alloc = A()
    ): p_table(size, hf, eqf, alloc) {}

    Set(): Set(0) {}
    explicit Set(const A &alloc): Set(0, H(), C(), alloc) {}

    Set(octa::Size size, const A &alloc): Set(size, H(), C(), alloc) {}
    Set(octa::Size size, const H &hf, const A &alloc): Set(size, hf, C(), alloc) {}

    Set(const Set &m): p_table(m.p_table,
        octa::allocator_container_copy(m.p_table.get_alloc())) {}

    Set(const Set &m, const A &alloc): p_table(m.p_table, alloc) {}

    Set(Set &&m): p_table(octa::move(m.p_table)) {}
    Set(Set &&m, const A &alloc): p_table(octa::move(m.p_table), alloc) {}

    template<typename R>
    Set(R range, octa::Size size = 0, const H &hf = H(),
        const C &eqf = C(), const A &alloc = A(),
        octa::EnableIf<
            octa::IsInputRange<R>::value &&
            octa::IsConvertible<RangeReference<R>, Value>::value,
            bool
        > = true
    ): p_table(size ? size : octa::detail::estimate_hrsize(range),
               hf, eqf, alloc) {
        for (; !range.empty(); range.pop_front())
            emplace(range.front());
        p_table.rehash_up();
    }

    template<typename R>
    Set(R range, octa::Size size, const A &alloc)
    : Set(range, size, H(), C(), alloc) {}

    template<typename R>
    Set(R range, octa::Size size, const H &hf, const A &alloc)
    : Set(range, size, hf, C(), alloc) {}

    Set(octa::InitializerList<Value> init, octa::Size size = 0,
        const H &hf = H(), const C &eqf = C(), const A &alloc = A()
    ): Set(octa::each(init), size, hf, eqf, alloc) {}

    Set(octa::InitializerList<Value> init, octa::Size size, const A &alloc)
    : Set(octa::each(init), size, H(), C(), alloc) {}

    Set(octa::InitializerList<Value> init, octa::Size size, const H &hf,
        const A &alloc
    ): Set(octa::each(init), size, hf, C(), alloc) {}

    Set &operator=(const Set &m) {
        p_table = m.p_table;
        return *this;
    }

    Set &operator=(Set &&m) {
        p_table = octa::move(m.p_table);
        return *this;
    }

    template<typename R>
    octa::EnableIf<
        octa::IsInputRange<R>::value &&
        octa::IsConvertible<RangeReference<R>, Value>::value,
        Set &
    > operator=(R range) {
        clear();
        p_table.reserve_at_least(octa::detail::estimate_hrsize(range));
        for (; !range.empty(); range.pop_front())
            emplace(range.front());
        p_table.rehash_up();
        return *this;
    }

    Set &operator=(InitializerList<Value> il) {
        const Value *beg = il.begin(), *end = il.end();
        clear();
        p_table.reserve_at_least(end - beg);
        while (beg != end)
            emplace(*beg++);
        return *this;
    }

    bool empty() const { return p_table.empty(); }
    octa::Size size() const { return p_table.size(); }
    octa::Size max_size() const { return p_table.max_size(); }

    octa::Size bucket_count() const { return p_table.bucket_count(); }
    octa::Size max_bucket_count() const { return p_table.max_bucket_count(); }

    octa::Size bucket(const T &key) const { return p_table.bucket(key); }
    octa::Size bucket_size(octa::Size n) const { return p_table.bucket_size(n); }

    void clear() { p_table.clear(); }

    A get_allocator() const {
        return p_table.get_alloc();
    }

    template<typename ...Args>
    octa::Pair<Range, bool> emplace(Args &&...args) {
        return p_table.emplace(octa::forward<Args>(args)...);
    }

    octa::Size erase(const T &key) {
        return p_table.remove(key);
    }

    octa::Size count(const T &key) {
        return p_table.count(key);
    }

    Range find(const Key &key) { return p_table.find(key); }
    ConstRange find(const Key &key) const { return p_table.find(key); }

    float load_factor() const { return p_table.load_factor(); }
    float max_load_factor() const { return p_table.max_load_factor(); }
    void max_load_factor(float lf) { p_table.max_load_factor(lf); }

    void rehash(octa::Size count) {
        p_table.rehash(count);
    }

    void reserve(octa::Size count) {
        p_table.reserve(count);
    }

    Range each() { return p_table.each(); }
    ConstRange each() const { return p_table.each(); }
    ConstRange ceach() const { return p_table.ceach(); }

    LocalRange each(octa::Size n) { return p_table.each(n); }
    ConstLocalRange each(octa::Size n) const { return p_table.each(n); }
    ConstLocalRange ceach(octa::Size n) const { return p_table.each(n); }

    void swap(Set &v) {
        octa::swap(p_table, v.p_table);
    }
};

template<
    typename T,
    typename H = octa::ToHash<T>,
    typename C = octa::Equal<T>,
    typename A = octa::Allocator<T>
> struct Multiset {
private:
    using Base = octa::detail::Hashtable<
        octa::detail::SetBase<T, A>, T, T, T, H, C, A, true
    >;
    Base p_table;

public:
    using Key = T;
    using Size = octa::Size;
    using Difference = octa::Ptrdiff;
    using Hasher = H;
    using KeyEqual = C;
    using Value = T;
    using Reference = Value &;
    using Pointer = octa::AllocatorPointer<A>;
    using ConstPointer = octa::AllocatorConstPointer<A>;
    using Range = octa::HashRange<T>;
    using ConstRange = octa::HashRange<const T>;
    using LocalRange = octa::BucketRange<T>;
    using ConstLocalRange = octa::BucketRange<const T>;
    using Allocator = A;

    explicit Multiset(octa::Size size, const H &hf = H(),
        const C &eqf = C(), const A &alloc = A()
    ): p_table(size, hf, eqf, alloc) {}

    Multiset(): Multiset(0) {}
    explicit Multiset(const A &alloc): Multiset(0, H(), C(), alloc) {}

    Multiset(octa::Size size, const A &alloc):
        Multiset(size, H(), C(), alloc) {}
    Multiset(octa::Size size, const H &hf, const A &alloc):
        Multiset(size, hf, C(), alloc) {}

    Multiset(const Multiset &m): p_table(m.p_table,
        octa::allocator_container_copy(m.p_table.get_alloc())) {}

    Multiset(const Multiset &m, const A &alloc): p_table(m.p_table, alloc) {}

    Multiset(Multiset &&m): p_table(octa::move(m.p_table)) {}
    Multiset(Multiset &&m, const A &alloc):
        p_table(octa::move(m.p_table), alloc) {}

    template<typename R>
    Multiset(R range, octa::Size size = 0, const H &hf = H(),
        const C &eqf = C(), const A &alloc = A(),
        octa::EnableIf<
            octa::IsInputRange<R>::value &&
            octa::IsConvertible<RangeReference<R>, Value>::value,
            bool
        > = true
    ): p_table(size ? size : octa::detail::estimate_hrsize(range),
               hf, eqf, alloc) {
        for (; !range.empty(); range.pop_front())
            emplace(range.front());
        p_table.rehash_up();
    }

    template<typename R>
    Multiset(R range, octa::Size size, const A &alloc)
    : Multiset(range, size, H(), C(), alloc) {}

    template<typename R>
    Multiset(R range, octa::Size size, const H &hf, const A &alloc)
    : Multiset(range, size, hf, C(), alloc) {}

    Multiset(octa::InitializerList<Value> init, octa::Size size = 0,
        const H &hf = H(), const C &eqf = C(), const A &alloc = A()
    ): Multiset(octa::each(init), size, hf, eqf, alloc) {}

    Multiset(octa::InitializerList<Value> init, octa::Size size, const A &alloc)
    : Multiset(octa::each(init), size, H(), C(), alloc) {}

    Multiset(octa::InitializerList<Value> init, octa::Size size, const H &hf,
        const A &alloc
    ): Multiset(octa::each(init), size, hf, C(), alloc) {}

    Multiset &operator=(const Multiset &m) {
        p_table = m.p_table;
        return *this;
    }

    Multiset &operator=(Multiset &&m) {
        p_table = octa::move(m.p_table);
        return *this;
    }

    template<typename R>
    octa::EnableIf<
        octa::IsInputRange<R>::value &&
        octa::IsConvertible<RangeReference<R>, Value>::value,
        Multiset &
    > operator=(R range) {
        clear();
        p_table.reserve_at_least(octa::detail::estimate_hrsize(range));
        for (; !range.empty(); range.pop_front())
            emplace(range.front());
        p_table.rehash_up();
        return *this;
    }

    Multiset &operator=(InitializerList<Value> il) {
        const Value *beg = il.begin(), *end = il.end();
        clear();
        p_table.reserve_at_least(end - beg);
        while (beg != end)
            emplace(*beg++);
        return *this;
    }

    bool empty() const { return p_table.empty(); }
    octa::Size size() const { return p_table.size(); }
    octa::Size max_size() const { return p_table.max_size(); }

    octa::Size bucket_count() const { return p_table.bucket_count(); }
    octa::Size max_bucket_count() const { return p_table.max_bucket_count(); }

    octa::Size bucket(const T &key) const { return p_table.bucket(key); }
    octa::Size bucket_size(octa::Size n) const { return p_table.bucket_size(n); }

    void clear() { p_table.clear(); }

    A get_allocator() const {
        return p_table.get_alloc();
    }

    template<typename ...Args>
    octa::Pair<Range, bool> emplace(Args &&...args) {
        return p_table.emplace(octa::forward<Args>(args)...);
    }

    octa::Size erase(const T &key) {
        return p_table.remove(key);
    }

    octa::Size count(const T &key) {
        return p_table.count(key);
    }

    Range find(const T &key) { return p_table.find(key); }
    ConstRange find(const T &key) const { return p_table.find(key); }

    float load_factor() const { return p_table.load_factor(); }
    float max_load_factor() const { return p_table.max_load_factor(); }
    void max_load_factor(float lf) { p_table.max_load_factor(lf); }

    void rehash(octa::Size count) {
        p_table.rehash(count);
    }

    void reserve(octa::Size count) {
        p_table.reserve(count);
    }

    Range each() { return p_table.each(); }
    ConstRange each() const { return p_table.each(); }
    ConstRange ceach() const { return p_table.ceach(); }

    LocalRange each(octa::Size n) { return p_table.each(n); }
    ConstLocalRange each(octa::Size n) const { return p_table.each(n); }
    ConstLocalRange ceach(octa::Size n) const { return p_table.each(n); }

    void swap(Multiset &v) {
        octa::swap(p_table, v.p_table);
    }
};

} /* namespace detail */

#endif