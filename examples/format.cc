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

/* implementing formatting for custom objects */
template<>
struct format_traits<Foo> {
    template<typename R>
    static void to_format(Foo const &, R &writer, format_spec const &fs) {
        switch (fs.spec()) {
            case 'i':
                range_put_all(writer, "Foo1"_sr);
                break;
            default:
                range_put_all(writer, "Foo2"_sr);
                break;
        }
    }
};

int main() {
    std::vector<int> x = { 5, 10, 15, 20 };
    /* prints [5|10|15|20] (using | as the delimiter and %s for each item),
     * the syntax for ranges is %(CONTENTS%) where CONTENTS is a sequence
     * up until and including the last format mark followed by a delimiter,
     * so for example "%s, " has "%s" for formatting and ", " for the delimiter
     * and "%d: %s, " has "%d: %s" for format and ", " for the delimiter; if
     * you need to specify a complicated manual delimiter, you can use the
     * "FORMAT%|DELIMITER" syntax, where %(%s, %) equals %(%s%|, %)
     */
    writefln("[%(%s|%)]", x);
    /* prints a range with default format {item, item, item, ...}
     * you can enable item escaping by passing the @ flag
     */
    writefln("%s", x);

    int y[] = { 2, 4, 8, 16, 32 };
    /* prints { 2, 4, 8, 16, 32 } using ", " as the delimiter */
    writefln("{ %(%s, %) }", y);

    /* nested range printing - prints each item of the main
     * range with [ %(%s, %) ] and ",\n" as the delimiter
     */
    writefln("[\n%([ %(%s, %) ]%|,\n%)\n]", map(range(10), [](int v) {
        return range(v + 1);
    }));

    std::unordered_map<std::string, int> m = {
        { "foo", 5  },
        { "bar", 10 },
        { "baz", 15 }
    };
    /* prints something like { "baz": 15, "bar": 10, "foo": 5 }, note that
     * the tuple is expanded into two formats (using the # flag) and the
     * items are escaped with the @ flag (applies to strings and chars)
     */
    writefln("{ %#(%@s: %d, %) }", m);
    /* not escaped, you get { baz: 15, bar: 10, foo: 5} */
    writefln("{ %#(%s: %d, %) }", m);
    /* no expansion of the items, print entire tuple with default format,
     * gets you something like { <"baz", 15>, <"bar", 10>, <"foo", 5> }
     * because the default tuple format is <item, item, item, ...>
     */
    writefln("{ %(%s, %) }", m);

    /* as the @ flag enables escaping on strings and chars,
     * you can use it standalone outside of range/tuple format
     */
    writefln("not escaped: %s, escaped: %@s", "foo", "bar");

    /* you can expand tuples similarly to ranges, with %<CONTENTS%> where
     * CONTENTS is a regular format string like if the tuple was formatted
     * separately with each item of the tuple passed as a separate argument
     */
    std::tuple<std::string, int, float, std::string> tup{
        "hello world", 1337, 3.14f, "test"
    };
    writefln("the tuple contains %<%@s, %d, %f, %s%>.", tup);
    writefln("auto tuple: %s", tup);
    writefln("auto tuple with escape: %@s", tup);

    /* formatting a range of tuples, with each tuple expanded using #
     */
    std::tuple<int, float, char const *> xt[] = {
        std::make_tuple(5, 3.14f, "foo"),
        std::make_tuple(3, 1.23f, "bar"),
        std::make_tuple(9, 8.66f, "baz")
    };
    writefln("[ %#(<%d|%f|%@s>%|, %) ]", xt);

    /* formatting custom objects, the information about the format mark
     * is passed into the to_format function and the object can read it
     */
    writefln("%s", Foo{});
    writefln("%i", Foo{});

    /* formatting into a string sink (can be any output range, but
     * appender makes sure the capacity is unlimited so it's safe)
     */
    auto s = appender_range<std::string>{};
    format(s, "hello %s", "world");
    writeln(s.get());
}
