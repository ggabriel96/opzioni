.PHONY: all build conan-build conan-install clean

all: conan-install conan-build

build:
	ninja -C build/

conan-build:
	conan build -bf build/ .

conan-install:
	conan install -if build/ -b missing -o opzioni:build_examples=True -s opzioni:build_type=Debug .

clean:
	rm build/ -rf