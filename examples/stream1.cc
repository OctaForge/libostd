#include <ostd/platform.hh>
#include <ostd/io.hh>

using namespace ostd;

void print_result(uint32_t x) {
    writefln("got x: 0x%X", x);
}

int main() {
    FileStream wtest{"test.bin", StreamMode::write};
    copy(
        iter({ 0xABCD1214, 0xBADC3264, 0xDEADBEEF, 0xBEEFDEAD }),
        wtest.iter<uint32_t>()
    );
    wtest.close();

    FileStream rtest{"test.bin"};
    writefln("stream size: %d", rtest.size());

    for (uint32_t x: map(rtest.iter<uint32_t>(), FromBigEndian<uint32_t>())) {
        print_result(x);
    }

    return 0;
}
