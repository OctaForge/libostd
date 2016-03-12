#include <stdio.h>
#include <stdlib.h>

#include <ostd/platform.hh>
#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/map.hh>
#include <ostd/filesystem.hh>

using namespace ostd;

ConstCharRange get_env(ConstCharRange var, ConstCharRange def = nullptr) {
    const char *ret = getenv(String(var).data());
    if (!ret || !ret[0])
        return def;
    return ret;
}

int main() {
    ConstCharRange compiler = get_env("CXX", "c++");
    ConstCharRange cxxflags = "-std=c++14 -Wall -Wextra -Wshadow "
                              "-Wno-missing-braces " /* clang false positive */
                              "-I.";
    ConstCharRange testdir = get_env("TESTDIR", "tests");
    ConstCharRange srcext = ".cc";

    Map<ConstCharRange, ConstCharRange> colors = {
#ifndef OSTD_PLATFORM_WIN32
        { "red", "\033[91m" },
        { "green", "\033[92m" },
        { "blue", "\033[94m" },
        { "bold", "\033[1m" },
        { "end", "\033[0m" }
#else
        { "red", "" },
        { "green", "" },
        { "blue", "" },
        { "bold", "" },
        { "end", "" }
#endif
    };

    String cxxf = cxxflags;
    cxxf += get_env("CXXFLAGS", "");

    int nsuccess = 0, nfailed = 0;

    auto print_result = [&colors, &nsuccess, &nfailed]
                        (ConstCharRange modname, ConstCharRange fmsg = nullptr) {
        if (!fmsg.empty()) {
            writeln(modname, "...\t", colors["red"], colors["bold"],
                    "(", fmsg, ")", colors["end"]);
            ++nfailed;
        } else {
            writeln(modname, "...\t", colors["green"], colors["bold"],
                    "(success)", colors["end"]);
            ++nsuccess;
        }
    };

    DirectoryStream ds(testdir);
    for (auto v: ds.iter()) {
        if (v.type() != FileType::regular)
            continue;
        if (v.extension() != srcext)
            continue;

        String exepath = testdir;
        exepath += PathSeparator;
        exepath += v.stem();

        String cxxcmd = compiler;
        cxxcmd += " ";
        cxxcmd += testdir;
        cxxcmd += PathSeparator;
        cxxcmd += v.filename();
        cxxcmd += " -o ";
        cxxcmd += exepath;
        cxxcmd += " ";
        cxxcmd += cxxf;

        int ret = system(cxxcmd.data());
        if (ret) {
            print_result(v.stem(), "compile errror");
            continue;
        }

        ret = system(exepath.data());
        if (ret) {
            print_result(v.stem(), "runtime error");
            continue;
        }

        remove(exepath.data());
        print_result(v.stem());
    }

    writeln("\n", colors["blue"], colors["bold"], "testing done:", colors["end"]);
    writeln(colors["green"], "SUCCESS: ", nsuccess, colors["end"]);
    writeln(colors["red"], "FAILURE: ", nfailed, colors["end"]);
}
