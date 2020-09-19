test_runner_exe = executable('test_runner',
    ['test_runner.cc'],
    dependencies: libostd,
    include_directories: libostd_includes,
    install: false
)

libostd_gen_test_exe = executable('gen_test',
    ['gen_test.cc'],
    install: false,
    native: true
)

libostd_tests_names = [
    'algorithm',
    'range'
]

libostd_tests_indices = [
    0, 1
]

libostd_tests_src = []

foreach test_name: libostd_tests_names
    libostd_tests_src += custom_target('test_' + test_name,
        output: ['test_' + test_name + '.cc'],
        install: false,
        command: [
            libostd_gen_test_exe, test_name,
            join_paths(meson.current_build_dir(), 'test_' + test_name + '.cc')
        ]
    )
endforeach

test_target = []
foreach test_idx: libostd_tests_indices
    test_target += executable('test_' + libostd_tests_names[test_idx],
        [libostd_tests_src[test_idx]],
        dependencies: libostd,
        include_directories: libostd_includes,
        install: false
    )
endforeach

test('libostd',
    test_runner_exe,
    args: [meson.current_build_dir()],
    workdir: meson.current_build_dir(),
    depends: test_target
)
