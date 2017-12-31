/* String implementation details, mainly regarding Unicode support.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstdint>
#include "ostd/string.hh"

namespace ostd {
namespace utf {

constexpr std::uint32_t MaxCodepoint = 0x10FFFF;

static inline bool codepoint_dec(string_range &r, char32_t &cret) {
    static const std::uint32_t ulim[] = { 0xFF, 0x7F, 0x7FF, 0xFFFF };
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
    std::size_t n = sr.data() - r.data();
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
    cret = ret;
    r = sr;
    return true;
}

bool codepoint(string_range &r, char32_t &ret) {
    return codepoint_dec(r, ret);
}

std::size_t length(string_range r, string_range &cont) {
    std::size_t ret = 0;
    for (char32_t ch = U'\0'; codepoint_dec(r, ch); ++ret) {
        continue;
    }
    cont = r;
    return ret;
}

std::size_t length(string_range r) {
    return length(r, r);
}

} /* namespace utf */
} /* namespace ostd */
