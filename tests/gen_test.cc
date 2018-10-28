#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        return 1;
    }
    FILE *f = fopen(argv[2], "w");
    if (!f) {
        return 1;
    }
    fprintf(f,
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
        argv[1], argv[1]
    );
    fclose(f);
    return 0;
}
