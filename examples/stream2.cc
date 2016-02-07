#include <ostd/algorithm.hh>
#include <ostd/string.hh>
#include <ostd/io.hh>

using namespace ostd;

int main() {
    writeln("writing sample file...");

    FileStream wtest("test.txt", StreamMode::write);

    String smpl = "This is a test file for later read.\n"
                  "It contains some sample text in order to see whether "
                  "things actually read correctly.\n\n\n"
                  ""
                  "This is after a few newlines. The file continues here.\n"
                  "The file ends here.\n";

    copy(smpl.iter(), wtest.iter());
    wtest.close();

    FileStream test("test.txt", StreamMode::read);

    writeln("## WHOLE FILE READ ##\n");

    String ts1(test.iter());
    writefln("-- str beg --\n%s-- str end --", ts1);

    test.seek(0);

    writeln("\n## PART FILE READ ##\n");

    String ts2(take(test.iter(), 25));
    writefln("-- str beg --\n%s\n-- str end --", ts2);

    return 0;
}
