#include <assert.h>
#include <string.h>
#include "octa/array.h"
#include "octa/string.h"

using namespace octa;

int main() {
    Array<int, 5> x = { 2, 4, 8, 16, 32 };

    assert(x.first() == 2);
    assert(x.last() == 32);

    assert(x[0] == 2);
    assert(x[2] == 8);

    assert(x.at(0) == x[0]);
    assert(x.at(3) == x[3]);

    assert(x.size() == 5);

    assert(!x.empty());

    assert(x.in_range(4));
    assert(x.in_range(0));
    assert(!x.in_range(5));

    assert(x.data()[0] == x[0]);

    auto r = x.each();
    assert(r.first() == 2);
    assert(r.last() == 32);

    Array<int, 5> z;
    x.swap(z);

    assert(z.first() == 2);
    assert(z.last() == 32);

    assert(z.size() == 5);

    assert(!strcmp(to_string(z).data(), "{2, 4, 8, 16, 32}"));

    return 0;
}