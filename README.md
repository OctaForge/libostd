# octastd

OctaSTD is an extension of the C++17 standard library which mainly provides
ranges (to replace iterators) but also various other utilities like proper
streams, string formatting, concurrency utilities and others. It's meant
to replace the more poorly designed parts of the C++ standard library to
make the language easier and more convenient to use.

Documentation for OctaSTD can be found at https://wiki.octaforge.org/docs/octastd.

It's not necessary that your compiler fully supports C++17, only C++14
language is used, but new C++17 containers have to be present, such as
`string_view` and `optional`.

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
lead to potentially incompatible APIs. However, OctaSTD does not make wide
use of C++17 language features, limiting itself mostly to library features
which have been present for more or less a pretty long time (in experimental
namespace for example) which should avoid buggy behavior. There is still the
problem of requiring a very recent toolchain, but this situation should solve
itself in near future.

## Supported operating systems

Most of OctaSTD is entirely platform independent and relies only on the
standard library. Therefore it can be used on any operating system that
provides the right toolchain.

There are certain parts (currently the filesystem module) that however do rely
on system specific APIs. These are restricted to POSIX compliant operating
systems and Windows, with testing done on Linux, FreeBSD, macOS and Windows -
they should work on other POSIX compliant operating systems as well, and
potential patches are welcome.
