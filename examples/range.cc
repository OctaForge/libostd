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
    for (float f: map(range(10), [](int v) { return v + 0.5f; }))
        writeln(f);

    /* alrogithm: filter */
    writeln("range filter test");
    auto il = { 5, 5, 5, 5, 5, 10, 15, 4, 8, 2 };
    for (int i: filter(iter(il), [](int v) { return v > 5; }))
        writeln(i);

    /* generate string ABCDEF */
    String s(map(range(6), [](int v) -> char { return v + 65; }));
    writeln(s);
}
