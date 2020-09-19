project('libostd', ['cpp'],
    version: '0.1.0',
    default_options: ['buildtype=plain', 'cpp_std=c++17', 'warning_level=3'],
    meson_version: '>=0.46'
)

dir_prefix  = get_option('prefix')
dir_include = join_paths(dir_prefix, get_option('includedir'))
dir_data    = join_paths(dir_prefix, get_option('datadir'))
dir_lib     = join_paths(dir_prefix, get_option('libdir'))

dir_package_include = join_paths(dir_include, 'ostd')

unicode_data = join_paths('data', 'UnicodeData-13.0.txt')

libostd_includes = [include_directories('.')]

cxx = meson.get_compiler('cpp')

if get_option('buildtype') != 'plain'
    if cxx.has_argument('-Wshadow')
        add_global_arguments('-Wshadow', language: 'cpp')
    endif
    if cxx.has_argument('-Wold-style-cast')
        add_global_arguments('-Wold-style-cast', language: 'cpp')
    endif
endif

if cxx.has_argument('-fvisibility=hidden')
    add_global_arguments('-fvisibility=hidden', language: 'cpp')
endif

subdir('src')

if get_option('build-tests')
    subdir('tests')
endif

if get_option('build-examples')
    subdir('examples')
endif

pkg = import('pkgconfig')

pkg.generate(
    libraries: libostd_lib,
    version: meson.project_version(),
    name: 'libostd',
    filebase: 'libostd',
    url: 'https://git.octaforge.org/octaforge/libostd',
    description: 'OctaForge C++ utility library'
)
