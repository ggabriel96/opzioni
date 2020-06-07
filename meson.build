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
gtest_dep = dependency('gtest', version: '^1.10.0')

# +--------------------+
# | Library definition |
# +--------------------+
opzioni_lib = library(
    'opzioni',
    ['src/opzioni.cpp', 'src/converters.cpp'],
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
test_deps = [opzioni_dep, gtest_dep]
converters_test = executable(
    'converters',
    'src/converters.test.cpp',
    dependencies: test_deps
)
test('converters', converters_test)

# +----------+
# | Examples |
# +----------+
if get_option('examples')
    subdir('examples/')
endif
