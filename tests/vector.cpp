#include <assert.h>
#include "octa/vector.h"

using namespace octa;

int main() {
    Vector<int> x = { 5, 10, 15, 20 };

    assert(x.first() == 5);
    assert(x.last() == 20);

    assert(x[0] == 5);
    assert(x[2] == 15);

    assert(x.size() == 4);

    Vector<int> y(5, 10);

    assert(y.size() == 5);
    assert(y.first() == 10);
    assert(y.last() == 10);

    return 0;
}