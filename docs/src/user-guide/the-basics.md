# The basics

opzioni is made up of simple concepts: programs and arguments.
A program is either the root or is a command of another program.
Meanwhile, arguments describe the inputs that a program expects from the user.
In code, programs are represented by `Program` and arguments by `Arg`.


## Including opzioni

Including opzioni is as simple as `include <opzioni.hpp>` (or in quotes if [building as an example](#building-as-an-example)).
For instance:

```cpp
#include <string_view>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using namespace opzioni;
}
```

The `using` directive for the `opzioni` namespace is just to make things easier for now.
You can always import the desired names individually later.

Also, I'm using the awesome [fmt](https://fmt.dev) library.
You should check it out!


## Declaring a program

In order to create a `Program`, we call one of its two constructors:
one just takes a name; the other takes a name and a title:

```cpp
constexpr auto curl = Program("curl", "transfer a URL");
```

Note that it is marked `constexpr`, so it means that `curl` is a *constant expression*, that is, its value is known at compile-time.


## Adding arguments

An argument can be one of three types: positional, option, or flag.
These types are created via the functions `Pos()`, `Opt()`, and `Flg()`, respectively.
`Pos()` only takes a name, while `Opt()` and `Flg()` take a name and an abbreviation.
These functions are just helpers, since they all construct an `Arg`, but they set the appropriate values for the type of argument being created.
These defaults are:

- positionals and options are initially of type `std::string_view`
- flags are initially of type `bool`
- positionals are required
- options and flags are optional
- options have `""` as default value
- flags have `false` as default value

Although possible, if the constructor of `Arg` were to be called, the argument type and all these defaults would have to be manually specified, requiring more typing.
`Arg` is an aggregate, though, which eases things a bit.

Arguments are added to a program via its `add` member function. Let's add some to our `curl` example:

```cpp
constexpr auto curl =
  Program("curl", "transfer a URL")
    .add(Pos("url").help("The URL to transfer"))
    .add(Opt("request", "X").help("The HTTP method to use"))
    .add(Flg("verbose", "v").help("Make the operation more talkative"))
    .add(Help());
```

Since we get an `Arg` after calling one of the argument factories, we can further customize it with its member functions.
The first one we used here is `.help()`, which serves the purpose of setting descriptions to arguments.
These descriptions are what appears in the automatic help text that opzioni generates.

To tell opzioni that automatic help text is desired, we simply add a `Help()`.
Similar to the previous functions, it's just a helper, but this time to create a very common flag, which is to show the program help.


## Parsing the CLI

Now that our very simple `curl` has some arguments, we can try parsing the CLI and see what we get.
We just need to call `curl` with `argc` and `argv`:

```cpp
auto const map = curl(argc, argv);
```

The result is a map of arguments of what was parsed from the CLI.
Note that `map` is *not* `constexpr`, since its value depends on runtime information.

opzioni also provide automatic error handling when calling the call operator of `Program`.
This means that, if the user gives `curl` some invalid arguments, the default behavior is to terminate the program and show an error message alongside the usage of the program.
This is explained later, but this behavior can be changed either while keeping the automatic error handling or not.


## Getting the results

There are a few ways of getting the results out of the map:

```cpp
std::string_view const url = map["url"]; // 1
auto const url = map.as<std::string_view>("url"); // 2
auto const url = map["url"].as<std::string_view>(); // 3
```

Choose whichever you like most.
However, it is always the long name of the argument that is used to get the result from the map.
So, for example, to get the value of the `request` option, it's always via the name `request`, never `X`.

Now let's print them out!

```cpp
std::string_view const url = map["url"];
std::string_view const request = map["request"];
bool const verbose = map["verbose"];
fmt::print("url: {}\n", url);
fmt::print("request: {}\n", request);
fmt::print("verbose: {}\n", verbose);
```


## Full C++ code

```cpp
#include <string_view>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
  using namespace opzioni;

  constexpr auto curl =
    Program("curl", "transfer a URL")
      .add(Pos("url").help("The URL to transfer"))
      .add(Opt("request", "X").help("The HTTP method to use"))
      .add(Flg("verbose", "v").help("Make the operation more talkative"))
      .add(Help());

  auto const map = curl(argc, argv);

  std::string_view const url = map["url"];
  std::string_view const request = map["request"];
  bool const verbose = map["verbose"];
  fmt::print("url: {}\n", url);
  fmt::print("request: {}\n", request);
  fmt::print("verbose: {}\n", verbose);
}
```

## Building as an example

1. Put the code above in a file in the `examples/` directory, say, `curl.cpp`.

1. Add an `executable` entry for it in `examples/meson.build`, just like the other examples.

    ```
    curl = executable(
        'curl', 'curl.cpp',
        dependencies: [fmt_dep, opzioni_dep]
    )
    ```

1. Run `make build`

1. Test your new CLI!

    See the automatic help:
    
    ```sh
    ./build/examples/curl --help
    ```
    
    Test that URL is really required by giving no arguments to the executable:

    ```sh
    ./build/examples/curl
    ```

    Give it a valid set of arguments and see the output:    

    ```sh
    ./build/examples/curl -X GET google.com
    ```


## Supported formats of arguments

There are many ways in which a user might provide arguments to a program.
The following list contains the rules that opzioni follows when parsing them.

- Arguments that don't start with `-` or are just a single `-` are considered positionals.

    Example: `rm file.txt`, where `file.txt` is a positional argument of `rm`.

- Commands are a special case of positional arguments in which they exactly match the name of some program added as command.

    Example: `git clone <clone args...>`, where `clone` is a command of `git`.
  
- An argument starting with a single `-` followed by one or more letters can be
    one short flag or many short flags together if all characters match the abbreviation of existing flags;
    or a short option immediately followed by its value if the first character matches the abbreviation of an existing option.
    In the case of an option, the value may be optionally preceded by `=` or it can be the next argument in the command-line.

    Example:

    - `pip -h`, where `h` is a flag of `pip`.

    - `tar -vxz`, where `v`, `x`, and `z` are three flags of `tar`.

    - `g++ -O2` or `g++ -O=2`, where `O` is an option of `g++` and `2` is its value.

        Note that both `-O2` and `-O=2` are a single argument.

    - `curl -X GET`, where `X` is an option of `curl` and `GET` is its value.

        This is a special case in which the argument (`-X`) is an existing option and the argument that immediately follows it would be parsed as a positional argument.
    
    Passing arguments like `set -euxo pipefail` **is intentionally not supported**.
    The format `-abc` **always** means many flags together or an option `a` followed by its value `bc`.

- An argument starting with two `-` followed by any number of characters is either a flag or an option.
   it is considered a flag If the name exactly matches an existing flag, otherwise it is considered an option.
   In the case of an option, the value may be optionally preceded by `=` or it can be the next argument in the command-line.

    Example:

    - `pip --help`, where `help` is a flag of `pip`.

    - `wget --timeout=10`, where `timeout` is an option of `wget` and `10` is its value.

        Note that `--timeout=10` is a single argument and `--timeout10` would not work the same way (it would be parsed as a flag).

    - `wget --user-agent Firefox`, where `user-agent` is an option of `wget` and `Firefox` is its value.

        Similarly to short flags and options, this is a special case in which the argument (`--user-agent`) is an existing option and the argument that immediately follows it would be parsed as a positional argument.

- An argument that is exactly `--` indicates that everything that follows should always be parsed as positional arguments.
