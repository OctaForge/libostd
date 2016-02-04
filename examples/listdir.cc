#include <ostd/io.hh>
#include <ostd/filesystem.hh>

using namespace ostd;

void list_dirs(ConstCharRange path, int off = 0) {
    DirectoryStream ds(path);
    /* iterate all items in directory */
    for (auto v: ds.iter()) {
        if (v.type() != FileType::directory)
            continue;
        for (int i = 0; i < off; ++i) write(' ');
        writeln(v.filename());
        list_dirs(v.path(), off + 1);
    }
}

int main(int argc, char **argv) {
    if (argc < 1) return 1;
    list_dirs(argv[1]);
    return 0;
}