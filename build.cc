/* A build system for libostd. */

/* for Windows so that we avoid dllimport/dllexport */
#define OSTD_BUILD_LIB

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

#include "ostd/io.hh"
#include "ostd/range.hh"
#include "ostd/format.hh"
#include "ostd/string.hh"
#include "ostd/filesystem.hh"
#include "ostd/thread_pool.hh"
#include "ostd/channel.hh"
#include "ostd/argparse.hh"

namespace fs = ostd::filesystem;

/* ugly, but do not explicitly compile */
#include "src/io.cc"
#include "src/process.cc"

using strvec = std::vector<std::string>;
using pathvec = std::vector<fs::path>;

/* THESE VARIABLES CAN BE ALTERED */

static pathvec EXAMPLES = {
    "format", "listdir", "range", "range_pipe", "signal",
    "stream1", "stream2", "coroutine1", "coroutine2", "concurrency",
    "argparse"
};

static fs::path ASM_SOURCE_DIR = fs::path{"src"} / "asm";
static pathvec ASM_SOURCES = {
    "jump_all_gas", "make_all_gas", "ontop_all_gas"
};

static fs::path CXX_SOURCE_DIR = "src";
static pathvec CXX_SOURCES = {
    "context_stack", "environ", "io", "concurrency", "process"
};

static fs::path TEST_DIR = "tests";
static pathvec TEST_CASES = {
    "algorithm", "range"
};

static fs::path OSTD_SHARED_LIB = "libostd.so";
static fs::path OSTD_STATIC_LIB = "libostd.a";

static std::string DEFAULT_CXXFLAGS = "-std=c++1z -I. -O2 -Wall -Wextra "
                                      "-Wshadow -Wold-style-cast";
static std::string DEFAULT_LDFLAGS  = "-pthread";
static std::string DEFAULT_ASFLAGS  = "";

static std::string DEBUG_CXXFLAGS = "-g";

static std::string SHARED_CXXFLAGS = "-fPIC";
static std::string SHARED_LDFLAGS  = "-shared";
static std::string SHARED_ASFLAGS  = "-fPIC";

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

static void exec_command(strvec const &args) {
    if (int ret = ostd::subprocess{nullptr, ostd::iter(args)}.close(); ret) {
        throw std::runtime_error{ostd::format(
            ostd::appender<std::string>(), "command failed with code %d", ret
        ).get()};
    }
}

static std::string get_command(strvec const &args) {
    std::string ret;
    for (auto &s: args) {
        if (!ret.empty()) {
            ret += ' ';
        }
        ret += s;
    }
    return ret;
}

static void add_args(strvec &args, std::string const &cmdl) {
    ostd::split_args([&args](ostd::string_range arg) {
        args.emplace_back(arg);
    }, cmdl);
}

static fs::path path_with_ext(fs::path const &p, fs::path const &ext) {
    fs::path rp = p;
    rp.replace_extension(ext);
    return rp;
}

static void try_remove(fs::path const &path, bool all = false) {
    try {
        if (all) {
            fs::remove_all(path);
        } else {
            fs::remove(path);
        }
    } catch (fs::filesystem_error const &) {}
}

static bool verbose = false;

int main(int argc, char **argv) {
    bool build_examples = true;
    bool build_testsuite = true;
    bool build_static = true;
    bool build_shared = false;
    std::string build_cfg = "debug";
    bool clean = false;

    std::string cxxflags = DEFAULT_CXXFLAGS;
    std::string ldflags  = DEFAULT_LDFLAGS;
    std::string asflags  = DEFAULT_ASFLAGS;

    ostd::arg_parser ap;

    auto &help = ap.add_optional("-h", "--help", 0)
        .help("print this message and exit")
        .action(ostd::arg_print_help(ap));

    ap.add_optional("--no-examples", 0)
        .help("do not build examples")
        .action(ostd::arg_store_false(build_examples));

    ap.add_optional("--no-test-suite", 0)
        .help("do not build test suite")
        .action(ostd::arg_store_false(build_testsuite));

    ap.add_optional("--no-static", 0)
        .help("do not build static libostd")
        .action(ostd::arg_store_false(build_static));

    ap.add_optional("--shared", 0)
        .help("build shared libostd")
        .action(ostd::arg_store_true(build_shared));

    ap.add_optional("--config", 1)
        .help("build configuration (debug/release)")
        .action(ostd::arg_store_str(build_cfg));

    ap.add_optional("-v", "--verbose", 0)
        .help("print entire commands")
        .action(ostd::arg_store_true(verbose));

    ap.add_positional("target", ostd::arg_value::OPTIONAL)
        .help("the action to perform")
        .action([&clean](auto vals) {
            if (!vals.empty()) {
                if (vals.front() == "clean") {
                    clean = true;
                } else if (vals.front() != "build") {
                    throw ostd::arg_error{"invalid build action"};
                }
            }
        });

    try {
        ap.parse(argc, argv);
    } catch (ostd::arg_error const &e) {
        ostd::cerr.writefln("argument parsing failed: %s", e.what());
        ap.print_help(ostd::cerr.iter());
        return 1;
    }

    if (help.used()) {
        return 0;
    }

    std::string default_lib = OSTD_SHARED_LIB.string();
    if (build_static) {
        default_lib = OSTD_STATIC_LIB.string();
    }

    auto strip = from_env_or("STRIP", "strip");
    auto cxx   = from_env_or("CXX", "c++");
    auto as    = from_env_or("AS", "c++");
    auto ar    = from_env_or("AR", "ar");

    add_cross(strip);
    add_cross(cxx);
    add_cross(as);
    add_cross(ar);

    if (build_cfg == "debug") {
        cxxflags += ' ';
        cxxflags += DEBUG_CXXFLAGS;
    }

    add_env(cxxflags, "CXXFLAGS");
    add_env(ldflags, "LDFLAGS");
    add_env(asflags, "ASFLAGS");

    if (clean) {
        ostd::writeln("Cleaning...");

        for (auto &ex: EXAMPLES) {
            auto rp = "examples" / ex;
            try_remove(rp);
            rp.replace_extension(".o");
            try_remove(rp);
        }
        for (auto &aso: ASM_SOURCES) {
            auto rp = path_with_ext(ASM_SOURCE_DIR / aso, ".o");
            try_remove(rp);
            rp.replace_filename(aso.string() + "_dyn.o");
            try_remove(rp);
        }
        for (auto &cso: CXX_SOURCES) {
            auto rp = path_with_ext(CXX_SOURCE_DIR / cso, ".o");
            try_remove(rp);
            rp.replace_filename(cso.string() + "_dyn.o");
            try_remove(rp);
        }
        try_remove(OSTD_STATIC_LIB);
        try_remove(OSTD_SHARED_LIB);
        try_remove("test_runner.o");
        try_remove("test_runner");
        try_remove("tests", true);

        return 0;
    }

    /* a queue of stuff to print to stdout */
    ostd::channel<std::string> io_msgs;

    /* a thread which reads from the queue */
    std::thread io_thread{[&io_msgs]() {
        try {
            for (;;) {
                /* wait for a msg; if closed, throw channel_error */
                ostd::writeln(io_msgs.get());
            }
        } catch (ostd::channel_error const &) {
            /* the queue is empty and closed, thread is done */
            return;
        }
    }};

    auto echo_q = [&io_msgs](ostd::string_range fmt, auto const &...args) {
        if (!verbose) {
            auto app = ostd::appender<std::string>();
            ostd::format(app, fmt, args...);
            io_msgs.put(std::move(app.get()));
        }
    };

    auto exec_v = [&io_msgs](strvec const &args) {
        if (verbose) {
            io_msgs.put(get_command(args));
        }
        exec_command(args);
    };

    auto call_cxx = [&](
        fs::path const &input, fs::path const &output, bool lib, bool shared
    ) {
        strvec args = { cxx };
        add_args(args, cxxflags);

        auto ifs = input.string();
        auto outp = output;

        if (shared) {
            outp.replace_extension();
            outp += "_dyn.o";
            echo_q("CXX (shared): %s", ifs);
            add_args(args, SHARED_CXXFLAGS);
        } else {
            echo_q("CXX: %s", ifs);
        }

        if (lib) {
            args.push_back("-DOSTD_BUILD_LIB");
            if (shared) {
                args.push_back("-DOSTD_BUILD_DLL");
            }
        }

        args.push_back("-c");
        args.push_back("-o");
        args.push_back(outp.string());
        args.push_back(ifs);

        exec_v(args);

        return outp;
    };

    /* mostly unnecessary to separately compile shared, but
     * the files may check for __PIC__ (at least mips32 does)
     */
    auto call_as = [&](
        fs::path const &input, fs::path const &output, bool, bool shared
    ) {
        strvec args = { as };
        add_args(args, asflags);

        auto ifs = input.string();
        auto outp = output;

        if (shared) {
            outp.replace_extension();
            outp += "_dyn.o";
            echo_q("AS (shared): %s", ifs);
            add_args(args, SHARED_ASFLAGS);
        } else {
            echo_q("AS: %s", ifs);
        }

        args.push_back("-c");
        args.push_back("-o");
        args.push_back(outp.string());
        args.push_back(ifs);

        exec_v(args);

        return outp;
    };

    auto call_ld = [&](
        fs::path const &output, pathvec const &files, strvec const &flags
    ) {
        echo_q("LD: %s", output);

        strvec args = { cxx };
        add_args(args, cxxflags);

        args.push_back("-o");
        args.push_back(output.string());
        for (auto &p: files) {
            args.push_back(p.string());
        }
        args.insert(args.cend(), flags.begin(), flags.end());

        add_args(args, ldflags);

        exec_v(args);

        if (build_cfg == "release") {
            args.clear();
            args.push_back(strip);
            args.push_back(output.string());
            exec_v(args);
        }
    };

    auto call_ldlib = [&](
        fs::path const &output, pathvec const &files, bool shared
    ) {
        if (shared) {
            strvec flags;
            add_args(flags, SHARED_CXXFLAGS);
            add_args(flags, SHARED_LDFLAGS);
            call_ld(output, files, flags);
        } else {
            strvec args = { ar };
            echo_q("AR: %s", output);

            args.push_back("rcs");
            args.push_back(output.string());
            for (auto &p: files) {
                args.push_back(p.string());
            }

            exec_v(args);
        }
    };

    auto build_example = [&](fs::path const &name) {
        auto base = "examples" / name;
        auto ccf = path_with_ext(base, ".cc");
        auto obf = path_with_ext(base, ".o");

        call_cxx(ccf, obf, false, false);
        call_ld(base, { obf }, { default_lib });

        try_remove(obf);
    };

    auto build_test = [&](fs::path const &name) {
        auto base = TEST_DIR / name;
        auto ccf = path_with_ext(base, ".cc");
        auto obf = path_with_ext(base, ".o");

        try_remove(ccf);
        ostd::file_stream f{ccf.string(), ostd::stream_mode::WRITE};
        if (!f.is_open()) {
            auto app = ostd::appender<std::string>();
            ostd::format(app, "cannot open '%s' for writing", ccf);
            throw std::runtime_error{app.get()};
        }
        f.writef(
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
        f.close();

        call_cxx(ccf, obf, false, false);
        call_ld(base, { obf }, { default_lib });
        try_remove(obf);
    };

    auto build_test_runner = [&]() {
        call_cxx("test_runner.cc", "test_runner.o", false, false);
        call_ld("test_runner", { "test_runner.o" }, { default_lib });
        try_remove("test_runner.o");
    };

    ostd::thread_pool tp;
    tp.start();

    std::queue<std::future<fs::path>> future_obj, future_dynobj;

    /* build object files in static and shared (PIC) variants */
    auto build_all = [&](
        pathvec const &list, fs::path const &spath,
        fs::path const &sext, auto &buildf
    ) {
        auto build_obj = [&](fs::path const &fpath,  bool shared) {
            auto srcf = path_with_ext(fpath, sext);
            auto srco = path_with_ext(srcf, ".o");
            auto &fq = (shared ? future_dynobj : future_obj);
            fq.push(tp.push([&buildf, srcf, srco, shared]() {
                return buildf(srcf, srco, true, shared);
            }));
        };
        for (auto &sf: list) {
            auto sp = spath / sf;
            if (build_static) {
                build_obj(sp, false);
            }
            if (build_shared) {
                build_obj(sp, true);
            }
        }
    };

    echo_q("Building the library...");
    build_all(ASM_SOURCES, ASM_SOURCE_DIR, ".S", call_as);
    build_all(CXX_SOURCES, CXX_SOURCE_DIR, ".cc", call_cxx);

    if (build_static) {
        pathvec objs;
        while (!future_obj.empty()) {
            objs.push_back(future_obj.front().get());
            future_obj.pop();
        }
        call_ldlib(OSTD_STATIC_LIB, objs, false);
    }
    if (build_shared) {
        pathvec objs;
        while (!future_dynobj.empty()) {
            objs.push_back(future_dynobj.front().get());
            future_dynobj.pop();
        }
        call_ldlib(OSTD_SHARED_LIB, objs, true);
    }

    std::queue<std::future<void>> future_bin;

    if (build_examples) {
        echo_q("Building examples...");
        for (auto &ex: EXAMPLES) {
            future_bin.push(tp.push([&build_example, ex]() {
                build_example(ex);
            }));
        }
    }

    if (build_testsuite) {
        echo_q("Building tests...");
        build_test_runner();
        if (!fs::is_directory(TEST_DIR)) {
            if (!fs::create_directory(TEST_DIR)) {
                echo_q("Failed creating test directory");
                return 1;
            }
        }
        for (auto &test: TEST_CASES) {
            future_bin.push(tp.push([&build_test, test]() {
                build_test(test);
            }));
        }
    }

    while (!future_bin.empty()) {
        /* wait and propagate possible exception */
        future_bin.front().get();
        future_bin.pop();
    }

    if (build_testsuite) {
        exec_v({ "./test_runner", TEST_DIR.string() });
    }

    io_msgs.close();
    io_thread.join();

    return 0;
}
