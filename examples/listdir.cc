/** @example listdir.cc
 *
 * A simple example of integration of std::filesystem with ranges.
 */

#include <ostd/io.hh>
#include <ostd/filesystem.hh>
#include <ostd/range.hh>

using namespace ostd;

inline void list_dirs(filesystem::path const &path, int off = 0) {
    filesystem::directory_iterator ds{path};
    for (auto &v: ds) {
        auto p = filesystem::path{v};
        if (!filesystem::is_directory(p)) {
            continue;
        }
        for_each(range(off), [](int) { write(' '); });
        writeln(p.filename());
        list_dirs(p, off + 1);
    }
}

int main(int argc, char **argv) {
    if (argc < 1) {
        return 1;
    }
    list_dirs(argv[1]);
    return 0;
}
