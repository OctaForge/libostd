/** @example listdir.cc
 *
 * A simple example of using ostd::path and ostd::path::fs.
 */

#include <ostd/io.hh>
#include <ostd/path.hh>
#include <ostd/range.hh>

using namespace ostd;

inline void list_dirs(path const &path, int off = 0) {
    fs::directory_range ds{path};
    for (auto &v: ds) {
        if (!v.is_directory()) {
            continue;
        }
        for_each(range(off), [](int) { write(' '); });
        writeln(v.path().name());
        list_dirs(v.path(), off + 1);
    }
}

int main(int argc, char **argv) {
    if (argc < 1) {
        return 1;
    }
    list_dirs(argv[1]);
    return 0;
}
