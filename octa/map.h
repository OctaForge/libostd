/* Associative array for OctaSTD. Implemented as a hash table.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_MAP_H
#define OCTA_MAP_H

#include "octa/types.h"
#include "octa/utility.h"
#include "octa/memory.h"
#include "octa/functional.h"
#include "octa/initializer_list.h"

#include "octa/internal/hashtable.h"

namespace octa {

namespace detail {
    template<typename K, typename T, typename A> struct MapBase {
        using Element = octa::Pair<const K, T>;

        static inline const K &get_key(Element &e) {
            return e.first;
        }
        static inline T &get_data(Element &e) {
            return e.second;
        }
        template<typename U>
        static inline void set_key(Element &e, U &&key, A &alloc) {
            octa::allocator_destroy(alloc, &e);
            octa::allocator_construct(alloc, &e, octa::forward<U>(key),
                octa::move(T()));
        }
        static inline void swap_elem(Element &a, Element &b) {
            octa::swap(*((K *)&a.first), *((K *)&b.first));
            octa::swap(*((T *)&a.second), *((T *)&b.second));
        }
    };
}

template<
    typename K, typename T,
    typename H = octa::ToHash<K>,
    typename C = octa::Equal<K>,
    typename A = octa::Allocator<octa::Pair<const K, T>>
> struct Map {
private:
    using Base = octa::detail::Hashtable<
        octa::detail::MapBase<K, T, A>, octa::Pair<const K, T>, K, T, H, C, A
    >;
    Base p_table;

public:

    using Key = K;
    using Mapped = T;
    using Size = octa::Size;
    using Difference = octa::Ptrdiff;
    using Hasher = H;
    using KeyEqual = C;
    using Value = octa::Pair<const K, T>;
    using Reference = Value &;
    using Pointer = octa::AllocatorPointer<A>;
    using ConstPointer = octa::AllocatorConstPointer<A>;
    using Range = octa::HashRange<octa::Pair<const K, T>>;
    using ConstRange = octa::HashRange<const octa::Pair<const K, T>>;
    using LocalRange = octa::BucketRange<octa::Pair<const K, T>>;
    using ConstLocalRange = octa::BucketRange<const octa::Pair<const K, T>>;
    using Allocator = A;

    explicit Map(octa::Size size, const H &hf = H(),
        const C &eqf = C(), const A &alloc = A()
    ): p_table(size, hf, eqf, alloc) {}

    Map(): Map(0) {}
    explicit Map(const A &alloc): Map(0, H(), C(), alloc) {}

    Map(octa::Size size, const A &alloc): Map(size, H(), C(), alloc) {}
    Map(octa::Size size, const H &hf, const A &alloc): Map(size, hf, C(), alloc) {}

    Map(const Map &m): p_table(m.p_table,
        octa::allocator_container_copy(m.p_table.get_alloc())) {}

    Map(const Map &m, const A &alloc): p_table(m.p_table, alloc) {}

    Map(Map &&m): p_table(octa::move(m.p_table)) {}
    Map(Map &&m, const A &alloc): p_table(octa::move(m.p_table), alloc) {}

    template<typename R>
    Map(R range, octa::Size size = 0, const H &hf = H(),
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
    Map(R range, octa::Size size, const A &alloc)
    : Map(range, size, H(), C(), alloc) {}

    template<typename R>
    Map(R range, octa::Size size, const H &hf, const A &alloc)
    : Map(range, size, hf, C(), alloc) {}

    Map(octa::InitializerList<Value> init, octa::Size size = 0,
        const H &hf = H(), const C &eqf = C(), const A &alloc = A()
    ): Map(octa::each(init), size, hf, eqf, alloc) {}

    Map(octa::InitializerList<Value> init, octa::Size size, const A &alloc)
    : Map(octa::each(init), size, H(), C(), alloc) {}

    Map(octa::InitializerList<Value> init, octa::Size size, const H &hf,
        const A &alloc
    ): Map(octa::each(init), size, hf, C(), alloc) {}

    Map &operator=(const Map &m) {
        p_table = m.p_table;
        return *this;
    }

    Map &operator=(Map &&m) {
        p_table = octa::move(m.p_table);
        return *this;
    }

    template<typename R>
    octa::EnableIf<
        octa::IsInputRange<R>::value &&
        octa::IsConvertible<RangeReference<R>, Value>::value,
        Map &
    > operator=(R range) {
        clear();
        p_table.reserve_at_least(octa::detail::estimate_hrsize(range));
        for (; !range.empty(); range.pop_front())
            emplace(range.front());
        p_table.rehash_up();
        return *this;
    }

    Map &operator=(InitializerList<Value> il) {
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

    octa::Size bucket(const K &key) const { return p_table.bucket(key); }
    octa::Size bucket_size(octa::Size n) const { return p_table.bucket_size(n); }

    void clear() { p_table.clear(); }

    A get_allocator() const {
        return p_table.get_alloc();
    }

    T &at(const K &key) {
        return *p_table.access(key);
    }
    const T &at(const K &key) const {
        return *p_table.access(key);
    }

    T &operator[](const K &key) {
        octa::Size h;
        T *v = p_table.access_base(key, h);
        if (v) return *v;
        p_table.rehash_ahead(1);
        return p_table.insert(h, key);
    }
    T &operator[](K &&key) {
        octa::Size h;
        T *v = p_table.access_base(key, h);
        if (v) return *v;
        p_table.rehash_ahead(1);
        return p_table.insert(h, octa::move(key));
    }

    template<typename ...Args>
    octa::Pair<Range, bool> emplace(Args &&...args) {
        return p_table.emplace(octa::forward<Args>(args)...);
    }

    octa::Size erase(const K &key) {
        return p_table.remove<false>(key);
    }

    octa::Size count(const K &key) {
        return p_table.count<false>(key);
    }

    Range find(const K &key) { return p_table.find(key); }
    ConstRange find(const K &key) const { return p_table.find(key); }

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

    void swap(Map &v) {
        octa::swap(p_table, v.p_table);
    }
};

template<
    typename K, typename T,
    typename H = octa::ToHash<K>,
    typename C = octa::Equal<K>,
    typename A = octa::Allocator<octa::Pair<const K, T>>
> struct Multimap {
private:
    using Base = octa::detail::Hashtable<
        octa::detail::MapBase<K, T, A>, octa::Pair<const K, T>,
        K, T, H, C, A
    >;
    Base p_table;

public:

    using Key = K;
    using Mapped = T;
    using Size = octa::Size;
    using Difference = octa::Ptrdiff;
    using Hasher = H;
    using KeyEqual = C;
    using Value = octa::Pair<const K, T>;
    using Reference = Value &;
    using Pointer = octa::AllocatorPointer<A>;
    using ConstPointer = octa::AllocatorConstPointer<A>;
    using Range = octa::HashRange<octa::Pair<const K, T>>;
    using ConstRange = octa::HashRange<const octa::Pair<const K, T>>;
    using LocalRange = octa::BucketRange<octa::Pair<const K, T>>;
    using ConstLocalRange = octa::BucketRange<const octa::Pair<const K, T>>;
    using Allocator = A;

    explicit Multimap(octa::Size size, const H &hf = H(),
        const C &eqf = C(), const A &alloc = A()
    ): p_table(size, hf, eqf, alloc) {}

    Multimap(): Multimap(0) {}
    explicit Multimap(const A &alloc): Multimap(0, H(), C(), alloc) {}

    Multimap(octa::Size size, const A &alloc):
        Multimap(size, H(), C(), alloc) {}
    Multimap(octa::Size size, const H &hf, const A &alloc):
        Multimap(size, hf, C(), alloc) {}

    Multimap(const Multimap &m): p_table(m.p_table,
        octa::allocator_container_copy(m.p_table.get_alloc())) {}

    Multimap(const Multimap &m, const A &alloc): p_table(m.p_table, alloc) {}

    Multimap(Multimap &&m): p_table(octa::move(m.p_table)) {}
    Multimap(Multimap &&m, const A &alloc):
        p_table(octa::move(m.p_table), alloc) {}

    template<typename R>
    Multimap(R range, octa::Size size = 0, const H &hf = H(),
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
    Multimap(R range, octa::Size size, const A &alloc)
    : Multimap(range, size, H(), C(), alloc) {}

    template<typename R>
    Multimap(R range, octa::Size size, const H &hf, const A &alloc)
    : Multimap(range, size, hf, C(), alloc) {}

    Multimap(octa::InitializerList<Value> init, octa::Size size = 0,
        const H &hf = H(), const C &eqf = C(), const A &alloc = A()
    ): Multimap(octa::each(init), size, hf, eqf, alloc) {}

    Multimap(octa::InitializerList<Value> init, octa::Size size, const A &alloc)
    : Multimap(octa::each(init), size, H(), C(), alloc) {}

    Multimap(octa::InitializerList<Value> init, octa::Size size, const H &hf,
        const A &alloc
    ): Multimap(octa::each(init), size, hf, C(), alloc) {}

    Multimap &operator=(const Multimap &m) {
        p_table = m.p_table;
        return *this;
    }

    Multimap &operator=(Multimap &&m) {
        p_table = octa::move(m.p_table);
        return *this;
    }

    template<typename R>
    octa::EnableIf<
        octa::IsInputRange<R>::value &&
        octa::IsConvertible<RangeReference<R>, Value>::value,
        Multimap &
    > operator=(R range) {
        clear();
        p_table.reserve_at_least(octa::detail::estimate_hrsize(range));
        for (; !range.empty(); range.pop_front())
            emplace(range.front());
        p_table.rehash_up();
        return *this;
    }

    Multimap &operator=(InitializerList<Value> il) {
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

    octa::Size bucket(const K &key) const { return p_table.bucket(key); }
    octa::Size bucket_size(octa::Size n) const { return p_table.bucket_size(n); }

    void clear() { p_table.clear(); }

    A get_allocator() const {
        return p_table.get_alloc();
    }

    template<typename ...Args>
    octa::Pair<Range, bool> emplace(Args &&...args) {
        return p_table.emplace_multi(octa::forward<Args>(args)...);
    }

    octa::Size erase(const K &key) {
        return p_table.remove<true>(key);
    }

    octa::Size count(const K &key) {
        return p_table.count<true>(key);
    }

    Range find(const K &key) { return p_table.find(key); }
    ConstRange find(const K &key) const { return p_table.find(key); }

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

    void swap(Multimap &v) {
        octa::swap(p_table, v.p_table);
    }
};

} /* namespace detail */

#endif