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
#include <ctime>
#include <vector>
#include <array>
#include <stdexcept>
#include <initializer_list>

#include <ostd/io.hh>
#include <ostd/string.hh>
#include <ostd/algorithm.hh>

namespace ostd {
namespace unicode_gen {

using code_t = std::uint32_t;
using code_vec = std::vector<code_t>;

inline code_t hex_to_code(string_range hs) {
    code_t ret = 0;
    for (char c: hs) {
        if (!std::isxdigit(c |= 32)) {
            throw std::runtime_error{"malformed code point"};
        }
        ret = ret * 16 + code_t(c - ((c > '9') ? ('a' - 10) : '0'));
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
            bits[n++] = line.slice(0, std::size_t(sc.data() - line.data()));
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

    template<typename R>
    void build(
        R &writer, string_range name,
        code_vec const &codes,
        string_range cname = string_range{},
        code_vec const &cases = code_vec{}
    ) {
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
        auto match_range = [&cases, &match_pair](std::size_t i) {
            return match_pair(i, 1) && (
                cases.empty() || (cases[i + 1] == (cases[i] + 1))
            );
        };
        auto match_lace = [
            &codes, &cases, &match_pair
        ](std::size_t i, std::size_t offs) {
            int off = (!int(offs) * 2) - 1;
            return match_pair(i, 2) && (cases.empty() || (
                (cases[i + 1] == code_t(std::int32_t(codes[i + 1]) + off)) &&
                (cases[i    ] == code_t(std::int32_t(codes[i    ]) + off))
            ));
        };

        bool endseq = false;
        std::size_t i = 0;
        std::size_t ncodes = codes.size();
        while (i < ncodes) {
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
                endseq = true;
                continue;
            }
            if (size_t j = 0; match_lace(i, j) || match_lace(i, ++j)) {
                laces_beg[j].push_back(codes[i]);
                for (++i; match_lace(i, j); ++i) {
                    continue;
                }
                laces_end[j].push_back(codes[i]);
                endseq = true;
                continue;
            }
            if (!endseq) {
                singles.push_back(codes[i]);
                if (!cases.empty()) {
                    singles_cases.push_back(cases[i]);
                }
            }
            endseq = false;
            ++i;
        }

        auto build_list = [&writer, &name](
            string_range aname, std::size_t ncol,
            code_vec const &col1, code_vec const &col2, code_vec const &col3
        ) {
            if (col1.empty()) {
                return;
            }
            format(
                writer, "static char32_t const %s_%s[][%d] = {\n",
                name, aname, ncol
            );
            for (std::size_t j = 0; j < col1.size(); ++j) {
                switch (ncol) {
                    case 1:
                        format(writer, "    { 0x%06X },\n", col1[j]);
                        break;
                    case 2:
                        format(
                            writer, "    { 0x%06X, 0x%06X },\n",
                            col1[j], col2[j]
                        );
                        break;
                    case 3:
                        format(
                            writer, "    { 0x%06X, 0x%06X, 0x%06X },\n",
                            col1[j], col2[j], col3[j]
                        );
                        break;
                    default:
                        throw std::runtime_error{"invalid column number"};
                        break;
                }
            }
            format(writer, "};\n\n");
        };

        if (cases.empty()) {
            format(writer, "\n/* is%s */\n\n", name);
        } else {
            format(writer, "\n/* is%s, to%s */\n\n", name, cname);
        }

        build_list(
            "ranges", !cases.empty() + 2, ranges_beg, ranges_end, ranges_cases
        );
        build_list("laces1", 2, laces_beg[0], laces_end[0], laces_beg[0]);
        build_list("laces2", 2, laces_beg[1], laces_end[1], laces_beg[1]);
        build_list(
            "singles", !cases.empty() + 1, singles, singles_cases, singles
        );

        /* is_CTYPE(c) */
        build_func(
            writer, name, name, "is", "bool",
            ranges_beg, laces_beg[0], laces_beg[1], singles
        );

        /* to_CTYPE(c) */
        if (!cases.empty()) {
            writer.put('\n');
            build_func(
                writer, name, cname, "to", "char32_t",
                ranges_beg, laces_beg[0], laces_beg[1], singles
            );
        }
    }

    template<typename R>
    void build_header(R &writer) {
        char buf[64];
        time_t curt;
        std::time(&curt);
        strftime(buf, sizeof(buf), "%c", std::localtime(&curt));
        format(
            writer, "/* Generated on %s by gen_unicode (libostd) */\n",
            static_cast<char *>(buf)
        );
    }

    template<typename R>
    void build_func(
        R &writer,
        string_range name,
        string_range fname,
        string_range prefix,
        string_range ret_type,
        code_vec const &ranges,
        code_vec const &laces1,
        code_vec const &laces2,
        code_vec const &singles
    ) {
        format(
            writer, "OSTD_EXPORT %s %s%s(char32_t c) noexcept {\n",
            ret_type, prefix, fname
        );
        format(writer, "    return utf::uctype_func<\n");
        auto it1 = { &ranges, &laces1, &laces2, &singles };
        auto it2 = { "ranges", "laces1", "laces2", "singles" };
        for (std::size_t i = 0; i < it1.size(); ++i) {
            if (it1.begin()[i]->empty()) {
                format(writer, "        0, 0");
            } else {
                format(
                    writer, "        sizeof(%s_%s), sizeof(*%s_%s)",
                    name, it2.begin()[i], name, it2.begin()[i]
                );
            }
            if ((i + 1) != it1.size()) {
                format(writer, ",\n");
            } else {
                format(writer, "\n");
            }
        }
        format(writer, "    >::do_%s(\n        c, ", prefix);
        for (std::size_t i = 0; i < it1.size(); ++i) {
            if (i != 0) {
                format(writer, ", ");
            }
            if (it1.begin()[i]->empty()) {
                format(writer, "nullptr");
            } else {
                format(writer, "%s_%s", name, it2.begin()[i]);
            }
        }
        format(writer, "\n    );\n}\n");
    }

    template<typename R, typename IR>
    void build_all(R &writer, IR lines) {
            for (auto const &line: lines) {
                parse_line(line);
            }

            build_header(writer);

            build(writer, "alpha", alphas);
            build(writer, "cntrl", controls);
            build(writer, "digit", digits);
            build(writer, "lower", lowers, "upper", touppers);
            build(writer, "space", spaces);
            build(writer, "title", titles);
            build(writer, "upper", uppers, "lower", tolowers);
    }

    void build_all_from_file(string_range input, string_range output) {
        file_stream ifs{input};
        if (!ifs.is_open()) {
            throw std::runtime_error{"could not open input file"};
        }
        file_stream ofs{output, stream_mode::WRITE};
        if (!ofs.is_open()) {
            throw std::runtime_error{"could not open output file"};
        }
        auto writer = ofs.iter();
        build_all(writer, ifs.iter_lines());
    }
};

} /* namespace unicode_gen */
} /* namespace ostd */

#ifndef OSTD_GEN_UNICODE_INCLUDE
int main(int argc, char **argv) {
    if (argc <= 1) {
        ostd::writeln("not enough arguments");
        return 1;
    }
    ostd::string_range fname = argv[1];
    ostd::string_range outname = "src/string_utf.hh";
    if (argc >= 3) {
        outname = argv[2];
    }

    ostd::unicode_gen::parse_state ps;
    try {
        ps.build_all_from_file(fname, outname);
    } catch (std::runtime_error const &e) {
        ostd::writeln(e.what());
        return 1;
    }
}
#endif
