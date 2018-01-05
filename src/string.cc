/* String implementation details, mainly regarding Unicode support.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdint>
#include <cstdlib>
#include "ostd/string.hh"

namespace ostd {
namespace utf {

constexpr char32_t MaxCodepoint = 0x10FFFF;

namespace detail {
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
                return false;
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
        /* invalid sequence - out of bounds */
        if ((ret > MaxCodepoint) || (ret <= ulim[adv - 1])) {
            return 0;
        }
        /* invalid sequence - surrogate code point */
        if ((ret >= 0xD800) && (ret <= 0xDFFF)) {
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

    std::size_t u8_encode(
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
        if (ch <= MaxCodepoint) {
            ret[0] = char(0xF0 |  (ch >> 18));
            ret[1] = char(0x80 | ((ch >> 12) | 0x3F));
            ret[2] = char(0x80 | ((ch >>  6) | 0x3F));
            ret[3] = char(0x80 |  (ch        | 0x3F));
            return 4;
        }
        return 0;
    }

    std::size_t u16_encode(
        char16_t (&ret)[2], char32_t ch
    ) noexcept {
        /* surrogate code point or out of bounds */
        if (((ch >= 0xD800) && (ch <= 0xDFFF)) || (ch > MaxCodepoint)) {
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
} /* namespace detail */

bool decode(string_range &r, char32_t &ret) noexcept {
    auto tn = r.size();
    auto *beg = reinterpret_cast<unsigned char const *>(r.data());
    if (std::size_t n; (n = detail::u8_decode(beg, beg + tn, ret))) {
        r = r.slice(n, tn);
        return true;
    }
    return false;
}

bool decode(u16string_range &r, char32_t &ret) noexcept {
    auto tn = r.size();
    auto *beg = r.data();
    if (std::size_t n; (n = detail::u16_decode(beg, beg + tn, ret))) {
        r = r.slice(n, tn);
        return true;
    }
    return false;
}

bool decode(wstring_range &r, char32_t &ret) noexcept {
    std::size_t n, tn = r.size();
    if constexpr(sizeof(wchar_t) == sizeof(char32_t)) {
        if (!tn) {
            return false;
        }
        ret = char32_t(r.front());
        return true;
    } else if constexpr(sizeof(wchar_t) == sizeof(char16_t)) {
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

std::size_t length(string_range r, string_range &cont) noexcept {
    std::size_t ret = 0;
    for (char32_t ch = U'\0'; utf::decode(r, ch); ++ret) {
        continue;
    }
    cont = r;
    return ret;
}

/* unicode-aware ctype
 * the other ones use custom tables for lookups
 */

bool isalnum(char32_t c) noexcept {
    return (utf::isalpha(c) || utf::isdigit(c));
}

bool isblank(char32_t c) noexcept {
    return ((c == ' ') || (c == '\t'));
}

bool isgraph(char32_t c) noexcept {
    return (!utf::isspace(c) && utf::isprint(c));
}

bool isprint(char32_t c) noexcept {
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

bool ispunct(char32_t c) noexcept {
    return (utf::isgraph(c) && !utf::isalnum(c));
}

bool isvalid(char32_t c) noexcept {
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
    return (c <= MaxCodepoint);
}

bool isxdigit(char32_t c) noexcept {
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
bool isalpha(char32_t c) noexcept;
bool iscntrl(char32_t c) noexcept;
bool isdigit(char32_t c) noexcept;
bool islower(char32_t c) noexcept;
bool isspace(char32_t c) noexcept;
bool istitle(char32_t c) noexcept;
bool isupper(char32_t c) noexcept;
char32_t tolower(char32_t c) noexcept;
char32_t toupper(char32_t c) noexcept;

#if __has_include("string_utf.hh")
#include "string_utf.hh"
#else

/* break the cycle (build system and generator use libostd, but string_utf.hh
 * is generated during build) by providing a bunch of ASCII only fallbacks
 */

bool isalpha(char32_t c) noexcept {
    return (utf::isupper(c) || utf::islower(c));
}

bool iscntrl(char32_t c) noexcept {
    return ((c <= 0x1F) || (c == 0x7F));
}

bool isdigit(char32_t c) noexcept {
    return ((c >= '0') && (c <= '9'));
}

bool islower(char32_t c) noexcept {
    return ((c >= 'a') && (c <= 'z'));
}

bool isspace(char32_t c) noexcept {
    return ((c == ' ') || ((c >= 0x09) && (c <= 0x0D)));
}

bool istitle(char32_t) noexcept {
    return false;
}

bool isupper(char32_t c) noexcept {
    return ((c >= 'A') && (c <= 'Z'));
}

char32_t tolower(char32_t c) noexcept {
    if (utf::isupper(c)) {
        return c | 32;
    }
    return c;
}

char32_t toupper(char32_t c) noexcept {
    if (utf::islower(c)) {
        return c ^ 32;
    }
    return c;
}

#endif /* __has_include("string_utf.hh") */

int case_compare(string_range s1, string_range s2) noexcept {
    std::size_t s1l = s1.size(), s2l = s2.size(), ms = std::min(s1l, s2l);
    s1 = s1.slice(0, ms);
    s2 = s2.slice(0, ms);
    for (;;) {
        /* enforce correct semantics with signed chars; first convert to
         * 8-bit unsigned and then to char32_t (which is always unsigned)
         * in order to not get large values from 32-bit unsigned underflow
         */
        auto ldec = char32_t(static_cast<unsigned char>(s1.front()));
        auto rdec = char32_t(static_cast<unsigned char>(s2.front()));
        if ((ldec <= 0x7F) || !utf::decode(s1, ldec)) {
            s1.pop_front();
        }
        if ((rdec <= 0x7F) || !utf::decode(s2, rdec)) {
            s2.pop_front();
        }
        int d = int(utf::tolower(ldec)) - int(utf::tolower(rdec));
        if (d) {
            return d;
        }
        if (s1.empty() || s2.empty()) {
            s1l = s1.size();
            s2l = s2.size();
            break;
        }
    }
    return (s1l < s2l) ? -1 : ((s1l > s2l) ? 1 : 0);
}

int case_compare(u32string_range s1, u32string_range s2) noexcept {
    std::size_t s1l = s1.size(), s2l = s2.size();
    for (std::size_t i = 0, ms = std::min(s1l, s2l); i < ms; ++i) {
        int d = int(utf::tolower(s1[i])) - int(utf::tolower(s2[i]));
        if (d) {
            return d;
        }
    }
    return (s1l < s2l) ? -1 : ((s1l > s2l) ? 1 : 0);
}

} /* namespace utf */
} /* namespace ostd */
