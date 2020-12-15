.PHONY: all install configure build test format clean

all: install configure build test

install:
	conan install -if build/ -b missing .

configure:
	meson setup --build.pkg-config-path=build/ -Dbuild-examples=True build/

build:
	ninja -C build/

test:
	meson test -C build/

format:
	clang-format --verbose -i $(shell find . -name '*.cpp') $(shell find . -name '*.hpp')

clean:
	rm build/ -rf
