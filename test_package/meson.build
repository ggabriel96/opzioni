project(
    'opzioni-test-package', 'cpp',
    default_options: ['cpp_std=c++2a', 'buildtype=debug']
)

opzioni_dep = dependency('opzioni', version: '0.44.3')

main = executable(
    'main', 'main.cpp',
    dependencies: [opzioni_dep]
)
