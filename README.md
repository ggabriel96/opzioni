# opzioni

opzioni is a command line arguments parser library for C++.
Its goals are, in order of importance:

1. **Be as simple and enjoyable as possible.**

    This mainly targets the user of the library, but also includes the user of the command line tool built with it.

1. **If it compiles, it works.**

    That's utopic, but that's what is being strived for.
    Most of the time, all the information needed to build a command line interface is available at compile-time, so we should take advantage of that.

1. **Be bleeding-edge.**

    This library requires C++20. That limits a lot its potential users, but also allows for the use of the new and powerful features of C++. It also helps to accomplish the previous goals.

These goals ought to be discussed in further detail in a separate document.

# Disclaimer

### I would say this is a hobby project to be used in other hobby projects:

- This is a **personal project** with **no promise of maintainability** for the time being.

    I started it to learn more about C++ and its new features.

- Although it is *not* in *early* development, since I'm working on it for months and iterated over it many times, **I do not consider it stable**.

- There are **many unit tests missing**.

- I frequently changed the interface of the library and I'm **not afraid of changing it radically again** if I think it would improve the UX.

    Another example is the names of the `namespaces` and what is in them.

- There is *a lot* of polish and optimization work to do.

# Hands-on

The code below is a fully working example, taken from [`examples/hello.cpp`](examples/hello.cpp), only reformatted and with quotes changed to angle brackets in the `#include`.

```cpp
#include <iostream>
#include <string_view>

#include <opzioni.hpp>

int main(int argc, char const *argv[]) {
  using namespace opzioni;

  auto const hello =
      Program("hello")
          .v("0.1")
          .intro("Greeting people since the dawn of computing") +
      Help() *
      Version() *
      Pos("name").help("Your name please, so I can greet you");

  auto const args = hello(argc, argv);
  std::string_view const name = args["name"];
  std::cout << "Hello, " << name << "!\n";
}
```

### That gives us:

1. Automatic help with `--help` or `-h`

    ```
    $ ./build/examples/hello -h
    hello 0.1

    Greeting people since the dawn of computing

    Usage:
        hello <name> [--help] [--version]

    Positionals:
        name                       Your name please, so I can greet you

    Options & Flags:
        -h, --help                 Display this information
        -V, --version              Display the software version
    ```

1. Automatic version with `--version` or `-V`

    ```sh
    $ ./build/examples/hello -V
    hello 0.1
    ```

1. Automatic error handling

    ```
    $ ./build/examples/hello 
    Missing required arguments: `name`

    $ ./build/examples/hello Gabriel Galli
    Unexpected positional argument `Galli`. This program expects 1 positional arguments
    ```

1. And finally:

    ```
    $ ./build/examples/hello "Gabriel Galli"
    Hello, Gabriel Galli!
    ```

### Let's examine this code step by step:

1. First, include opzioni with `#include <opzioni.hpp>`.

1. To gain easy access to opzioni's types and functions, add `using namespace opzioni;`.

    But that's a strong statement, so each name can be pulled into the current namespace too:

    ```cpp
    using opzioni::Program, opzioni::Help, opzioni::Version; // etc.
    ```

1. An instance of `Program` is created.

    It is given the name `hello`, version `0.1`, and a short introduction.

1. Arguments are added to `Program`.

    The built-in help and version and a positional called `name`.
    Note that there is an `operator*` between each argument and an `operator+` between the program and its arguments. This is better explained later.

1. The command line arguments are parsed.

    As simple as calling a function with `argc` and `argv` and returns a map of the results.

1. The parsed name is extracted from the resulting map and printed to `stdout`.

    It could also be used directly in `std::cout`:
    ```cpp
    std::cout << args.as<std::string_view>("name") << '\n'; // 1
    std::cout << args["name"].as<std::string_view>() << '\n'; // 2
    ```

### There are more complex examples in the [`examples/`](examples/) directory.

# License

opzioni's license is the [Boost Software License (BSL) 1.0](LICENSE/).

This means you are **free to use** this library **as you wish** and see fit.

It is only needed to provide a copy of the license if the source is also being distributed.

In other words, **there is no need to bundle opzioni's license with your binary**.
