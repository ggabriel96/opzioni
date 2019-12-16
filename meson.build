project(
    'opzioni', 'cpp',
    version: '0.1.0',
    license: 'MIT',
    default_options: ['cpp_std=c++2a', 'buildtype=debug']
)

include_dir = include_directories('include/')

opzioni_lib = library(
    'opzioni',
    ['src/opzioni.cpp', 'src/converters.cpp'],
    include_directories: include_dir,
    install: true
)

# Make this library usable as a Meson subproject.
opzioni_dep = declare_dependency(
    include_directories: include_dir,
    link_with: opzioni_lib
)

if get_option('examples')
    subdir('examples/')
endif

# install_headers('include/opzioni.hpp', subdir : 'opzioni/')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
    opzioni_lib,
    url: 'https://github.com/ggabriel96/opzioni',
    filebase: 'opzioni',
    subdirs: 'opzioni/',
    version: '0.1.0',
)
