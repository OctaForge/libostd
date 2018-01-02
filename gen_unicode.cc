/* This simple program generates the tables necessary for Unicode character
 * types. It's inspired by the mkrunetype.awk generator from the libutf
 * project, see COPYING.md.
 *
 * This file is a part of the libostd project. Libostd is licensed under the
 * University of Illinois/NCSA Open Source License, as is this file. See the
 * COPYING.md file further information.
 */

#include <cstdint>
#include <cctype>
#include <vector>
#include <array>
#include <stdexcept>

#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/algorithm.hh>

using ostd::string_range;

using code_t = std::uint32_t;
using code_vec = std::vector<code_t>;

code_t hex_to_code(string_range hs) {
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

    void parse_line(string_range line) {
        std::array<string_range, 15> bits;
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

    void build(code_vec const &codes, code_vec const &cases = code_vec{}) {
        code_vec singles;
        code_vec singles_cases;
        code_vec ranges_beg;
        code_vec ranges_end;
        code_vec ranges_cases;
        code_vec laces_beg[2];
        code_vec laces_end[2];

        if (!cases.empty() && (cases.size() != codes.size())) {
            throw std::runtime_error{"mismatched code lists"};
        }

        auto match_pair = [&codes](std::size_t i, std::size_t off) {
            return (
                ((i + 1) < codes.size()) && (codes[i + 1] == (codes[i] + off))
            );
        };
        auto match_range = [&codes, &cases, &match_pair](std::size_t i) {
            return match_pair(i, 1) && (
                cases.empty() || (cases[i + 1] == (cases[i] + 1))
            );
        };
        auto match_lace = [
            &codes, &cases, &match_pair
        ](std::size_t i, int off) {
            return match_pair(i, 2) && (cases.empty() || (
                (cases[i + 1] == (codes[i + 1] + off)) &&
                (cases[i    ] == (codes[i    ] + off))
            ));
        };

        for (std::size_t i = 0, ncodes = codes.size(); i < ncodes; ++i) {
            if (match_range(i)) {
                ranges_beg.push_back(codes[i]);
                if (!cases.empty()) {
                    ranges_cases.push_back(cases[i]);
                }
                /* go to the end of sequence */
                for (++i; match_range(i); ++i) {
                    continue;
                }
                /* end of range, try others */
                ranges_end.push_back(codes[i]);
                continue;
            }
            if (size_t j = 0; match_lace(i, 1) || match_lace(i, -1)) {
                laces_beg[j].push_back(codes[i]);
                for (++i; match_lace(i, j); ++i) {
                    continue;
                }
                laces_end[j].push_back(codes[i]);
                continue;
            }
            singles.push_back(codes[i]);
            if (!cases.empty()) {
                singles_cases.push_back(cases[i]);
            }
        }

        auto build_list = [](
            string_range name, std::size_t ncol,
            code_vec const &col1, code_vec const &col2, code_vec const &col3
        ) {
            if (col1.empty()) {
                return;
            }
            ostd::writefln("%s:", name);
            for (std::size_t i = 0; i < col1.size(); ++i) {
                switch (ncol) {
                    case 1:
                        ostd::writefln("  0x%06X", col1[i]);
                        break;
                    case 2:
                        ostd::writefln("  0x%06X, 0x%06X", col1[i], col2[i]);
                        break;
                    case 3:
                        ostd::writefln(
                            "  0x%06X, 0x%06X, 0x%06X",
                            col1[i], col2[i], col3[i]
                        );
                        break;
                    default:
                        throw std::runtime_error{"invalid column number"};
                        break;
                }
            }
        };

        build_list(
            "ranges", !cases.empty() + 2, ranges_beg, ranges_end, ranges_cases
        );
        build_list("laces1", 2, laces_beg[0], laces_end[0], laces_beg[0]);
        build_list("laces2", 2, laces_beg[1], laces_end[1], laces_beg[1]);
        build_list(
            "singles", !cases.empty() + 1, singles, singles_cases, singles
        );
    }
};

int main(int argc, char **argv) {
    if (argc <= 1) {
        ostd::writeln("not enough arguments");
        return 1;
    }

    string_range fname = argv[1];
    ostd::file_stream f{fname};
    if (!f.is_open()) {
        ostd::writefln("cannot open file '%s'", fname);
        return 1;
    }

    parse_state ps;

    try {
        for (auto const &line: f.iter_lines()) {
            ps.parse_line(line);
        }
    } catch (std::runtime_error const &e) {
        ostd::writeln(e.what());
        return 1;
    }

    ostd::writeln("ALPHAS:");
    ps.build(ps.alphas);
    ostd::writeln("CONTROL:");
    ps.build(ps.controls);
    ostd::writeln("DIGITS:");
    ps.build(ps.digits);
    ostd::writeln("LOWERCASE:");
    ps.build(ps.lowers, ps.touppers);
    ostd::writeln("SPACES:");
    ps.build(ps.spaces);
    ostd::writeln("TITLES:");
    ps.build(ps.titles);
    ostd::writeln("UPPERCASE:");
    ps.build(ps.uppers, ps.tolowers);
}
