#include <ostd/io.hh>
#include <ostd/coroutine.hh>

using namespace ostd;

int main() {
    auto f = [](auto yield) {
        for (int i: range(5, 26, 5)) {
            yield(i);
        }
    };

    writeln("generator test");
    for (int i: generator<int>{f}) {
        writefln("generated: %s", i);
    }
}

/*
generator test
generated: 5
generated: 10
generated: 15
generated: 20
generated: 25
*/
