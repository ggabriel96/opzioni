.PHONY: all install configure build format clean

all: install configure build

install:
	conan install -if build/ -b missing .

configure:
	meson setup --build.pkg-config-path=build/ -Dbuild-examples=True build/

build:
	ninja -C build/

format:
	clang-format --verbose -i $(shell find . -name '*.cpp') $(shell find . -name '*.hpp')

clean:
	rm build/ -rf
