#include <atomic>
#include <unordered_map>

#include <ostd/platform.hh>
#include <ostd/range.hh>
#include <ostd/format.hh>
#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/filesystem.hh>
#include <ostd/environ.hh>
#include <ostd/thread_pool.hh>

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

#ifndef OSTD_DEFAULT_LIB
#define OSTD_DEFAULT_LIB "libostd.a"
#endif

static void write_test_src(file_stream &s, string_range modname) {
    s.writefln(
        "#define OSTD_BUILD_TESTS libostd_%s\n"
        "\n"
        "#include <ostd/unit_test.hh>\n"
        "#include <ostd/%s.hh>\n"
        "#include <ostd/io.hh>\n"
        "\n"
        "int main() {\n"
        "    auto [ succ, fail ] = ostd::test::run();\n"
        "    ostd::writeln(succ, \" \", fail);\n"
        "    return 0;\n"
        "}",
        modname, modname
    );
}

static void write_padded(string_range s, std::size_t n) {
    write(s, "...");
    if ((s.size() + 3) >= n) {
        return;
    }
    for (size_t i = 0; i < (n - (s.size() + 3)); ++i) {
        write(' ');
    }
}

int main() {
    /* configurable section */

    auto compiler = env_get("CXX").value_or("c++");
    auto cxxflags = "-std=c++1z -I. -Wall -Wextra -Wshadow -Wold-style-cast "
                    "-Wno-missing-braces"; /* clang false positive */

    auto userflags = env_get("CXXFLAGS").value_or("");

    /* do not change past this point */

    std::atomic_int nsuccess = 0, nfailed = 0;

    auto print_error = [&](string_range modname, string_range fmsg) {
        write_padded(modname, 20);
        writeln(COLOR_RED, COLOR_BOLD, '(', fmsg, ')', COLOR_END);
        ++nfailed;
    };

    ostd::thread_pool tpool;
    tpool.start(std::thread::hardware_concurrency());

    filesystem::directory_iterator ds{"ostd"};
    for (auto &v: ds) {
        auto p = filesystem::path{v};
        if (
            (!filesystem::is_regular_file(p)) ||
            (p.extension().string() != ".hh")
        ) {
            continue;
        }
        tpool.push([&, modname = p.stem().string()]() {
#ifdef OSTD_PLATFORM_WIN32
            std::string exepath = ".\\test_";
            exepath += modname;
            exepath += ".exe";
#else
            std::string exepath = "./test_";
            exepath += modname;
#endif

            auto cxxcmd = appender<std::string>();
            format(cxxcmd, "%s -o %s %s", compiler, exepath, cxxflags);
            if (!userflags.empty()) {
                cxxcmd.get() += ' ';
                cxxcmd.get() += userflags;
            }
            cxxcmd.get() += " -xc++ - -xnone ";
            cxxcmd.get() += OSTD_DEFAULT_LIB;

            /* compile */
            auto cf = popen(cxxcmd.get().data(), "w");
            if (!cf) {
                print_error(modname, "compile errror");
                return;
            }

            file_stream cfs{cf};
            write_test_src(cfs, modname);

            if (pclose(cf)) {
                print_error(modname, "compile errror");
                return;
            }

            /* run */
            auto rf = popen(exepath.data(), "r");
            if (!rf) {
                print_error(modname, "runtime error");
                return;
            }

            int succ = 0, fail = 0;
            if (fscanf(rf, "%d %d", &succ, &fail) != 2) {
                print_error(modname, "runtime error");
            }

            if (pclose(rf)) {
                print_error(modname, "runtime error");
                return;
            }

            write_padded(modname, 20);
            writefln(
                "%s%s%d out of %d%s (%d failures)",
                fail ? COLOR_RED : COLOR_GREEN, COLOR_BOLD,
                succ, succ + fail, COLOR_END, fail
            );

            remove(exepath.data());
            ++nsuccess;
        });
    }

    /* wait for tasks */
    tpool.destroy();

    writeln("\n", COLOR_BLUE, COLOR_BOLD, "testing done:", COLOR_END);
    writeln(COLOR_GREEN, "SUCCESS: ", int(nsuccess), COLOR_END);
    writeln(COLOR_RED, "FAILURE: ", int(nfailed), COLOR_END);
}
