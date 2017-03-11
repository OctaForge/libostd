#include <ostd/io.hh>
#include <ostd/coroutine.hh>

using namespace ostd;

struct foo {
    foo() {
        writeln("<constructing foo>");
    }
    ~foo() {
        writeln("<destroying foo>");
    }
};

int main() {
    writeln("starting main...");
    for (int steps: range(1, 10)) {
        if (steps != 1) {
            /* separate the results */
            writeln();
        }
        writefln("    main loop: step %s", steps);
        writeln("    coroutine creation");
        coroutine<int(int)> f = [](auto yield, int x) {
            writefln("        coroutine call, first arg: %s", x);
            foo test;
            for (int i: range(x)) {
                writefln("        loop inside coroutine %s", i + 1);
                writefln("        yielding %s...", i * 10);
                auto yr = yield(i * 10);
                writefln("        yielded: %s", yr);
            }
            writeln("        return from coroutine (returning 1234)...");
            return 1234;
        };
        writeln("    coroutine call loop");
        int val = 5;
        for (int i: range(steps)) {
            writeln("    calling into coroutine...");
            int v;
            try {
                v = f(val);
            } catch (coroutine_error const &e) {
                writefln("coroutine error: %s", e.what());
                return 0;
            }
            writefln("    called into coroutine which yielded: %s", v);
            writefln("    call loop iteration %s done", i + 1);
            writefln("    coroutine dead: %s", !f);
            val += 5;
        }
        writefln("    main loop iteration %s done", steps);
    }
    writeln("... main has ended");
}

/*
starting main...
    main loop: step 1
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    main loop iteration 1 done
<destroying foo>

    main loop: step 2
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    calling into coroutine...
        yielded: 10
        loop inside coroutine 2
        yielding 10...
    called into coroutine which yielded: 10
    call loop iteration 2 done
    coroutine dead: false
    main loop iteration 2 done
<destroying foo>

    main loop: step 3
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    calling into coroutine...
        yielded: 10
        loop inside coroutine 2
        yielding 10...
    called into coroutine which yielded: 10
    call loop iteration 2 done
    coroutine dead: false
    calling into coroutine...
        yielded: 15
        loop inside coroutine 3
        yielding 20...
    called into coroutine which yielded: 20
    call loop iteration 3 done
    coroutine dead: false
    main loop iteration 3 done
<destroying foo>

    main loop: step 4
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    calling into coroutine...
        yielded: 10
        loop inside coroutine 2
        yielding 10...
    called into coroutine which yielded: 10
    call loop iteration 2 done
    coroutine dead: false
    calling into coroutine...
        yielded: 15
        loop inside coroutine 3
        yielding 20...
    called into coroutine which yielded: 20
    call loop iteration 3 done
    coroutine dead: false
    calling into coroutine...
        yielded: 20
        loop inside coroutine 4
        yielding 30...
    called into coroutine which yielded: 30
    call loop iteration 4 done
    coroutine dead: false
    main loop iteration 4 done
<destroying foo>

    main loop: step 5
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    calling into coroutine...
        yielded: 10
        loop inside coroutine 2
        yielding 10...
    called into coroutine which yielded: 10
    call loop iteration 2 done
    coroutine dead: false
    calling into coroutine...
        yielded: 15
        loop inside coroutine 3
        yielding 20...
    called into coroutine which yielded: 20
    call loop iteration 3 done
    coroutine dead: false
    calling into coroutine...
        yielded: 20
        loop inside coroutine 4
        yielding 30...
    called into coroutine which yielded: 30
    call loop iteration 4 done
    coroutine dead: false
    calling into coroutine...
        yielded: 25
        loop inside coroutine 5
        yielding 40...
    called into coroutine which yielded: 40
    call loop iteration 5 done
    coroutine dead: false
    main loop iteration 5 done
<destroying foo>

    main loop: step 6
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    calling into coroutine...
        yielded: 10
        loop inside coroutine 2
        yielding 10...
    called into coroutine which yielded: 10
    call loop iteration 2 done
    coroutine dead: false
    calling into coroutine...
        yielded: 15
        loop inside coroutine 3
        yielding 20...
    called into coroutine which yielded: 20
    call loop iteration 3 done
    coroutine dead: false
    calling into coroutine...
        yielded: 20
        loop inside coroutine 4
        yielding 30...
    called into coroutine which yielded: 30
    call loop iteration 4 done
    coroutine dead: false
    calling into coroutine...
        yielded: 25
        loop inside coroutine 5
        yielding 40...
    called into coroutine which yielded: 40
    call loop iteration 5 done
    coroutine dead: false
    calling into coroutine...
        yielded: 30
        return from coroutine (returning 1234)...
<destroying foo>
    called into coroutine which yielded: 1234
    call loop iteration 6 done
    coroutine dead: true
    main loop iteration 6 done

    main loop: step 7
    coroutine creation
    coroutine call loop
    calling into coroutine...
        coroutine call, first arg: 5
<constructing foo>
        loop inside coroutine 1
        yielding 0...
    called into coroutine which yielded: 0
    call loop iteration 1 done
    coroutine dead: false
    calling into coroutine...
        yielded: 10
        loop inside coroutine 2
        yielding 10...
    called into coroutine which yielded: 10
    call loop iteration 2 done
    coroutine dead: false
    calling into coroutine...
        yielded: 15
        loop inside coroutine 3
        yielding 20...
    called into coroutine which yielded: 20
    call loop iteration 3 done
    coroutine dead: false
    calling into coroutine...
        yielded: 20
        loop inside coroutine 4
        yielding 30...
    called into coroutine which yielded: 30
    call loop iteration 4 done
    coroutine dead: false
    calling into coroutine...
        yielded: 25
        loop inside coroutine 5
        yielding 40...
    called into coroutine which yielded: 40
    call loop iteration 5 done
    coroutine dead: false
    calling into coroutine...
        yielded: 30
        return from coroutine (returning 1234)...
<destroying foo>
    called into coroutine which yielded: 1234
    call loop iteration 6 done
    coroutine dead: true
    calling into coroutine...
coroutine error: dead coroutine
*/
