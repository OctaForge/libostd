/** @example stream1.cc
 *
 * An example of using streams to read and write binary files
 * with some help from the standard range algorithms.
 */

#include <ostd/platform.hh>
#include <ostd/io.hh>

using namespace ostd;

void print_result(uint32_t x) {
    writefln("got x: 0x%X", x);
}

int main() {
    file_stream wtest{"test.bin", stream_mode::WRITE};
    copy(
        iter({ 0xABCD1214, 0xBADC3264, 0xDEADBEEF, 0xBEEFDEAD }),
        wtest.iter<uint32_t>()
    );
    wtest.close();

    file_stream rtest{"test.bin"};
    writefln("stream size: %d", rtest.size());

    for (uint32_t x: map(rtest.iter<uint32_t>(), from_big_endian<uint32_t>())) {
        print_result(x);
    }

    return 0;
}
