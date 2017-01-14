# octastd

OctaSTD is an extension of the C++14 standard library which mainly provides
ranges (to replace iterators) but also various other utilities like proper
streams, string formatting, concurrency utilities and others. It's meant
to replace the more poorly designed parts of the C++ standard library to
make the language easier and more convenient to use.

Documentation for OctaSTD can be found at https://wiki.octaforge.org/docs/octastd.

Full C++14 support is required in your compiler.

## Supported compilers

Compiler | Version
-------- | -------
gcc/g++  | 5.4+, 6+
clang    | 3.8+ (most platforms including LLVM for macOS), 8.0.0+ (macOS Xcode)

Other C++14 compliant compilers might work as well. OctaSTD does not utilize
compiler specific extensions.

While Clang 3.6 does implement a sufficient level of C++14 support, it suffers
from a bug in its variable template implementation that prevents OctaSTD from
functioning. Therefore version 3.8 or higher is necessary (where this bug was
finally fixed).

GCC has implemented a sufficient feature level of C++14 since version 5.1, but
also is too buggy until version 5.4. Version 5.1 and 5.2 have variable template
partial specialization issues and version 5.3 has an internal compiler error
triggered by the tuple implementation. Version 5.4 appears to be the first one
to compile this without issues. GCC 6.1 also appears to compile without problems.

MSVC++ is currently unsupported but being worked on. You will need at least
2015 Update 3 for this to work.

## Supported operating systems

Currently supported OSes in OctaSTD are Linux, FreeBSD, macOS and Windows. Other
systems might work as well, as long as a sufficient compiler is provided.

MacOS support requires Xcode 8 or newer to work (or alternatively, official
LLVM distribution for macOS or any supported compiler from other channels
such as Homebrew). That is the first version to ship a Clang 3.8 based
toolchain, so things will not compile with an older version of Xcode.

Windows support includes MinGW, Clang and soon MSVC++.