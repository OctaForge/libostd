#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

int main() {
    /* have an array, split it in two halves and sum each half in a separate
     * task, which may or may not run in parallel with the other one depending
     * on the scheduler currently in use - several schedulers are shown
     */

    /* this version uses Go-style channels to exchange data; multiple
     * tasks can put data into channels, the channel itself is a thread
     * safe queue; it goes the other way around too, multiple tasks can
     * wait on a channel for some data to be received
     */
    auto foo = []() {
        auto arr = { 150, 38, 76, 25, 67, 18, -15,  215, 25, -10 };

        auto c = make_channel<int>();
        auto f = [](auto c, auto half) {
            c.put(foldl(half, 0));
        };
        spawn(f, c, iter(arr).slice(0, arr.size() / 2));
        spawn(f, c, iter(arr).slice(arr.size() / 2));

        int a = c.get();
        int b = c.get();
        writefln("    %s + %s = %s", a, b, a + b);
    };

    /* this version uses future-style tid objects that represent the tasks
     * themselves including the return value of the task; the return value
     * can be retrieved using .get(), which will wait for the value to be
     * stored unless it already is, becoming invalid after retrieval of
     * the value; the tid also intercepts any possible exceptions thrown
     * by the task and propagates them in .get(), allowing the user to
     * perform safe exception handling across tasks
     */
    auto bar = []() {
        auto arr = { 150, 38, 76, 25, 67, 18, -15,  215, 25, -10 };

        auto f = [](auto half) {
            return foldl(half, 0);
        };
        auto t1 = spawn(f, iter(arr).slice(0, arr.size() / 2));
        auto t2 = spawn(f, iter(arr).slice(arr.size() / 2));

        int a = t1.get();
        int b = t2.get();
        writefln("%s + %s = %s", a, b, a + b);
    };

    /* tries both examples above */
    auto baz = [&foo, &bar]() {
        writeln("  testing channels...");
        foo();
        writeln("  testing futures...");
        foo();
    };

    /* using thread_scheduler results in an OS thread spawned per task,
     * implementing a 1:1 (kernel-level) scheduling - very expensive on
     * Windows, less expensive on Unix-likes (but more than coroutines)
     */
    thread_scheduler{}.start([&baz]() {
        writeln("(1) 1:1 scheduler: starting...");
        baz();
        writeln("(1) 1:1 scheduler: finishing...");
    });
    writeln();

    /* using simple_coroutine_scheduler results in a coroutine spawned
     * per task, implementing N:1 (user-level) scheduling - very cheap
     * and portable everywhere but obviously limited to only one thread
     */
    simple_coroutine_scheduler{}.start([&baz]() {
        writeln("(2) N:1 scheduler: starting...");
        baz();
        writeln("(2) N:1 scheduler: finishing...");
    });
    writeln();

    /* using coroutine_scheduler results in a coroutine spawned per
     * task, but mapped onto a certain number of OS threads, implementing
     * a hybrid M:N approach - this benefits from multicore systems and
     * also is relatively cheap (you can create a big number of tasks)
     */
    coroutine_scheduler{}.start([&baz]() {
        writeln("(3) M:N scheduler: starting...");
        baz();
        writeln("(3) M:N scheduler: finishing...");
    });
}

/*
(1) 1:1 scheduler: starting...
  testing channels...
    356 + 233 = 589
  testing futures...
    356 + 233 = 589
(1) 1:1 scheduler: finishing...

(2) N:1 scheduler: starting...
  testing channels...
    356 + 233 = 589
  testing futures...
    356 + 233 = 589
(2) N:1 scheduler: finishing...

(3) M:N scheduler: starting...
  testing channels...
    356 + 233 = 589
  testing futures...
    356 + 233 = 589
(3) M:N scheduler: finishing...
*/
