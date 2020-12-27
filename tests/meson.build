catch2_dep = dependency('catch2', version: ['>=2.13.0', '<=3.0.0'])

test_deps = [opzioni_dep, catch2_dep]

converters_test = executable(
    'converters',
    ['catch2_main.cpp', 'converters.cpp'],
    dependencies: test_deps
)
program_test = executable(
    'program',
    ['catch2_main.cpp', 'opzioni.program.cpp'],
    dependencies: test_deps
)

test('converters', converters_test)
test('program', program_test)
