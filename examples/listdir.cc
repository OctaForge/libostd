#include <ostd/io.hh>
#include <ostd/filesystem.hh>
#include <ostd/range.hh>

using namespace ostd;

void list_dirs(string_range path, int off = 0) {
    directory_stream ds{path};
    /* iterate all items in directory */
    for (auto v: iter(ds)) {
        if (v.type() != file_type::DIRECTORY) {
            continue;
        }
        for_each(range(off), [](int) { write(' '); });
        writeln(v.filename());
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
