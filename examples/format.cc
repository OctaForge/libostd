#include <ostd/algorithm.hh>
#include <ostd/vector.hh>
#include <ostd/map.hh>
#include <ostd/range.hh>
#include <ostd/io.hh>
#include <ostd/tuple.hh>

using namespace ostd;

struct Foo {
};

/* implementing formatting for custom objects - external function */
template<typename R>
bool to_format(Foo const &, R &writer, FormatSpec const &fs) {
    switch (fs.spec()) {
        case 'i':
            writer.put_string("Foo1");
            break;
        default:
            writer.put_string("Foo2");
            break;
    }
    return true;
}

struct Bar {
    /* implementing formatting for custom objects - method */
    template<typename R>
    bool to_format(R &writer, FormatSpec const &fs) const {
        switch (fs.spec()) {
            case 'i':
                writer.put_string("Bar1");
                break;
            default:
                writer.put_string("Bar2");
                break;
        }
        return true;
    }
};

int main() {
    Vector<int> x = { 5, 10, 15, 20 };
    writefln("[%(%s|%)]", x);

    int y[] = { 2, 4, 8, 16, 32 };
    writefln("{ %(%s, %) }", y);

    writefln("[\n%([ %(%s, %) ]%|,\n%)\n]", map(range(10), [](int v) {
        return range(v + 1);
    }));

    Map<String, int> m = {
        { "foo", 5  },
        { "bar", 10 },
        { "baz", 15 }
    };
    /* strings and chars are automatically escaped */
    writefln("{ %#(%s: %d, %) }", m);
    /* can override escaping with the - flag,
     * # flag expands the element into multiple values
     */
    writefln("{ %-#(%s: %d, %) }", m);

    /* tuple format test */
    Tuple<int, float, char const *> xt[] = {
        make_tuple(5, 3.14f, "foo"),
        make_tuple(3, 1.23f, "bar"),
        make_tuple(9, 8.66f, "baz")
    };
    writefln("[ %#(<%d|%f|%s>%|, %) ]", xt);

    /* custom format */
    writefln("%s", Foo{});
    writefln("%i", Foo{});

    /* custom format via method */
    writefln("%s", Bar{});
    writefln("%i", Bar{});

    /* format into string */
    auto s = appender<String>();
    format(s, "hello %s", "world");
    writeln(s.get());
}
