/* String utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_STRING_HH
#define OSTD_STRING_HH

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <functional>
#include <utility>

#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/algorithm.hh"

namespace ostd {

template<typename T, typename TR = std::char_traits<std::remove_const_t<T>>>
struct basic_char_range: input_range<basic_char_range<T>> {
    using range_category  = contiguous_range_tag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

private:
    struct nat {};

public:
    basic_char_range(): p_beg(nullptr), p_end(nullptr) {};
    basic_char_range(T *beg, T *end): p_beg(beg), p_end(end) {}
    basic_char_range(std::nullptr_t): p_beg(nullptr), p_end(nullptr) {}

    template<typename U>
    basic_char_range(U &&beg, std::enable_if_t<
        std::is_convertible_v<U, T *>, nat
    > = nat{}): p_beg(beg) {
        if constexpr(std::is_array_v<std::remove_reference_t<U>>) {
            size_t N = std::extent_v<std::remove_reference_t<U>>;
            p_end = beg + N - (beg[N - 1] == '\0');
        } else {
            p_end = beg + (beg ? TR::length(beg) : 0);
        }
    }

    template<typename STR, typename A>
    basic_char_range(std::basic_string<std::remove_const_t<T>, STR, A> const &s):
        p_beg(s.data()), p_end(s.data() + s.size())
    {}

    template<typename U, typename TTR, typename = std::enable_if_t<
        std::is_convertible_v<U *, T *>
    >>
    basic_char_range(basic_char_range<U, TTR> const &v):
        p_beg(&v[0]), p_end(&v[v.size()])
    {}

    basic_char_range &operator=(basic_char_range const &v) {
        p_beg = v.p_beg; p_end = v.p_end; return *this;
    }

    template<typename STR, typename A>
    basic_char_range &operator=(std::basic_string<T, STR, A> const &s) {
        p_beg = s.data(); p_end = s.data() + s.size(); return *this;
    }

    basic_char_range &operator=(T *s) {
        p_beg = s; p_end = s + (s ? TR::length(s) : 0); return *this;
    }

    bool empty() const { return p_beg == p_end; }

    void pop_front() {
        ++p_beg;
        if (p_beg > p_end) {
            throw std::out_of_range{"pop_front on empty range"};
        }
    }

    T &front() const { return *p_beg; }

    void pop_back() {
        if (p_end == p_beg) {
            return;
        }
        --p_end;
    }

    T &back() const { return *(p_end - 1); }

    size_t size() const { return p_end - p_beg; }

    basic_char_range slice(size_t start, size_t end) const {
        return basic_char_range(p_beg + start, p_beg + end);
    }
    basic_char_range slice(size_t start) const {
        return slice(start, size());
    }

    T &operator[](size_t i) const { return p_beg[i]; }

    void put(T v) {
        if (p_beg == p_end) {
            throw std::out_of_range{"put into an empty range"};
        }
        *(p_beg++) = v;
    }

    T *data() { return p_beg; }
    T const *data() const { return p_beg; }

    /* non-range */
    int compare(basic_char_range<T const> s) const {
        size_t s1 = size(), s2 = s.size();
        int ret;
        if (!s1 || !s2) {
            goto diffsize;
        }
        if ((ret = TR::compare(data(), s.data(), std::min(s1, s2)))) {
            return ret;
        }
diffsize:
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    int case_compare(basic_char_range<T const> s) const {
        size_t s1 = size(), s2 = s.size();
        for (size_t i = 0, ms = std::min(s1, s2); i < ms; ++i) {
            int d = toupper(p_beg[i]) - toupper(s[i]);
            if (d) {
                return d;
            }
        }
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    /* that way we can assign, append etc to std::string */
    operator std::basic_string_view<std::remove_cv_t<T>>() const {
        return std::basic_string_view<std::remove_cv_t<T>>{data(), size()};
    }

private:
    T *p_beg, *p_end;
};

using char_range = basic_char_range<char>;
using string_range = basic_char_range<char const>;

/* comparisons between ranges */

template<typename T, typename TR>
inline bool operator==(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return !lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator!=(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator<(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) < 0;
}

template<typename T, typename TR>
inline bool operator>(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) > 0;
}

template<typename T, typename TR>
inline bool operator<=(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) <= 0;
}

template<typename T, typename TR>
inline bool operator>=(
    basic_char_range<T, TR> lhs, basic_char_range<T, TR> rhs
) {
    return lhs.compare(rhs) >= 0;
}

/* comparisons between mutable ranges and char arrays */

template<typename T, typename TR>
inline bool operator==(basic_char_range<T, TR> lhs, T const *rhs) {
    return !lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator!=(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator<(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) < 0;
}

template<typename T, typename TR>
inline bool operator>(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) > 0;
}

template<typename T, typename TR>
inline bool operator<=(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) <= 0;
}

template<typename T, typename TR>
inline bool operator>=(basic_char_range<T, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) >= 0;
}

template<typename T, typename TR>
inline bool operator==(T const *lhs, basic_char_range<T, TR> rhs) {
    return !rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator!=(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator<(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) > 0;
}

template<typename T, typename TR>
inline bool operator>(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) < 0;
}

template<typename T, typename TR>
inline bool operator<=(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) >= 0;
}

template<typename T, typename TR>
inline bool operator>=(T const *lhs, basic_char_range<T, TR> rhs) {
    return rhs.compare(lhs) <= 0;
}

/* comparisons between immutable ranges and char arrays */

template<typename T, typename TR>
inline bool operator==(basic_char_range<T const, TR> lhs, T const *rhs) {
    return !lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator!=(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs);
}

template<typename T, typename TR>
inline bool operator<(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) < 0;
}

template<typename T, typename TR>
inline bool operator>(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) > 0;
}

template<typename T, typename TR>
inline bool operator<=(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) <= 0;
}

template<typename T, typename TR>
inline bool operator>=(basic_char_range<T const, TR> lhs, T const *rhs) {
    return lhs.compare(rhs) >= 0;
}

template<typename T, typename TR>
inline bool operator==(T const *lhs, basic_char_range<T const, TR> rhs) {
    return !rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator!=(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs);
}

template<typename T, typename TR>
inline bool operator<(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) > 0;
}

template<typename T, typename TR>
inline bool operator>(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) < 0;
}

template<typename T, typename TR>
inline bool operator<=(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) >= 0;
}

template<typename T, typename TR>
inline bool operator>=(T const *lhs, basic_char_range<T const, TR> rhs) {
    return rhs.compare(lhs) <= 0;
}

inline bool starts_with(string_range a, string_range b
) {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

template<typename T, typename TR, typename A>
struct ranged_traits<std::basic_string<T, TR, A>> {
    using range = basic_char_range<T, TR>;

    static range iter(std::basic_string<T, TR, A> &v) {
        return range{v.data(), v.data() + v.size()};
    }
};

template<typename T, typename TR, typename A>
struct ranged_traits<std::basic_string<T, TR, A> const> {
    using range = basic_char_range<T const, TR>;

    static range iter(std::basic_string<T, TR, A> const &v) {
        return range{v.data(), v.data() + v.size()};
    }
};

template<
    typename T, typename TR = std::char_traits<T>,
    typename A = std::allocator<T>, typename R
>
inline std::basic_string<T, TR, A> make_string(R range, A const &alloc = A{}) {
    std::basic_string<T, TR, A> ret{alloc};
    using C = range_category_t<R>;
    if constexpr(std::is_convertible_v<C, contiguous_range_tag>) {
        ret.reserve(range.size());
        ret.insert(ret.end(), range.data(), range.data() + range.size());
    } else {
        for (; !range.empty(); range.pop_front()) {
            ret.push_back(range.front());
        }
    }
    return ret;
}

template<
    typename R, typename TR = std::char_traits<std::remove_cv_t<range_value_t<R>>>,
    typename A = std::allocator<std::remove_cv_t<range_value_t<R>>>
>
inline std::basic_string<std::remove_cv_t<range_value_t<R>>, TR, A> make_string(
    R range, A const &alloc = A{}
) {
    return make_string<std::remove_cv_t<range_value_t<R>>, TR, A>(
        std::move(range), alloc
    );
}

/* string literals */

inline namespace literals {
inline namespace string_literals {
    inline string_range operator "" _sr(char const *str, size_t len) {
        return string_range(str, str + len);
    }
}
}

template<typename R>
struct temp_c_string {
private:
    std::remove_cv_t<range_value_t<R>> *p_buf;
    bool p_allocated;

public:
    temp_c_string() = delete;
    temp_c_string(temp_c_string const &) = delete;
    temp_c_string(temp_c_string &&s): p_buf(s.p_buf), p_allocated(s.p_allocated) {
        s.p_buf = nullptr;
        s.p_allocated = false;
    }
    temp_c_string(R input, std::remove_cv_t<range_value_t<R>> *sbuf, size_t bufsize)
    : p_buf(nullptr), p_allocated(false) {
        if (input.empty()) {
            return;
        }
        if (input.size() >= bufsize) {
            p_buf = new std::remove_cv_t<range_value_t<R>>[input.size() + 1];
            p_allocated = true;
        } else {
            p_buf = sbuf;
        }
        char_range bufr{p_buf, p_buf + input.size() + 1};
        range_put_all(bufr, input);
        bufr.put('\0');
    }
    ~temp_c_string() {
        if (p_allocated) {
            delete[] p_buf;
        }
    }

    temp_c_string &operator=(temp_c_string const &) = delete;
    temp_c_string &operator=(temp_c_string &&s) {
        swap(s);
        return *this;
    }

    operator std::remove_cv_t<range_value_t<R>> const *() const { return p_buf; }
    std::remove_cv_t<range_value_t<R>> const *get() const { return p_buf; }

    void swap(temp_c_string &s) {
        using std::swap;
        swap(p_buf, s.p_buf);
        swap(p_allocated, s.p_allocated);
    }
};

template<typename R>
inline void swap(temp_c_string<R> &a, temp_c_string<R> &b) {
    a.swap(b);
}

template<typename R>
inline temp_c_string<R> to_temp_cstr(
    R input, std::remove_cv_t<range_value_t<R>> *buf, size_t bufsize
) {
    return temp_c_string<R>(input, buf, bufsize);
}

} /* namespace ostd */

namespace std {

template<typename T, typename TR>
struct hash<ostd::basic_char_range<T, TR>> {
    size_t operator()(ostd::basic_char_range<T, TR> const &v) const {
        return hash<std::basic_string_view<std::remove_const_t<T>, TR>>{}(v);
    }
};

}

#endif
