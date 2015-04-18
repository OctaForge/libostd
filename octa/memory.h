/* Memory utilities for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_MEMORY_H
#define OCTA_MEMORY_H

namespace octa {
    template<typename T> T *address_of(T &v) {
        return reinterpret_cast<T *>(&const_cast<char &>
            (reinterpret_cast<const volatile char &>(v)));
    }
}

#endif