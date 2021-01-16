# Starting from scratch

This guide shows how to use opzioni right from the beginning: from `#include`-ing it to specifying the CLI and consuming the parsed values.
As an example, we'll write a **very** simple `curl`.

## Include opzioni

Including opzioni is as simple as `include <opzioni.hpp>` (or in quotes if [building as an example](#build-as-an-example)).
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

## Declare a program

Now let's specify our CLI.
First, declare a `Program`.

```cpp
constexpr auto curl = Program("curl", "transfer a URL");
```

This only creates a program named `curl` with a little title, so it doesn't do anything yet.
Note that it is marked `constexpr`, so it means that `curl` is a *constant expression*, that is, its value is known at compile-time.


## Add arguments

Let's add a positional argument for the URL, an option for the HTTP method, a flag for verbosity, and some help text.

```cpp
constexpr auto curl = Program("curl", "transfer a URL") +
                        Pos("url").help("The URL to transfer") *
                        Opt("request", "X").help("The HTTP method to use") *
                        Flg("verbose", "v").help("Make the operation more talkative") *
                        Help();
```

Note that:

- a positional is created by calling the `Pos()` function with a name.
    **Positionals are required by default.**

- an option is created by calling the `Opt()` function with a name and an optional short name.
    **Options are optional by default and have an empty string as default value.**

- a flag is created by calling the `Flg()` function with a name and an optional short name.
    **Flags are optional by default and have `false` as default value.**

- `Pos`, `Opt`, and `Flg` are functions, not constructors.
    This is because the actual *type* of the arguments is `Arg` and it represents any type of argument.
    If its constructor were to be called, the type would have to be specified, requiring more typing.
    So these functions exist as helpers (or factories) for the supported kinds: positional, option, and flag.

- the `.help()` member function is how descriptions are added to arguments.
    They will appear in the automatic help text.

- calling `Help()` adds a help flag that handles the automatic help text.
    It is just a shorthand for this very common flag, so there's nothing special about it.
    Here how it's done:

    ```cpp
    Flg("help", "h").help(description).action(actions::print_help);
    ```

    Where `description` is `"Display this information"` by default.

- there is a `*` between every pair of arguments.
    This creates an array of arguments *at compile-time* to be added to the `Program`.

- there is a `+` between the `Program` and the arguments.
    This adds the array of arguments to the `Program` *at compile-time*.

There are way more features available (see [Customization](customization.md)), but we'll keep things simple for now.

## Parse the CLI

We just need to call `curl` with `argc` and `argv`. As simple as that!

```cpp
auto const map = curl(argc, argv);
```

The result is a map of arguments of what was parsed from the CLI.
Note that `map` is *not* `constexpr`, since its value depends on runtime information.

## Get the results

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

    constexpr auto curl = Program("curl", "transfer a URL") +
                            Pos("url").help("The URL to transfer") *
                            Opt("request", "X").help("The HTTP method to use") *
                            Flg("verbose", "v").help("Make the operation more talkative") *
                            Help();

    auto const map = curl(argc, argv);

    std::string_view const url = map["url"];
    std::string_view const request = map["request"];
    bool const verbose = map["verbose"];
    fmt::print("url: {}\n", url);
    fmt::print("request: {}\n", request);
    fmt::print("verbose: {}\n", verbose);
}
```

## Build as an example

1. Put the newly created file in the `examples/` directory, say, `curl.cpp`.

1. Add an `executable` entry for it in `examples/meson.build`, just like the other examples.

    ```py
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
