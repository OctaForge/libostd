# octastd

OctaSTD is a collection of C++ utilities to aid the upcoming OctaForge C++
API. It provides containers (dynamic arrays etc) as well as other utilities.

Documentation for OctaSTD can be found at https://wiki.octaforge.org/docs/octastd.

Full C++14 support is required in your compiler.

## Supported compilers

Compiler | Version
-------- | -------
gcc/g++  | 5+
clang    | 3.8+

Other C++14 compliant compilers might work as well. OctaSTD does not utilize
compiler specific extensions except certain builtin type traits - to implement
traits that are not normally possible to implement without compiler support.

OctaSTD does not provide fallbacks for those traits. The compiler is expected
to support these builtins. So far the 2 above-mentioned compilers support them
(MSVC++ supports most of these as well).

While Clang 3.6 does implement a sufficient level of C++14 support, it suffers
from a bug in its template variable implementation that prevents OctaSTD from
functioning. Therefore version 3.8 or higher is necessary (where this bug was
finally fixed).

MSVC++ is currently unsupported. It is likely that it will never be supported,
as MS recently introduced Clang frontend support in Visual Studio; however,
if their own frontend gains all the necessary features, support will be
considered.

## Supported operating systems

Currently supported OSes in OctaSTD are Linux, FreeBSD and OS X. Other
systems that implement POSIX API will also work (if they don't, bug reports
are welcome).

Windows is supported at least with the MinGW (gcc) and Clang compilers. MS
Visual Studio is currently unsupported.