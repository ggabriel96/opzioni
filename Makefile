.PHONY: all install configure build test format create clean

all: install configure build test

install:
	conan install -b missing -s compiler.libcxx=libstdc++11 .

configure:
	meson setup --build.pkg-config-path=./ -Dexamples=True build/

build:
	meson compile -C build/

test:
	meson test -C build/

format:
	clang-format --verbose -i $(shell find . -name '*.cpp') $(shell find . -name '*.hpp')

create:
	conan create . opzioni/stable

clean:
	rm build/ -rf
