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
    coroutine_context &ctx = x;

    writefln(
        "generator is coroutine<int()>: %s",
        bool(ctx.coroutine<coroutine<int()>>())
    );
    writefln(
        "generator is generator<int>: %s",
        bool(ctx.coroutine<generator<int>>())
    );

    generator<int> &gr = *ctx.coroutine<generator<int>>();
    writefln("value of cast back generator: %s", gr.value());
}

/*
generator test
generated: 5
generated: 10
generated: 15
generated: 20
generated: 25
*/
