/* Initializer list support for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_INITIALIZER_LIST_HH
#define OSTD_INITIALIZER_LIST_HH

#include <initializer_list>

#include "ostd/range.hh"

namespace ostd {

template<typename T>
IteratorRange<T const *> iter(std::initializer_list<T> init) noexcept {
    return IteratorRange<T const *>(init.begin(), init.end());
}

template<typename T>
IteratorRange<T const *> citer(std::initializer_list<T> init) noexcept {
    return IteratorRange<T const *>(init.begin(), init.end());
}

}

#endif
