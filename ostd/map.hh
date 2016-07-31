/* Associative array for OctaSTD. Implemented as a hash table.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_MAP_HH
#define OSTD_MAP_HH

#include "ostd/types.hh"
#include "ostd/utility.hh"
#include "ostd/memory.hh"
#include "ostd/functional.hh"
#include "ostd/initializer_list.hh"

#include "ostd/internal/hashtable.hh"

namespace ostd {

namespace detail {
    template<typename K, typename T, typename A> struct MapBase {
        using Element = Pair<K const, T>;

        static inline K const &get_key(Element &e) {
            return e.first;
        }
        static inline T &get_data(Element &e) {
            return e.second;
        }
        template<typename U>
        static inline void set_key(Element &e, U &&key, A &alloc) {
            allocator_destroy(alloc, &e);
            allocator_construct(alloc, &e, forward<U>(key), move(T()));
        }
        static inline void swap_elem(Element &a, Element &b) {
            swap_adl(const_cast<K &>(a.first), const_cast<K &>(b.first));
            swap_adl(a.second, b.second);
        }
    };

    template<
        typename K, typename T, typename H,
        typename C, typename A, bool IsMultihash
    >
    struct MapImpl: detail::Hashtable<detail::MapBase<K, T, A>,
        Pair<K const, T>, K, T, H, C, A, IsMultihash
    > {
    private:
        using Base = detail::Hashtable<detail::MapBase<K, T, A>,
            Pair<K const, T>, K, T, H, C, A, IsMultihash
        >;

    public:
        using Key = K;
        using Mapped = T;
        using Size = ostd::Size;
        using Difference = Ptrdiff;
        using Hasher = H;
        using KeyEqual = C;
        using Value = Pair<K const, T>;
        using Reference = Value &;
        using Pointer = AllocatorPointer<A>;
        using ConstPointer = AllocatorConstPointer<A>;
        using Range = HashRange<Pair<K const, T>>;
        using ConstRange = HashRange<Pair<K const, T> const>;
        using LocalRange = BucketRange<Pair<K const, T>>;
        using ConstLocalRange = BucketRange<Pair<K const, T> const>;
        using Allocator = A;

        explicit MapImpl(
            Size size, H const &hf = H(),
            C const &eqf = C(), A const &alloc = A()
        ):
            Base(size, hf, eqf, alloc)
        {}

        MapImpl(): MapImpl(0) {}
        explicit MapImpl(A const &alloc): MapImpl(0, H(), C(), alloc) {}

        MapImpl(Size size, A const &alloc):
            MapImpl(size, H(), C(), alloc)
        {}
        MapImpl(Size size, H const &hf, A const &alloc):
            MapImpl(size, hf, C(), alloc)
        {}

        MapImpl(MapImpl const &m):
            Base(m, allocator_container_copy(m.get_alloc()))
        {}

        MapImpl(MapImpl const &m, A const &alloc): Base(m, alloc) {}

        MapImpl(MapImpl &&m): Base(move(m)) {}
        MapImpl(MapImpl &&m, A const &alloc): Base(move(m), alloc) {}

        template<typename R, typename = EnableIf<
            IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
        >>
        MapImpl(
            R range, Size size = 0, H const &hf = H(),
            C const &eqf = C(), A const &alloc = A()
        ):
            Base(size ? size : detail::estimate_hrsize(range), hf, eqf, alloc)
        {
            for (; !range.empty(); range.pop_front()) {
                Base::emplace(range.front());
            }
            Base::rehash_up();
        }

        template<typename R>
        MapImpl(R range, Size size, A const &alloc):
            MapImpl(range, size, H(), C(), alloc)
        {}

        template<typename R>
        MapImpl(R range, Size size, H const &hf, A const &alloc):
            MapImpl(range, size, hf, C(), alloc)
        {}

        MapImpl(
            InitializerList<Value> init, Size size = 0,
            H const &hf = H(), C const &eqf = C(), A const &alloc = A()
        ):
            MapImpl(iter(init), size, hf, eqf, alloc)
        {}

        MapImpl(InitializerList<Value> init, Size size, A const &alloc):
            MapImpl(iter(init), size, H(), C(), alloc)
        {}

        MapImpl(
            InitializerList<Value> init, Size size, H const &hf, A const &alloc
        ):
            MapImpl(iter(init), size, hf, C(), alloc)
        {}

        MapImpl &operator=(MapImpl const &m) {
            Base::operator=(m);
            return *this;
        }

        MapImpl &operator=(MapImpl &&m) {
            Base::operator=(move(m));
            return *this;
        }

        template<typename R, typename = EnableIf<
            IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
        >>
        MapImpl &operator=(R range) {
            Base::assign_range(range);
            return *this;
        }

        MapImpl &operator=(InitializerList<Value> il) {
            Base::assign_init(il);
            return *this;
        }

        T *at(K const &key) {
            static_assert(!IsMultihash, "at() only allowed on regular maps");
            return Base::access(key);
        }
        T const *at(K const &key) const {
            static_assert(!IsMultihash, "at() only allowed on regular maps");
            return Base::access(key);
        }

        T &operator[](K const &key) {
            static_assert(!IsMultihash, "operator[] only allowed on regular maps");
            return Base::access_or_insert(key);
        }
        T &operator[](K &&key) {
            static_assert(!IsMultihash, "operator[] only allowed on regular maps");
            return Base::access_or_insert(move(key));
        }

        void swap(MapImpl &v) {
            Base::swap(v);
        }
    };
}

template<
    typename K, typename T,
    typename H = ToHash<K>,
    typename C = EqualWithCstr<K>,
    typename A = Allocator<Pair<K const, T>>
>
using Map = detail::MapImpl<K, T, H, C, A, false>;

template<
    typename K, typename T,
    typename H = ToHash<K>,
    typename C = EqualWithCstr<K>,
    typename A = Allocator<Pair<K const, T>>
>
using Multimap = detail::MapImpl<K, T, H, C, A, true>;

} /* namespace ostd */

#endif
