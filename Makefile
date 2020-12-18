.PHONY: all install configure build test format clean create

all: install configure build test

install:
	conan install -if build/ -b missing -s compiler.libcxx=libstdc++11 .

configure:
	meson setup --build.pkg-config-path=build/ -Dexamples=True build/

build:
	meson compile -C build/

test:
	meson test -C build/

format:
	clang-format --verbose -i $(shell find . -name '*.cpp') $(shell find . -name '*.hpp')

clean:
	rm build/ -rf

# --------------------
# related to packaging
# --------------------

create:
	conan create . opzioni/stable
