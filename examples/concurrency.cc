#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

thread_scheduler sched;

void foo() {
    auto arr = ostd::iter({ 150, 38, 76, 25, 67, 18, -15, -38, 25, -10 });

    auto h1 = arr.slice(0, arr.size() / 2);
    auto h2 = arr.slice(arr.size() / 2, arr.size());

    auto c = sched.make_channel<int>();
    auto f = [&c](auto half) {
        int ret = 0;
        for (int i: half) {
            ret += i;
        }
        c.put(ret);
    };
    sched.spawn(f, h1);
    sched.spawn(f, h2);

    int a, b;
    c.get(a);
    c.get(b);
    writefln("first half: %s", a);
    writefln("second half: %s", b);
    writefln("total: %s", a + b);
}

int main() {
    sched.start([]() {
        writeln("starting...");
        foo();
        writeln("finishing...");
    });
}

/*
starting...
first half: 356
second half: -20
total: 336
finishing...
*/
