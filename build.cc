/* A build system for libostd. */

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>
#include <stdexcept>

/* no dependency on the rest of ostd or the built library, so it's fine */
#include "ostd/thread_pool.hh"

#if __has_include(<filesystem>)
#  include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#  include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#  error "Unsupported platform"
#endif

/* THESE VARIABLES CAN BE ALTERED */

static char const *EXAMPLES[] = {
    "format", "listdir", "range", "range_pipe", "signal",
    "stream1", "stream2", "coroutine1", "coroutine2", "concurrency"
};

static char const *ASM_SOURCE_DIR = "src/asm";
static char const *ASM_SOURCES[] = {
    "jump_all_gas", "make_all_gas", "ontop_all_gas"
};

static char const *CXX_SOURCE_DIR = "src";
static char const *CXX_SOURCES[] = {
    "context_stack", "io", "concurrency"
};

static char const *TEST_DIR = "tests";
static char const *TEST_CASES[] = {
    "algorithm", "range"
};

static char const *OSTD_SHARED_LIB = "libostd.so";
static char const *OSTD_STATIC_LIB = "libostd.a";

/* DO NOT CHANGE PAST THIS POINT */

static std::string from_env_or(char const *evar, char const *defval) {
    char const *varv = std::getenv(evar);
    if (!varv || !varv[0]) {
        return defval;
    }
    return varv;
}

static void add_cross(std::string &arg) {
    char const *cross = std::getenv("CROSS");
    if (!cross || !cross[0]) {
        return;
    }
    arg.insert(0, cross);
}

static void add_env(std::string &val, char const *evar) {
    char const *varv = std::getenv(evar);
    if (!varv || !varv[0]) {
        return;
    }
    val += ' ';
    val += varv;
}

static void print_help(char const *arg0) {
    std::printf(
        "%s [options]\n"
        "Available options:\n"
        "  [no-]examples   - (do not) build examples (default: yes)\n"
        "  [no-]test-suite - (do not) build test suite (default: yes)\n"
        "  [no-]static-lib - (do not) build static libostd (default: yes)\n"
        "  [no-]shared-lib - (do not) build shared libostd (default: no)\n"
        "  release         - release build (strip, no -g)\n"
        "  debug           - debug build (default, no strip, -g)\n"
        "  verbose         - print entire commands\n"
        "  clean           - remove generated files and exit\n"
        "  help            - print this and exit\n",
        arg0
    );
}

int main(int argc, char **argv) {
    bool build_examples = true;
    bool build_testsuite = true;
    bool build_static = true;
    bool build_shared = false;
    char const *build_cfg = "debug";
    bool verbose = false;
    bool clean = false;

    std::string cxxflags = "-std=c++1z -I. -O2 -Wall -Wextra -Wshadow "
                           "-Wold-style-cast";
    std::string cppflags;
    std::string ldflags = "-pthread";
    std::string asflags = "";

#define ARG_BOOL(arg, name, var) \
    if (!std::strcmp(arg, name) || !std::strcmp(arg, "no-" name)) { \
        var = !!std::strncmp(arg, "no-", 3); \
        continue; \
    }

    for (int i = 1; i < argc; ++i) {
        char const *arg = argv[i];
        ARG_BOOL(arg, "examples", build_examples);
        ARG_BOOL(arg, "test-suite", build_testsuite);
        ARG_BOOL(arg, "static-lib", build_static);
        ARG_BOOL(arg, "shared-lib", build_shared);
        if (!std::strcmp(arg, "release") || !std::strcmp(arg, "debug")) {
            build_cfg = arg;
            continue;
        }
        if (!std::strcmp(arg, "verbose")) {
            verbose = true;
            continue;
        }
        if (!std::strcmp(arg, "clean")) {
            clean = true;
            continue;
        }
        if (!std::strcmp(arg, "help")) {
            print_help(argv[0]);
            return 0;
        }
        printf("unknown argument: %s\n", arg);
        return 1;
    }

#undef ARG_BOOL

    char const *default_lib = OSTD_SHARED_LIB;
    if (build_static) {
        default_lib = OSTD_STATIC_LIB;
    }

    auto strip = from_env_or("STRIP", "strip");
    auto cxx   = from_env_or("CXX", "c++");
    auto cpp   = from_env_or("CPP", "cpp");
    auto as    = from_env_or("AS", "as");
    auto ar    = from_env_or("AR", "ar");

    add_cross(strip);
    add_cross(cxx);
    add_cross(cpp);
    add_cross(as);
    add_cross(ar);

    if (!std::strcmp(build_cfg, "debug")) {
        cxxflags += " -g";
    }

    add_env(cxxflags, "CXXFLAGS");
    add_env(cppflags, "CPPFLAGS");
    add_env(ldflags, "LDFLAGS");
    add_env(asflags, "ASFLAGS");

    auto echo_q = [verbose](auto const &...args) {
        if (!verbose) {
            std::printf(args...);
        }
    };

    auto system_v = [verbose](char const *cmd) {
        if (verbose) {
            std::printf("%s\n", cmd);
        }
        int c = std::system(cmd);
        if (c) {
            std::printf("command exited with code %d\n", c);
            std::exit(c);
        }
    };

    if (clean) {
        std::printf("Cleaning...\n");

        auto remove_nt = [](fs::path const &p) {
             try {
                fs::remove(p);
             } catch (fs::filesystem_error const &) {}
        };

        fs::path exp = "examples";
        for (auto ex: EXAMPLES) {
            auto rp = exp / ex;
            remove_nt(rp);
            rp.replace_extension(".o");
            remove_nt(rp);
        }
        fs::path asp = ASM_SOURCE_DIR;
        for (auto aso: ASM_SOURCES) {
            auto rp = asp / aso;
            rp.replace_extension(".o");
            remove_nt(rp);
        }
        fs::path csp = CXX_SOURCE_DIR;
        for (auto cso: CXX_SOURCES) {
            auto rp = csp / cso;
            rp.replace_extension(".o");
            remove_nt(rp);
            std::string fname = cso;
            fname += "_dyn.o";
            rp.replace_filename(fname);
            remove_nt(rp);
        }
        remove_nt(OSTD_STATIC_LIB);
        remove_nt(OSTD_SHARED_LIB);
        remove_nt("test_runner.o");
        remove_nt("test_runner");
        try {
            fs::remove_all("tests");
        } catch (fs::filesystem_error const &) {}

        return 0;
    }

    auto call_cxx = [&](char const *input, char const *output, bool shared) {
        std::string flags = cppflags;
        flags += ' ';
        flags += cxxflags;

        if (shared) {
            echo_q("CXX (shared): %s\n", input);
            flags += " -fPIC";
        } else {
            echo_q("CXX: %s\n", input);
        }

        std::string cmd = cxx;
        cmd += ' ';
        cmd += flags;
        cmd += " -c -o \"";
        cmd += output;
        cmd += "\" \"";
        cmd += input;
        cmd += "\"";

        system_v(cmd.data());
    };

    auto call_as = [&](char const *input, char const *output) {
        echo_q("AS: %s\n", input);

        std::string cmd = cpp;
        cmd += " -x assembler-with-cpp \"";
        cmd += input;
        cmd += "\" | ";
        cmd += as;
        cmd += ' ';
        cmd += asflags;
        cmd += " -o \"";
        cmd += output;
        cmd += "\"";

        system_v(cmd.data());
    };

    auto call_ld = [&](char const *output, char const *files, char const *extra) {
        echo_q("LD: %s\n", output);

        std::string cmd = cxx;
        cmd += ' ';
        cmd += cppflags;
        cmd += ' ';
        cmd += cxxflags;
        cmd += " -o \"";
        cmd += output;
        cmd += "\" ";
        cmd += files;
        cmd += ' ';
        cmd += extra;
        cmd += ' ';
        cmd += ldflags;

        system_v(cmd.data());

        if (!std::strcmp(build_cfg, "release")) {
            cmd = strip;
            cmd += " \"";
            cmd += output;
            cmd += "\"";

            system_v(cmd.data());
        }
    };

    auto call_ldlib = [&](char const *output, char const *files, bool shared) {
        if (shared) {
            call_ld(output, files, "-shared");
        } else {
            echo_q("AR: %s\n", output);

            std::string cmd = ar;
            cmd += " rcs \"";
            cmd += output;
            cmd += "\"";
            cmd += ' ';
            cmd += files;

            system_v(cmd.data());
        }
    };

    auto build_example = [&](char const *name) {
        std::string base = "examples/";
        base += name;

        auto ccf = base + ".cc";
        auto obf = base + ".o";

        call_cxx(ccf.data(), obf.data(), false);
        call_ld(base.data(), obf.data(), default_lib);
        std::remove(obf.data());
    };

    auto build_test = [&](char const *name) {
        std::string base = TEST_DIR;
        base += "/";
        base += name;

        auto ccf = base + ".cc";
        auto obf = base + ".o";

        std::remove(ccf.data());
        auto f = std::fopen(ccf.data(), "w");
        if (!f) {
            std::printf("cannot open '%s' for writing\n", ccf.data());
            std::exit(1);
        }
        fprintf(
            f,
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
            "}\n",
            name, name
        );
        fclose(f);

        call_cxx(ccf.data(), obf.data(), false);
        call_ld(base.data(), obf.data(), default_lib);
        std::remove(obf.data());
    };

    auto build_test_runner = [&]() {
        call_cxx("test_runner.cc", "test_runner.o", false);
        call_ld("test_runner", "test_runner.o", default_lib);
        std::remove("test_runner.o");
    };

    std::string asm_obj, cxx_obj, cxx_dynobj;

    ostd::thread_pool tp;
    tp.start();

    std::printf("Building the library...\n");
    for (auto aso: ASM_SOURCES) {
        std::string base = ASM_SOURCE_DIR;
        base += "/";
        base += aso;
        auto ass = base + ".S";
        auto asob = base + ".o";
        tp.push([&call_as, ass, asob]() {
            call_as(ass.data(), asob.data());
        });
        asm_obj += ' ';
        asm_obj += asob;
    }
    for (auto cso: CXX_SOURCES) {
        std::string base = CXX_SOURCE_DIR;
        base += "/";
        base += cso;
        auto css = base + ".cc";
        if (build_static) {
            auto csob = base + ".o";
            tp.push([&call_cxx, css, csob]() {
                call_cxx(css.data(), csob.data(), false);
            });
            cxx_obj += ' ';
            cxx_obj += csob;
        }
        if (build_shared) {
            auto csob = base + "_dyn.o";
            tp.push([&call_cxx, css, csob]() {
                call_cxx(css.data(), csob.data(), true);
            });
            cxx_dynobj += ' ';
            cxx_dynobj += csob;
        }
    }

    /* excessive, but whatever... maybe add better
     * sync later without restarting all the threads
     */
    tp.destroy();

    if (build_static) {
        std::string all_obj = asm_obj;
        all_obj += ' ';
        all_obj += cxx_obj;
        call_ldlib(OSTD_STATIC_LIB, all_obj.data(), false);
    }
    if (build_shared) {
        std::string all_obj = asm_obj;
        all_obj += ' ';
        all_obj += cxx_dynobj;
        call_ldlib(OSTD_SHARED_LIB, all_obj.data(), true);
    }

    /* examples are parallel */
    tp.start();

    if (build_examples) {
        std::printf("Building examples...\n");
        for (auto ex: EXAMPLES) {
            tp.push([&build_example, ex]() {
                build_example(ex);
            });
        }
    }

    if (build_testsuite) {
        std::printf("Building tests...\n");
        build_test_runner();
        if (!fs::is_directory(TEST_DIR)) {
            if (!fs::create_directory(TEST_DIR)) {
                std::printf("Failed creating test directory\n");
                return 1;
            }
        }
        for (auto test: TEST_CASES) {
            tp.push([&build_test, test]() {
                build_test(test);
            });
        }
    }

    /* wait for tests and examples to be done */
    tp.destroy();

    if (build_testsuite) {
        std::string cmd = "./test_runner ";
        cmd += TEST_DIR;
        system_v(cmd.data());
    }

    return 0;
}
