#include <cstdint>
#include <cctype>
#include <vector>
#include <stdexcept>

#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/algorithm.hh>

using code_t = std::uint32_t;
using code_vec = std::vector<code_t>;

code_t hex_to_code(ostd::string_range hs) {
    code_t ret = 0;
    for (char c: hs) {
        if (!std::isxdigit(c |= 32)) {
            throw std::runtime_error{"malformed code point"};
        }
        ret = ret * 16 + (c - ((c > '9') ? ('a' - 10) : '0'));
    }
    return ret;
}

struct parse_state {
    code_vec controls;
    code_vec alphas;
    code_vec lowers;
    code_vec uppers;
    code_vec tolowers;
    code_vec touppers;
    code_vec titles;
    code_vec digits;
    code_vec spaces;

    void assert_line(bool b) {
        if (!b) {
            throw std::runtime_error{"malformed line"};
        }
    }

    void parse_line(ostd::string_range line) {
        std::array<ostd::string_range, 15> bits;
        for (std::size_t n = 0;;) {
            auto sc = ostd::find(line, ';');
            if (!sc) {
                assert_line(n >= (bits.size() - 1));
                bits[n] = line;
                break;
            }
            bits[n++] = line.slice(0, sc.data() - line.data());
            sc.pop_front();
            line = sc;
        }
        assert_line(!bits[0].empty() && (bits[2].size() == 2));
        code_t n = hex_to_code(bits[0]);

        /* control chars */
        if (bits[2] == "Cc") {
            controls.push_back(n);
            return;
        }
        /* alphabetics */
        if (bits[2][0] == 'L') {
            alphas.push_back(n);
            /* can match more */
        }
        /* lowercase */
        if (bits[2] == "Ll") {
            lowers.push_back(n);
            touppers.push_back(bits[12].empty() ? n : hex_to_code(bits[12]));
            return;
        }
        /* uppercase */
        if (bits[2] == "Lu") {
            uppers.push_back(n);
            tolowers.push_back(bits[13].empty() ? n : hex_to_code(bits[13]));
            return;
        }
        /* titlecase */
        if (bits[2] == "Lt") {
            titles.push_back(n);
            return;
        }
        /* digits */
        if (bits[2] == "Nd") {
            digits.push_back(n);
            return;
        }
        /* whitespace */
        if (
            (bits[2][0] == 'Z') &&
            (bits[4] == "B" || bits[4] == "S" || bits[4] == "WS")
        ) {
            spaces.push_back(n);
            return;
        }
        /* good enough for now, ignore the rest */
    }
};

int main(int argc, char **argv) {
    if (argc <= 1) {
        ostd::writeln("not enough arguments");
        return 1;
    }

    ostd::string_range fname = argv[1];
    ostd::file_stream f{fname};
    if (!f.is_open()) {
        ostd::writefln("cannot open file '%s'", fname);
        return 1;
    }

    parse_state ps;

    for (auto const &line: f.iter_lines()) {
        ps.parse_line(line);
    }
}
