# octastd

OctaSTD is a collection of C++ utilities to aid the upcoming OctaForge C++
API. It provides containers (dynamic arrays etc) as well as other utilities.

It utilizes C++11. It also implements equivalents of certain C++14 library
features that are possible to implement with the C++11 language. It does not
go beyond C++11 level when it comes to core language features.

## Supported compilers

Compiler | Version
-------- | -------
gcc/g++  | 4.8+
clang    | 3.3+
MSVC++   | 2015+

Other C++11 compliant compilers might work as well. OctaSTD does not utilize
compiler specific extensions except certain builtin type traits - to implement
traits that are not normally possible to implement without compiler support.

OctaSTD does not provide fallbacks for those traits. The compiler is expected
to support these builtins. So far the 3 above-mentioned compilers support them.
