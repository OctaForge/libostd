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

namespace octa {

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

        struct Chain {
            E value;
            Chain *next;
        };
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

        DataPair p_data;

        const H &get_hash() const { return p_data.second().second().first(); }
        const C &get_eq() const { return p_data.second().second().second(); }

        CPA &get_cpalloc() { return p_data.second().first().first(); }
        CHA &get_challoc() { return p_data.second().first().second(); }

        Hashtable(octa::Size size, const H &hf, const C &eqf, const A &alloc):
        p_size(size), p_len(0), p_chunks(nullptr), p_unused(nullptr),
        p_data(nullptr, FAPair(AllocPair(alloc, alloc), FuncPair(hf, eqf))) {
            p_data.first() = octa::allocator_allocate(get_cpalloc(), size);
            memset(p_data.first(), 0, size * sizeof(Chain *));
        }

        ~Hashtable() {
            octa::allocator_deallocate(get_cpalloc(), p_data.first(), p_size);
            delete_chunks();
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

        void delete_chunks() {
            for (Chunk *nc; p_chunks; p_chunks = nc) {
                nc = p_chunks->next;
                octa::allocator_destroy(get_challoc(), p_chunks);
                octa::allocator_deallocate(get_challoc(), p_chunks, 1);
            }
        }

        void clear() {
            if (!p_len) return;
            memset(p_data.first(), 0, p_size * sizeof(Chain *));
            p_len = 0;
            p_unused = nullptr;
            delete_chunks();
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

        float load_factor() const {
            return p_len / float(p_size);
        }

        octa::Size bucket_count() const { return p_size; }
        octa::Size max_bucket_count() const { return Size(~0) / sizeof(Chain); }

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