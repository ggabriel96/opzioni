project(
    'opzioni', 'cpp',
    version: '0.4.0',
    license: 'MIT',
    default_options: ['cpp_std=c++2a', 'buildtype=debug']
)

include_dir = include_directories('include/')

# +--------------+
# | Dependencies |
# +--------------+
fmt_dep = dependency('fmt', version: '^6.2.1')
catch2_dep = dependency('catch2', version: '^2.12.4')

# +--------------------+
# | Library definition |
# +--------------------+
opzioni_lib = library(
    'opzioni',
    ['src/opzioni.cpp', 'src/converters.cpp', 'src/types.cpp', 'src/parsing.cpp'],
    dependencies: fmt_dep,
    include_directories: include_dir,
    install: true
)
# Make it usable as a Meson subproject.
opzioni_dep = declare_dependency(
    dependencies: fmt_dep,
    include_directories: include_dir,
    link_with: opzioni_lib
)

# +-------+
# | Tests |
# +-------+
test_deps = [opzioni_dep, catch2_dep]
converters_test = executable(
    'converters',
    ['src/catch2_main.cpp', 'src/converters.test.cpp'],
    dependencies: test_deps
)
parsing_test = executable(
    'parsing',
    ['src/catch2_main.cpp', 'src/parsing.test.cpp'],
    dependencies: test_deps
)
types_test = executable(
    'types',
    ['src/catch2_main.cpp', 'src/types.test.cpp'],
    dependencies: test_deps
)
test('converters', converters_test)
test('parsing', parsing_test)
test('types', types_test)

# +----------+
# | Examples |
# +----------+
if get_option('examples')
    subdir('examples/')
endif
