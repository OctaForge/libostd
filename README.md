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
from a bug in its variable template implementation that prevents OctaSTD from
functioning. Therefore version 3.8 or higher is necessary (where this bug was
finally fixed).

GCC has implemented a sufficient feature level of C++14 since version 5.1, but
also is too buggy until version 5.4. Version 5.1 and 5.2 have variable template
partial specialization issues and version 5.3 has an internal compiler error
triggered by the tuple implementation. Version 5.4 appears to be the first one
to compile this without issues. GCC 6.1 also appears to compile without problems.

MSVC++ is currently unsupported. Support is currently being investigated and
might be added at least for VS 2015 Update 2, assuming I don't run into any
significant bugs or missing features. MSVC++ with Clang frontend will be
supported once Microsoft updates it to Clang 3.8 (3.7 as is currently shipped
suffers from the issue mentioned above).

## Supported operating systems

Currently supported OSes in OctaSTD are Linux, FreeBSD and OS X. Other
systems that implement POSIX API will also work (if they don't, bug reports
are welcome).

OS X support requires Xcode 8 or newer to work. That is the first version to
ship a Clang 3.8 based toolchain, so things will not compile with an older
version of Xcode. Alternatively you are free to use any other supported
compiler from other distribution channels (official Clang, homebrew gcc
or clang, etc.).

Windows is supported with GCC (MinGW) and Clang. The MS C runtime is supported
as well, so compiling with Clang targeting MSVC compatibility will work.

## Exceptions

The library is written with the idea that no API in it ever throws exceptions (with
possibly some rare... exceptions ;)). However, this does not mean it cannot be used
in code that utilizes exceptions. Because of this, the library attempts to provide
some degree of exception safety as well as proper `noexcept` annotations. It also
provides type traits to check things such as whether something can be constructed
or assigned to without throwing.

This is currently a work in progress though. Here is a list of files where things
have been correctly annotated:

* array.hh
* initializer_list.hh
* memory.hh
* new.hh
* platform.hh
* utility.hh
* type_traits.hh

This list will expand over time. As for exception safety of containers, it's is
currently not done/checked. They will be made exception safe over time too.
