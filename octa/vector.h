/* Self-expanding dynamic array implementation for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_VECTOR_H
#define OCTA_VECTOR_H

#include "octa/new.h"
#include "octa/traits.h"

namespace octa {
    template<typename T>
    class Vector {
        T *buf;
        size_t length, capacity;

    public:
        explicit vector(): buf(NULL), length(0), capacity(0) {}

        vector(const vector &v): buf(NULL), length(0), capacity(0) {
            *this = v;
        }

        ~vector() {
            resize(0);
            if (buf) delete[] (unsigned char *)buf;
        }

        vector<T> &operator=(const vector<T> &v) {
            resize(0);
            if (v.length() > capacity) resize(v.length());
            for (size_t i = 0; i < v.length(); ++i) push(v[i]);
            return *this;
        }

        T &push(const T &v) {
            if (length == capacity) resize(length + 1);
            new (&buf[length]) T(v);
            return buf[length++];
        }

        T &push() {
            if (length == capacity) resize(length + 1);
            new (&buf[length]) T;
            return buf[length++];
        }

        void resize(size_t n) {
            if (n <= length) {
                if (n == length) return;
                while (length > n) pop();
                return;
            }
            int old = capacity;
            if (!old)
                capacity = max(n, 8);
            else
                while (capacity < n) capacity += (capacity / 2);
            if (capacity <= old)
                return;
            unsigned char *nbuf = new unsigned char[capacity * sizeof(T)];
            if (old > 0) {
                if (length > 0) memcpy(nbuf, (void *)buf, length * sizeof(T));
                delete[] (unsigned char *)buf;
            }
            buf = (T *)buf;
        }
    };
}

#endif