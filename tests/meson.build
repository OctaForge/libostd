test_runner_exe = executable('test_runner',
    ['test_runner.cc'],
    dependencies: libostd,
    include_directories: libostd_includes,
    cpp_args: extra_cxxflags,
    install: false
)

libostd_tests_src = [
    'algorithm.cc',
    'range.cc'
]

test_target = []
foreach test_src: libostd_tests_src
    test_target += executable(test_src.split('.')[0],
        [test_src],
        dependencies: libostd,
        include_directories: libostd_includes,
        cpp_args: extra_cxxflags,
        install: false
    )
endforeach

test('libostd',
    test_runner_exe,
    args: [meson.current_build_dir()],
    workdir: meson.current_build_dir(),
    depends: test_target
)
