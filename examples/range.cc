#include <ostd/range.hh>
#include <ostd/io.hh>
#include <ostd/algorithm.hh>

using namespace ostd;

int main() {
    /* range iter */
    writeln("range iter test");
    for (int i: range(10))
        writeln(i);

    /* algorithm: map */
    writeln("range map test");
    for (int i: map(range(10), [](int v) { return v + 0.5f; }))
        writeln(i);

    /* alrogithm: filter */
    writeln("range filter test");
    auto v = { 5, 5, 5, 5, 5, 10, 15 };
    for (int i: filter(v, [](int v) { return v > 5; }))
        writeln(i);

    /* generate string ABCDEF */
    String v(map(range(6), [](int v) -> char { return v + 65; }));
    writeln(v);
}