project(
    'opzioni', 'cpp',
    version: '0.4.0',
    license: 'MIT',
    default_options: ['cpp_std=c++2a', 'buildtype=debug']
)

include_dir = include_directories('../include/')

fmt_dep = dependency('fmt', version: '^6.2.1')

opzioni_lib = library(
    'opzioni',
    ['opzioni.cpp', 'converters.cpp'],
    dependencies: fmt_dep,
    include_directories: include_dir,
    install: true
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
    opzioni_lib,
    url: 'https://github.com/ggabriel96/opzioni',
    filebase: 'opzioni',
    subdirs: 'opzioni/',
    version: meson.project_version(),
)
