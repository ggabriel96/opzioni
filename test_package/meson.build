project(
    'opzioni-test-package', 'cpp',
    default_options: ['cpp_std=c++2a', 'buildtype=debug']
)

fmt_dep = dependency('fmt', version: '^6.2.1')
opzioni_dep = dependency('opzioni', version: '^0.4.0')

main = executable(
    'main', 'main.cpp',
    dependencies: [fmt_dep, opzioni_dep]
)
