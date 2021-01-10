# opzioni

opzioni is a command line arguments parser library for C++.

# Goals

The goals of this library, in order of importance, are:

1. **Be as simple and enjoyable as possible.**

    This mainly targets the user of the library, but also includes the user of the command line tool built with it.

1. **`constexpr`-all-the-things.**

    Most of the time, all the information needed to build a command line interface is available at compile-time, so we should take advantage of that.

1. **If it compiles, it works.**

    That's utopic, but that's what is being strived for.
    It's also very closely related to the previous goal.
    We should be able to detect most errors at compile-time and provide decent diagnostics.

1. **_Try_ not to repeat yourself.**

    When specifying a CLI, if some information was already given to the library, that same information should not be needed again.
    For example, if the type of an argument was already specified, the user should not be asked to tell the type again.
    Unfortunately that is very hard, so some places still require duplicate information.

1. **Be bleeding-edge.**

    This library requires C++20.
    That limits a lot its potential users, but also allows for the use of the new and powerful features of C++.
    It also helps to accomplish the previous goals.

# Disclaimer

- This is a **personal project** with **no promise of maintainability** for the time being.

    I started it to learn more about C++ and its new features.

- Although it is *not* in *early* development, since I'm working on it for months and iterated over it many times,
    **it is not stable or production-ready**.

- There are **many unit tests missing**.

- I frequently changed the interface of the library and I'm **not afraid of changing it radically again** if I think it would improve the UX.

    Another example is the names of the `namespaces` and what is in them.

- There is *a lot* of polish and optimization work to do.

- There is a whole documentation to write.

# Sneak peek

The code below is a fully working example, taken from [`examples/hello.cpp`][examples/hello], only reformatted and with quotes changed to angle brackets in the `#include`.
Feel free to take a look at the other, more complex, examples in the same directory.

```cpp
#include <iostream>
#include <string_view>

#include <opzioni.hpp>

int main(int argc, char const *argv[]) {
  using namespace opzioni;

  constexpr auto hello =
      Program("hello").version("0.1").intro(
          "Greeting people since the dawn of computing") +
      Help() * Version() *
          Pos("name").help("Your name please, so I can greet you");

  auto const args = hello(argc, argv);
  std::string_view const name = args["name"];
  std::cout << "Hello, " << name << "!\n";
}
```

That gives us:

1. Automatic help with `--help` or `-h`

    ```
    $ ./build/examples/hello -h
    hello 0.1

    Greeting people since the dawn of computing

    Usage:
        hello <name> [--help] [--version]

    Positionals:
        name             Your name please, so I can greet you

    Options & Flags:
        -h, --help       Display this information
        -V, --version    Display the software version
    ```

1. Automatic version with `--version` or `-V`

    ```sh
    $ ./build/examples/hello -V
    hello 0.1
    ```

1. Automatic error handling

    ```
    $ ./build/examples/hello Gabriel Galli
    Unexpected positional argument `Galli`. This program expects 1 positional arguments

    Usage:
        hello <name> [--help] [--version]
    ```

1. And finally:

    ```
    $ ./build/examples/hello "Gabriel Galli"
    Hello, Gabriel Galli!
    ```

# Getting started

opzioni is not published anywhere yet.
The goal is to eventually make it available on:

- [Conan](https://conan.io/)
- [vcpkg](https://github.com/microsoft/vcpkg)
- Meson's [Wrap DB](https://wrapdb.mesonbuild.com/)
- [conda-forge](https://conda-forge.org/)
- [Spack](https://spack.io/)

Meanwhile, there are a few options to build and try it out. See the TLDR below.

## TLDR

There are few options to get up to speed on building opzioni:

- If you're familiar with Docker, there is a `Dockerfile` and a `docker-compose.yml` in [`.devcontainer/`][.devcontainer/].
    Also, the project is already configured to work with [VS Code Remote Containers](https://code.visualstudio.com/docs/remote/containers).

- If you're familiar with [NixOS](https://nixos.org/), there is a [`shell.nix`][shell.nix] ready to use.

Once in your chosen environment, simply run `make`.
That should download the build dependencies with Conan and build the whole project with Meson and ninja.
The [`Makefile`][Makefile] is just a simple helper to avoid having to remember all the commands.

## Dependencies

All dependencies below might still work if on earlier minor versions, but I'm not sure.
I still gotta pin them (except for `fmt` and `Catch2`, which are somewhat pinned in the build definitions).
**I always try to go for the latest versions.**

- GCC >= 10.2. Concepts and Ranges support is required.
- [meson](https://mesonbuild.com/) >= 0.50 (gotta check that)
- [ninja](https://ninja-build.org/) >= 1.10
- [fmt](https://fmt.dev/) >= 7.0.3
- [Catch2](https://github.com/catchorg/Catch2/) >= 2.13.0
- ClangFormat 10 if willing to apply the formatting as checked in CI

# License

opzioni's license is the [Boost Software License (BSL) 1.0][license].

This means you are **free to use** this library **as you wish** and see fit.

It is only needed to provide a copy of the license if the source is also being distributed.

In other words, **there is no need to bundle opzioni's license with your binary**.

<!-- links -->
[issues/5]: https://github.com/ggabriel96/opzioni/issues/5
[examples/hello]: https://github.com/ggabriel96/opzioni/blob/main/examples/hello.cpp
[.devcontainer]: https://github.com/ggabriel96/opzioni/blob/main/.devcontainer/
[Makefile]: https://github.com/ggabriel96/opzioni/blob/main/Makefile
[shell.nix]: https://github.com/ggabriel96/opzioni/blob/main/shell.nix
[license]: https://github.com/ggabriel96/opzioni/blob/main/LICENSE
