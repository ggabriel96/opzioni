.PHONY: all install build package export ninja format clean

all: install build

install:
	conan install . \
		-if build/ \
		-b missing \
		-o opzioni:build_examples=True \
		-s opzioni:build_type=Debug \
		-s compiler.libcxx=libstdc++11

build:
	conan build -bf build/ .

package: install build
	conan package -bf build/ -pf package/ .

export: package
	conan export-pkg -pf package/ .

ninja:
	ninja -C build/

format:
	clang-format --verbose -i $(shell find . -name '*.cpp') $(shell find . -name '*.hpp')

clean:
	rm build/ package/ -rf
