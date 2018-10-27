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

Libostd is built using Meson. Therefore, you need to install Meson and then
you can compile it as usual. Typically, this will be something like

~~~
mkdir build && cd build
meson ..
ninja all
~~~

This will typically build using either GCC or Clang with the default standard
library. **Keep in mind that it is you need at least Clang 4.0 or
GCC 7.1 to build.**
