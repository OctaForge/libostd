#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

int main() {
    auto foo = [](auto &sched) {
        auto arr = ostd::iter({ 150, 38, 76, 25, 67, 18, -15,  215, 25, -10 });

        auto c = make_channel<int>(sched);
        auto f = [](auto c, auto half) {
            c.put(foldl(half, 0));
        };
        spawn(sched, f, c, arr.slice(0, arr.size() / 2));
        spawn(sched, f, c, arr + (arr.size() / 2));

        int a = c.get(), b = c.get();
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
