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

#include "octa/internal/hashtable.h"

namespace octa {

namespace detail {
    template<typename K, typename T> struct MapBase {
        typedef octa::Pair<const K, T> Element;

        static inline const K &get_key(Element &e) {
            return e.first;
        }
        static inline T &get_data(Element &e) {
            return e.second;
        }
        template<typename U>
        static inline void set_key(Element &e, U &&key) {
            e.first.~K();
            new ((K *)&e.first) K(octa::forward<U>(key));
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
        octa::detail::MapBase<K, T>, octa::Pair<const K, T>, K, T, H, C, A
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
    using Allocator = A;

    Map(octa::Size size = 1 << 10, const H &hf = H(), const C &eqf = C(),
        const A &alloc = A()): p_table(size, hf, eqf, alloc) {}

    bool empty() const { return p_table.empty(); }
    octa::Size size() const { return p_table.size(); }
    octa::Size max_size() const { return p_table.max_size(); }

    octa::Size bucket_count() const { return p_table.bucket_count(); }
    octa::Size max_bucket_count() const { return p_table.max_bucket_count(); }

    void clear() { p_table.clear(); }

    A get_allocator() const {
        return p_table.get_challoc();
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
        return p_table.insert(h, key);
    }
    T &operator[](K &&key) {
        octa::Size h;
        T *v = p_table.access_base(key, h);
        if (v) return *v;
        return p_table.insert(h, octa::move(key));
    }

    octa::Size erase(const K &key) {
        if (p_table.remove(key)) return 1;
        return 0;
    }

    float load_factor() const { return p_table.load_factor(); }
    float max_load_factor() const { return p_table.max_load_factor(); }
    void max_load_factor(float lf) { p_table.max_load_factor(lf); }

    void swap(Map &v) {
        octa::swap(p_table, v.p_table);
    }
};

} /* namespace detail */

#endif