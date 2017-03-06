#include <ostd/io.hh>
#include <ostd/coroutine.hh>

using namespace ostd;

struct foo {
    foo() {
        writeln("***foo ctor***");
    }
    ~foo() {
        writeln("***foo dtor***");
    }
};

int main() {
    writeln("MAIN START");
    for (int steps: range(1, 10)) {
        if (steps != 1) {
            /* separate the results */
            writeln();
        }
        writefln("** SCOPE START %d **", steps);
        coroutine<int(int)> f = [](auto &coro, int x) {
            foo test;
            writeln("got: ", x);
            for (int i: range(x)) {
                writeln("loop: ", i);
                writeln("yield: ", coro.yield(i * 10));
            }
            writeln("return");
            return 1234;
        };
        writeln("CALL");
        int val = 5;
        for (int i: range(steps)) {
            writeln("COROUTINE RETURNED: ", f(val));
            writefln("OUT %d (dead: %s)", i, !f);
            val += 5;
        }
        writefln("** SCOPE END %d **", steps);
    }
    writeln("MAIN END");
}

/*
MAIN START
** SCOPE START 1 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
** SCOPE END 1 **
***foo dtor***

** SCOPE START 2 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
yield: 10
loop: 1
COROUTINE RETURNED: 10
OUT 1 (dead: false)
** SCOPE END 2 **
***foo dtor***

** SCOPE START 3 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
yield: 10
loop: 1
COROUTINE RETURNED: 10
OUT 1 (dead: false)
yield: 15
loop: 2
COROUTINE RETURNED: 20
OUT 2 (dead: false)
** SCOPE END 3 **
***foo dtor***

** SCOPE START 4 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
yield: 10
loop: 1
COROUTINE RETURNED: 10
OUT 1 (dead: false)
yield: 15
loop: 2
COROUTINE RETURNED: 20
OUT 2 (dead: false)
yield: 20
loop: 3
COROUTINE RETURNED: 30
OUT 3 (dead: false)
** SCOPE END 4 **
***foo dtor***

** SCOPE START 5 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
yield: 10
loop: 1
COROUTINE RETURNED: 10
OUT 1 (dead: false)
yield: 15
loop: 2
COROUTINE RETURNED: 20
OUT 2 (dead: false)
yield: 20
loop: 3
COROUTINE RETURNED: 30
OUT 3 (dead: false)
yield: 25
loop: 4
COROUTINE RETURNED: 40
OUT 4 (dead: false)
** SCOPE END 5 **
***foo dtor***

** SCOPE START 6 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
yield: 10
loop: 1
COROUTINE RETURNED: 10
OUT 1 (dead: false)
yield: 15
loop: 2
COROUTINE RETURNED: 20
OUT 2 (dead: false)
yield: 20
loop: 3
COROUTINE RETURNED: 30
OUT 3 (dead: false)
yield: 25
loop: 4
COROUTINE RETURNED: 40
OUT 4 (dead: false)
yield: 30
return
***foo dtor***
COROUTINE RETURNED: 1234
OUT 5 (dead: true)
** SCOPE END 6 **

** SCOPE START 7 **
CALL
***foo ctor***
got: 5
loop: 0
COROUTINE RETURNED: 0
OUT 0 (dead: false)
yield: 10
loop: 1
COROUTINE RETURNED: 10
OUT 1 (dead: false)
yield: 15
loop: 2
COROUTINE RETURNED: 20
OUT 2 (dead: false)
yield: 20
loop: 3
COROUTINE RETURNED: 30
OUT 3 (dead: false)
yield: 25
loop: 4
COROUTINE RETURNED: 40
OUT 4 (dead: false)
yield: 30
return
***foo dtor***
COROUTINE RETURNED: 1234
OUT 5 (dead: true)
terminating with uncaught exception of type ostd::coroutine_error: dead coroutine
*/
