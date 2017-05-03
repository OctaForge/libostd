/** @example coroutine2.cc
 *
 * An example of using coroutines as generators as well as nested resume/yield.
 */

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

    generator<int> x{f};
    /* coroutine_context exists as a base type for any coroutine */
    coroutine_context *ctx = &x;

    writefln(
        "generator is coroutine<int()>: %s",
        bool(dynamic_cast<coroutine<int()> *>(ctx))
    );
    writefln(
        "generator is generator<int>: %s",
        bool(dynamic_cast<generator<int> *>(ctx))
    );

    generator<int> &gr = dynamic_cast<generator<int> &>(*ctx);
    writefln("value of cast back generator: %s", gr.value());

    writeln("-- nested coroutine test --");

    coroutine<void()> c1 = [](auto yield) {
        coroutine<void()> c2 = [&yield](auto) {
            writeln("inside c2 1");
            yield();
            writeln("inside c2 2");
            yield();
            writeln("inside c2 3");
        };
        writeln("inside c1 1");
        c2();
        writeln("inside c1 2");
    };
    writeln("outside 1");
    c1();
    writeln("outside 2");
    c1();
    writeln("outside 3");
    c1();
    writeln("outside exit");
}

/*
generator test
generated: 5
generated: 10
generated: 15
generated: 20
generated: 25
generator is coroutine<int()>: false
generator is generator<int>: true
value of cast back generator: 5
-- nested coroutine test --
outside 1
inside c1 1
inside c2 1
outside 2
inside c2 2
outside 3
inside c2 3
inside c1 2
outside exit
*/
