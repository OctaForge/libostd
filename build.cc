/* A build system for libostd. */

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

#include <unistd.h>
#include <wordexp.h>
#include <sys/wait.h>

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

static void exec_command(
    std::string const &cmd, std::vector<std::string> const &args
) {
    auto argp = std::make_unique<char *[]>(args.size() + 2);
    argp[0] = const_cast<char *>(cmd.data());
    for (std::size_t i = 0; i < args.size(); ++i) {
        argp[i + 1] = const_cast<char *>(args[i].data());
    }
    argp[args.size() + 1] = nullptr;

    auto pid = fork();
    if (pid == -1) {
        std::printf("command failed\n");
        argp.reset();
        std::exit(1);
    } else if (!pid) {
        if (!execvp(argp[0], argp.get())) {
            argp.reset();
            std::exit(1);
        }
    } else {
        if (int retc = -1; (waitpid(pid, &retc, 0) == -1) || retc) {
            std::printf("command failed with code %d\n", retc);
            argp.reset();
            std::exit(1);
        }
    }
}

static void print_command(
    std::string const &cmd, std::vector<std::string> const &args
) {
    std::printf("%s", cmd.data());
    for (auto &s: args) {
        std::printf(" %s", s.data());
    }
    std::printf("\n");
}

static void add_args(std::vector<std::string> &args, std::string const &cmdl) {
    wordexp_t p;
    if (wordexp(cmdl.data(), &p, 0)) {
        return;
    }
    for (std::size_t i = 0; i < p.we_wordc; ++i) {
        args.push_back(p.we_wordv[i]);
    }
}

static fs::path path_with_ext(fs::path const &p, fs::path const &ext) {
    fs::path rp = p;
    rp.replace_extension(ext);
    return rp;
}

static void try_remove(fs::path const &path) {
    try {
       fs::remove(path);
    } catch (fs::filesystem_error const &) {}
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
    auto as    = from_env_or("AS", "c++");
    auto ar    = from_env_or("AR", "ar");

    add_cross(strip);
    add_cross(cxx);
    add_cross(as);
    add_cross(ar);

    if (!std::strcmp(build_cfg, "debug")) {
        cxxflags += " -g";
    }

    add_env(cxxflags, "CXXFLAGS");
    add_env(ldflags, "LDFLAGS");
    add_env(asflags, "ASFLAGS");

    auto echo_q = [verbose](auto const &...args) {
        if (!verbose) {
            std::printf(args...);
        }
    };

    auto exec_v = [verbose](
        std::string const &cmd, std::vector<std::string> const &args
    ) {
        if (verbose) {
            print_command(cmd, args);
        }
        exec_command(cmd, args);
    };

    if (clean) {
        std::printf("Cleaning...\n");

        fs::path exp = "examples";
        for (auto ex: EXAMPLES) {
            auto rp = exp / ex;
            try_remove(rp);
            rp.replace_extension(".o");
            try_remove(rp);
        }
        fs::path asp = ASM_SOURCE_DIR;
        for (auto aso: ASM_SOURCES) {
            auto rp = asp / aso;
            rp.replace_extension(".o");
            try_remove(rp);
            std::string fname = aso;
            fname += "_dyn.o";
            rp.replace_filename(fname);
            try_remove(rp);
        }
        fs::path csp = CXX_SOURCE_DIR;
        for (auto cso: CXX_SOURCES) {
            auto rp = csp / cso;
            rp.replace_extension(".o");
            try_remove(rp);
            std::string fname = cso;
            fname += "_dyn.o";
            rp.replace_filename(fname);
            try_remove(rp);
        }
        try_remove(OSTD_STATIC_LIB);
        try_remove(OSTD_SHARED_LIB);
        try_remove("test_runner.o");
        try_remove("test_runner");
        try {
            fs::remove_all("tests");
        } catch (fs::filesystem_error const &) {}

        return 0;
    }

    auto call_cxx = [&](
        std::string const &input, std::string const &output, bool shared
    ) {
        std::vector<std::string> args;
        add_args(args, cxxflags);

        if (shared) {
            echo_q("CXX (shared): %s\n", input.data());
            args.push_back("-fPIC");
        } else {
            echo_q("CXX: %s\n", input.data());
        }

        args.push_back("-c");
        args.push_back("-o");
        args.push_back(output);
        args.push_back(input);

        exec_v(cxx, args);
    };

    /* mostly unnecessary to separately compile shared, but
     * the files may check for __PIC__ (at least mips32 does)
     */
    auto call_as = [&](
        std::string const &input, std::string const &output, bool shared
    ) {
        std::vector<std::string> args;
        add_args(args, asflags);

        if (shared) {
            echo_q("AS (shared): %s\n", input.data());
            args.push_back("-fPIC");
        } else {
            echo_q("AS: %s\n", input.data());
        }

        args.push_back("-c");
        args.push_back("-o");
        args.push_back(output);
        args.push_back(input);

        exec_v(as, args);
    };

    auto call_ld = [&](
        std::string const &output,
        std::vector<std::string> const &files,
        std::vector<std::string> const &flags
    ) {
        echo_q("LD: %s\n", output.data());

        std::vector<std::string> args;
        add_args(args, cxxflags);

        args.push_back("-o");
        args.push_back(output);
        args.insert(args.cend(), files.begin(), files.end());
        args.insert(args.cend(), flags.begin(), flags.end());

        add_args(args, ldflags);

        exec_v(cxx, args);

        if (!std::strcmp(build_cfg, "release")) {
            args.clear();
            args.push_back(output);
            exec_v(strip, args);
        }
    };

    auto call_ldlib = [&](
        std::string const &output, std::vector<std::string> const &files,
        bool shared
    ) {
        if (shared) {
            call_ld(output, files, { "-shared" });
        } else {
            echo_q("AR: %s\n", output.data());

            std::vector<std::string> args;
            args.push_back("rcs");
            args.push_back(output);
            args.insert(args.cend(), files.begin(), files.end());

            exec_v(ar, args);
        }
    };

    auto build_example = [&](std::string const &name) {
        auto base = fs::path{"examples"} / name;
        auto ccf = path_with_ext(base, ".cc");
        auto obf = path_with_ext(base, ".o");

        call_cxx(ccf.string(), obf.string(), false);
        call_ld(base.string(), { obf.string() }, { default_lib });

        try_remove(obf);
    };

    auto build_test = [&](std::string const &name) {
        auto base = fs::path{TEST_DIR} / name;
        auto ccf = path_with_ext(base, ".cc");
        auto obf = path_with_ext(base, ".o");

        try_remove(ccf);
        auto f = std::fopen(ccf.string().data(), "w");
        if (!f) {
            std::printf("cannot open '%s' for writing\n", ccf.string().data());
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
            name.data(), name.data()
        );
        std::fclose(f);

        call_cxx(ccf.string(), obf.string(), false);
        call_ld(base.string(), { obf.string() }, { default_lib });
        try_remove(obf);
    };

    auto build_test_runner = [&]() {
        call_cxx("test_runner.cc", "test_runner.o", false);
        call_ld("test_runner", { "test_runner.o" }, { default_lib });
        try_remove("test_runner.o");
    };

    std::vector<std::string> asm_obj, cxx_obj, asm_dynobj, cxx_dynobj;

    ostd::thread_pool tp;
    tp.start();

    /* build object file in static and shared (PIC) variants */
    auto build_obj = [&tp, build_static, build_shared](
        fs::path const &fpath, fs::path const &sext, auto &buildf,
        auto &obj, auto &dynobj
    ) {
        auto srcf = path_with_ext(fpath, sext);
        if (build_static) {
            auto srco = path_with_ext(srcf, ".o");
            tp.push([&, srcf, srco]() {
                buildf(srcf.string(), srco.string(), false);
            });
            obj.push_back(srco.string());
        }
        if (build_shared) {
            auto srco = srcf;
            srco.replace_extension();
            srco += "_dyn.o";
            tp.push([&, srcf, srco]() {
                buildf(srcf.string(), srco.string(), true);
            });
            dynobj.push_back(srco.string());
        }
    };

    std::printf("Building the library...\n");
    for (auto aso: ASM_SOURCES) {
        build_obj(
            fs::path{ASM_SOURCE_DIR} / aso, ".S",
            call_as, asm_obj, asm_dynobj
        );
    }
    for (auto cso: CXX_SOURCES) {
        build_obj(
            fs::path{CXX_SOURCE_DIR} / cso, ".cc",
            call_cxx, cxx_obj, cxx_dynobj
        );
    }

    /* excessive, but whatever... maybe add better
     * sync later without restarting all the threads
     */
    tp.destroy();

    if (build_static) {
        std::vector<std::string> all_obj;
        all_obj.insert(all_obj.cend(), asm_obj.begin(), asm_obj.end());
        all_obj.insert(all_obj.cend(), cxx_obj.begin(), cxx_obj.end());
        call_ldlib(OSTD_STATIC_LIB, all_obj, false);
    }
    if (build_shared) {
        std::vector<std::string> all_obj;
        all_obj.insert(all_obj.cend(), asm_dynobj.begin(), asm_dynobj.end());
        all_obj.insert(all_obj.cend(), cxx_dynobj.begin(), cxx_dynobj.end());
        call_ldlib(OSTD_SHARED_LIB, all_obj, true);
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
        exec_v("./test_runner", { TEST_DIR });
    }

    return 0;
}
