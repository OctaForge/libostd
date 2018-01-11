/* String implementation details, mainly regarding Unicode support.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <type_traits>

#include "ostd/platform.hh"
#include "ostd/string.hh"
#include "ostd/format.hh"

namespace ostd {
namespace detail {

template<typename C>
inline std::size_t tstrlen_impl(C const *p) noexcept {
    using SL = std::numeric_limits<std::size_t>;
    using UL = std::numeric_limits<std::make_unsigned_t<C>>;
    constexpr std::size_t Lbits = SL::max() / UL::max();
    constexpr std::size_t Hbits = Lbits << (UL::digits - 1);

    C const *bp = p;
    if constexpr(sizeof(C) >= sizeof(std::size_t)) {
        goto sloop;
    }
    for (; std::uintptr_t(p) % sizeof(std::size_t); ++p) {
        if (!*p) {
            return (p - bp);
        }
    }
    {
        auto *wp = reinterpret_cast<std::size_t const *>(p);
        for (; !(((*wp - Lbits) & ~*wp) & Hbits); ++wp) {}
        p = reinterpret_cast<C const *>(wp);
    }
sloop:
    for (; *p; ++p) {}
    return (p - bp);
}

OSTD_EXPORT std::size_t tstrlen(char const *p) noexcept {
    return tstrlen_impl(p);
}
OSTD_EXPORT std::size_t tstrlen(char16_t const *p) noexcept {
    return tstrlen_impl(p);
}
OSTD_EXPORT std::size_t tstrlen(char32_t const *p) noexcept {
    return tstrlen_impl(p);
}
OSTD_EXPORT std::size_t tstrlen(wchar_t const *p) noexcept {
    return tstrlen_impl(p);
}

} /* namespace detail */
} /* namespace ostd */

namespace ostd {
namespace utf {

/* place the vtable in here */
utf_error::~utf_error() {}

namespace detail {
    inline bool is_invalid_u32(char32_t c) {
        return (((c >= 0xD800) && (c <= 0xDFFF)) || (c > utf::max_unicode));
    }

    static inline std::size_t u8_decode(
        unsigned char const *beg, unsigned char const *end, char32_t &cret
    ) noexcept {
        static char32_t const ulim[] = { 0xFF, 0x7F, 0x7FF, 0xFFFF };
        if (beg == end) {
            return 0;
        }
        char32_t ch = *beg;
        if (ch <= 0x7F) {
            /* ASCII */
            cret = ch;
            return 1;
        }
        char32_t ret = 0;
        unsigned char const *sbeg = beg + 1;
        /* continuation bytes */
        for (; ch & 0x40; ch <<= 1) {
            /* need a continuation byte but nothing left in the string */
            if (sbeg == end) {
                return 0;
            }
            /* the continuation byte */
            char32_t nch = *sbeg++;
            /* lower 6 bits */
            char32_t bch = nch & 0x3F;
            /* not a continuation byte */
            if ((nch ^ bch) != 0x80) {
                return 0;
            }
            /* the 6 bits go in the result */
            ret = (ret << 6) | bch;
        }
        /* by how many bytes we advance (continuation bytes + 1) */
        auto adv = std::size_t(sbeg - beg);
        /* invalid sequence - too many continuation bits */
        if (adv > 4) {
            return 0;
        }
        /* add the up to 7 bits from the first byte, already shifted left by n */
        ret |= (ch & 0x7F) << ((adv - 1) * 5);
        /* invalid sequence */
        if (is_invalid_u32(ret) || (ret <= ulim[adv - 1])) {
            return 0;
        }
        cret = ret;
        return adv;
    }

    static inline std::size_t u16_decode(
        char16_t const *beg, char16_t const *end, char32_t &cret
    ) noexcept {
        if (beg == end) {
            return 0;
        }
        char32_t ch = *beg;
        /* lead surrogate code point */
        if ((ch >= 0xD800) && (ch <= 0xDBFF)) {
            /* unpaired */
            if ((end - beg) < 2) {
                return 0;
            }
            char32_t nch = beg[1];
            /* trail surrogate code point */
            if ((nch >= 0xDC00) && (nch <= 0xDFFF)) {
                cret = 0x10000 + (((ch - 0xD800) << 10) | (nch - 0xDC00));
                return 2;
            }
            return 0;
        }
        cret = ch;
        return 1;
    }

    OSTD_EXPORT std::size_t encode(
        char (&ret)[4], char32_t ch
    ) noexcept {
        if (ch <= 0x7F) {
            ret[0] = char(ch);
            return 1;
        }
        if (ch <= 0x7FF) {
            ret[0] = char(0xC0 | (ch >> 6));
            ret[1] = char(0x80 | (ch & 0x3F));
            return 2;
        }
        if (ch <= 0xFFFF) {
            /* TODO: optional WTF-8 semantics
             * for now simply reject surrogate code points
             */
            if ((ch >= 0xD800) && (ch <= 0xDFFF)) {
                return 0;
            }
            ret[0] = char(0xE0 |  (ch >> 12));
            ret[1] = char(0x80 | ((ch >> 6) & 0x3F));
            ret[2] = char(0x80 |  (ch       & 0x3F));
            return 3;
        }
        if (ch <= utf::max_unicode) {
            ret[0] = char(0xF0 |  (ch >> 18));
            ret[1] = char(0x80 | ((ch >> 12) & 0x3F));
            ret[2] = char(0x80 | ((ch >>  6) & 0x3F));
            ret[3] = char(0x80 |  (ch        & 0x3F));
            return 4;
        }
        return 0;
    }

    OSTD_EXPORT std::size_t encode(
        char16_t (&ret)[2], char32_t ch
    ) noexcept {
        /* surrogate code point or out of bounds */
        if (((ch >= 0xD800) && (ch <= 0xDFFF)) || (ch > utf::max_unicode)) {
            return 0;
        }
        if (ch <= 0xFFFF) {
            ret[0] = char16_t(ch);
            return 1;
        }
        /* 20-bit number */
        ch -= 0x10000;
        ret[0] = char16_t(0xD800 + (ch >> 10));
        ret[1] = char16_t(0xDC00 + (ch & 0x3FF));
        return 2;
    }

    template<typename C>
    inline std::size_t length(
        basic_char_range<C const> &r, basic_char_range<C const> &cont
    ) noexcept {
        std::size_t ret = 0;
        for (char32_t ch; utf::decode(r, ch); ++ret) {
            continue;
        }
        cont = r;
        return ret;
    }

    template<typename C>
    inline std::size_t length(basic_char_range<C const> &r) noexcept {
        std::size_t ret = 0;
        if constexpr(utf::max_units<C> == 1) {
            ret = r.size();
        } else {
            for (;; ++ret) {
                if (char32_t ch; !utf::decode(r, ch)) {
                    if (r.empty()) {
                        break;
                    }
                    r.pop_front();
                }
            }
        }
        return ret;
    }
} /* namespace detail */

OSTD_EXPORT bool decode(string_range &r, char32_t &ret) noexcept {
    auto tn = r.size();
    auto *beg = reinterpret_cast<unsigned char const *>(r.data());
    if (std::size_t n; (n = detail::u8_decode(beg, beg + tn, ret))) {
        r = r.slice(n, tn);
        return true;
    }
    return false;
}

OSTD_EXPORT bool decode(u16string_range &r, char32_t &ret) noexcept {
    auto tn = r.size();
    auto *beg = r.data();
    if (std::size_t n; (n = detail::u16_decode(beg, beg + tn, ret))) {
        r = r.slice(n, tn);
        return true;
    }
    return false;
}

OSTD_EXPORT bool decode(u32string_range &r, char32_t &ret) noexcept {
    if (r.empty()) {
        return false;
    }
    auto c = r.front();
    if (detail::is_invalid_u32(c)) {
        return false;
    }
    ret = c;
    r.pop_front();
    return true;
}

OSTD_EXPORT bool decode(wstring_range &r, char32_t &ret) noexcept {
    std::size_t n, tn = r.size();
    if constexpr(is_wchar_u32) {
        if (!tn) {
            return false;
        }
        auto c = char32_t(r.front());
        if (detail::is_invalid_u32(c)) {
            return false;
        }
        ret = c;
        r.pop_front();
        return true;
    } else if constexpr(is_wchar_u16) {
        auto *beg = reinterpret_cast<char16_t const *>(r.data());
        n = detail::u16_decode(beg, beg + tn, ret);
    } else {
        auto *beg = reinterpret_cast<unsigned char const *>(r.data());
        n = detail::u8_decode(beg, beg + tn, ret);
    }
    if (n) {
        r = r.slice(n, tn);
        return true;
    }
    return false;
}

OSTD_EXPORT std::size_t length(string_range r, string_range &cont) noexcept {
    return detail::length(r, cont);
}

OSTD_EXPORT std::size_t length(u16string_range r, u16string_range &cont)
    noexcept
{
    return detail::length(r, cont);
}

OSTD_EXPORT std::size_t length(u32string_range r, u32string_range &cont)
    noexcept
{
    return detail::length(r, cont);
}

OSTD_EXPORT std::size_t length(wstring_range r, wstring_range &cont) noexcept {
    return detail::length(r, cont);
}

OSTD_EXPORT std::size_t length(string_range r) noexcept {
    return detail::length(r);
}

OSTD_EXPORT std::size_t length(u16string_range r) noexcept {
    return detail::length(r);
}

OSTD_EXPORT std::size_t length(u32string_range r) noexcept {
    return detail::length(r);
}

OSTD_EXPORT std::size_t length(wstring_range r) noexcept {
    return detail::length(r);
}

/* unicode-aware ctype
 * the other ones use custom tables for lookups
 */

OSTD_EXPORT bool isalnum(char32_t c) noexcept {
    return (utf::isalpha(c) || utf::isdigit(c));
}

OSTD_EXPORT bool isblank(char32_t c) noexcept {
    return ((c == ' ') || (c == '\t'));
}

OSTD_EXPORT bool isgraph(char32_t c) noexcept {
    return (!utf::isspace(c) && utf::isprint(c));
}

OSTD_EXPORT bool isprint(char32_t c) noexcept {
    switch (c) {
        case 0x2028:
        case 0x2029:
        case 0xFFF9:
        case 0xFFFA:
        case 0xFFFB:
            return false;
        default:
            return !utf::iscntrl(c);
    }
}

OSTD_EXPORT bool ispunct(char32_t c) noexcept {
    return (utf::isgraph(c) && !utf::isalnum(c));
}

OSTD_EXPORT bool isvalid(char32_t c) noexcept {
    /* surrogate code points */
    if ((c >= 0xD800) && (c <= 0xDFFF)) {
        return false;
    }
    /* non-characters */
    if ((c >= 0xFDD0) && (c <= 0xFDEF)) {
        return false;
    }
    /* end of plane */
    if ((c & 0xFFFE) == 0xFFFE) {
        return false;
    }
    /* must be within range */
    return (c <= utf::max_unicode);
}

OSTD_EXPORT bool isxdigit(char32_t c) noexcept {
    if ((c >= '0') && (c <= '9')) {
        return true;
    }
    auto uc = c | 32;
    return ((uc >= 'a') && (uc <= 'f'));
}

inline int codepoint_cmp1(void const *a, void const *b) noexcept {
    char32_t c1 = *static_cast<char32_t const *>(a);
    char32_t c2 = *static_cast<char32_t const *>(b);
    return (int(c1) - int(c2));
}

inline int codepoint_cmp2(void const *a, void const *b) noexcept {
    char32_t        c = *static_cast<char32_t const *>(a);
    char32_t const *p =  static_cast<char32_t const *>(b);
    if ((c >= p[0]) && (c <= p[1])) {
        return 0;
    }
    return (int(c) - int(p[0]));
}

template<
    std::size_t RN, std::size_t RS,
    std::size_t L1N, std::size_t L1S,
    std::size_t L2N, std::size_t L2S,
    std::size_t SN, std::size_t SS
>
struct uctype_func {
    template<std::size_t N, std::size_t S>
    static char32_t *search(
        char32_t c, void const *arr, int (*cmp)(void const *, void const *)
    ) noexcept {
        return static_cast<char32_t *>(std::bsearch(&c, arr, N / S, S, cmp));
    }

    static bool do_is(
        char32_t c,
        void const *ranges  [[maybe_unused]],
        void const *laces1  [[maybe_unused]],
        void const *laces2  [[maybe_unused]],
        void const *singles [[maybe_unused]]
    ) noexcept {
        if constexpr(RN != 0) {
            char32_t *found = search<RN, RS>(c, ranges, codepoint_cmp2);
            if (found) {
                return true;
            }
        }
        if constexpr(L1N != 0) {
            char32_t *found = search<L1N, L1S>(c, laces1, codepoint_cmp2);
            if (found) {
                return !((c - found[0]) % 2);
            }
        }
        if constexpr(L2N != 0) {
            char32_t *found = search<L2N, L2S>(c, laces2, codepoint_cmp2);
            if (found) {
                return !((c - found[0]) % 2);
            }
        }
        if constexpr(SN != 0) {
            char32_t *found = search<SN, SS>(c, singles, codepoint_cmp1);
            if (found) {
                return true;
            }
        }
        return false;
    }

    static char32_t do_to(
        char32_t c,
        void const *ranges  [[maybe_unused]],
        void const *laces1  [[maybe_unused]],
        void const *laces2  [[maybe_unused]],
        void const *singles [[maybe_unused]]
    ) noexcept {
        if constexpr(RN != 0) {
            char32_t *found = search<RN, RS>(c, ranges, codepoint_cmp2);
            if (found) {
                return (found[2] + (c - found[0]));
            }
        }
        if constexpr(L1N != 0) {
            char32_t *found = search<L1N, L1S>(c, laces1, codepoint_cmp2);
            if (found) {
                if ((c - found[0]) % 2) {
                    return c;
                }
                return c + 1;
            }
        }
        if constexpr(L2N != 0) {
            char32_t *found = search<L2N, L2S>(c, laces2, codepoint_cmp2);
            if (found) {
                if ((c - found[0]) % 2) {
                    return c;
                }
                return c - 1;
            }
        }
        if constexpr(SN != 0) {
            char32_t *found = search<SN, SS>(c, singles, codepoint_cmp1);
            if (found) {
                return found[1];
            }
        }
        return c;
    }
};

/* these are geneated */
OSTD_EXPORT bool isalpha(char32_t c) noexcept;
OSTD_EXPORT bool iscntrl(char32_t c) noexcept;
OSTD_EXPORT bool isdigit(char32_t c) noexcept;
OSTD_EXPORT bool islower(char32_t c) noexcept;
OSTD_EXPORT bool isspace(char32_t c) noexcept;
OSTD_EXPORT bool istitle(char32_t c) noexcept;
OSTD_EXPORT bool isupper(char32_t c) noexcept;
OSTD_EXPORT char32_t tolower(char32_t c) noexcept;
OSTD_EXPORT char32_t toupper(char32_t c) noexcept;

#if __has_include("string_utf.hh")
#include "string_utf.hh"
#else

/* break the cycle (build system and generator use libostd, but string_utf.hh
 * is generated during build) by providing a bunch of ASCII only fallbacks
 */

OSTD_EXPORT bool isalpha(char32_t c) noexcept {
    return (utf::isupper(c) || utf::islower(c));
}

OSTD_EXPORT bool iscntrl(char32_t c) noexcept {
    return ((c <= 0x1F) || (c == 0x7F));
}

OSTD_EXPORT bool isdigit(char32_t c) noexcept {
    return ((c >= '0') && (c <= '9'));
}

OSTD_EXPORT bool islower(char32_t c) noexcept {
    return ((c >= 'a') && (c <= 'z'));
}

OSTD_EXPORT bool isspace(char32_t c) noexcept {
    return ((c == ' ') || ((c >= 0x09) && (c <= 0x0D)));
}

OSTD_EXPORT bool istitle(char32_t) noexcept {
    return false;
}

OSTD_EXPORT bool isupper(char32_t c) noexcept {
    return ((c >= 'A') && (c <= 'Z'));
}

OSTD_EXPORT char32_t tolower(char32_t c) noexcept {
    if (utf::isupper(c)) {
        return c | 32;
    }
    return c;
}

OSTD_EXPORT char32_t toupper(char32_t c) noexcept {
    if (utf::islower(c)) {
        return c ^ 32;
    }
    return c;
}

#endif /* __has_include("string_utf.hh") */

namespace detail {
    template<typename C>
    inline int case_compare(
        C const *beg1, C const *end1,
        C const *beg2, C const *end2
    ) {
        auto s1l = std::size_t(end1 - beg1);
        auto s2l = std::size_t(end2 - beg2);

        auto ms = std::min(s1l, s2l);
        end1 = beg1 + ms;
        end2 = beg2 + ms;

        while (beg1 != end1) {
            auto ldec = char32_t(*beg1);
            auto rdec = char32_t(*beg2);
            if constexpr(std::is_same_v<C, char32_t>) {
                ++beg1;
                ++beg2;
            } else if constexpr(std::is_same_v<C, char16_t>) {
                std::size_t ndec;
                if ((ldec <= 0x7F) || !(ndec = u16_decode(beg1, end1, ldec))) {
                    ++beg1;
                } else {
                    beg1 += ndec;
                }
                if ((rdec <= 0x7F) || !(ndec = u16_decode(beg2, end2, rdec))) {
                    ++beg2;
                } else {
                    beg2 += ndec;
                }
            } else if constexpr(std::is_same_v<C, unsigned char>) {
                std::size_t ndec;
                if ((ldec <= 0x7F) || !(ndec = u8_decode(beg1, end1, ldec))) {
                    ++beg1;
                } else {
                    beg1 += ndec;
                }
                if ((rdec <= 0x7F) || !(ndec = u8_decode(beg2, end2, rdec))) {
                    ++beg2;
                } else {
                    beg2 += ndec;
                }
            }
            int d = int(utf::tolower(ldec)) - int(utf::tolower(rdec));
            if (d) {
                return d;
            }
        }
        return (s1l < s2l) ? -1 : ((s1l > s2l) ? 1 : 0);
    }
}

OSTD_EXPORT int case_compare(string_range s1, string_range s2) noexcept {
    auto *beg1 = reinterpret_cast<unsigned char const *>(s1.data());
    auto *beg2 = reinterpret_cast<unsigned char const *>(s2.data());
    return detail::case_compare(beg1, beg1 + s1.size(), beg2, beg2 + s2.size());
}

OSTD_EXPORT int case_compare(u16string_range s1, u16string_range s2) noexcept {
    auto *beg1 = s1.data(), *beg2 = s2.data();
    return detail::case_compare(beg1, beg1 + s1.size(), beg2, beg2 + s2.size());
}

OSTD_EXPORT int case_compare(u32string_range s1, u32string_range s2) noexcept {
    auto *beg1 = s1.data(), *beg2 = s2.data();
    return detail::case_compare(beg1, beg1 + s1.size(), beg2, beg2 + s2.size());
}

OSTD_EXPORT int case_compare(wstring_range s1, wstring_range s2) noexcept {
    using C = std::conditional_t<is_wchar_u8, unsigned char, wchar_fixed_t>;
    auto *beg1 = reinterpret_cast<C const *>(s1.data());
    auto *beg2 = reinterpret_cast<C const *>(s2.data());
    return detail::case_compare(beg1, beg1 + s1.size(), beg2, beg2 + s2.size());
}

} /* namespace utf */

/* place the vtable in here */
format_error::~format_error() {}

} /* namespace ostd */
