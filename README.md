# opzioni

opzioni is a command line arguments parser library for C++.

## Goals

The goals of this library, in order of importance, are:

1. **Be as simple and enjoyable as possible.**

    This mainly targets the user of the library, but also includes the user of the command line tool built with it.

1. **`constexpr`-all-the-things.**

    Most of the time, all the information needed to build a command line interface is available at compile-time, so we should take advantage of that.

1. **If it compiles, it works.**

    That's utopic, but that's what is being strived for.
    It's also very closely related to the previous goal.
    We should be able to detect most errors at compile-time and provide decent diagnostics.

1. **Don't repeat yourself.**

    When specifying the CLI, if some information was already given to the library, that same information should not be needed again.
    For example, the type of an argument is always known during setup, so the user should not be asked to inform the type when querying that argument.

1. **Be bleeding-edge.**

    This isn't really a goal, but a fact: this library requires C++23.
    That limits a lot its potential users (so there are plans to at least go back to C++20),
    but also allows for the use of the new and powerful features of C++.
    It also helps to accomplish the previous goals.

## Disclaimer

- This is a **personal project** with **no promise of maintainability** for the time being.

- I have just rewritten the entire library, so **it is neither stable nor production-ready**.

- There are **no unit tests** (since the rewrite).

- There is *a lot* of polish and optimization work to do.

- There is a whole documentation to write.

## Sneak peek

The code below is a fully working example, taken from [`examples/hello.cpp`](examples/hello.cpp),
only with quotes changed to angle brackets in the `#include`.
Feel free to take a look at the other, more complex, examples in the same directory.

```cpp
#include <print>

#include <opzioni/cmd.hpp>

int main(int argc, char const *argv[]) {
  auto hello_cmd = opz::new_cmd("hello", "1.0")
                     .intro("Greeting people since the dawn of computing")
                     .pos<"name">({.help = "Your name please, so I can greet you"})
                     .flg<"help", "h">(opz::default_help)
                     .flg<"version", "v">(opz::default_version);

  auto const map = hello_cmd(argc, argv);
  auto const name = map.get<"name">();
  std::print("Hello, {}!\n", name);
}
```

That gives us:

1. Type deduction of arguments (note the use of `auto`):

    ```cpp
    auto const name = map.get<"name">();
    ```

1. Automatic help with `--help` or `-h`

    ```
    $ ./build/examples/hello -h
    hello 1.0

    Greeting people since the dawn of computing

    Usage:
        hello <name> [--help] [--version]

    Arguments:
        name             Your name please, so I can greet you
        -h, --help       Display this information
        -v, --version    Display hello's version
    ```

1. Automatic version with `--version` or `-v`

    ```
    $ ./build/examples/hello -v
    hello 1.0
    ```

1. Automatic error handling

    ```
    $ ./build/examples/hello Gabriel Galli
    Unexpected positional argument for `hello`: `Galli` (only 1 are expected)
    Usage:
        hello <name> [--help] [--version]
    ```

1. And finally:

    ```
    $ ./build/examples/hello "Gabriel Galli"
    Hello, Gabriel Galli!
    ```

## Getting started

opzioni is not published anywhere yet.
Meanwhile, it is kinda straightforward to build it locally, since just a simple conda environment is enough to bootstrap a development environment.
The build system is the awesome [Meson](https://mesonbuild.com/).

1. Download and install conda if you haven't already.

    You can either use [miniforge](https://conda-forge.org) (recommended, personally),
    or [miniconda](https://www.anaconda.com/docs/getting-started/miniconda/main).

1. Clone this repository:

    ```sh
    git clone https://github.com/ggabriel96/opzioni.git
    ```

1. Create the conda environment:

    ```sh
    conda env create -f environment.yml
    ```

1. Activate the newly created conda environment:

    ```sh
    conda activate opzioni
    ```

1. Build it!

    ```sh
    make
    ```

Note that the [`Makefile`](Makefile) is just a shortcut to the actual commands.
Feel free to inspect it and not use it.

## License

opzioni's license is the [Boost Software License (BSL) 1.0](LICENSE).

This means you are **free to use** this library **as you wish** and see fit.

It is only needed to provide a copy of the license if the source is also being distributed.

In other words, **there is no need to bundle opzioni's license with your binary**.
