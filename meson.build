project(
    'opzioni', 'cpp',
    version: '0.25.0',
    license: 'BSL-1.0',
    default_options: ['cpp_std=c++2a', 'buildtype=debug']
)

include_dir = include_directories('include/')

# +--------------+
# | Dependencies |
# +--------------+
fmt_dep = dependency('fmt', version: ['>=7.0.3', '<8.0.0'])
catch2_dep = dependency('catch2', version: ['>=2.13.0', '<=3.0.0'])

# +--------------------+
# | Library definition |
# +--------------------+
opzioni_lib = library(
    'opzioni',
    ['src/args.cpp', 'src/converters.cpp', 'src/parsing.cpp', 'src/program.cpp'],
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
test('converters', converters_test)
test('parsing', parsing_test)

# +----------+
# | Examples |
# +----------+
if get_option('examples')
    subdir('examples/')
endif
