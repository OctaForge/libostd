#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/event.hh>

using namespace ostd;

struct SignalTest {
    /* const on the class means that a const reference to the event
     * can actually emit (in that case, the reference passed to each
     * callback will always be const to make sure nothing changes)
     */
    Signal<const SignalTest, int, const char *> on_simple = this;
    Signal<      SignalTest, float            > on_param  = this;

    SignalTest(): p_param(3.14f) {
        /* we can connect methods */
        on_simple.connect(&SignalTest::simple_method);
        writeln("constructed signal test");
    }

    float get_param() const { return p_param; }
    void  set_param(float nv) {
        float oldval = p_param;
        p_param = nv;
        /* types passed to emit must match the types of the event */
        on_param.emit(oldval);
    }

    void foo() const {
        /* because on_simple is const, we can emit form const method */
        on_simple.emit(150, "hello world");
    }

    void simple_method(int v, const char *str) const {
        writefln("simple method handler: %d, %s", v, str);
    }

private:
    float p_param;
};

int main() {
    writeln("=== program start ===");
    SignalTest st;

    int test = 42;

    /* we can connect lambdas, including closures
     * this callback can access "test" easily and it will still work
     */
    auto idx = st.on_simple.connect([&](const SignalTest &, int v,
                                        const char *str) {
        writefln("and lambda test: %d, %s (%d)", v, str, test);
    });

    writeln("--- test emit ---");
    st.foo();

    /* we can disconnect callbacks too */
    st.on_simple.disconnect(idx);

    /* this should not print from the lambda above */
    writeln("--- test emit after disconnect ---");
    st.foo();

    writeln("--- set value ---");
    st.set_param(6.28f);

    /* the reference to SignalTest here is mutable */
    st.on_param.connect([](SignalTest &self, float oldval) {
        writeln("value changed...");
        writefln("   old value: %f, new value: %f", oldval,
            self.get_param());

        /* when we have a mutable reference we can change the original
         * object, for example re-emit its own signal once again
         */
        if (self.get_param() > 140.0f) return;
        self.set_param(self.get_param() + 1.0f);
    });

    /* trigger the callback above */
    writeln("--- test set value ---");
    st.set_param(134.28f);

    writeln("=== program end ===");
    return 0;
}
