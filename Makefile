.PHONY: all setup-gcc build test format clean

all: setup-gcc build test

setup-gcc:
	meson setup --native-file conda-gcc.ini -Dexamples=True build/

build:
	meson compile -C build/

test:
	meson test -C build/ --print-errorlogs

format:
	clang-format --verbose -i \
		$(shell find . -path './subprojects' -prune , -name '*.cpp') \
		$(shell find . -path './subprojects' -prune , -name '*.hpp')

clean:
	rm build/ -rf
