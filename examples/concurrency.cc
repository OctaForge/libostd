#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

int main() {
    auto foo = [](auto &sched) {
        auto arr = ostd::iter({ 150, 38, 76, 25, 67, 18, -15,  215, 25, -10 });

        auto c = sched.template make_channel<int>();
        auto f = [](auto c, auto half) {
            c.put(foldl(half, 0));
        };
        sched.spawn(f, c, arr.slice(0, arr.size() / 2));
        sched.spawn(f, c, arr.slice(arr.size() / 2, arr.size()));

        int a, b;
        c.get(a), c.get(b);
        writefln("%s + %s = %s", a, b, a + b);
    };

    thread_scheduler tsched;
    tsched.start([&tsched, &foo]() {
        writeln("thread scheduler: starting...");
        foo(tsched);
        writeln("thread scheduler: finishing...");
    });

    simple_coroutine_scheduler scsched;
    scsched.start([&scsched, &foo]() {
        writeln("simple coroutine scheduler: starting...");
        foo(scsched);
        writeln("simple coroutine scheduler: finishing...");
    });
}

/*
thread scheduler: starting...
356 + 233 = 589
thread scheduler: finishing...
simple coroutine scheduler: starting...
356 + 233 = 589
simple coroutine scheduler: finishing...
*/
