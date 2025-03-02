project(
    'opzioni', 'cpp',
    version: '0.50.0',
    license: 'BSL-1.0',
    default_options: ['cpp_std=c++23', 'buildtype=debug']
)

# +--------------+
# | Dependencies |
# +--------------+
fmt_dep = dependency('fmt', version: ['>=7.0.3', '<8.0.0'])

# +--------------------+
# | Library definition |
# +--------------------+
include_dir = include_directories('include/')
opzioni_lib = library(
    'explib',
    ['src/experimental/all.cpp', 'src/experimental/arg.cpp', 'src/experimental/parsing.cpp', 'src/converters.cpp'],
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
# subdir('tests/')

# +----------+
# | Examples |
# +----------+
if get_option('examples')
    subdir('examples/')
endif
