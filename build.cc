/* A simple build system to build libostd.
 *
 * This file is a part of the libostd project. Libostd is licensed under the
 * University of Illinois/NCSA Open Source License, as is this file. See the
 * COPYING.md file further information.
 */

/* just glibc bein awful */
#define _FILE_OFFSET_BITS 64

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

#include "ostd/io.hh"
#include "ostd/range.hh"
#include "ostd/format.hh"
#include "ostd/string.hh"
#include "ostd/path.hh"
#include "ostd/thread_pool.hh"
#include "ostd/channel.hh"
#include "ostd/argparse.hh"

namespace fs = ostd::fs;
using path = ostd::path;

/* ugly, but do not explicitly compile */
#include "src/io.cc"
#include "src/process.cc"
#include "src/path.cc"
#include "src/thread_pool.cc"
#include "src/channel.cc"
#include "src/string.cc"
#include "src/argparse.cc"

#define OSTD_GEN_UNICODE_INCLUDE
#include "gen_unicode.cc"

using strvec = std::vector<std::string>;
using pathvec = std::vector<path>;

/* THESE VARIABLES CAN BE ALTERED */

/* all examples in the directory are built */
static path EXAMPLES_DIR = "examples";

static path ASM_SOURCE_DIR = path{"src"} / "asm";
static pathvec ASM_SOURCES = {
    "jump_all_gas", "make_all_gas", "ontop_all_gas"
};

/* all sources in the directory are built */
static path CXX_SOURCE_DIR = "src";

static path TEST_DIR = "tests";
static pathvec TEST_CASES = {
    "algorithm", "range"
};

static path OSTD_UNICODE_DATA = "data/UnicodeData-10.0.txt";
static path OSTD_UNICODE_SRC  = CXX_SOURCE_DIR / "string_utf.hh";

static path OSTD_SHARED_LIB = "libostd.so";
static path OSTD_STATIC_LIB = "libostd.a";

static std::string DEFAULT_CXXFLAGS = "-std=c++1z -I. -O2 -Wall -Wextra "
                                      "-Wshadow -Wold-style-cast -fPIC "
                                      "-D_FILE_OFFSET_BITS=64";
static std::string DEFAULT_LDFLAGS  = "-pthread";
static std::string DEFAULT_ASFLAGS  = "-fPIC";

static std::string DEBUG_CXXFLAGS = "-g";

static std::string SHARED_CXXFLAGS = "";
static std::string SHARED_LDFLAGS  = "-shared";
static std::string SHARED_ASFLAGS  = "";

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

static void try_remove(path const &path, bool all = false) {
    try {
        if (all) {
            fs::remove_all(path);
        } else {
            fs::remove(path);
        }
    } catch (fs::fs_error const &) {}
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

    auto default_lib = std::string{OSTD_SHARED_LIB.string()};
    if (build_static) {
        default_lib = std::string{OSTD_STATIC_LIB.string()};
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
    } else if (build_cfg != "release") {
        ostd::cerr.writefln("invalid build configuration: %s", build_cfg);
        ap.print_help(ostd::cerr.iter());
        return 1;
    }

    add_env(cxxflags, "CXXFLAGS");
    add_env(ldflags, "LDFLAGS");
    add_env(asflags, "ASFLAGS");

    auto examples = ostd::appender<pathvec>();
    fs::glob_match(examples, EXAMPLES_DIR / "*.cc");
    for (auto &ex: examples.get()) {
        ex.replace_suffix();
    }

    auto cxx_sources = ostd::appender<pathvec>();
    fs::glob_match(cxx_sources, CXX_SOURCE_DIR / "*.cc");
    for (auto &cc: cxx_sources.get()) {
        cc.replace_suffix();
    }

    if (clean) {
        ostd::writeln("Cleaning...");

        for (auto &ex: examples.get()) {
            auto rp = ex;
            try_remove(rp);
            rp.replace_suffix(".o");
            try_remove(rp);
        }
        for (auto &aso: ASM_SOURCES) {
            auto rp = (ASM_SOURCE_DIR / aso).with_suffix(".o");
            try_remove(rp);
            rp.replace_name((aso + "_dyn.o").string());
            try_remove(rp);
        }
        for (auto &cso: cxx_sources.get()) {
            auto rp = cso.with_suffix(".o");
            try_remove(rp);
            rp.replace_name((cso + "_dyn.o").string());
            try_remove(rp);
        }
        try_remove(OSTD_UNICODE_SRC);
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
        path const &input, path const &output, bool lib, bool shared
    ) {
        strvec args = { cxx };
        add_args(args, cxxflags);

        auto ifs = input.string();
        auto outp = output;

        if (shared) {
            outp.replace_suffix();
            outp += "_dyn.o";
            echo_q("CXX (shared): %s", ifs);
            add_args(args, SHARED_CXXFLAGS);
        } else {
            echo_q("CXX: %s", ifs);
        }

        if (lib) {
            args.push_back("-DOSTD_BUILD");
            if (shared) {
                args.push_back("-DOSTD_DLL");
            }
        }

        args.push_back("-c");
        args.push_back("-o");
        args.push_back(std::string{outp.string()});
        args.push_back(std::string{ifs});

        exec_v(args);

        return outp;
    };

    /* mostly unnecessary to separately compile shared, but
     * the files may check for __PIC__ (at least mips32 does)
     */
    auto call_as = [&](
        path const &input, path const &output, bool, bool shared
    ) {
        strvec args = { as };
        add_args(args, asflags);

        auto ifs = input.string();
        auto outp = output;

        if (shared) {
            outp.replace_suffix();
            outp += "_dyn.o";
            echo_q("AS (shared): %s", ifs);
            add_args(args, SHARED_ASFLAGS);
        } else {
            echo_q("AS: %s", ifs);
        }

        args.push_back("-c");
        args.push_back("-o");
        args.push_back(std::string{outp.string()});
        args.push_back(std::string{ifs});

        exec_v(args);

        return outp;
    };

    auto call_ld = [&](
        path const &output, pathvec const &files, strvec const &flags
    ) {
        echo_q("LD: %s", output);

        strvec args = { cxx };
        add_args(args, cxxflags);

        args.push_back("-o");
        args.push_back(std::string{output.string()});
        for (auto &p: files) {
            args.push_back(std::string{p.string()});
        }
        args.insert(args.cend(), flags.begin(), flags.end());

        add_args(args, ldflags);

        exec_v(args);

        if (build_cfg == "release") {
            args.clear();
            args.push_back(strip);
            args.push_back(std::string{output.string()});
            exec_v(args);
        }
    };

    auto call_ldlib = [&](
        path const &output, pathvec const &files, bool shared
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
            args.push_back(std::string{output.string()});
            for (auto &p: files) {
                args.push_back(std::string{p.string()});
            }

            exec_v(args);
        }
    };

    auto build_example = [&](path const &name) {
        auto base = name;
        auto ccf = base.with_suffix(".cc");
        auto obf = base.with_suffix(".o");

        call_cxx(ccf, obf, false, false);
        call_ld(base, { obf }, { default_lib });

        try_remove(obf);
    };

    auto build_test = [&](path const &name) {
        auto base = TEST_DIR / name;
        auto ccf = base.with_suffix(".cc");
        auto obf = base.with_suffix(".o");

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

    ostd::thread_pool tp{};
    tp.start();

    std::queue<std::future<path>> future_obj, future_dynobj;

    /* build object files in static and shared (PIC) variants */
    auto build_all = [&](
        pathvec const &list, path const &spath,
        path const &sext, auto &buildf
    ) {
        auto build_obj = [&](path const &fpath,  bool shared) {
            auto srcf = fpath.with_suffix(sext.string());
            auto srco = srcf.with_suffix(".o");
            auto &fq = (shared ? future_dynobj : future_obj);
            fq.push(tp.push([&buildf, srcf, srco, shared]() {
                return buildf(srcf, srco, true, shared);
            }));
        };
        for (auto &sf: list) {
            auto sp = spath.empty() ? sf : (spath / sf);
            if (build_static) {
                build_obj(sp, false);
            }
            if (build_shared) {
                build_obj(sp, true);
            }
        }
    };

    echo_q("Generating Unicode tables...");
    ostd::unicode_gen::parse_state{}.build_all_from_file(
        OSTD_UNICODE_DATA.string(), OSTD_UNICODE_SRC.string()
    );

    echo_q("Building the library...");
    build_all(ASM_SOURCES, ASM_SOURCE_DIR, ".S", call_as);
    build_all(cxx_sources.get(), path{}, ".cc", call_cxx);

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
        for (auto &ex: examples.get()) {
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
        exec_v({ "./test_runner", std::string{TEST_DIR.string()} });
    }

    io_msgs.close();
    io_thread.join();

    return 0;
}
