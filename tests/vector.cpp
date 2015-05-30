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

    Vector<int> z(x);

    assert(x.first() == z.first());
    assert(x.last() == z.last());

    z.clear();

    assert(z.size() == 0);
    assert(z.capacity() != 0);
    assert(z.empty());

    z = move(y);

    assert(z.size() == 5);
    assert(y.size() == 0);
    assert(z.first() == 10);
    assert(z.last() == 10);

    z.resize(150, 5);
    assert(z.size() == 150);
    assert(z.first() == 10);
    assert(z.last() == 5);

    assert(z.push(30) == 30);
    assert(z.last() == 30);

    assert(z.emplace_back(20) == 20);
    assert(z.last() == 20);

    z.clear();
    z.resize(10, 5);

    assert(z.in_range(9));
    assert(z.in_range(0));
    assert(!z.in_range(10));

    z.insert(2, 4);
    assert(z[2] == 4);
    assert(z[0] == 5);
    assert(z[3] == 5);
    assert(z.size() == 11);

    auto r = z.each();
    assert(r.first() == 5);
    assert(r.last() == 5);
    assert(r[2] == 4);

    auto r2 = each(z);
    assert(r.first() == r2.first());

    Vector<int> w;
    w.swap(z);

    assert(z.size() == 0);
    assert(w.size() != 0);
    assert(w.first() == 5);
    assert(w.last() == 5);

    return 0;
}