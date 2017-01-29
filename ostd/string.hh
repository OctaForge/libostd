/* String for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_STRING_HH
#define OSTD_STRING_HH

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>

#include "ostd/utility.hh"
#include "ostd/range.hh"
#include "ostd/vector.hh"
#include "ostd/functional.hh"
#include "ostd/type_traits.hh"
#include "ostd/algorithm.hh"

namespace ostd {
static constexpr Size npos = -1;

template<typename T, typename A = Allocator<T>>
class StringBase;

template<typename T>
struct CharRangeBase: InputRange<
    CharRangeBase<T>, ContiguousRangeTag, T
> {
private:
    struct Nat {};

public:
    CharRangeBase(): p_beg(nullptr), p_end(nullptr) {};

    template<typename U>
    CharRangeBase(T *beg, U end, EnableIf<
        (IsPointer<U> || IsNullPointer<U>) && IsConvertible<U, T *>, Nat
    > = Nat()): p_beg(beg), p_end(end) {}

    CharRangeBase(T *beg, Size n): p_beg(beg), p_end(beg + n) {}

    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    template<typename U>
    CharRangeBase(
        U beg, EnableIf<IsConvertible<U, T *> && !IsArray<U>, Nat> = Nat()
    ): p_beg(beg), p_end(static_cast<T *>(beg) + (beg ? strlen(beg) : 0)) {}

    CharRangeBase(Nullptr): p_beg(nullptr), p_end(nullptr) {}

    template<typename U, Size N>
    CharRangeBase(U (&beg)[N], EnableIf<IsConvertible<U *, T *>, Nat> = Nat()):
        p_beg(beg), p_end(beg + N - (beg[N - 1] == '\0'))
    {}

    template<typename U, typename A>
    CharRangeBase(StringBase<U, A> const &s, EnableIf<
        IsConvertible<U *, T *>, Nat
    > = Nat()):
        p_beg(s.data()), p_end(s.data() + s.size())
    {}

    template<typename U, typename = EnableIf<IsConvertible<U *, T *>>>
    CharRangeBase(CharRangeBase<U> const &v):
        p_beg(&v[0]), p_end(&v[v.size()])
    {}

    CharRangeBase &operator=(CharRangeBase const &v) {
        p_beg = v.p_beg; p_end = v.p_end; return *this;
    }

    template<typename A>
    CharRangeBase &operator=(StringBase<T, A> const &s) {
        p_beg = s.data(); p_end = s.data() + s.size(); return *this;
    }
    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    CharRangeBase &operator=(T *s) {
        p_beg = s; p_end = s + (s ? strlen(s) : 0); return *this;
    }

    bool empty() const { return p_beg == p_end; }

    bool pop_front() {
        if (p_beg == p_end) {
            return false;
        }
        ++p_beg;
        return true;
    }
    bool push_front() { --p_beg; return true; }

    Size pop_front_n(Size n) {
        Size olen = p_end - p_beg;
        p_beg += n;
        if (p_beg > p_end) {
            p_beg = p_end;
            return olen;
        }
        return n;
    }

    Size push_front_n(Size n) { p_beg -= n; return true; }

    T &front() const { return *p_beg; }

    bool equals_front(CharRangeBase const &range) const {
        return p_beg == range.p_beg;
    }

    Ptrdiff distance_front(CharRangeBase const &range) const {
        return range.p_beg - p_beg;
    }

    bool pop_back() {
        if (p_end == p_beg) {
            return false;
        }
        --p_end;
        return true;
    }
    bool push_back() { ++p_end; return true; }

    Size pop_back_n(Size n) {
        Size olen = p_end - p_beg;
        p_end -= n;
        if (p_end < p_beg) {
            p_end = p_beg;
            return olen;
        }
        return n;
    }

    Size push_back_n(Size n) { p_end += n; return true; }

    T &back() const { return *(p_end - 1); }

    bool equals_back(CharRangeBase const &range) const {
        return p_end == range.p_end;
    }

    Ptrdiff distance_back(CharRangeBase const &range) const {
        return range.p_end - p_end;
    }

    Size size() const { return p_end - p_beg; }

    CharRangeBase slice(Size start, Size end) const {
        return CharRangeBase(p_beg + start, p_beg + end);
    }

    T &operator[](Size i) const { return p_beg[i]; }

    bool put(T v) {
        if (empty()) {
            return false;
        }
        *(p_beg++) = v;
        return true;
    }

    Size put_n(T const *p, Size n) {
        Size an = ostd::min(n, size());
        memcpy(p_beg, p, an * sizeof(T));
        p_beg += an;
        return an;
    }

    T *data() { return p_beg; }
    T const *data() const { return p_beg; }

    Size to_hash() const {
        return detail::mem_hash(data(), size());
    }

    /* non-range */
    int compare(CharRangeBase<T const> s) const {
        ostd::Size s1 = size(), s2 = s.size();
        int ret;
        if (!s1 || !s2) {
            goto diffsize;
        }
        if ((ret = memcmp(data(), s.data(), ostd::min(s1, s2)))) {
            return ret;
        }
diffsize:
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    int case_compare(CharRangeBase<T const> s) const {
        ostd::Size s1 = size(), s2 = s.size();
        for (ostd::Size i = 0, ms = ostd::min(s1, s2); i < ms; ++i) {
            int d = toupper(p_beg[i]) - toupper(s[i]);
            if (d) {
                return d;
            }
        }
        return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
    }

    template<typename R>
    EnableIf<IsOutputRange<R>, Size> copy(R &&orange, Size n = -1) {
        return orange.put_n(data(), ostd::min(n, size()));
    }

    Size copy(RemoveCv<T> *p, Size n = -1) {
        Size c = ostd::min(n, size());
        memcpy(p, data(), c * sizeof(T));
        return c;
    }

private:
    T *p_beg, *p_end;
};

using CharRange = CharRangeBase<char>;
using ConstCharRange = CharRangeBase<char const>;

inline bool operator==(ConstCharRange lhs, ConstCharRange rhs) {
    return !lhs.compare(rhs);
}

inline bool operator!=(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs);
}

inline bool operator<(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator>(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator<=(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator>=(ConstCharRange lhs, ConstCharRange rhs) {
    return lhs.compare(rhs) >= 0;
}

inline bool starts_with(ConstCharRange a, ConstCharRange b) {
    if (a.size() < b.size()) {
        return false;
    }
    return a.slice(0, b.size()) == b;
}

template<typename T, typename A>
class StringBase {
    using StrPair = detail::CompressedPair<AllocatorPointer<A>, A>;

    ostd::Size p_len, p_cap;
    StrPair p_buf;

    template<typename R>
    void ctor_from_range(R &range, EnableIf<
        IsFiniteRandomAccessRange<R> && IsSame<T, RemoveCv<RangeValue<R>>>, bool
    > = true) {
        if (range.empty()) {
            return;
        }
        RangeSize<R> l = range.size();
        reserve(l);
        p_len = l;
        range.copy(p_buf.first(), l);
        p_buf.first()[l] = '\0';
    }

    template<typename R>
    void ctor_from_range(R &range, EnableIf<
        !IsFiniteRandomAccessRange<R> || !IsSame<T, RemoveCv<RangeValue<R>>>,
        bool
    > = true) {
        if (range.empty()) {
            return;
        }
        Size i = 0;
        for (; !range.empty(); range.pop_front()) {
            reserve(i + 1);
            allocator_construct(
                p_buf.second(), &p_buf.first()[i],
                range.front()
            );
            ++i;
            p_len = i;
        }
        p_buf.first()[p_len] = '\0';
    }

public:
    using Size = ostd::Size;
    using Difference = Ptrdiff;
    using Value = T;
    using Reference = T &;
    using ConstReference = T const &;
    using Pointer = AllocatorPointer<A>;
    using ConstPointer = AllocatorConstPointer<A>;
    using Range = CharRangeBase<T>;
    using ConstRange = CharRangeBase<T const>;
    using Allocator = A;

    StringBase(A const &a = A()):
        p_len(0), p_cap(0),
        p_buf(reinterpret_cast<Pointer>(&p_len), a)
    {}

    explicit StringBase(Size n, T val = T(), A const &al = A()):
        StringBase(al)
    {
        if (!n) {
            return;
        }
        p_buf.first() = allocator_allocate(p_buf.second(), n + 1);
        p_len = p_cap = n;
        Pointer cur = p_buf.first(), last = p_buf.first() + n;
        while (cur != last) {
            *cur++ = val;
        }
        *cur = '\0';
    }

    StringBase(StringBase const &s):
        p_len(0), p_cap(0), p_buf(reinterpret_cast<Pointer>(&p_len),
        allocator_container_copy(s.p_buf.second()))
    {
        if (!s.p_len) {
            return;
        }
        reserve(s.p_len);
        p_len = s.p_len;
        memcpy(p_buf.first(), s.p_buf.first(), (p_len + 1) * sizeof(T));
    }
    StringBase(StringBase const &s, A const &a):
        p_len(0), p_cap(0), p_buf(reinterpret_cast<Pointer>(&p_len), a)
     {
        if (!s.p_len) {
            return;
        }
        reserve(s.p_len);
        p_len = s.p_len;
        memcpy(p_buf.first(), s.p_buf.first(), (p_len + 1) * sizeof(T));
    }
    StringBase(StringBase &&s):
        p_len(s.p_len), p_cap(s.p_cap),
        p_buf(s.p_buf.first(), std::move(s.p_buf.second()))
    {
        s.p_len = s.p_cap = 0;
        s.p_buf.first() = reinterpret_cast<Pointer>(&s.p_len);
    }
    StringBase(StringBase &&s, A const &a):
        p_len(0), p_cap(0), p_buf(reinterpret_cast<Pointer>(&p_len), a)
    {
        if (!s.p_len) {
            return;
        }
        if (a != s.p_buf.second()) {
            reserve(s.p_cap);
            p_len = s.p_len;
            memcpy(p_buf.first(), s.p_buf.first(), (p_len + 1) * sizeof(T));
            return;
        }
        p_buf.first() = s.p_buf.first();
        p_len = s.p_len;
        p_cap = s.p_cap;
        s.p_len = s.p_cap = 0;
        s.p_buf.first() = &s.p_cap;
    }

    StringBase(
        StringBase const &s, Size pos, Size len = npos, A const &a = A()
    ): StringBase(a) {
        Size end = (len == npos) ? s.size() : (pos + len);
        Size nch = (end - pos);
        reserve(nch);
        memcpy(p_buf.first(), s.p_buf.first() + pos, nch);
        p_len += nch;
        p_buf.first()[p_len] = '\0';
    }

    /* TODO: traits for utf-16/utf-32 string lengths, for now assume char */
    StringBase(ConstRange v, A const &a = A()): StringBase(a) {
        if (!v.size()) {
            return;
        }
        reserve(v.size());
        memcpy(p_buf.first(), &v[0], v.size());
        p_buf.first()[v.size()] = '\0';
        p_len = v.size();
    }

    template<typename U>
    StringBase(U v, EnableIf<
        IsConvertible<U, Value const *> && !IsArray<U>, A
    > const &a = A()): StringBase(ConstRange(v), a) {}

    template<typename U, Size N>
    StringBase(U (&v)[N], EnableIf<
        IsConvertible<U *, Value const *>, A
    > const &a = A()): StringBase(ConstRange(v), a) {}

    template<typename R, typename = EnableIf<
        IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
    >>
    StringBase(R range, A const &a = A()): StringBase(a) {
        ctor_from_range(range);
    }

    ~StringBase() {
        if (!p_cap) {
            return;
        }
        allocator_deallocate(p_buf.second(), p_buf.first(), p_cap + 1);
    }

    void clear() {
        if (!p_len) {
            return;
        }
        p_len = 0;
        *p_buf.first() = '\0';
    }

    StringBase &operator=(StringBase const &v) {
        if (this == &v) {
            return *this;
        }
        clear();
        if (AllocatorPropagateOnContainerCopyAssignment<A>) {
            if ((p_buf.second() != v.p_buf.second()) && p_cap) {
                allocator_deallocate(p_buf.second(), p_buf.first(), p_cap);
                p_cap = 0;
                p_buf.first() = reinterpret_cast<Pointer>(&p_len);
            }
            p_buf.second() = v.p_buf.second();
        }
        reserve(v.p_cap);
        p_len = v.p_len;
        if (p_len) {
            memcpy(p_buf.first(), v.p_buf.first(), p_len);
            p_buf.first()[p_len] = '\0';
        } else {
            p_buf.first() = reinterpret_cast<Pointer>(&p_len);
        }
        return *this;
    }

    StringBase &operator=(StringBase &&v) {
        clear();
        if (p_cap) {
            allocator_deallocate(p_buf.second(), p_buf.first(), p_cap);
        }
        if (AllocatorPropagateOnContainerMoveAssignment<A>) {
            p_buf.second() = v.p_buf.second();
        }
        p_len = v.p_len;
        p_cap = v.p_cap;
        p_buf.~StrPair();
        new (&p_buf) StrPair(v.release(), std::move(v.p_buf.second()));
        if (!p_cap) {
            p_buf.first() = reinterpret_cast<Pointer>(&p_len);
        }
        return *this;
    }

    StringBase &operator=(ConstRange v) {
        reserve(v.size());
        if (v.size()) {
            memcpy(p_buf.first(), &v[0], v.size());
        }
        p_buf.first()[v.size()] = '\0';
        p_len = v.size();
        return *this;
    }

    template<typename U>
    EnableIf<
        IsConvertible<U, Value const *> && !IsArray<U>, StringBase &
    > operator=(U v) {
        return operator=(ConstRange(v));
    }

    template<typename U, Size N>
    EnableIf<
        IsConvertible<U *, Value const *>, StringBase &
    > operator=(U (&v)[N]) {
        return operator=(ConstRange(v));
    }

    template<typename R, typename = EnableIf<
        IsInputRange<R> && IsConvertible<RangeReference<R>, Value>
    >> StringBase &operator=(R const &r) {
        clear();
        ctor_from_range(r);
        return *this;
    }

    void resize(Size n, T v = T()) {
        if (!n) {
            clear();
            return;
        }
        Size l = p_len;
        reserve(n);
        p_len = n;
        for (Size i = l; i < p_len; ++i) {
            p_buf.first()[i] = T(v);
        }
        p_buf.first()[l] = '\0';
    }

    void reserve(Size n) {
        if (n <= p_cap) {
            return;
        }
        Size oc = p_cap;
        if (!oc) {
            p_cap = max(n, Size(8));
        } else {
            while (p_cap < n) p_cap *= 2;
        }
        Pointer tmp = allocator_allocate(p_buf.second(), p_cap + 1);
        if (oc > 0) {
            memcpy(tmp, p_buf.first(), (p_len + 1) * sizeof(T));
            allocator_deallocate(p_buf.second(), p_buf.first(), oc + 1);
        }
        tmp[p_len] = '\0';
        p_buf.first() = tmp;
    }

    T &operator[](Size i) { return p_buf.first()[i]; }
    T const &operator[](Size i) const { return p_buf.first()[i]; }

    T &at(Size i) { return p_buf.first()[i]; }
    T const &at(Size i) const { return p_buf.first()[i]; }

    T &front() { return p_buf.first()[0]; }
    T const &front() const { return p_buf.first()[0]; };

    T &back() { return p_buf.first()[size() - 1]; }
    T const &back() const { return p_buf.first()[size() - 1]; }

    Value *data() { return p_buf.first(); }
    Value const *data() const { return p_buf.first(); }

    Size size() const {
        return p_len;
    }

    Size capacity() const {
        return p_cap;
    }

    void advance(Size s) { p_len += s; }

    Size length() const {
        /* TODO: unicode */
        return size();
    }

    bool empty() const { return (size() == 0); }

    Value *release() {
        Pointer r = p_buf.first();
        p_buf.first() = nullptr;
        p_len = p_cap = 0;
        return reinterpret_cast<Value *>(r);
    }

    void push(T v) {
        reserve(p_len + 1);
        p_buf.first()[p_len++] = v;
        p_buf.first()[p_len] = '\0';
    }

    StringBase &append(ConstRange r) {
        if (!r.size()) {
            return *this;
        }
        reserve(p_len + r.size());
        memcpy(p_buf.first() + p_len, &r[0], r.size());
        p_len += r.size();
        p_buf.first()[p_len] = '\0';
        return *this;
    }

    StringBase &append(Size n, T c) {
        if (!n) {
            return *this;
        }
        reserve(p_len + n);
        for (Size i = 0; i < n; ++i) {
            p_buf.first()[p_len + i] = c;
        }
        p_len += n;
        p_buf.first()[p_len] = '\0';
        return *this;
    }

    template<typename R, typename = EnableIf<
        IsInputRange<R> && IsConvertible<RangeReference<R>, Value> &&
        !IsConvertible<R, ConstRange>
    >> StringBase &append(R range) {
        Size nadd = 0;
        for (; !range.empty(); range.pop_front()) {
            reserve(p_len + nadd + 1);
            p_buf.first()[p_len + nadd++] = range.front(); 
        }
        p_len += nadd;
        p_buf.first()[p_len] = '\0';
        return *this;
    }

    StringBase &operator+=(ConstRange r) {
        return append(r);
    }
    StringBase &operator+=(T c) {
        return append(1, c);
    }
    template<typename R>
    StringBase &operator+=(R const &v) {
        return append(v);
    }

    int compare(ConstRange r) const {
        return iter().compare(r);
    }

    int case_compare(ConstRange r) const {
        return iter().case_compare(r);
    }

    Range iter() {
        return Range(p_buf.first(), size());
    }
    ConstRange iter() const {
        return ConstRange(p_buf.first(), size());
    }
    ConstRange citer() const {
        return ConstRange(p_buf.dfirst(), size());
    }

    Range iter_cap() {
        return Range(p_buf.first(), capacity());
    }

    void swap(StringBase &v) {
        detail::swap_adl(p_len, v.p_len);
        detail::swap_adl(p_cap, v.p_cap);
        detail::swap_adl(p_buf.first(), v.p_buf.first());
        if (AllocatorPropagateOnContainerSwap<A>) {
            detail::swap_adl(p_buf.second(), v.p_buf.second());
        }
    }

    Size to_hash() const {
        return iter().to_hash();
    }

    A get_allocator() const {
        return p_buf.second();
    }
};

using String = StringBase<char>;

/* string literals */

inline namespace literals {
inline namespace string_literals {
    inline String operator "" _s(char const *str, Size len) {
        return String(ConstCharRange(str, len));
    }

    inline ConstCharRange operator "" _S(char const *str, Size len) {
        return ConstCharRange(str, len);
    }
}
}

namespace detail {
    template<
        typename T, bool = IsConvertible<T, ConstCharRange>,
        bool = IsConvertible<T, char>
    >
    struct ConcatPut;

    template<typename T, bool B>
    struct ConcatPut<T, true, B> {
        template<typename R>
        static bool put(R &sink, ConstCharRange v) {
            return v.size() && (sink.put_n(&v[0], v.size()) == v.size());
        }
    };

    template<typename T>
    struct ConcatPut<T, false, true> {
        template<typename R>
        static bool put(R &sink, char v) {
            return sink.put(v);
        }
    };
}

template<typename R, typename T, typename F>
bool concat(R &&sink, T const &v, ConstCharRange sep, F func) {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return true;
    }
    for (;;) {
        if (!detail::ConcatPut<
            decltype(func(range.front()))
        >::put(sink, func(range.front()))) {
            return false;
        }
        range.pop_front();
        if (range.empty()) {
            break;
        }
        sink.put_n(&sep[0], sep.size());
    }
    return true;
}

template<typename R, typename T>
bool concat(R &&sink, T const &v, ConstCharRange sep = " ") {
    auto range = ostd::iter(v);
    if (range.empty()) {
        return true;
    }
    for (;;) {
        ConstCharRange ret = range.front();
        if (!ret.size() || (sink.put_n(&ret[0], ret.size()) != ret.size())) {
            return false;
        }
        range.pop_front();
        if (range.empty()) {
            break;
        }
        sink.put_n(&sep[0], sep.size());
    }
    return true;
}

template<typename R, typename T, typename F>
bool concat(R &&sink, std::initializer_list<T> v, ConstCharRange sep, F func) {
    return concat(sink, ostd::iter(v), sep, func);
}

template<typename R, typename T>
bool concat(R &&sink, std::initializer_list<T> v, ConstCharRange sep = " ") {
    return concat(sink, ostd::iter(v), sep);
}

namespace detail {
    template<typename R>
    struct TostrRange: OutputRange<TostrRange<R>, char> {
        TostrRange() = delete;
        TostrRange(R &out): p_out(out), p_written(0) {}
        bool put(char v) {
            bool ret = p_out.put(v);
            p_written += ret;
            return ret;
        }
        Size put_n(char const *v, Size n) {
            Size ret = p_out.put_n(v, n);
            p_written += ret;
            return ret;
        }
        Size put_string(ConstCharRange r) {
            return put_n(&r[0], r.size());
        }
        Size get_written() const { return p_written; }
    private:
        R &p_out;
        Size p_written;
    };

    template<typename T, typename R>
    static auto test_stringify(int) ->
        BoolConstant<IsSame<decltype(std::declval<T>().stringify()), String>>;

    template<typename T, typename R>
    static True test_stringify(decltype(std::declval<T const &>().to_string
        (std::declval<R &>())) *);

    template<typename, typename>
    static False test_stringify(...);

    template<typename T, typename R>
    constexpr bool StringifyTest = decltype(test_stringify<T, R>(0))::value;

    template<typename T>
    static True test_iterable(decltype(ostd::iter(std::declval<T>())) *);
    template<typename>
    static False test_iterable(...);

    template<typename T>
    constexpr bool IterableTest = decltype(test_iterable<T>(0))::value;
}

template<typename T, typename = void>
struct ToString;

template<typename T>
struct ToString<T, EnableIf<detail::IterableTest<T>>> {
    using Argument = RemoveCv<RemoveReference<T>>;
    using Result = String;

    String operator()(T const &v) const {
        String ret("{");
        auto x = appender<String>();
        if (concat(x, ostd::iter(v), ", ", ToString<
            RemoveConst<RemoveReference<
                RangeReference<decltype(ostd::iter(v))>
            >>
        >())) {
            ret += x.get();
        }
        ret += "}";
        return ret;
    }
};

template<typename T>
struct ToString<T, EnableIf<
    detail::StringifyTest<T, detail::TostrRange<AppenderRange<String>>>
>> {
    using Argument = RemoveCv<RemoveReference<T>>;
    using Result = String;

    String operator()(T const &v) const {
        auto app = appender<String>();
        detail::TostrRange<AppenderRange<String>> sink(app);
        if (!v.to_string(sink)) {
            return String();
        }
        return std::move(app.get());
    }
};

namespace detail {
    template<typename T>
    void str_printf(String &s, char const *fmt, T v) {
        char buf[256];
        int n = snprintf(buf, sizeof(buf), fmt, v);
        s.clear();
        s.reserve(n);
        if (n >= int(sizeof(buf))) {
            snprintf(s.data(), n + 1, fmt, v);
        } else if (n > 0) {
            memcpy(s.data(), buf, n + 1);
        } else {
            s.clear();
        }
        *reinterpret_cast<Size *>(&s) = n;
    }
}

template<>
struct ToString<bool> {
    using Argument = bool;
    using Result = String;
    String operator()(bool b) {
        return b ? "true" : "false";
    }
};

template<>
struct ToString<char> {
    using Argument = char;
    using Result = String;
    String operator()(char c) {
        String ret;
        ret.push(c);
        return ret;
    }
};

#define OSTD_TOSTR_NUM(T, fmt) \
template<> \
struct ToString<T> { \
    using Argument = T; \
    using Result = String; \
    String operator()(T v) { \
        String ret; \
        detail::str_printf(ret, fmt, v); \
        return ret; \
    } \
};

OSTD_TOSTR_NUM(sbyte, "%hhd")
OSTD_TOSTR_NUM(short, "%hd")
OSTD_TOSTR_NUM(int, "%d")
OSTD_TOSTR_NUM(long, "%ld")
OSTD_TOSTR_NUM(float, "%f")
OSTD_TOSTR_NUM(double, "%f")

OSTD_TOSTR_NUM(byte, "%hhu")
OSTD_TOSTR_NUM(ushort, "%hu")
OSTD_TOSTR_NUM(uint, "%u")
OSTD_TOSTR_NUM(ulong, "%lu")
OSTD_TOSTR_NUM(llong, "%lld")
OSTD_TOSTR_NUM(ullong, "%llu")
OSTD_TOSTR_NUM(ldouble, "%Lf")

#undef OSTD_TOSTR_NUM

template<typename T>
struct ToString<T *> {
    using Argument = T *;
    using Result = String;
    String operator()(Argument v) {
        String ret;
        detail::str_printf(ret, "%p", v);
        return ret;
    }
};

template<>
struct ToString<char const *> {
    using Argument = char const *;
    using Result = String;
    String operator()(char const *s) {
        return String(s);
    }
};

template<>
struct ToString<char *> {
    using Argument = char *;
    using Result = String;
    String operator()(char *s) {
        return String(s);
    }
};

template<>
struct ToString<String> {
    using Argument = String;
    using Result = String;
    String operator()(Argument const &s) {
        return s;
    }
};

template<>
struct ToString<CharRange> {
    using Argument = CharRange;
    using Result = String;
    String operator()(Argument const &s) {
        return String(s);
    }
};

template<>
struct ToString<ConstCharRange> {
    using Argument = ConstCharRange;
    using Result = String;
    String operator()(Argument const &s) {
        return String(s);
    }
};

template<typename T, typename U>
struct ToString<std::pair<T, U>> {
    using Argument = std::pair<T, U>;
    using Result = String;
    String operator()(Argument const &v) {
        String ret("{");
        ret += ToString<RemoveCv<RemoveReference<T>>>()(v.first);
        ret += ", ";
        ret += ToString<RemoveCv<RemoveReference<U>>>()(v.second);
        ret += "}";
        return ret;
    }
};

namespace detail {
    template<Size I, Size N>
    struct TupleToString {
        template<typename T>
        static void append(String &ret, T const &tup) {
            ret += ", ";
            ret += ToString<RemoveCv<RemoveReference<
                decltype(std::get<I>(tup))
            >>>()(std::get<I>(tup));
            TupleToString<I + 1, N>::append(ret, tup);
        }
    };

    template<Size N>
    struct TupleToString<N, N> {
        template<typename T>
        static void append(String &, T const &) {}
    };

    template<Size N>
    struct TupleToString<0, N> {
        template<typename T>
        static void append(String &ret, T const &tup) {
            ret += ToString<RemoveCv<RemoveReference<
                decltype(std::get<0>(tup))
            >>>()(std::get<0>(tup));
            TupleToString<1, N>::append(ret, tup);
        }
    };
}

template<typename ...T>
struct ToString<std::tuple<T...>> {
    using Argument = std::tuple<T...>;
    using Result = String;
    String operator()(Argument const &v) {
        String ret("{");
        detail::TupleToString<0, sizeof...(T)>::append(ret, v);
        ret += "}";
        return ret;
    }
};

template<typename T>
typename ToString<T>::Result to_string(T const &v) {
    return ToString<RemoveCv<RemoveReference<T>>>()(v);
}

template<typename T>
String to_string(std::initializer_list<T> init) {
    return to_string(iter(init));
}

/* TODO: rvalue ref versions for rhs when we have efficient prepend */

inline String operator+(String const &lhs, ConstCharRange rhs) {
    String ret(lhs); ret += rhs; return ret;
}

inline String operator+(ConstCharRange lhs, String const &rhs) {
    String ret(lhs); ret += rhs; return ret;
}

inline String operator+(String const &lhs, char rhs) {
    String ret(lhs); ret += rhs; return ret;
}

inline String operator+(char lhs, String const &rhs) {
    String ret(lhs); ret += rhs; return ret;
}

inline String operator+(String &&lhs, ConstCharRange rhs) {
    String ret(std::move(lhs)); ret += rhs; return ret;
}

inline String operator+(String &&lhs, char rhs) {
    String ret(std::move(lhs)); ret += rhs; return ret;
}

template<typename R>
struct TempCString {
private:
    RemoveCv<RangeValue<R>> *p_buf;
    bool p_allocated;

public:
    TempCString() = delete;
    TempCString(TempCString const &) = delete;
    TempCString(TempCString &&s): p_buf(s.p_buf), p_allocated(s.p_allocated) {
        s.p_buf = nullptr;
        s.p_allocated = false;
    }
    TempCString(R input, RemoveCv<RangeValue<R>> *sbuf, Size bufsize)
    : p_buf(nullptr), p_allocated(false) {
        if (input.empty()) {
            return;
        }
        if (input.size() >= bufsize) {
            p_buf = new RemoveCv<RangeValue<R>>[input.size() + 1];
            p_allocated = true;
        } else {
            p_buf = sbuf;
        }
        p_buf[input.copy(p_buf)] = '\0';
    }
    ~TempCString() {
        if (p_allocated) {
            delete[] p_buf;
        }
    }

    TempCString &operator=(TempCString const &) = delete;
    TempCString &operator=(TempCString &&s) {
        swap(s);
        return *this;
    }

    operator RemoveCv<RangeValue<R>> const *() const { return p_buf; }
    RemoveCv<RangeValue<R>> const *get() const { return p_buf; }

    void swap(TempCString &s) {
        detail::swap_adl(p_buf, s.p_buf);
        detail::swap_adl(p_allocated, s.p_allocated);
    }
};

template<typename R>
inline TempCString<R> to_temp_cstr(
    R input, RemoveCv<RangeValue<R>> *buf, Size bufsize
) {
    return TempCString<R>(input, buf, bufsize);
}

} /* namespace ostd */

#endif
