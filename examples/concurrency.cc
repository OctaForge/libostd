/** @example concurrency.cc
 *
 * A simple example of the concurrency module usage, explaining schdulers,
 * ostd::spawn(), ostd::tid as well as ostd::channel.
 */

#include <ostd/io.hh>
#include <ostd/concurrency.hh>

using namespace ostd;

/* this version uses Go-style channels to exchange data; multiple
 * tasks can put data into channels, the channel itself is a thread
 * safe queue; it goes the other way around too, multiple tasks can
 * wait on a channel for some data to be received
 */
template<typename Slice>
static void test_channel(Slice first_half, Slice second_half) {
    auto c = make_channel<int>();
    auto f = [](auto ch, auto half) {
        ch.put(foldl(half, 0));
    };
    spawn(f, c, first_half);
    spawn(f, c, second_half);

    int a = c.get();
    int b = c.get();
    writefln("    %s + %s = %s", a, b, a + b);
}

/* this version uses future-style tid objects that represent the tasks
 * themselves including the return value of the task; the return value
 * can be retrieved using .get(), which will wait for the value to be
 * stored unless it already is, becoming invalid after retrieval of
 * the value; the tid also intercepts any possible exceptions thrown
 * by the task and propagates them in .get(), allowing the user to
 * perform safe exception handling across tasks
 *
 * keep in mind that unlike the above, this will always wait for t1
 * first (though t2 can still run in parallel to it depending on the
 * scheduler currently in use), so a will always come from t1 and b
 * from t2; in the above test, a can come from either task
 */
template<typename Slice>
static void test_tid(Slice first_half, Slice second_half) {
    auto f = [](auto half) {
        return foldl(half, 0);
    };
    auto t1 = spawn(f, first_half);
    auto t2 = spawn(f, second_half);

    int a = t1.get();
    int b = t2.get();
    writefln("    %s + %s = %s", a, b, a + b);
}

static void test_all() {
    /* have an array, split it in two halves and sum each half in a separate
     * task, which may or may not run in parallel with the other one depending
     * on the scheduler currently in use - several schedulers are shown
     */
    static auto input = { 150, 38, 76, 25, 67, 18, -15, 215, 25, -10 };

    static auto first_half  = iter(input).slice(0, input.size() / 2);
    static auto second_half = iter(input).slice(input.size() / 2);

    writeln("  testing channels...");
    test_channel(first_half, second_half);
    writeln("  testing futures...");
    test_tid(first_half, second_half);
}

int main() {
    /* using thread_scheduler results in an OS thread spawned per task,
     * implementing a 1:1 (kernel-level) scheduling - very expensive on
     * Windows, less expensive on Unix-likes (but more than coroutines)
     */
    thread_scheduler{}.start([]() {
        writeln("(1) 1:1 scheduler: starting...");
        test_all();
        writeln("(1) 1:1 scheduler: finishing...");
    });
    writeln();

    /* using simple_coroutine_scheduler results in a coroutine spawned
     * per task, implementing N:1 (user-level) scheduling - very cheap
     * and portable everywhere but obviously limited to only one thread
     */
    simple_coroutine_scheduler{}.start([]() {
        writeln("(2) N:1 scheduler: starting...");
        test_all();
        writeln("(2) N:1 scheduler: finishing...");
    });
    writeln();

    /* using coroutine_scheduler results in a coroutine spawned per
     * task, but mapped onto a certain number of OS threads, implementing
     * a hybrid M:N approach - this benefits from multicore systems and
     * also is relatively cheap (you can create a big number of tasks)
     */
    coroutine_scheduler{}.start([]() {
        writeln("(3) M:N scheduler: starting...");
        test_all();
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
