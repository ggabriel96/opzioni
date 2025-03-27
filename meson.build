project(
    'opzioni', 'cpp',
    version: '0.90.0',
    license: 'BSL-1.0',
    default_options: ['cpp_std=c++23', 'buildtype=debug']
)

# +--------------+
# | Dependencies |
# +--------------+
fmt_dep = dependency('fmt', version: ['>=11.1.1', '<12.0.0'])

# +--------------------+
# | Library definition |
# +--------------------+
include_dir = include_directories('include/')
opzioni_lib = library(
    'opzioni',
    ['src/actions.cpp', 'src/arg.cpp', 'src/cmd_info.cpp', 'src/converters.cpp', 'src/strings.cpp'],
    dependencies: fmt_dep,
    include_directories: include_dir,
    install: true
)

install_subdir(
    'include/',
    install_dir: get_option('includedir') / meson.project_name(),
    strip_directory: true
)
pkg = import('pkgconfig')
pkg.generate(opzioni_lib)

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
