# libostd

**Work in progress, not ready for production use.**

Libostd is an extension of the C++17 standard library which mainly provides
ranges (to replace iterators) but also various other utilities like proper
streams, string formatting, coroutines, concurrency utilities and others. It's
meant to replace the more poorly designed parts of the C++ standard library to
make the language easier and more convenient to use.

It is not feature complete right now, as most things are still being worked on.

Documentation for libostd can be found at https://docs.octaforge.org/libostd.
Please refer to it for further information (the main page should be answer
some more of your questions). You can also read `doc/main_page.md` and other
files in there directly if you don't need the API documentation.

## Building

Libostd is built using the supplied C++ build tool. You need to compile the
build tool first using the compiler you will use to build the library itself.

On a typical setup, this will involve:

~~~
c++ build.cc -o build -std=c++1z -I. -pthread
./build --help
~~~

This will typically build using either GCC or Clang with typically libstdc++
as a standard library implementation. **Keep in mind that it is you need
at least Clang 4.0 or GCC 7.1 to build.** To switch to libc++:

~~~
c++ build.cc -o build -std=c++1z -I. -pthread -stdlib=libc++
CXXFLAGS="-stdlib=libc++" ./build
~~~

If you get undefined references about filesystem, you will need to add
extra linkage.

For libstdc++:

~~~
c++ build.cc -o build -std=c++1z -I. -pthread -lstdc++fs
LDFLAGS="-lstdc++fs" ./build
~~~

For libc++:

~~~
c++ build.cc -o build -std=c++1z -I. -pthread -stdlib=libc++ -lc++experimental
CXXFLAGS="-stdlib=libc++" LDFLAGS="-lc++experimental" ./build
~~~

You can skip `-pthread` on Windows.

Using the tool should be straightforward. The `./build --help` command lists
the available options.

It also recognizes the environment variables `CXX` (the C++ compiler used
to build, defaults to `c++`), `AS` (the assembler used to build, defaults to
`c++` as well, as Clang and GCC can compile assembly files), `AR` (the tool
to create static lib archives, `ar` by default) and `STRIP` (the tool used
to strip the library in release mode, `strip` by default).

Additionally, the `CXXFLAGS`, `LDFLAGS` and `ASFLAGS` environment variables
are also used. The `CXXFLAGS` are passed when compiling C++ source files as
well as when linking (the compiler is used to link). The `LDFLAGS` are passed
additionally to `CXXFLAGS` only when linking. The `ASFLAGS` are passed to
the assembler (`CXXFLAGS` are not, even when Clang/GCC is used).
