# octastd

**Work in progress, not ready for production use.**

OctaSTD is an extension of the C++17 standard library which mainly provides
ranges (to replace iterators) but also various other utilities like proper
streams, string formatting, coroutines, concurrency utilities and others. It's
meant to replace the more poorly designed parts of the C++ standard library to
make the language easier and more convenient to use.

It is not feature complete right now, as most things are still being worked on.

Documentation for OctaSTD can be found at https://docs.octaforge.org/octastd.

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

## Supported operating systems and architectures

OctaSTD targets POSIX compliant operating systems and Windows. Features are
written with those in mind, and other targets are currently not supported.

Tier 1 targets are regularly tested. Tier 2 targets are irregularly tested.
Tier 3 targets are rarely or never tested, but most likely working and
accepting patches for. Unsupported targets may or may not work depending
on their POSIX compliance and ABI.

|                 | Linux      | FreeBSD     | macOS      | Windows     | Other BSD | Solaris  | AIX      | Others |
|-----------------|:----------:|:-----------:|:----------:|:-----------:|:---------:|:--------:|:--------:|:------:|
| **x86**         | **Tier 1** | **Tier 1**  | Tier 2     | **Tier 1**  | *Tier 3*  | *Tier 3* | **No**   | **No** |
| **x86_64**      | **Tier 1** | **Tier 1**  | **Tier 1** | **Tier 1**  | *Tier 3*  | *Tier 3* | **No**   | **No** |
| **ARM/AArch64** | Tier 2     | Tier 2      | *Tier 3*   | *Tier 3*    | *Tier 3*  | **No**   | **No**   | **No** |
| **MIPS32**      | *Tier 3*   | *Tier 3*    | **No**     | **No**      | *Tier 3*  | **No**   | **No**   | **No** |
| **PPC32/64**    | *Tier 3*   | *Tier 3*    | **No**     | **No**      | *Tier 3*  | **No**   | *Tier 3* | **No** |
| **Others**      | **No**     | **No**      | **No**     | **No**      | **No**    | **No**   | **No**   | **No** |

### Coroutines

Coroutines use platform specific assembly code taken from Boost.Context. There
is assembly for all of the targets mentioned above.

There is also support for stack allocators inspired again by the Boost.Context
library, with fixed size protected and unprotected allocators available on all
platforms, as well as a stack pool which allocates stacks in batches and
recycles dead stacks.

Compile with the `OSTD_USE_VALGRIND` macro defined if you want useful Valgrind
output when using coroutines - this makes Valgrind aware of the custom stacks.
