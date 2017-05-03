/** @example stream2.cc
 *
 * An example of using streams to process strings, including some
 * standard range algorithm use.
 */

#include <vector>

#include <ostd/algorithm.hh>
#include <ostd/string.hh>
#include <ostd/io.hh>

using namespace ostd;

int main() {
    writeln("writing sample file...");

    file_stream wtest{"test.txt", stream_mode::WRITE};

    std::string smpl =
        "This is a test file for later read.\n"
        "It contains some sample text in order to see whether "
        "things actually read correctly.\n\n\n"
        ""
        "This is after a few newlines. The file continues here.\n"
        "The file ends here.\n";

    copy(iter(smpl), wtest.iter());
    wtest.close();

    file_stream test{"test.txt"};

    writeln("## WHOLE FILE READ ##\n");

    auto fr = test.iter();
    writefln(
        "-- str beg --\n%s-- str end --",
        std::string{fr.iter_begin(), fr.iter_end()}
    );

    test.seek(0);

    writeln("\n## PART FILE READ ##\n");

    auto fr2 = test.iter().take(25);
    writefln(
        "-- str beg --\n%s\n-- str end --",
        std::string{fr2.iter_begin(), fr2.iter_end()}
    );

    test.seek(0);

    writeln("\n## BY LINE READ ##\n");

    for (auto const &line: test.iter_lines()) {
        writeln("got line: ", line);
    }

    test.close();

    writeln("\n## FILE SORT ##\n");

    wtest.open("test.txt", stream_mode::WRITE);

    smpl = "foo\n"
        "bar\n"
        "baz\n"
        "test\n"
        "this\n"
        "will\n"
        "be\n"
        "in\n"
        "order\n";

    copy(iter(smpl), wtest.iter());
    wtest.close();

    test.open("test.txt");

    auto lns = test.iter_lines();
    std::vector<std::string> x{lns.iter_begin(), lns.iter_end()};
    writeln("before sort: ", x);
    sort(iter(x));
    writeln("after sort: ", x);

    return 0;
}
