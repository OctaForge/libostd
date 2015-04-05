/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.txt for futher information.
 */

#ifndef OCTASTD_VECTOR_H
#define OCTASTD_VECTOR_H

#include <octastd/new.h>

namespace octastd {
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