/* A set container for OctaSTD. Implemented as a hash table.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_SET_HH
#define OSTD_SET_HH

#include "ostd/types.hh"
#include "ostd/utility.hh"
#include "ostd/memory.hh"
#include "ostd/functional.hh"
#include "ostd/initializer_list.hh"

#include "ostd/internal/hashtable.hh"

namespace ostd {

namespace detail {
    template<typename T, typename A>
    struct SetBase {
        static inline T const &get_key(T const &e) {
            return e;
        }
        static inline T &get_data(T &e) {
            return e;
        }
        template<typename U>
        static inline void set_key(T &, U const &, A &) {}
        static inline void swap_elem(T &a, T &b) { swap_adl(a, b); }
    };

    template<typename T, typename H, typename C, typename A, bool IsMultihash>
    struct SetImpl: detail::Hashtable<
        detail::SetBase<T, A>, T, T, T, H, C, A, IsMultihash
    > {
    private:
        using Base = detail::Hashtable<
            detail::SetBase<T, A>, T, T, T, H, C, A, IsMultihash
        >;

    public:
        using Key = T;
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

        explicit SetImpl(
            Size size, H const &hf = H(),
            C const &eqf = C(), A const &alloc = A()
        ):
            Base(size, hf, eqf, alloc)
        {}

        SetImpl(): SetImpl(0) {}
        explicit SetImpl(A const &alloc): SetImpl(0, H(), C(), alloc) {}

        SetImpl(Size size, A const &alloc):
            SetImpl(size, H(), C(), alloc)
        {}
        SetImpl(Size size, H const &hf, A const &alloc):
            SetImpl(size, hf, C(), alloc)
        {}

        SetImpl(SetImpl const &m):
            Base(m, allocator_container_copy(m.get_alloc()))
        {}

        SetImpl(SetImpl const &m, A const &alloc): Base(m, alloc) {}

        SetImpl(SetImpl &&m): Base(move(m)) {}
        SetImpl(SetImpl &&m, A const &alloc): Base(move(m), alloc) {}

        template<typename R, typename = EnableIf<
            IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
        >>
        SetImpl(
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
        SetImpl(R range, Size size, A const &alloc):
            SetImpl(range, size, H(), C(), alloc)
        {}

        template<typename R>
        SetImpl(R range, Size size, H const &hf, A const &alloc):
            SetImpl(range, size, hf, C(), alloc)
        {}

        SetImpl(
            std::initializer_list<Value> init, Size size = 0,
            H const &hf = H(), C const &eqf = C(), A const &alloc = A()
        ):
            SetImpl(iter(init), size, hf, eqf, alloc)
        {}

        SetImpl(std::initializer_list<Value> init, Size size, A const &alloc):
            SetImpl(iter(init), size, H(), C(), alloc)
        {}

        SetImpl(
            std::initializer_list<Value> init, Size size, H const &hf, A const &alloc
        ):
            SetImpl(iter(init), size, hf, C(), alloc)
        {}

        SetImpl &operator=(SetImpl const &m) {
            Base::operator=(m);
            return *this;
        }

        SetImpl &operator=(SetImpl &&m) {
            Base::operator=(move(m));
            return *this;
        }

        template<typename R, typename = EnableIf<
            IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
        >>
        SetImpl &operator=(R range) {
            Base::assign_range(range);
            return *this;
        }

        SetImpl &operator=(std::initializer_list<Value> il) {
            Base::assign_init(il);
            return *this;
        }

        void swap(SetImpl &v) {
            Base::swap(v);
        }
    };
}

template<
    typename T,
    typename H = ToHash<T>,
    typename C = EqualWithCstr<T>,
    typename A = Allocator<T>
>
using Set = detail::SetImpl<T, H, C, A, false>;

template<
    typename T,
    typename H = ToHash<T>,
    typename C = EqualWithCstr<T>,
    typename A = Allocator<T>
>
using Multiset = detail::SetImpl<T, H, C, A, true>;

} /* namespace ostd */

#endif
