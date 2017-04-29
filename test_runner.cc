#include <cstddef>
#include <unordered_map>

#include <ostd/platform.hh>
#include <ostd/range.hh>
#include <ostd/format.hh>
#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/filesystem.hh>

using namespace ostd;

#ifndef OSTD_PLATFORM_WIN32
constexpr auto COLOR_RED = "\033[91m";
constexpr auto COLOR_GREEN = "\033[92m";
constexpr auto COLOR_BLUE = "\033[94m";
constexpr auto COLOR_BOLD = "\033[1m";
constexpr auto COLOR_END = "\033[0m";
#else
constexpr auto COLOR_RED = "";
constexpr auto COLOR_GREEN = "";
constexpr auto COLOR_BLUE = "";
constexpr auto COLOR_BOLD = "";
constexpr auto COLOR_END = "";
#define popen _popen
#endif

static void write_padded(string_range s, std::size_t n) {
    write(s, "...");
    if ((s.size() + 3) >= n) {
        return;
    }
    for (size_t i = 0; i < (n - (s.size() + 3)); ++i) {
        write(' ');
    }
}

int main(int argc, char **argv) {
    /* configurable section */
    char const *testdir = "tests";
    if (argc > 1) {
        testdir = argv[1];
    }

    /* do not change past this point */

    std::size_t nsuccess = 0, nfailed = 0;

    auto print_error = [&](string_range modname) {
        write_padded(modname, 20);
        writeln(COLOR_RED, COLOR_BOLD, "(runtime error)", COLOR_END);
        ++nfailed;
    };

    filesystem::directory_iterator ds{testdir};
    for (auto &v: ds) {
        auto p = filesystem::path{v};
        if (!filesystem::is_regular_file(p)) {
            continue;
        }
#ifdef OSTD_PLATFORM_WIN32
        if (p.extension().string() != ".exe")
#else
        if (p.extension().string() != "")
#endif
        {
            continue;
        }
        auto modname = p.stem().string();
        auto exepath = p.string();

        auto rf = popen(exepath.data(), "r");
        if (!rf) {
            print_error(modname);
            continue;
        }

        int succ = 0, fail = 0;
        if (fscanf(rf, "%d %d", &succ, &fail) != 2) {
            pclose(rf);
            print_error(modname);
            continue;
        }

        if (pclose(rf)) {
            print_error(modname);
            continue;
        }

        write_padded(modname, 20);
        writefln(
            "%s%s%d out of %d%s (%d failures)",
            fail ? COLOR_RED : COLOR_GREEN, COLOR_BOLD,
            succ, succ + fail, COLOR_END, fail
        );

        if (fail) {
            ++nfailed;
        } else {
            ++nsuccess;
        }
    }

    writeln("\n", COLOR_BLUE, COLOR_BOLD, "testing done:", COLOR_END);
    writeln(COLOR_GREEN, "SUCCESS: ", int(nsuccess), COLOR_END);
    writeln(COLOR_RED, "FAILURE: ", int(nfailed), COLOR_END);
}
