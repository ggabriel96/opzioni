.PHONY: all setup-gcc build test format clean

all: setup-gcc build test

setup-gcc:
	meson setup --wrap-mode forcefallback -Dexamples=True build/

build:
	meson compile -C build/

test:
	meson test -C build/ --print-errorlogs

format:
	clang-format --verbose -i \
		examples/*.cpp \
		include/opzioni/*.hpp \
		src/*.cpp \
		tests/*.cpp

clean:
	rm -rf build/
