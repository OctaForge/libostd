# OctaSTD Documentation {#index}

## What is OctaSTD?

OctaSTD is an extension library for C++17. It enhances the standard library
to make the language easier and more convenient to use. It's based on the
idea is that everything should do what one would expect it to; it strives
to aid programmers in writing clean and readable code.

## The principles

* It's time to drop the legacy. It creates unnecessary burden that gets in
  the way of writing clean code.
* APIs should do mostly one thing and do it well. There's nothing worse than
  things that have corner cases with strange semantics.
* If the standard library thing sucks, just replace it.
* At least attempt backwards compatibility and easy migration, but don't fear
  dropping it if it gets in the way.

## What does it provide?

### Ranges

OctaSTD started as a range library. Ranges replace iterators; you no longer
need two objects to represent a range and ranges themselves have a clean
interface that is easy to adapt for your own thing. Additionally, there is
backwards compatibility with iterators, so you can create ranges out of
iterator pairs and even turn a range into an iterator! However, the primary
way of creating ranges is using native range types. The compatibility stuff
is there mostly so that OctaSTD can work with existing libraries. Additionally,
a large library of generic range algorithms is provided.

### Concurrency

OctaSTD is a complete concurrency framework, implementing a clean and
extensible system for working with logical tasks. It allows for custom
schedulers and by default implements several scheduling approaches (1:1,
N:1, M:N). It also implements stackful coroutines (on which the latter
two schedulers are based) so you get full cooperative concurrency as
well as iterable generators. Safe synchronization and data transfer is
provided using Go-style channels and possibly other methods. Thanks to
all this, you can write scalable and safe applications that make proper
use of modern multicore machines. There is also a thread pool and other
utilities you might want to use in different places.

### Streams

There is a brand new I/O stream system to replace the aging and ugly iostreams
system from the standard library. Besides a much nicer interface, it's also
significantly more lightweight and intergrates with ranges! You can use
standard range algorithms to write into and read from files besides other
things.

### Strings and formatting

Thanks to ranges, OctaSTD provides very lightweight string slices. The
slices are not zero terminated, so creating sub-slices is fast and avoids
tons of potential heap allocations. None of the OctaSTD string APIs ever
assumes termination.

Additionally, a completely type-safe string formatting system with C-like
format strings is provided. Thanks to being type-safe, you can do things
like using the `%s` format marks for any type and you never have to specify
sizes for numerical types. The system obviously integrates with ranges, so
you can format directly into any output range type. You can also implement
formatting for custom types, with complete control over the format mark.
All of this is zero-allocation, it lets the output range take care of that.

### Others

There are other APIs, too, including environment variable handling, simple
platform specific checks, a signal-slot event system or vector math. The
amount is expected to grow in the future, as OctaSTD is still a work in
progress.

## Why C++17?

C++17 includes several things that remove previous blockers, such as being
able to hash string range types sanely. Additionally, there are several
language features (such as `if constexpr`) that greatly simplify the code.
OctaSTD does not make full use of the standard, so it works with current
compilers.

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
