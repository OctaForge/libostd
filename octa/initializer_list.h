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
        const _T *__buf;
        size_t __len;

        initializer_list(const _T *__v, size_t __n): __buf(__v), __len(__n) {}
    public:
        initializer_list(): __buf(nullptr), __len(0) {}

        size_t size() const { return __len; }

        const _T *begin() const { return __buf; }
        const _T *end() const { return __buf + __len; }
    };
}
#else
#include <initializer_list>
#endif

namespace octa {
    template<typename _T> using InitializerList = std::initializer_list<_T>;

    template<typename _T>
    octa::PointerRange<const _T> each(std::initializer_list<_T> __init) {
        return octa::PointerRange<const _T>(__init.begin(), __init.end());
    }
}

#endif