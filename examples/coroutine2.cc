#include <ostd/io.hh>
#include <ostd/coroutine.hh>

using namespace ostd;

int main() {
    generator<int> g = [](auto yield) {
        yield(5);
        yield(10);
        yield(15);
        yield(20);
        return 25;
    };

    writeln("generator test");
    for (int i: g) {
        writeln("generated: ", i);
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
