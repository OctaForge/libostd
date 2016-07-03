# octastd

OctaSTD is a new "standard library" for C++14. It provides containers
(dynamic arrays etc) as well as other utilities.

Documentation for OctaSTD can be found at https://wiki.octaforge.org/docs/octastd.

Full C++14 support is required in your compiler.

## Supported compilers

Compiler | Version
-------- | -------
gcc/g++  | 5.4+, 6+
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

GCC has implemented a sufficient feature level of C++14 since version 5.1, but
also is too buggy until version 5.4. Version 5.1 and 5.2 have template variable
partial specialization issues and version 5.3 has an internal compiler error
triggered by the tuple implementation. Version 5.4 appears to be the first one
to compile this without issues. GCC 6.1 also appears to compile without problems.

MSVC++ is currently unsupported. It is likely that it will never be supported,
as MS recently introduced Clang frontend support in Visual Studio; however,
if their own frontend gains all the necessary features, support will be
considered.

## Supported operating systems

Currently supported OSes in OctaSTD are Linux, FreeBSD and OS X. Other
systems that implement POSIX API will also work (if they don't, bug reports
are welcome).

OS X support requires Xcode 8 or newer to work. That is the first version to
ship a Clang 3.8 based toolchain, so things will not compile with an older
version of Xcode. Alternatively you are free to use any other supported
compiler from other distribution channels (official Clang, homebrew gcc
or clang, etc.).

Windows is supported at least with the MinGW (gcc) and Clang compilers. MS
Visual Studio is currently unsupported.
