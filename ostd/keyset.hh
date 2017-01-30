/* Keyed set for OctaSTD. Implemented as a hash table.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_KEYSET_HH
#define OSTD_KEYSET_HH

#include "ostd/types.hh"
#include "ostd/utility.hh"
#include "ostd/memory.hh"
#include "ostd/functional.hh"
#include "ostd/initializer_list.hh"
#include "ostd/type_traits.hh"

#include "ostd/internal/hashtable.hh"

namespace ostd {

namespace detail {
    template<typename T>
    using KeysetKeyRet = decltype(std::declval<T const &>().get_key());
    template<typename T>
    using KeysetKey = Decay<KeysetKeyRet<T>> const;

    template<typename T, typename A> struct KeysetBase {
        using Key = KeysetKey<T>;

        using RetKey = Conditional<IsReference<KeysetKeyRet<T>>, Key &, Key>;
        static inline RetKey get_key(T const &e) {
            return e.get_key();
        }
        static inline T &get_data(T &e) {
            return e;
        }
        template<typename U>
        static inline void set_key(T &, U const &, A &) {}
        static inline void swap_elem(T &a, T &b) {
            using std::swap;
            swap(a, b);
        }
    };

    template<
        typename T, typename H, typename C, typename A, bool IsMultihash
    >
    struct KeysetImpl: detail::Hashtable<detail::KeysetBase<T, A>,
        T, KeysetKey<T>, T, H, C, A, IsMultihash
    > {
    private:
        using Base = detail::Hashtable<detail::KeysetBase<T, A>,
            T, KeysetKey<T>, T, H, C, A, IsMultihash
        >;

    public:
        using Key = KeysetKey<T>;
        using Mapped = T;
        using Size = ostd::Size;
        using Difference = Ptrdiff;
        using Hasher = H;
        using KeyEqual = C;
        using Value = T;
        using Reference = Value &;
        using Pointer = AllocatorPointer<A>;
        using ConstPointer = AllocatorConstPointer<A>;
        using Range = HashRange<T>;
        using ConstRange = HashRange<T const>;
        using LocalRange = BucketRange<T>;
        using ConstLocalRange = BucketRange<T const>;
        using Allocator = A;

        explicit KeysetImpl(
            Size size, H const &hf = H(), C const &eqf = C(),
            A const &alloc = A()
        ): Base(size, hf, eqf, alloc) {}

        KeysetImpl(): KeysetImpl(0) {}
        explicit KeysetImpl(A const &alloc): KeysetImpl(0, H(), C(), alloc) {}

        KeysetImpl(Size size, A const &alloc):
            KeysetImpl(size, H(), C(), alloc)
        {}
        KeysetImpl(Size size, H const &hf, A const &alloc):
            KeysetImpl(size, hf, C(), alloc)
        {}

        KeysetImpl(KeysetImpl const &m):
            Base(m, allocator_container_copy(m.get_alloc()))
        {}

        KeysetImpl(KeysetImpl const &m, A const &alloc): Base(m, alloc) {}

        KeysetImpl(KeysetImpl &&m): Base(std::move(m)) {}
        KeysetImpl(KeysetImpl &&m, A const &alloc): Base(std::move(m), alloc) {}

        template<typename R, typename = EnableIf<
            IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
        >>
        KeysetImpl(
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
        KeysetImpl(R range, Size size, A const &alloc):
            KeysetImpl(range, size, H(), C(), alloc)
        {}

        template<typename R>
        KeysetImpl(R range, Size size, H const &hf, A const &alloc):
            KeysetImpl(range, size, hf, C(), alloc)
        {}

        KeysetImpl(
            std::initializer_list<Value> init, Size size = 0,
            H const &hf = H(), C const &eqf = C(), A const &alloc = A()
        ):
            KeysetImpl(iter(init), size, hf, eqf, alloc)
        {}

        KeysetImpl(std::initializer_list<Value> init, Size size, A const &alloc):
            KeysetImpl(iter(init), size, H(), C(), alloc)
        {}

        KeysetImpl(
            std::initializer_list<Value> init, Size size, H const &hf, A const &alloc
        ):
            KeysetImpl(iter(init), size, hf, C(), alloc)
        {}

        KeysetImpl &operator=(KeysetImpl const &m) {
            Base::operator=(m);
            return *this;
        }

        KeysetImpl &operator=(KeysetImpl &&m) {
            Base::operator=(std::move(m));
            return *this;
        }

        template<typename R, typename = EnableIf<
            IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
        >>
        KeysetImpl &operator=(R range) {
            Base::assign_range(range);
            return *this;
        }

        KeysetImpl &operator=(std::initializer_list<Value> il) {
            Base::assign_init(il);
            return *this;
        }

        T *at(Key const &key) {
            static_assert(!IsMultihash, "at() only allowed on regular keysets");
            return Base::access(key);
        }
        T const *at(Key const &key) const {
            static_assert(!IsMultihash, "at() only allowed on regular keysets");
            return Base::access(key);
        }

        T &operator[](Key const &key) {
            static_assert(!IsMultihash, "operator[] only allowed on regular keysets");
            return Base::access_or_insert(key);
        }
        T &operator[](Key &&key) {
            static_assert(!IsMultihash, "operator[] only allowed on regular keysets");
            return Base::access_or_insert(std::move(key));
        }

        void swap(KeysetImpl &v) {
            Base::swap(v);
        }
    };
}

template<
    typename T,
    typename H = std::hash<detail::KeysetKey<T>>,
    typename C = EqualWithCstr<detail::KeysetKey<T>>,
    typename A = Allocator<T>
>
using Keyset = detail::KeysetImpl<T, H, C, A, false>;

template<typename T, typename H, typename C, typename A>
inline void swap(Keyset<T, H, C, A> &a, Keyset<T, H, C, A> &b) {
    a.swap(b);
}

template<
    typename T,
    typename H = std::hash<detail::KeysetKey<T>>,
    typename C = EqualWithCstr<detail::KeysetKey<T>>,
    typename A = Allocator<T>
>
using Multikeyset = detail::KeysetImpl<T, H, C, A, true>;

template<typename T, typename H, typename C, typename A>
inline void swap(Multikeyset<T, H, C, A> &a, Multikeyset<T, H, C, A> &b) {
    a.swap(b);
}

} /* namespace ostd */

#endif
