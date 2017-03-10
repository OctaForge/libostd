#include <ostd/platform.hh>
#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/unordered_map.hh>
#include <ostd/filesystem.hh>
#include <ostd/environ.hh>

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
#endif

int main() {
    /* configurable section */

    auto compiler = env_get("CXX").value_or("c++");
    auto cxxflags = "-std=c++1z -I. -Wall -Wextra -Wshadow -Wold-style-cast "
                    "-Wno-missing-braces"; /* clang false positive */
    auto testdir = env_get("TESTDIR").value_or("tests");
    auto srcext = ".cc";

    auto userflags = env_get("CXXFLAGS").value_or("");

    /* do not change past this point */

    int nsuccess = 0, nfailed = 0;
    std::string modname = nullptr;

    auto print_result = [&](string_range fmsg = nullptr) {
        write(modname, "...\t");
        if (!fmsg.empty()) {
            writeln(COLOR_RED, COLOR_BOLD, '(', fmsg, ')', COLOR_END);
            ++nfailed;
        } else {
            writeln(COLOR_GREEN, COLOR_BOLD, "(success)", COLOR_END);
            ++nsuccess;
        }
    };

    filesystem::directory_iterator ds{testdir};
    for (auto &v: ds) {
        auto p = filesystem::path{v};
        if (
            (!filesystem::is_regular_file(p)) ||
            (p.extension().string() != srcext)
        ) {
            continue;
        }

        modname = p.stem().string();

        std::string exepath = testdir;
        exepath += char(filesystem::path::preferred_separator);
        exepath += modname;
#ifdef OSTD_PLATFORM_WIN32
        exepath += ".exe";
#endif

        auto cxxcmd = appender<std::string>();
        format(
            cxxcmd, "%s %s%s%s -o %s %s", compiler, testdir,
            char(filesystem::path::preferred_separator),
            p.filename(), exepath, cxxflags
        );
        if (!userflags.empty()) {
            cxxcmd.get() += ' ';
            cxxcmd.get() += userflags;
        }
        int ret = system(cxxcmd.get().data());
        if (ret) {
            print_result("compile errror");
            continue;
        }

        ret = system(exepath.data());
        if (ret) {
            print_result("runtime error");
            continue;
        }

        remove(exepath.data());
        print_result();
    }

    writeln("\n", COLOR_BLUE, COLOR_BOLD, "testing done:", COLOR_END);
    writeln(COLOR_GREEN, "SUCCESS: ", nsuccess, COLOR_END);
    writeln(COLOR_RED, "FAILURE: ", nfailed, COLOR_END);
}
