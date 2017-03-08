# octastd

**Work in progress, not ready for production use.**

OctaSTD is an extension of the C++17 standard library which mainly provides
ranges (to replace iterators) but also various other utilities like proper
streams, string formatting, coroutines, concurrency utilities and others. It's
meant to replace the more poorly designed parts of the C++ standard library to
make the language easier and more convenient to use.

It is not feature complete right now, as most things are still being worked on.

Documentation (currently outdated and incomplete) for OctaSTD can be found at
https://wiki.octaforge.org/docs/octastd.

## Supported compilers

Compiler | Version
-------- | -------
gcc/g++  | 7+
clang    | 4+

You need those mainly to get the right standard library version (libstdc++
or libc++). Other compilers might work as well, as long as the necessary
standard library features are supplied.

MSVC++ is unsupported and for the time being will remain unsupported. As of MS
Visual Studio 2017 RC, basic C++11 features are still broken and prevent usage
of the library, with no reasonable workarounds. I will be testing new versions
as they get released and mark it supported as soon as it actually works, but no
active effort will be put towards making it work. On Windows, you're free to
use GCC/Clang, if you need Visual Studio, LLVM integration exists.

## Why is C++17 necessary?

Sadly, it's not possible to properly integrate `std::string` and `std::hash`
with OctaSTD ranges without utilizing `std::string_view`. Also, C++17 provides
library features that OctaSTD would have to implement otherwise, which would
lead to potentially incompatible APIs. C++17 also provides some nice language
features (such as `if constexpr` and fold expressions) which allow a lot of
code to be written in a cleaner way. However, it is made sure that no features
beyond the minimum supported compiler are necessary to use the library.

## Supported operating systems

Most of OctaSTD is entirely platform independent and relies only on the
standard library. Therefore it could in theory be used on any operating
system that provides the right toolchain. However, to make things easier
to deal with, it currently assumes either Windows or POSIX environment.
Some parts (such as filesystem and context/coroutines) also use platform
specific code that assumes these two.

OctaSTD is actively supported on Windows (x86 and x86\_64) as well as Linux,
FreeBSD and macOS. It should also work on other POSIX systems such as the other
BSDs or Solaris - if it doesn't, please report your problem or better, send
patches.

### Coroutine platform support

Coroutines work on POSIX and Windows systems. Context switching is done with
platform specific assembly taken from Boost.Context, the provided assembly
code should be enough for all supported platforms, but you need to compile
the correct ones.

There is also support for stack allocators inspired again by the Boost.Context
library, with fixed size protected and unprotected allocators available on all
platforms and segmented stacks available on POSIX platforms with GCC and Clang.
In order to use segmented stacks, there are 2 things you have to do:

* Enable `OSTD_USE_SEGMENTED_STACKS`
* Build with `-fsplit-stack -static-libgcc`

Segmented stacks are used by default when enabled, otherwise unprotected fixed
size stacks are used (on Windows the latter is always used by default).

There is also Valgrind support, enabled with `OSTD_USE_VALGRIND`.