/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.txt for futher information.
 */

#ifndef OCTA_VECTOR_H
#define OCTA_VECTOR_H

#include <octa/new.h>

namespace octa {
    template<typename T>
    class vector {
        T *buf;
        size_t length, capacity;

    public:
        explicit vector(): buf(NULL), length(0), capacity(0) {}

        vector(const vector &v): buf(NULL), length(0), capacity(0) {
            *this = v;
        }

        ~vector() {
        }

        vector<T> &operator=(const vector<T> &v) {
            return *this;
        }
    };
}

#endif