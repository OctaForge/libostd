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
    writeln("string gen test");
    String s(map(range(6), [](int v) -> char { return v + 65; }));
    writeln(s);

    /* join a few ranges together - prints 11, 22, 33 ... 99 each on new line */
    writeln("range join test");
    auto x = { 11, 22, 33 };
    auto y = { 44, 55, 66 };
    auto z = { 77, 88, 99 };
    for (auto i: iter(x).join(iter(y), iter(z)))
        writeln(i);

    /* chunk a range into subranges - prints
     * {11, 22, 33}
     * {44, 55, 66}
     * {77, 88, 99}
     */
    writeln("range chunk test");
    auto cr = { 11, 22, 33, 44, 55, 66, 77, 88, 99 };
    for (auto r: iter(cr).chunks(3))
        writeln(r);

    /* take test, prints only first 4 */
    writeln("range take test");
    for (auto r: iter(cr).take(4))
        writeln(r);

    /* {11, 44, 77}, {22, 55, 88}, {33, 66, 99} */
    writeln("range zip test");
    for (auto v: iter(x).zip(iter(y), iter(z)))
        writeln(v);

    /* 2-tuple zip uses Pair */
    writeln("2-tuple range zip");
    for (auto v: iter({ 5, 10, 15, 20 }).zip(iter({ 6, 11, 16, 21 })))
        writeln(v.first, ", ", v.second);
}
