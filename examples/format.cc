#include <tuple>

#include <ostd/algorithm.hh>
#include <ostd/vector.hh>
#include <ostd/unordered_map.hh>
#include <ostd/range.hh>
#include <ostd/io.hh>

using namespace ostd;
using namespace ostd::string_literals;

struct Foo {
};

/* implementing formatting for custom objects - external function */
template<typename R>
void to_format(Foo const &, R &writer, format_spec const &fs) {
    switch (fs.spec()) {
        case 'i':
            range_put_all(writer, "Foo1"_sr);
            break;
        default:
            range_put_all(writer, "Foo2"_sr);
            break;
    }
}

struct Bar {
    /* implementing formatting for custom objects - method */
    template<typename R>
    void to_format(R &writer, format_spec const &fs) const {
        switch (fs.spec()) {
            case 'i':
                range_put_all(writer, "Bar1"_sr);
                break;
            default:
                range_put_all(writer, "Bar2"_sr);
                break;
        }
    }
};

int main() {
    std::vector<int> x = { 5, 10, 15, 20 };
    writefln("[%(%s|%)]", x);
    writefln("%s", x);

    int y[] = { 2, 4, 8, 16, 32 };
    writefln("{ %(%s, %) }", y);

    writefln("[\n%([ %(%s, %) ]%|,\n%)\n]", map(range(10), [](int v) {
        return range(v + 1);
    }));

    std::unordered_map<std::string, int> m = {
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
    /* without expansion */
    writefln("{ %(%s, %) }", m);

    /* tuple formatting */
    std::tuple<std::string, int, float> tup{"hello world", 1337, 3.14f};
    writefln("the tuple contains %<%s, %d, %f%>.", tup);

    /* tuple format test */
    std::tuple<int, float, char const *> xt[] = {
        std::make_tuple(5, 3.14f, "foo"),
        std::make_tuple(3, 1.23f, "bar"),
        std::make_tuple(9, 8.66f, "baz")
    };
    writefln("[ %#(<%d|%f|%s>%|, %) ]", xt);

    /* custom format */
    writefln("%s", Foo{});
    writefln("%i", Foo{});

    /* custom format via method */
    writefln("%s", Bar{});
    writefln("%i", Bar{});

    /* format into string */
    auto s = appender_range<std::string>{};
    format(s, "hello %s", "world");
    writeln(s.get());
}
