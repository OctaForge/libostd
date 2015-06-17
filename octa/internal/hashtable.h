/* Internal hash table implementation. Used as a base for set, map etc.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_INTERNAL_HASHTABLE_H
#define OCTA_INTERNAL_HASHTABLE_H

#include <string.h>

#include "octa/types.h"
#include "octa/utility.h"
#include "octa/memory.h"
#include "octa/range.h"

namespace octa {

namespace detail {
    template<typename T>
    struct HashChain {
        HashChain<T> *next;
        T value;
    };
}

template<typename T>
struct HashRange: octa::InputRange<HashRange<T>, octa::ForwardRangeTag, T> {
private:
    using Chain = octa::detail::HashChain<T>;

    Chain **p_beg;
    Chain **p_end;
    Chain *p_node;

    void advance() {
        while ((p_beg != p_end) && !p_beg[0])
            ++p_beg;
        if (p_beg != p_end) {
            p_node = p_beg[0];
            ++p_beg;
        }
    }
public:
    HashRange(): p_beg(), p_end(), p_node() {}
    HashRange(const HashRange &v): p_beg(v.p_beg), p_end(v.p_end),
        p_node(v.p_node) {}
    HashRange(Chain **beg, Chain **end): p_beg(beg), p_end(end), p_node() {
        advance();
    }
    HashRange(Chain **beg, Chain **end, Chain *node): p_beg(beg), p_end(end),
        p_node(node) {}

    template<typename U>
    HashRange(const HashRange<U> &v, octa::EnableIf<
        octa::IsSame<RemoveCv<T>, RemoveCv<U>>::value, bool
    > = true): p_beg(*((Chain ***)&v)), p_end(*(((Chain ***)&v) + 1)),
        p_node(*(((Chain **)&v) + 2)) {}

    HashRange &operator=(const HashRange &v) {
        p_beg = v.p_beg;
        p_end = v.p_end;
        p_node = v.p_node;
        return *this;
    }

    bool empty() const { return !p_node; }

    bool pop_front() {
        if (!p_node) return false;
        p_node = p_node->next;
        if (p_node) return true;
        advance();
        return true;
    }

    bool equals_front(const HashRange &v) const {
        return p_node == v.p_node;
    }

    T &front() const { return p_node->value; }
};

template<typename T>
struct BucketRange: octa::InputRange<BucketRange<T>, octa::ForwardRangeTag, T> {
private:
    using Chain = octa::detail::HashChain<T>;
    Chain *p_node;
public:
    BucketRange(): p_node() {}
    BucketRange(Chain *node): p_node(node) {}
    BucketRange(const BucketRange &v): p_node(v.p_node) {}

    template<typename U>
    BucketRange(const BucketRange<U> &v, octa::EnableIf<
        octa::IsSame<RemoveCv<T>, RemoveCv<U>>::value, bool
    > = true): p_node(*((Chain **)&v)) {}

    BucketRange &operator=(const BucketRange &v) {
        p_node = v.p_node;
        return *this;
    }

    bool empty() const { return !p_node; }

    bool pop_front() {
        if (!p_node) return false;
        p_node = p_node->next;
        return true;
    }

    bool equals_front(const BucketRange &v) const {
        return p_node == v.p_node;
    }

    T &front() const { return p_node->value; }
};

namespace detail {
    template<
        typename B, /* contains methods specific to each ht type */
        typename E, /* element type */
        typename K, /* key type */
        typename T, /* value type */
        typename H, /* hash func */
        typename C, /* equality check */
        typename A  /* allocator */
    > struct Hashtable {
        static constexpr octa::Size CHUNKSIZE = 64;

        using Chain = octa::detail::HashChain<E>;

        struct Chunk {
            Chain chains[CHUNKSIZE];
            Chunk *next;
        };

        octa::Size p_size;
        octa::Size p_len;

        Chunk *p_chunks;
        Chain *p_unused;

        using CPA = octa::AllocatorRebind<A, Chain *>;
        using CHA = octa::AllocatorRebind<A, Chunk>;

        using CoreAllocPair = octa::detail::CompressedPair<CPA, CHA>;
        using AllocPair = octa::detail::CompressedPair<A, CoreAllocPair>;
        using FuncPair = octa::detail::CompressedPair<H, C>;
        using FAPair = octa::detail::CompressedPair<AllocPair, FuncPair>;
        using DataPair = octa::detail::CompressedPair<Chain **, FAPair>;

        using Range = octa::HashRange<E>;
        using ConstRange = octa::HashRange<const E>;
        using LocalRange = octa::BucketRange<E>;
        using ConstLocalRange = octa::BucketRange<const E>;

        DataPair p_data;

        float p_maxlf;

        const H &get_hash() const { return p_data.second().second().first(); }
        const C &get_eq() const { return p_data.second().second().second(); }

        A &get_alloc() { return p_data.second().first().first(); }
        const A &get_alloc() const { return p_data.second().first().first(); }

        CPA &get_cpalloc() { return p_data.second().first().second().first(); }
        const CPA &get_cpalloc() const {
            return p_data.second().first().second().first();
        }

        CHA &get_challoc() { return p_data.second().first().second().second(); }
        const CHA &get_challoc() const {
            return p_data.second().first().second().second();
        }

        Hashtable(octa::Size size, const H &hf, const C &eqf, const A &alloc):
        p_size(size), p_len(0), p_chunks(nullptr), p_unused(nullptr),
        p_data(nullptr, FAPair(AllocPair(alloc, CoreAllocPair(alloc, alloc)),
            FuncPair(hf, eqf))),
        p_maxlf(1.0f) {
            if (!size) return;
            p_data.first() = octa::allocator_allocate(get_cpalloc(), size);
            memset(p_data.first(), 0, size * sizeof(Chain *));
        }

        Hashtable(const Hashtable &ht, const A &alloc): p_size(ht.p_size),
        p_len(0), p_chunks(nullptr), p_unused(nullptr),
        p_data(nullptr, FAPair(AllocPair(alloc, CoreAllocPair(alloc, alloc)),
            FuncPair(ht.get_hash(), ht.get_eq()))),
        p_maxlf(ht.p_maxlf) {
            if (!p_size) return;
            p_data.first() = octa::allocator_allocate(get_cpalloc(), p_size);
            memset(p_data.first(), 0, p_size * sizeof(Chain *));
            Chain **och = ht.p_data.first();
            for (octa::Size h = 0; h < p_size; ++h) {
                Chain *oc = och[h];
                for (; oc; oc = oc->next) {
                    Chain *nc = insert(h);
                    octa::allocator_destroy(get_alloc(), &nc->value);
                    octa::allocator_construct(get_alloc(), &nc->value, oc->value);
                }
            }
        }

        Hashtable(Hashtable &&ht): p_size(ht.p_size), p_len(ht.p_len),
        p_chunks(ht.p_chunks), p_unused(ht.p_unused),
        p_data(octa::move(ht.p_data)), p_maxlf(ht.p_maxlf) {
            ht.p_size = ht.p_len = 0;
            ht.p_chunks = nullptr;
            ht.p_unused = nullptr;
            ht.p_data.first() = nullptr;
        }

        Hashtable(Hashtable &&ht, const A &alloc): p_data(nullptr,
            FAPair(AllocPair(alloc, CoreAllocPair(alloc, alloc)),
                FuncPair(ht.get_hash(), ht.get_eq()))) {
            p_size = ht.p_size;
            if (alloc == ht.get_alloc()) {
                p_len = ht.p_len;
                p_chunks = ht.p_chunks;
                p_unused = ht.p_unused;
                p_data.first() = ht.p_data.first();
                p_maxlf = ht.p_maxlf;
                ht.p_size = ht.p_len = 0;
                ht.p_chunks = nullptr;
                ht.p_unused = nullptr;
                ht.p_data.first() = nullptr;
                return;
            }
            p_len = 0;
            p_chunks = nullptr;
            p_unused = nullptr;
            p_data.first() = octa::allocator_allocate(get_cpalloc(), p_size);
            memset(p_data.first(), 0, p_size * sizeof(Chain *));
            Chain **och = ht.p_data.first();
            for (octa::Size h = 0; h < p_size; ++h) {
                Chain *oc = och[h];
                for (; oc; oc = oc->next) {
                    Chain *nc = insert(h);
                    B::swap_elem(oc->value, nc->value);
                }
            }
        }

        ~Hashtable() {
            if (p_size) octa::allocator_deallocate(get_cpalloc(),
                p_data.first(), p_size);
            delete_chunks(p_chunks);
        }

        Hashtable &operator=(const Hashtable &ht) {
            clear();
            if (octa::AllocatorPropagateOnContainerCopyAssignment<A>::value) {
                if ((get_cpalloc() != ht.get_cpalloc()) && p_size) {
                    octa::allocator_deallocate(get_cpalloc(),
                        p_data.first(), p_size);
                    p_data.first() = octa::allocator_allocate(get_cpalloc(),
                        p_size);
                    memset(p_data.first(), 0, p_size * sizeof(Chain *));
                }
                get_alloc() = ht.get_alloc();
                get_cpalloc() = ht.get_cpalloc();
                get_challoc() = ht.get_challoc();
            }
            for (ConstRange range = ht.each(); !range.empty(); range.pop_front())
                emplace(range.front());
            return *this;
        }

        Hashtable &operator=(Hashtable &&ht) {
            clear();
            octa::swap(p_size, ht.p_size);
            octa::swap(p_len, ht.p_len);
            octa::swap(p_chunks, ht.p_chunks);
            octa::swap(p_unused, ht.p_unused);
            octa::swap(p_data.first(), ht.p_data.first());
            octa::swap(p_data.second().second(), ht.p_data.second().second());
            if (octa::AllocatorPropagateOnContainerMoveAssignment<A>::value)
                octa::swap(p_data.second().first(), ht.p_data.second().first());
            return *this;
        }

        bool empty() const { return p_len == 0; }
        octa::Size size() const { return p_len; }
        Size max_size() const { return Size(~0) / sizeof(E); }

        Chain *insert(octa::Size h) {
            if (!p_unused) {
                Chunk *chunk = octa::allocator_allocate(get_challoc(), 1);
                octa::allocator_construct(get_challoc(), chunk);
                chunk->next = p_chunks;
                p_chunks = chunk;
                for (Size i = 0; i < (CHUNKSIZE - 1); ++i)
                    chunk->chains[i].next = &chunk->chains[i + 1];
                chunk->chains[CHUNKSIZE - 1].next = p_unused;
                p_unused = chunk->chains;
            }
            Chain *c = p_unused;
            p_unused = p_unused->next;
            c->next = p_data.first()[h];
            p_data.first()[h] = c;
            ++p_len;
            return c;
        }

        template<typename U>
        T &insert(octa::Size h, U &&key) {
            Chain *c = insert(h);
            B::set_key(c->value, octa::forward<U>(key), get_alloc());
            return B::get_data(c->value);
        }

        template<typename U>
        bool remove(const U &key) {
            octa::Size h = get_hash()(key) & (p_size - 1);
            Chain *c = p_data.first()[h];
            Chain **p = &c;
            while (c) {
                if (get_eq()(key, B::get_key(c->value))) {
                    *p = c->next;
                    octa::allocator_destroy(get_alloc(), &c->value);
                    octa::allocator_construct(get_alloc(), &c->value);
                    c->next = p_unused;
                    p_unused = c;
                    --p_len;
                    return true;
                }
                c = c->next;
                p = &c;
            }
            return false;
        }

        void delete_chunks(Chunk *chunks) {
            for (Chunk *nc; chunks; chunks = nc) {
                nc = chunks->next;
                octa::allocator_destroy(get_challoc(), chunks);
                octa::allocator_deallocate(get_challoc(), chunks, 1);
            }
        }

        void clear() {
            if (!p_len) return;
            memset(p_data.first(), 0, p_size * sizeof(Chain *));
            p_len = 0;
            p_unused = nullptr;
            delete_chunks(p_chunks);
        }

        template<typename U>
        T *access_base(const U &key, octa::Size &h) const {
            if (!p_size) return NULL;
            h = get_hash()(key) & (p_size - 1);
            for (Chain *c = p_data.first()[h]; c; c = c->next) {
                if (get_eq()(key, B::get_key(c->value)))
                    return &B::get_data(c->value);
            }
            return NULL;
        }

        template<typename U>
        T *access(const U &key) const {
            octa::Size h;
            return access_base(key, h);
        }

        template<typename U, typename V>
        T &access(const U &key, const V &val) {
            octa::Size h;
            T *found = access_base(key, h);
            if (found) return *found;
            return (insert(h, key) = val);
        }

        template<typename ...Args>
        octa::Pair<Range, bool> emplace(Args &&...args) {
            rehash_ahead(1);
            E elem(octa::forward<Args>(args)...);
            octa::Size h = get_hash()(B::get_key(elem)) & (p_size - 1);
            Chain *found = nullptr;
            bool ins = true;
            for (Chain *c = p_data.first()[h]; c; c = c->next) {
                if (get_eq()(B::get_key(elem), B::get_key(c->value))) {
                    found = c;
                    ins = false;
                    break;
                }
            }
            if (!found) {
                found = insert(h);
                B::swap_elem(found->value, elem);
            }
            Chain **hch = p_data.first();
            auto ret = octa::make_pair(Range(hch + h + 1, hch + bucket_count(),
                found), ins);
            rehash_up();
            return octa::move(ret);
        }

        template<typename U>
        bool find(const U &key, octa::Size &h, Chain *&oc) const {
            if (!p_size) return false;
            h = get_hash()(key) & (p_size - 1);
            for (Chain *c = p_data.first()[h]; c; c = c->next) {
                if (get_eq()(key, B::get_key(c->value))) {
                    oc = c;
                    return true;
                }
            }
            return false;
        }

        template<typename U>
        Range find(const U &key) {
            octa::Size h;
            Chain *c;
            if (find(key, h, c)) return each_from(c, h);
            return Range();
        }

        template<typename U>
        ConstRange find(const U &key) const {
            octa::Size h;
            Chain *c;
            if (find(key, h, c)) return each_from(c, h);
            return ConstRange();
        }

        float load_factor() const { return float(p_len) / p_size; }
        float max_load_factor() const { return p_maxlf; }
        void max_load_factor(float lf) { p_maxlf = lf; }

        octa::Size bucket_count() const { return p_size; }
        octa::Size max_bucket_count() const { return Size(~0) / sizeof(Chain); }

        template<typename U>
        octa::Size bucket(const U &key) const {
            return get_hash()(key) & (p_size - 1);
        }

        octa::Size bucket_size(octa::Size n) const {
            octa::Size ret = 0;
            if (ret >= p_size) return ret;
            Chain *c = p_data.first()[n];
            if (!c) return ret;
            for (; c; c = c->next)
                ++ret;
            return ret;
        }

        void rehash(octa::Size count) {
            octa::Size fbcount = octa::Size(p_len / max_load_factor());
            if (fbcount > count) count = fbcount;

            Chain **och = p_data.first();
            Chain **nch = octa::allocator_allocate(get_cpalloc(), count);
            memset(nch, 0, count * sizeof(Chain *));
            p_data.first() = nch;

            octa::Size osize = p_size;
            p_size = count;

            for (octa::Size i = 0; i < osize; ++i) {
                for (Chain *oc = och[i]; oc;) {
                    octa::Size h = get_hash()(B::get_key(oc->value)) & (p_size - 1);
                    Chain *nxc = oc->next;
                    oc->next = nch[h];
                    nch[h] = oc;
                    oc = nxc;
                }
            }

            if (och && osize) octa::allocator_deallocate(get_cpalloc(),
                och, osize);
        }

        void rehash_up() {
            if (load_factor() <= max_load_factor()) return;
            rehash(octa::Size(size() / max_load_factor()) * 2);
        }

        void reserve(octa::Size count) {
            rehash(octa::Size(ceil(count / max_load_factor())));
        }

        void reserve_at_least(octa::Size count) {
            octa::Size nc = octa::Size(ceil(count / max_load_factor()));
            if (p_size > nc) return;
            rehash(nc);
        }

        void rehash_ahead(octa::Size n) {
            if (!bucket_count())
                reserve(n);
            else if ((float(size() + n) / bucket_count()) > max_load_factor())
                rehash(octa::Size((size() + 1) / max_load_factor()) * 2);
        }

        Range each() {
            return Range(p_data.first(), p_data.first() + bucket_count());
        }
        ConstRange each() const {
            using Chain = octa::detail::HashChain<const E>;
            return ConstRange((Chain **)p_data.first(),
                              (Chain **)(p_data.first() + bucket_count()));
        }
        ConstRange ceach() const {
            using Chain = octa::detail::HashChain<const E>;
            return ConstRange((Chain **)p_data.first(),
                              (Chain **)(p_data.first() + bucket_count()));
        }

        LocalRange each(octa::Size n) {
            if (n >= p_size) return LocalRange();
            return LocalRange(p_data.first()[n]);
        }
        ConstLocalRange each(octa::Size n) const {
            using Chain = octa::detail::HashChain<const E>;
            if (n >= p_size) return ConstLocalRange();
            return ConstLocalRange((Chain *)p_data.first()[n]);
        }
        ConstLocalRange ceach(octa::Size n) const {
            using Chain = octa::detail::HashChain<const E>;
            if (n >= p_size) return ConstLocalRange();
            return ConstLocalRange((Chain *)p_data.first()[n]);
        }

        Range each_from(Chain *c, octa::Size h) {
            return Range(p_data.first() + h + 1,
                p_data.first() + bucket_count(), c);
        }
        ConstRange each_from(Chain *c, octa::Size h) const {
            using RChain = octa::detail::HashChain<const E>;
            return ConstRange((RChain **)(p_data.first() + h + 1),
                              (RChain **)(p_data.first() + bucket_count()),
                              (RChain *)c);
        }

        void swap(Hashtable &ht) {
            octa::swap(p_size, ht.p_size);
            octa::swap(p_len, ht.p_len);
            octa::swap(p_chunks, ht.p_chunks);
            octa::swap(p_unused, ht.p_unused);
            octa::swap(p_data.first(), ht.p_data.first());
            octa::swap(p_data.second().second(), ht.p_data.second().second());
            if (octa::AllocatorPropagateOnContainerSwap<A>::value)
                octa::swap(p_data.second().first(), ht.p_data.second().first());
        }
    };
} /* namespace detail */

} /* namespace octa */

#endif