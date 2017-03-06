#include <ostd/io.hh>
#include <ostd/coroutine.hh>

using namespace ostd;

int main() {
    generator<int> g = [](auto &coro) {
        coro.yield(5);
        coro.yield(10);
        coro.yield(15);
        coro.yield(20);
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
