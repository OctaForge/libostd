libostd_examples_src = [
    'argparse.cc',
    'concurrency.cc',
    'coroutine1.cc',
    'coroutine2.cc',
    'format.cc',
    'glob.cc',
    'listdir.cc',
    'range.cc',
    'range_pipe.cc',
    'signal.cc',
    'stream1.cc',
    'stream2.cc'
]

thread_dep = dependency('threads')

foreach example: libostd_examples_src
    executable(example.split('.')[0],
        [example],
        dependencies: [libostd, thread_dep],
        include_directories: libostd_includes,
        install: false
    )
endforeach
