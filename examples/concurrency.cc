#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

int main() {
    /* have an array, split it in two halves and sum each half in a separate
     * task, which may or may not run in parallel with the other one depending
     * on the scheduler currently in use - several schedulers are shown
     */
    auto foo = [](auto &sched) {
        auto arr = ostd::iter({ 150, 38, 76, 25, 67, 18, -15,  215, 25, -10 });

        auto c = make_channel<int>(sched);
        auto f = [](auto c, auto half) {
            c.put(foldl(half, 0));
        };
        spawn(sched, f, c, arr.slice(0, arr.size() / 2));
        spawn(sched, f, c, arr + (arr.size() / 2));

        int a = c.get();
        int b = c.get();
        writefln("%s + %s = %s", a, b, a + b);
    };

    /* using thread_scheduler results in an OS thread spawned per task,
     * implementing a 1:1 (kernel-level) scheduling - very expensive on
     * Windows, less expensive on Unix-likes (but more than coroutines)
     */
    thread_scheduler tsched;
    tsched.start([&tsched, &foo]() {
        writeln("(1) 1:1 scheduler: starting...");
        foo(tsched);
        writeln("(1) 1:1 scheduler: finishing...");
    });
    writeln();

    /* using simple_coroutine_scheduler results in a coroutine spawned
     * per task, implementing N:1 (user-level) scheduling - very cheap
     * and portable everywhere but obviously limited to only one thread
     */
    simple_coroutine_scheduler scsched;
    scsched.start([&scsched, &foo]() {
        writeln("(2) N:1 scheduler: starting...");
        foo(scsched);
        writeln("(2) N:1 scheduler: finishing...");
    });
    writeln();

    /* using coroutine_scheduler results in a coroutine spawned per
     * task, but mapped onto a certain number of OS threads, implementing
     * a hybrid M:N approach - this benefits from multicore systems and
     * also is relatively cheap (you can create a big number of tasks)
     */
    coroutine_scheduler csched;
    csched.start([&csched, &foo]() {
        writeln("(3) M:N scheduler: starting...");
        foo(csched);
        writeln("(3) M:N scheduler: finishing...");
    });
}

/*
(1) 1:1 scheduler: starting...
356 + 233 = 589
(1) 1:1 scheduler: finishing...

(2) N:1 scheduler: starting...
356 + 233 = 589
(2) N:1 scheduler: finishing...

(3) M:N scheduler: starting...
356 + 233 = 589
(3) M:N scheduler: finishing...
*/
