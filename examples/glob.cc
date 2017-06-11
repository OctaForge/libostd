/** @example glob.cc
 *
 * An example of using glob patterns,
 */

#include <vector>

#include <ostd/io.hh>
#include <ostd/filesystem.hh>

using namespace ostd;

int main() {
    writeln("-- all example sources (examples/*.cc) --\n");
    {
        auto app = appender<std::vector<filesystem::path>>();
        glob_match(app, "examples/*.cc");

        for (auto &ex: app.get()) {
            writefln("found: %s", ex);
        }
    }
    writeln("\n-- recursive source files (src/**/*.cc) --\n");
    {
        auto app = appender<std::vector<filesystem::path>>();
        glob_match(app, "src/**/*.cc");

        for (auto &ex: app.get()) {
            writefln("found: %s", ex);
        }
    }
    writeln("\n-- 5-character headers (ostd/?????.hh) --\n");
    {
        auto app = appender<std::vector<filesystem::path>>();
        glob_match(app, "ostd/?????.hh");

        for (auto &ex: app.get()) {
            writefln("found: %s", ex);
        }
    }
    writeln("\n-- examples starting with f-r (examples/[f-r]*.cc) --\n");
    {
        auto app = appender<std::vector<filesystem::path>>();
        glob_match(app, "examples/[f-r]*.cc");

        for (auto &ex: app.get()) {
            writefln("found: %s", ex);
        }
    }
    writeln("\n-- examples not starting with f-r (examples/[!f-r]*.cc) --\n");
    {
        auto app = appender<std::vector<filesystem::path>>();
        glob_match(app, "examples/[!f-r]*.cc");

        for (auto &ex: app.get()) {
            writefln("found: %s", ex);
        }
    }
    writeln("\n-- headers starting with c, f or s (ostd/[cfs]*.hh) --\n");
    {
        auto app = appender<std::vector<filesystem::path>>();
        glob_match(app, "ostd/[cfs]*.hh");

        for (auto &ex: app.get()) {
            writefln("found: %s", ex);
        }
    }
}

// output:
// -- all example sources (examples/*.cc) --
//
// found: examples/format.cc
// found: examples/stream2.cc
// found: examples/stream1.cc
// found: examples/glob.cc
// found: examples/coroutine1.cc
// found: examples/range.cc
// found: examples/signal.cc
// found: examples/argparse.cc
// found: examples/range_pipe.cc
// found: examples/listdir.cc
// found: examples/concurrency.cc
// found: examples/coroutine2.cc
//
// -- recursive source files (src/**/*.cc) --
//
// found: src/context_stack.cc
// found: src/io.cc
// found: src/process.cc
// found: src/concurrency.cc
// found: src/environ.cc
// found: src/posix/context_stack.cc
// found: src/posix/process.cc
// found: src/win32/context_stack.cc
// found: src/win32/process.cc
//
// -- 5-character headers (ostd/?????.hh) --
//
// found: ostd/range.hh
// found: ostd/event.hh
//
// -- examples starting with f-r (examples/[f-r]*.cc) --
//
// found: examples/format.cc
// found: examples/glob.cc
// found: examples/range.cc
// found: examples/range_pipe.cc
// found: examples/listdir.cc
//
// -- examples not starting with f-r (examples/[!f-r]*.cc) --
//
// found: examples/stream2.cc
// found: examples/stream1.cc
// found: examples/coroutine1.cc
// found: examples/signal.cc
// found: examples/argparse.cc
// found: examples/concurrency.cc
// found: examples/coroutine2.cc
//
// -- headers starting with c, f or s (ostd/[cfs]*.hh) --
//
// found: ostd/context_stack.hh
// found: ostd/channel.hh
// found: ostd/format.hh
// found: ostd/string.hh
// found: ostd/stream.hh
// found: ostd/concurrency.hh
// found: ostd/coroutine.hh
// found: ostd/filesystem.hh
