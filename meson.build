project('libostd', ['cpp'],
    version: '0.1.0',
    default_options: ['buildtype=plain', 'cpp_std=c++17'],
    meson_version: '>=0.46'
)

dir_prefix  = get_option('prefix')
dir_include = join_paths(dir_prefix, get_option('includedir'))
dir_data    = join_paths(dir_prefix, get_option('datadir'))
dir_lib     = join_paths(dir_prefix, get_option('libdir'))

dir_package_include = join_paths(dir_include, 'ostd')

unicode_data = join_paths('data', 'UnicodeData-11.0.txt')

libostd_includes = [include_directories('.')]

tgt_compiler_id = meson.get_compiler('cpp').get_id()
if tgt_compiler_id == 'gcc' or tgt_compiler_id == 'clang'
    extra_cxxflags = ['-Wextra', '-Wshadow', '-Wold-style-cast']
else
    extra_cxxflags = []
endif

subdir('src')

if get_option('build-tests')
    subdir('tests')
endif

if get_option('build-examples')
    subdir('examples')
endif
