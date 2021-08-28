catch2_proj = subproject('catch2', version: ['>=2.13.0', '<=3.0.0'])
catch2_dep = catch2_proj.get_variable('catch2_dep')

test_deps = [opzioni_dep, catch2_dep]

arg_test = executable(
    'arg',
    ['catch2_main.cpp', 'opzioni.arg.cpp'],
    dependencies: test_deps
)
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
string_test = executable(
    'string',
    ['catch2_main.cpp', 'string.cpp'],
    dependencies: test_deps
)

test('arg', arg_test)
test('converters', converters_test)
test('program', program_test)
test('string', string_test)
