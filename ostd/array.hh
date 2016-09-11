/* Static array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_ARRAY_HH
#define OSTD_ARRAY_HH

#include <stddef.h>

#include "ostd/algorithm.hh"
#include "ostd/range.hh"
#include "ostd/string.hh"
#include "ostd/utility.hh"
#include "ostd/internal/tuple.hh"

namespace ostd {

template<typename T, Size N>
struct Array {
    using Size = ostd::Size;
    using Difference = Ptrdiff;
    using Value = T;
    using Reference = T &;
    using ConstReference = T const &;
    using Pointer = T *;
    using ConstPointer = T const *;
    using Range = PointerRange<T>;
    using ConstRange = PointerRange<T const>;

    T &operator[](Size i) noexcept { return p_buf[i]; }
    T const &operator[](Size i) const noexcept { return p_buf[i]; }

    T *at(Size i) noexcept {
        if (!in_range(i)) {
            return nullptr;
        }
        return &p_buf[i];
    }
    T const *at(Size i) const noexcept {
        if (!in_range(i)) {
            return nullptr;
        }
        return &p_buf[i];
    }

    T &front() noexcept { return p_buf[0]; }
    T const &front() const noexcept { return p_buf[0]; }

    T &back() noexcept { return p_buf[(N > 0) ? (N - 1) : 0]; }
    T const &back() const noexcept { return p_buf[(N > 0) ? (N - 1) : 0]; }

    Size size() const noexcept { return N; }
    Size max_size() const noexcept { return Size(~0) / sizeof(T); }

    bool empty() const noexcept { return N == 0; }

    bool in_range(Size idx) noexcept { return idx < N; }
    bool in_range(int idx) noexcept { return idx >= 0 && Size(idx) < N; }
    bool in_range(ConstPointer ptr) noexcept {
        return ptr >= &p_buf[0] && ptr < &p_buf[N];
    }

    Pointer data() noexcept { return p_buf; }
    ConstPointer data() const noexcept { return p_buf; }

    Range iter() noexcept {
        return Range(p_buf, p_buf + N);
    }
    ConstRange iter() const noexcept {
        return ConstRange(p_buf, p_buf + N);
    }
    ConstRange citer() const noexcept {
        return ConstRange(p_buf, p_buf + N);
    }

    void swap(Array &v) noexcept(
        noexcept(ostd::swap(declval<T &>(), declval<T &>()))
    ) {
        ostd::swap_ranges(iter(), v.iter());
    }

    T p_buf[(N > 0) ? N : 1];
};

template<typename T, Size N>
constexpr Size TupleSize<Array<T, N>> = N;

template<Size I, typename T, Size N>
struct TupleElementBase<I, Array<T, N>> {
    using Type = T;
};

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> &get(Array<T, N> &a) noexcept {
    return a[I];
}

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> const &get(Array<T, N> const &a) noexcept {
    return a[I];
}

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> &&get(Array<T, N> &&a) noexcept {
    return move(a.p_buf[I]);
}

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> const &&get(Array<T, N> const &&a) noexcept {
    return move(a.p_buf[I]);
}

template<typename T, Size N>
inline bool operator==(Array<T, N> const &x, Array<T, N> const &y) {
    return equal(x.iter(), y.iter());
}

template<typename T, Size N>
inline bool operator!=(Array<T, N> const &x, Array<T, N> const &y) {
    return !(x == y);
}

template<typename T, Size N>
inline bool operator<(Array<T, N> const &x, Array<T, N> const &y) {
    return lexicographical_compare(x.iter(), y.iter());
}

template<typename T, Size N>
inline bool operator>(Array<T, N> const &x, Array<T, N> const &y) {
    return (y < x);
}

template<typename T, Size N>
inline bool operator<=(Array<T, N> const &x, Array<T, N> const &y) {
    return !(y < x);
}

template<typename T, Size N>
inline bool operator>=(Array<T, N> const &x, Array<T, N> const &y) {
    return !(x < y);
}

} /* namespace ostd */

#endif
