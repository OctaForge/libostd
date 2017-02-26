#include <tuple>
#include <clocale>

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
                range_put_all(writer, "Foo_i"_sr);
                break;
            default:
                range_put_all(writer, "Foo_s"_sr);
                break;
        }
        if (fs.flags() & FMT_FLAG_AT) {
            range_put_all(writer, "_esc"_sr);
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
    writeln("-- range format --");
    writefln("[%(%s|%)]", x);
    /* prints a range with default format {item, item, item, ...}
     * you can enable item escaping by passing the @ flag
     */
    writeln("\n-- range default format --");
    writefln("%s", x);

    int y[] = { 2, 4, 8, 16, 32 };
    /* prints { 2, 4, 8, 16, 32 } using ", " as the delimiter */
    writeln("\n-- range format of static array --");
    writefln("{ %(%s, %) }", y);

    /* nested range printing - prints each item of the main
     * range with [ %(%s, %) ] and ",\n" as the delimiter
     */
    writeln("\n-- range format of nested range --");
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
    writeln("\n-- range format of hash table --");
    writefln("{ %#(%@s: %d, %) }", m);
    /* not escaped, you get { baz: 15, bar: 10, foo: 5} */
    writeln("\n-- range format of hash table (no escape) --");
    writefln("{ %#(%s: %d, %) }", m);
    /* no expansion of the items, print entire tuple with default format,
     * gets you something like { <"baz", 15>, <"bar", 10>, <"foo", 5> }
     * because the default tuple format is <item, item, item, ...>
     */
    writeln("\n-- range format of hash table (no item expansion) --");
    writefln("{ %(%@s, %) }", m);

    /* as the @ flag enables escaping on strings and chars,
     * you can use it standalone outside of range/tuple format
     */
    writeln("\n-- format item escaping --");
    writefln("not escaped: %s, escaped: %@s", "foo", "bar");

    std::tuple<std::string, int, float, std::string> tup{
        "hello world", 1337, 3.14f, "test"
    };
    /* you can expand tuples similarly to ranges, with %<CONTENTS%> where
     * CONTENTS is a regular format string like if the tuple was formatted
     * separately with each item of the tuple passed as a separate argument
     */
    writeln("\n-- tuple format --");
    writefln("the tuple contains %<%@s, %d, %f, %s%>.", tup);
    writeln("\n-- tuple default format --");
    writefln("auto tuple: %s", tup);
    writeln("\n-- tuple default format (escaped) --");
    writefln("auto tuple with escape: %@s", tup);

    std::tuple<int, float, char const *> xt[] = {
        std::make_tuple(5, 3.14f, "foo"),
        std::make_tuple(3, 1.23f, "bar"),
        std::make_tuple(9, 8.66f, "baz")
    };
    /* formatting a range of tuples, with each tuple expanded using #
     */
    writeln("\n-- range of tuples format --");
    writefln("[ %#(<%d|%f|%@s>%|, %) ]", xt);

    /* formatting custom objects, the information about the format mark
     * is passed into the to_format function and the object can read it
     */
    writeln("\n-- custom object format --");
    writefln("%s", Foo{});
    writefln("%i", Foo{});
    writefln("%@s", Foo{});
    writefln("%@i", Foo{});

    auto s = appender_range<std::string>{};
    /* formatting into a string sink (can be any output range, but
     * appender makes sure the capacity is unlimited so it's safe)
     */
    writeln("\n-- format into a string --");
    format(s, "hello %s", "world");
    writeln(s.get());

    /* locale specific formatting */
    writeln("\n-- number format with C locale --");
    writefln(
        "\"%d\", \"%f\", \"%X\"", 123456789, 12345.6789123, 0x123456789ABCDEF
    );
    std::locale::global(std::locale{""});
    out.imbue(std::locale{});
    writefln("\n-- number format with system locale --");
    writefln(
        "\"%d\", \"%f\", \"%X\"", 123456789, 12345.6789123, 0x123456789ABCDEF
    );
}

/* output:

-- range format --
[5|10|15|20]

-- range default format --
{5, 10, 15, 20}

-- range format of static array --
{ 2, 4, 8, 16, 32 }

-- range format of nested range --
[
[ 0 ],
[ 0, 1 ],
[ 0, 1, 2 ],
[ 0, 1, 2, 3 ],
[ 0, 1, 2, 3, 4 ],
[ 0, 1, 2, 3, 4, 5 ],
[ 0, 1, 2, 3, 4, 5, 6 ],
[ 0, 1, 2, 3, 4, 5, 6, 7 ],
[ 0, 1, 2, 3, 4, 5, 6, 7, 8 ],
[ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ]
]

-- range format of hash table --
{ "baz": 15, "bar": 10, "foo": 5 }

-- range format of hash table (no escape) --
{ baz: 15, bar: 10, foo: 5 }

-- range format of hash table (no item expansion) --
{ <"baz", 15>, <"bar", 10>, <"foo", 5> }

-- format item escaping --
not escaped: foo, escaped: "bar"

-- tuple format --
the tuple contains "hello world", 1337, 3.140000, test.

-- tuple default format --
auto tuple: <hello world, 1337, 3.14, test>

-- tuple default format (escaped) --
auto tuple with escape: <"hello world", 1337, 3.14, "test">

-- range of tuples format --
[ <5|3.140000|"foo">, <3|1.230000|"bar">, <9|8.660000|"baz"> ]

-- custom object format --
Foo_s
Foo_i
Foo_s_esc
Foo_i_esc

-- format into a string --
hello world

-- number format with C locale --
"123456789", "12345.678912", "123456789ABCDEF"

-- number format with system locale --
"123 456 789", "12 345,678912", "123 456 789 ABC DEF"

*/
