/** @example argparse.cc
 *
 * An example of using the command line argument parser.
 */

#include <ostd/argparse.hh>
#include <ostd/io.hh>

using namespace ostd;

int main() {
    writeln("-- VERY SIMPLE EXAMPLE --\n");
    {
        arg_parser p{"test"};

        p.add_optional("-h", "--help", 0)
            .help("print this message and exit")
            .action(arg_print_help(p));

        p.add_optional("-t", "--test", 0)
            .help("test help")
            .action([](auto) { writeln("test invoked"); });

        p.add_optional("-f", "--foo", 1)
            .help("foo help")
            .action([](auto vals) { writeln("foo invoked: ", vals[0]); });

        writeln("--- without help:");
        p.parse(iter({ "-f", "150", "-t" }));

        writeln("\n--- with help:");
        p.parse(iter({ "--help" }));
    }

    writeln("\n-- DIFFERENT SYNTAX --\n");
    {
        arg_parser p{"test", "/+"};

        p.add_optional("/h", "/help", 0)
            .help("print this message and exit")
            .action(arg_print_help(p));

        p.add_optional("+t", "++test", 0)
            .help("test help")
            .action([](auto) { writeln("test invoked"); });

        p.add_optional("/f", "++foo", 1)
            .help("foo help")
            .action([](auto vals) { writeln("foo invoked: ", vals[0]); });

        writeln("--- without help:");
        p.parse(iter({ "/f", "150", "+t" }));

        writeln("\n--- with help:");
        p.parse(iter({ "/help" }));
    }

    writeln("\n-- GROUPS AND OTHER FEATURES --\n");
    {
        arg_parser p{"test"};

        p.add_optional("-h", "--help", 0)
            .help("print this message and exit")
            .action(arg_print_help(p));

        p.add_positional("foo", 1)
            .help("a positional arg");
        p.add_positional("bar", arg_value::REST)
            .help("all other arguments");

        auto &g1 = p.add_group("foo", "Group 1");

        g1.add_optional("-x", "--test1", 0)
            .help("test1 help");
        g1.add_optional("-y", "--test2", 1)
            .help("test2 help");
        g1.add_optional("-z", "--test3", arg_value::OPTIONAL)
            .help("test3 help");

        auto &g2 = p.add_group("bar", "Group 2");

        g2.add_optional("-a", "--test4", arg_value::ALL, 0)
            .help("test4 help");
        g2.add_optional("-b", "--test5", arg_value::ALL, 1)
            .help("test5 help");
        g2.add_optional("-c", "--test6", arg_value::ALL, 2)
            .help("test6 help");

        p.parse(iter({ "--help" }));
    }

    writeln("\n-- MUTUAL EXCLUSION --\n");
    {
        arg_parser p{"test"};

        p.add_optional("-h", "--help", 0)
            .help("print this message and exit")
            .action(arg_print_help(p));

        auto &mg = p.add_mutually_exclusive_group();
        mg.add_optional("--foo", 0);
        mg.add_optional("--bar", 0);

        auto &mgr = p.add_mutually_exclusive_group(true);
        mgr.add_optional("--test1", 0);
        mgr.add_optional("--test2", 0);

        writeln("--- help:");
        p.parse(iter({ "--help" }));

        writeln("\n--- only foo:");
        try {
            p.parse(iter({ "--foo" }));
        } catch (arg_error const &e) {
            writeln("---- error: ", e.what());
        }

        writeln("\n--- only required:");
        p.parse(iter({ "--test1" }));
        writeln("---- (no error)");

        writeln("\n--- mutually exclusive:");
        try {
            p.parse(iter({ "--test1", "--foo", "--bar" }));
        } catch (arg_error const &e) {
            writeln("---- error: ", e.what());
        }

        writeln("\n--- both sets:");
        p.parse(iter({ "--test1", "--foo" }));
        writeln("---- (no error)");
    }
}

/* output:
-- VERY SIMPLE EXAMPLE --

--- without help:
foo invoked: 150
test invoked

--- with help:
Usage: test [opts] [args]

Optional arguments:
  -h, --help         print this message and exit
  -t, --test         test help
  -f FOO, --foo FOO  foo help

-- DIFFERENT SYNTAX --

--- without help:
foo invoked: 150
test invoked

--- with help:
Usage: test [opts] [args]

Optional arguments:
  /h, /help          print this message and exit
  +t, ++test         test help
  /f FOO, ++foo FOO  foo help

-- GROUPS AND OTHER FEATURES --

Usage: test [opts] [args]

Positional arguments:
  foo                                                          a positional arg
  bar                                                          all other arguments

Optional arguments:
  -h, --help                                                   print this message and exit

Group 1:
  -x, --test1                                                  test1 help
  -y TEST2, --test2 TEST2                                      test2 help
  -z [TEST3], --test3 [TEST3]                                  test3 help

Group 2:
  -x, --test1                                                  test1 help
  -y TEST2, --test2 TEST2                                      test2 help
  -z [TEST3], --test3 [TEST3]                                  test3 help
  -a [TEST4 ...], --test4 [TEST4 ...]                          test4 help
  -b TEST5 [TEST5 ...], --test5 TEST5 [TEST5 ...]              test5 help
  -c TEST6 TEST6 [TEST6 ...], --test6 TEST6 TEST6 [TEST6 ...]  test6 help

-- MUTUAL EXCLUSION --

--- help:
Usage: test [opts] [args]

Optional arguments:
  -h, --help  print this message and exit
  --foo
  --bar
  --test1
  --test2

--- only foo:
---- error: one of the arguments '--test1', '--test2' is required

--- only required:
---- (no error)

--- mutually exclusive:
---- error: argument '--bar' not allowed with argument '--foo'

--- both sets:
---- (no error)
*/
