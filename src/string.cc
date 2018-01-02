/* String implementation details, mainly regarding Unicode support.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdint>
#include <cstdlib>
#include "ostd/string.hh"

namespace ostd {
namespace utf {

constexpr std::uint32_t MaxCodepoint = 0x10FFFF;

namespace detail {
    static inline bool u8_decode(string_range &r, char32_t &cret) noexcept {
        static std::uint32_t const ulim[] = { 0xFF, 0x7F, 0x7FF, 0xFFFF };
        if (r.empty()) {
            return false;
        }
        std::uint32_t ch = static_cast<unsigned char const>(r.front());
        if (ch <= 0x7F) {
            /* ASCII */
            cret = ch;
            r.pop_front();
            return true;
        }
        std::uint32_t ret = 0;
        string_range sr = r;
        sr.pop_front();
        /* continuation bytes */
        for (; ch & 0x40; ch <<= 1) {
            /* need a continuation byte but nothing left in the string */
            if (sr.empty()) {
                return false;
            }
            /* the continuation byte */
            std::uint32_t nch = static_cast<unsigned char const>(sr.front());
            sr.pop_front();
            /* lower 6 bits */
            std::uint32_t bch = nch & 0x3F;
            /* not a continuation byte */
            if ((nch ^ bch) != 0x80) {
                return false;
            }
            /* the 6 bits go in the result */
            ret = (ret << 6) | bch;
        }
        /* number of continuation bytes */
        std::size_t n = sr.data() - r.data() - 1;
        /* invalid sequence - too many continuation bits */
        if (n > 3) {
            return false;
        }
        /* add the up to 7 bits from the first byte, already shifted left by n */
        ret |= (ch & 0x7F) << (n * 5);
        /* invalid sequence - out of bounds */
        if ((ret > MaxCodepoint) || (ret <= ulim[n])) {
            return false;
        }
        /* invalid sequence - surrogate code point */
        if ((ret >= 0xD800) && (ret <= 0xDFFF)) {
            return false;
        }
        cret = ret;
        r = sr;
        return true;
    }

    std::uint8_t u8_encode(
        std::uint8_t (&ret)[4], std::uint32_t ch
    ) noexcept {
        if (ch <= 0x7F) {
            ret[0] = ch;
            return 1;
        }
        if (ch <= 0x7FF) {
            ret[0] = 0xC0 | (ch >> 6);
            ret[1] = 0x80 | (ch & 0x3F);
            return 2;
        }
        if (ch <= 0xFFFF) {
            /* TODO: optional WTF-8 semantics
             * for now simply reject surrogate code points
             */
            if ((ch >= 0xD800) && (ch <= 0xDFFF)) {
                return 0;
            }
            ret[0] = 0xE0 |  (ch >> 12);
            ret[1] = 0x80 | ((ch >> 6) & 0x3F);
            ret[2] = 0x80 |  (ch       & 0x3F);
            return 3;
        }
        if (ch <= MaxCodepoint) {
            ret[0] = 0xF0 |  (ch >> 18);
            ret[1] = 0x80 | ((ch >> 12) | 0x3F);
            ret[2] = 0x80 | ((ch >>  6) | 0x3F);
            ret[3] = 0x80 |  (ch        | 0x3F);
            return 4;
        }
        return 0;
    }
} /* namespace detail */

bool decode(string_range &r, char32_t &ret) noexcept {
    return detail::u8_decode(r, ret);
}

std::size_t length(string_range r, string_range &cont) noexcept {
    std::size_t ret = 0;
    for (char32_t ch = U'\0'; detail::u8_decode(r, ch); ++ret) {
        continue;
    }
    cont = r;
    return ret;
}

/* unicode-aware ctype
 * the other ones use custom tables for lookups
 */

bool isalnum(char32_t c) {
    return (utf::isalpha(c) || utf::isdigit(c));
}

bool isblank(char32_t c) {
    return ((c == ' ') || (c == '\t'));
}

bool isgraph(char32_t c) {
    return (!utf::isspace(c) && utf::isprint(c));
}

bool isprint(char32_t c) {
    switch (std::uint32_t(c)) {
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

bool ispunct(char32_t c) {
    return (utf::isgraph(c) && !utf::isalnum(c));
}

bool isvalid(char32_t c) {
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

bool isxdigit(char32_t c) {
    if ((c >= '0') && (c <= '9')) {
        return true;
    }
    auto uc = std::uint32_t(c) | 32;
    return ((uc >= 'a') && (uc <= 'f'));
}

inline int codepoint_cmp1(void const *a, void const *b) {
    char32_t c1 = *static_cast<char32_t const *>(a);
    char32_t c2 = *static_cast<char32_t const *>(b);
    return (c1 - c2);
}

inline int codepoint_cmp2(void const *a, void const *b) {
    char32_t        c = *static_cast<char32_t const *>(a);
    char32_t const *p =  static_cast<char32_t const *>(b);
    if ((c >= p[0]) && (c <= p[1])) {
        return 0;
    }
    return (c - p[0]);
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
    ) {
        return static_cast<char32_t *>(std::bsearch(&c, arr, N / S, S, cmp));
    }

    static bool do_is(
        char32_t c,
        void const *ranges  [[maybe_unused]],
        void const *laces1  [[maybe_unused]],
        void const *laces2  [[maybe_unused]],
        void const *singles [[maybe_unused]]
    ) {
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
    ) {
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
bool isalpha(char32_t c);
bool iscntrl(char32_t c);
bool isdigit(char32_t c);
bool islower(char32_t c);
bool isspace(char32_t c);
bool istitle(char32_t c);
bool isupper(char32_t c);
char32_t tolower(char32_t c);
char32_t toupper(char32_t c);

#if __has_include("string_utf.hh")
#include "string_utf.hh"
#else

/* break the cycle (build system and generator use libostd, but string_utf.hh
 * is generated during build) by providing a bunch of ASCII only fallbacks
 */

bool isalpha(char32_t c) {
    return (utf::isupper(c) || utf::islower(c));
}

bool iscntrl(char32_t c) {
    return ((c <= 0x1F) || (c == 0x7F));
}

bool isdigit(char32_t c) {
    return ((c >= '0') && (c <= '9'));
}

bool islower(char32_t c) {
    return ((c >= 'a') && (c <= 'z'));
}

bool isspace(char32_t c) {
    return ((c == ' ') || ((c >= 0x09) && (c <= 0x0D)));
}

bool istitle(char32_t) {
    return false;
}

bool isupper(char32_t c) {
    return ((c >= 'A') && (c <= 'Z'));
}

char32_t tolower(char32_t c) {
    if (utf::isupper(c)) {
        return c | 32;
    }
    return c;
}

char32_t toupper(char32_t c) {
    if (utf::islower(c)) {
        return c ^ 32;
    }
    return c;
}

#endif /* __has_include("string_utf.hh") */

} /* namespace utf */
} /* namespace ostd */
