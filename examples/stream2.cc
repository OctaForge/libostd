#include <ostd/algorithm.hh>
#include <ostd/string.hh>
#include <ostd/vector.hh>
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

    auto ts1 = make_string(test.iter());
    writefln("-- str beg --\n%s-- str end --", ts1);

    test.seek(0);

    writeln("\n## PART FILE READ ##\n");

    auto ts2 = make_string(test.iter().take(25));
    writefln("-- str beg --\n%s\n-- str end --", ts2);

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

    auto x = make_vector<std::string>(test.iter_lines());
    writeln("before sort: ", x);
    sort(iter(x));
    writeln("after sort: ", x);

    return 0;
}
