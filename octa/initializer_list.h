/* Initializer list support for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_INITIALIZER_LIST_H
#define OCTA_INITIALIZER_LIST_H

#include <stddef.h>

#include "octa/range.h"

#ifndef OCTA_ALLOW_CXXSTD
/* must be in std namespace otherwise the compiler won't know about it */
namespace std {

template<typename _T>
class initializer_list {
    const _T *p_buf;
    size_t p_len;

    initializer_list(const _T *v, size_t n): p_buf(v), p_len(n) {}
public:
    initializer_list(): p_buf(nullptr), p_len(0) {}

    size_t size() const { return p_len; }

    const _T *begin() const { return p_buf; }
    const _T *end() const { return p_buf + p_len; }
};

}
#else
#include <initializer_list>
#endif

namespace octa {

template<typename _T> using InitializerList = std::initializer_list<_T>;

template<typename _T>
octa::PointerRange<const _T> each(std::initializer_list<_T> init) {
    return octa::PointerRange<const _T>(init.begin(), init.end());
}

}

#endif