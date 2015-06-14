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
        T value;
        HashChain<T> *next;
    };
}

template<typename T>
struct HashRange: octa::InputRange<HashRange<T>, octa::ForwardRangeTag, T> {
private:
    using Chain = octa::detail::HashChain<T>;

    Chain **p_chains;
    Chain *p_node;
    octa::Size p_num;

    void advance() {
        while (p_num > 0 && !p_chains[0]) {
            --p_num;
            ++p_chains;
        }
        if (p_num > 0) {
            p_node = p_chains[0];
            --p_num;
            ++p_chains;
        } else {
            p_node = nullptr;
        }
    }
public:
    HashRange(): p_chains(), p_num(0) {}
    HashRange(Chain **ch, octa::Size n): p_chains(ch), p_num(n) {
        advance();
    }
    HashRange(const HashRange &v): p_chains(v.p_chains), p_node(v.p_node),
        p_num(v.p_num) {}

    HashRange &operator=(const HashRange &v) {
        p_chains = v.p_chains;
        p_node = v.p_node;
        p_num = v.p_num;
        return *this;
    }

    bool empty() const { return !p_node && p_num <= 0; }

    bool pop_front() {
        if (empty()) return false;
        if (p_node->next) {
            p_node = p_node->next;
            return true;
        }
        p_node = nullptr;
        advance();
        return true;
    }

    bool equals_front(const HashRange &v) const {
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

        using AllocPair = octa::detail::CompressedPair<CPA, CHA>;
        using FuncPair = octa::detail::CompressedPair<H, C>;
        using FAPair = octa::detail::CompressedPair<AllocPair, FuncPair>;
        using DataPair = octa::detail::CompressedPair<Chain **, FAPair>;

        using Range = octa::HashRange<E>;
        using ConstRange = octa::HashRange<const E>;

        DataPair p_data;

        float p_maxlf;

        const H &get_hash() const { return p_data.second().second().first(); }
        const C &get_eq() const { return p_data.second().second().second(); }

        CPA &get_cpalloc() { return p_data.second().first().first(); }
        CHA &get_challoc() { return p_data.second().first().second(); }

        Hashtable(octa::Size size, const H &hf, const C &eqf, const A &alloc):
        p_size(size), p_len(0), p_chunks(nullptr), p_unused(nullptr),
        p_data(nullptr, FAPair(AllocPair(alloc, alloc), FuncPair(hf, eqf))),
        p_maxlf(1.0f) {
            p_data.first() = octa::allocator_allocate(get_cpalloc(), size);
            memset(p_data.first(), 0, size * sizeof(Chain *));
        }

        ~Hashtable() {
            octa::allocator_deallocate(get_cpalloc(), p_data.first(), p_size);
            delete_chunks(p_chunks);
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
            B::set_key(c->value, octa::forward<U>(key));
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
                    c->value.~E();
                    new (&c->value) E;
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

        float load_factor() const { return float(p_len) / p_size; }
        float max_load_factor() const { return p_maxlf; }
        void max_load_factor(float lf) { p_maxlf = lf; }

        octa::Size bucket_count() const { return p_size; }
        octa::Size max_bucket_count() const { return Size(~0) / sizeof(Chain); }

        void rehash(octa::Size count) {
            count = octa::max(count, octa::Size(p_len / max_load_factor()));

            Chain **och = p_data.first();
            Chain **nch = octa::allocator_allocate(get_cpalloc(), count);
            memset(nch, 0, count * sizeof(Chain *));
            p_data.first() = nch;

            Chunk *chunks = p_chunks;
            octa::Size osize = p_size;

            p_chunks = nullptr;
            p_unused = nullptr;
            p_size = count;
            p_len = 0;

            for (octa::Size i = 0; i < osize; ++i) {
                for (Chain *oc = och[i]; oc; oc = oc->next) {
                    octa::Size h = get_hash()(B::get_key(oc->value)) & (count - 1);
                    Chain *nc = insert(h);
                    B::swap_elem(oc->value, nc->value);
                }
            }

            octa::allocator_deallocate(get_cpalloc(), och, osize);
            delete_chunks(chunks);
        }

        void reserve(octa::Size count) {
            rehash(octa::Size(ceil(count / max_load_factor())));
        }

        Range each() {
            return Range(p_data.first(), bucket_count());
        }
        ConstRange each() const {
            return ConstRange(p_data.first(), bucket_count());
        }
        ConstRange ceach() const {
            return ConstRange(p_data.first(), bucket_count());
        }

        void swap(Hashtable &h) {
            octa::swap(p_size, h.p_size);
            octa::swap(p_len, h.p_len);
            octa::swap(p_chunks, h.p_chunks);
            octa::swap(p_unused, h.p_unused);
            octa::swap(p_data, h.p_data);
        }
    };
} /* namespace detail */

} /* namespace octa */

#endif