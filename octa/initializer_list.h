/* Initializer list support for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_INITIALIZER_LIST_H
#define OCTA_INITIALIZER_LIST_H

#include <stddef.h>

#include "octa/range.h"

/* must be in std namespace otherwise the compiler won't know about it */
namespace std {
    template<typename T>
    class initializer_list {
        const T *p_buf;
        size_t p_len;

        initializer_list(const T *v, size_t n) noexcept: p_buf(v), p_len(n) {}
    public:
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef       T   value_type;
        typedef       T  &reference;
        typedef const T  &const_reference;
        typedef       T  *pointer;
        typedef const T  *const_pointer;

        initializer_list() noexcept: p_buf(nullptr), p_len(0) {}

        size_t length() const noexcept { return p_len; }

        const T *get() const noexcept { return p_buf; }

        octa::PointerRange<const T> range() noexcept {
            return octa::PointerRange<const T>(p_buf, p_len);
        }
    };
}

namespace octa {
    template<typename T> using InitializerList = std::initializer_list<T>;
}

#endif