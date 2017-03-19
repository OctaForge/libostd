#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

template<typename S>
void foo(S &sched) {
    auto arr = ostd::iter({ 150, 38, 76, 25, 67, 18, -15, -38, 25, -10 });

    auto h1 = arr.slice(0, arr.size() / 2);
    auto h2 = arr.slice(arr.size() / 2, arr.size());

    auto c = sched.template make_channel<int>();
    auto f = [c](auto half) {
        c->put(foldl(half, 0));
    };
    sched.spawn(f, h1);
    sched.spawn(f, h2);

    int a, b;
    c->get(a);
    c->get(b);
    writefln("first half: %s", a);
    writefln("second half: %s", b);
    writefln("total: %s", a + b);
}

int main() {
    thread_scheduler tsched;
    tsched.start([&tsched]() {
        writeln("thread scheduler: starting...");
        foo(tsched);
        writeln("thread scheduler: finishing...");
    });

    simple_coroutine_scheduler scsched;
    scsched.start([&scsched]() {
        writeln("simple coroutine scheduler: starting...");
        foo(scsched);
        writeln("simple coroutine scheduler: finishing...");
    });
}

/*
thread scheduler: starting...
first half: 356
second half: -20
total: 336
thread scheduler: finishing...
simple coroutine scheduler: starting...
first half: 356
second half: -20
total: 336
simple coroutine scheduler: finishing...
*/
