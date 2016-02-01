#include <ostd/functional.hh>
#include <ostd/io.hh>

using namespace ostd;

void print_result(Uint32 x) {
    writefln("got x: 0x%X", x);
}

int main() {
    FileStream wtest("test.bin", StreamMode::write);
    copy(iter({ 0xABCD1214, 0xBADC3264, 0xDEADBEEF, 0xBEEFDEAD }), wtest.iter<Uint32>());
    wtest.close();

    FileStream rtest("test.bin", StreamMode::read);
    printf("stream size: %zu\n", rtest.size());

    for (Uint32 x: map(rtest.iter<Uint32>(), FromBigEndian<Uint32>()))
        print_result(x);

    return 0;
}