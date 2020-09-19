libostd_header_src = [
    '../ostd/algorithm.hh',
    '../ostd/argparse.hh',
    '../ostd/channel.hh',
    '../ostd/concurrency.hh',
    '../ostd/context_stack.hh',
    '../ostd/coroutine.hh',
    '../ostd/environ.hh',
    '../ostd/event.hh',
    '../ostd/format.hh',
    '../ostd/generic_condvar.hh',
    '../ostd/io.hh',
    '../ostd/path.hh',
    '../ostd/platform.hh',
    '../ostd/process.hh',
    '../ostd/range.hh',
    '../ostd/stream.hh',
    '../ostd/string.hh',
    '../ostd/thread_pool.hh',
    '../ostd/unit_test.hh',
    '../ostd/vecmath.hh',

    '../ostd/build/make.hh',
    '../ostd/build/make_coroutine.hh',

    '../ostd/ext/sdl_rwops.hh'
]

libostd_src = [
    'argparse.cc',
    'build_make.cc',
    'channel.cc',
    'concurrency.cc',
    'context_stack.cc',
    'environ.cc',
    'io.cc',
    'path.cc',
    'process.cc',
    'string.cc',
    'thread_pool.cc',

    'asm/jump_all_gas.S',
    'asm/make_all_gas.S',
    'asm/ontop_all_gas.S'
]

thread_dep = dependency('threads')

libostd_gen_unicode_exe = executable('gen_unicode',
    ['../gen_unicode.cc'],
    include_directories: libostd_includes,
    cpp_args: ['-DOSTD_GEN_UNICODE_BUILD'],
    install: false,
    native: true
)

libostd_extra_src = []

libostd_extra_src += custom_target('libostd_unicode_tables',
    input: join_paths('..', unicode_data),
    output: ['string_utf.hh'],
    install: false,
    command: [
        libostd_gen_unicode_exe, '@INPUT@',
        join_paths(meson.current_build_dir(), 'string_utf.hh')
    ]
)

libostd_lib = both_libraries('ostd',
    libostd_src, libostd_extra_src,
    dependencies: thread_dep,
    include_directories: libostd_includes + [include_directories('.')],
    install: true,
    version: meson.project_version()
)

libostd = declare_dependency(
    include_directories: libostd_includes,
    link_with: libostd_lib.get_shared_lib()
)

libostd_static = declare_dependency(
    include_directories: libostd_includes,
    link_with: libostd_lib.get_static_lib()
)

install_headers(libostd_header_src, install_dir: dir_package_include)
