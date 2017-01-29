#include <ostd/platform.hh>
#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/map.hh>
#include <ostd/filesystem.hh>
#include <ostd/environ.hh>

using namespace ostd;

#ifndef OSTD_PLATFORM_WIN32
constexpr auto ColorRed = "\033[91m";
constexpr auto ColorGreen = "\033[92m";
constexpr auto ColorBlue = "\033[94m";
constexpr auto ColorBold = "\033[1m";
constexpr auto ColorEnd = "\033[0m";
#else
constexpr auto ColorRed = "";
constexpr auto ColorGreen = "";
constexpr auto ColorBlue = "";
constexpr auto ColorBold = "";
constexpr auto ColorEnd = "";
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
    ConstCharRange modname = nullptr;

    auto print_result = [&](ConstCharRange fmsg = nullptr) {
        write(modname, "...\t");
        if (!fmsg.empty()) {
            writeln(ColorRed, ColorBold, '(', fmsg, ')', ColorEnd);
            ++nfailed;
        } else {
            writeln(ColorGreen, ColorBold, "(success)", ColorEnd);
            ++nsuccess;
        }
    };

    DirectoryStream ds(testdir);
    for (auto v: iter(ds)) {
        if ((v.type() != FileType::regular) || (v.extension() != srcext))
            continue;

        modname = v.stem();

        String exepath = testdir;
        exepath += PathSeparator;
        exepath += modname;
#ifdef OSTD_PLATFORM_WIN32
        exepath += ".exe";
#endif

        auto cxxcmd = appender<String>();
        format(cxxcmd, "%s %s%s%s -o %s %s", compiler, testdir, PathSeparator,
               v.filename(), exepath, cxxflags);
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

    writeln("\n", ColorBlue, ColorBold, "testing done:", ColorEnd);
    writeln(ColorGreen, "SUCCESS: ", nsuccess, ColorEnd);
    writeln(ColorRed, "FAILURE: ", nfailed, ColorEnd);
}
