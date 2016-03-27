#include <ostd/range.hh>
#include <ostd/io.hh>
#include <ostd/algorithm.hh>

using namespace ostd;

int main() {
    /* range iter - prints 0 to 9 each on new line */
    writeln("range iter test");
    for (int i: range(10))
        writeln(i);

    /* algorithm: map - prints 0.5 to 9.5 each on new line */
    writeln("range map test");
    for (float f: map(range(10), [](int v) { return v + 0.5f; }))
        writeln(f);

    /* algorithm: filter - prints 10, 15, 8 each on new line */
    writeln("range filter test");
    auto il = { 5, 5, 5, 5, 5, 10, 15, 4, 8, 2 };
    for (int i: filter(iter(il), [](int v) { return v > 5; }))
        writeln(i);

    /* prints ABCDEF (ASCII 65, 66, 67, 68, 69, 70) */
    String s(map(range(6), [](int v) -> char { return v + 65; }));
    writeln(s);

    /* join a few ranges together - prints 11, 22, 33 ... 99 each on new line */
    auto x = { 11, 22, 33 };
    auto y = { 44, 55, 66 };
    auto z = { 77, 88, 99 };
    for (auto i: join(iter(x), iter(y), iter(z))) {
        writeln(i);
    }

    /* chunk a range into subranges - prints
     * {11, 22, 33}
     * {44, 55, 66}
     * {77, 88, 99}
     */
    auto cr = { 11, 22, 33, 44, 55, 66, 77, 88, 99 };
    for (auto r: chunks(iter(cr), 3))
        writeln(r);
}
