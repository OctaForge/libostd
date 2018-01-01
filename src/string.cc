/* String implementation details, mainly regarding Unicode support.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdint>
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
        if ((ret & 0xD800) == 0xD800) {
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
            if ((ch & 0xD800) == 0xD800) {
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

} /* namespace utf */
} /* namespace ostd */
