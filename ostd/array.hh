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

    T &operator[](Size i) { return p_buf[i]; }
    T const &operator[](Size i) const { return p_buf[i]; }

    T *at(Size i) {
        if (!in_range(i)) {
            return nullptr;
        }
        return &p_buf[i];
    }
    T const *at(Size i) const {
        if (!in_range(i)) {
            return nullptr;
        }
        return &p_buf[i];
    }

    T &front() { return p_buf[0]; }
    T const &front() const { return p_buf[0]; }

    T &back() { return p_buf[(N > 0) ? (N - 1) : 0]; }
    T const &back() const { return p_buf[(N > 0) ? (N - 1) : 0]; }

    Size size() const { return N; }
    Size max_size() const { return Size(~0) / sizeof(T); }

    bool empty() const { return N == 0; }

    bool in_range(Size idx) { return idx < N; }
    bool in_range(int idx) { return idx >= 0 && Size(idx) < N; }
    bool in_range(ConstPointer ptr) {
        return ptr >= &p_buf[0] && ptr < &p_buf[N];
    }

    Pointer data() { return p_buf; }
    ConstPointer data() const { return p_buf; }

    Range iter() {
        return Range(p_buf, p_buf + N);
    }
    ConstRange iter() const {
        return ConstRange(p_buf, p_buf + N);
    }
    ConstRange citer() const {
        return ConstRange(p_buf, p_buf + N);
    }

    void swap(Array &v) {
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
inline TupleElement<I, Array<T, N>> &get(Array<T, N> &a) {
    return a[I];
}

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> const &get(Array<T, N> const &a) {
    return a[I];
}

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> &&get(Array<T, N> &&a) {
    return move(a.p_buf[I]);
}

template<Size I, typename T, Size N>
inline TupleElement<I, Array<T, N>> const &&get(Array<T, N> const &&a) {
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
