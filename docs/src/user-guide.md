# User guide

## Running with Docker

Since opzioni is not available in any package repository yet, this guide will cover trying it out with Docker, just like I do during development.

1. First, make sure you have Docker and Docker Compose installed.
    
    Instructions for installing Docker can be found [here](https://docs.docker.com/engine/install/) (just select your OS) and for Compose [here](https://docs.docker.com/compose/install/).

1. If you're a VS Code user, you may want to install the [Remote Development](https://code.visualstudio.com/docs/remote/remote-overview) extension pack (ID `ms-vscode-remote.vscode-remote-extensionpack`) to open the container in VS Code.

1. Clone the repo.

    ```sh
    git clone https://github.com/ggabriel96/opzioni.git
    ```

1. Run the container.

    If on VS Code, hit <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd> and select the option `Remote-Containers: Reopen in Container`.
    If not on VS Code, you still might be able to open the project inside the container (e.g. with CLion), it just happens that I don't have a license and hence haven't tried it.

    If running the container through an IDE is not desired, you may open a terminal, navigate to the project, and run:

    ```sh
    docker build -f .devcontainer/Dockerfile -t opzioni .
    docker run --rm -it -v $(pwd):/home/conan/opzioni:Z -w /home/conan/opzioni/ opzioni
    ```

    The `:Z` option is unnecessary if not using SELinux.

    Having built and run the container from the terminal, you may edit any file from outside the container as usual.

1. Once inside the container, run `make`.

    This will fetch the dependencies with Conan and build the whole project with Meson, including the examples and unit tests.
    
    - run `make build` for additional builds
    - run `make test` to run the unit tests
    - run `make format` to format the code with ClangFormat
    - run `make clean` to clean everything (after this, another full `make` is needed)

1. You're done!

    The compiled examples will be in `build/examples/`.
    For example, to run the main example:

    ```sh
    ./build/examples/main --help
    ```

## Writing a new CLI from scratch

Let's start a **very** simple `curl`.

1. Create a new file in the `examples/` directory, say, `curl.cpp`.

1. Import opzioni and any other libraries you might want to use. Then add a `main`.
    For example:

    ```cpp
    #include <string_view>

    // the awesome fmt library is installed and available!
    #include <fmt/format.h>

    #include "opzioni.hpp"

    int main(int argc, char const *argv[]) {
        using namespace opzioni;
    }
    ```

    The `using` directive for the `opzioni` namespace is just to make things easier for now.
    You can always import the desired names individually later.

1. Now let's specify our CLI. First, declare a `Program`.

    ```cpp
    constexpr auto curl = Program("curl").intro("transfer a URL");
    ```

    This only creates a program named `curl` with a little introduction, so it doesn't do anything yet.

1. Add arguments to the program.

    Let's add a positional argument for the URL, an option for the HTTP method, and a help flag.

    ```cpp
    constexpr auto curl = Program("curl").intro("transfer a URL") +
                            Pos("url").help("The URL to transfer") *
                            Opt("request", "X").help("The HTTP method to use") *
                            Help();
    ```

    Note that:

    - a positional is created by calling `Pos()` with a name.
        Positionals are required by default.
    - an option is created by calling `Opt()` with a name and an optional short name.
        Options are optional by default and have an empty string as default value.
    - the automatic help text is created by calling `Help()`.
        It is a flag and, being so, is also optional by default.
    - the `.help()` member function is how descriptions are added to arguments.
        They will appear in the automatic help text.
    - there is a `*` between every pair of arguments. This creates an array of arguments to be added to the `Program`.
    - there is a `+` between the `Program` and the arguments. This will add the array of arguments to the `Program`.

1. Parse the CLI.

    We just need to call `curl` with `argc` and `argv`. As simple as that!

    ```cpp
    auto const map = curl(argc, argv);
    ```

    The result is a map of arguments of what was parsed from the CLI.

1. Print the results.

    There are a few ways of getting the results out of the map:

    ```cpp
    std::string_view const url = map["url"]; // 1
    auto const url = map.as<std::string_view>("url"); // 2
    auto const url = map["url"].as<std::string_view>(); // 3
    ```

    Choose whichever you like most.
    However, it is always the long name of the argument that is used to get the result from the map.

    Now let's print them out!

    ```cpp
    std::string_view const url = map["url"];
    std::string_view const request = map["request"];
    fmt::print("url: {}\n", url);
    fmt::print("request: {}\n", request);
    ```

1. And that's it! Now let's build the CLI and give it a try!

    Add an `executable` entry for it in `examples/meson.build`, just like the other examples.

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

    Give it all arguments and see the output:    

    ```sh
    ./build/examples/curl -X GET google.com
    ```

Just for convenience, here is the full C++ code:

```cpp
#include <string_view>

#include <fmt/format.h>

#include "opzioni.hpp"

int main(int argc, char const *argv[]) {
    using namespace opzioni;

    constexpr auto curl = Program("curl").intro("transfer a URL") +
                            Pos("url").help("The URL to transfer") *
                            Opt("request", "X").help("The HTTP method to use") *
                            Help();

    auto const map = curl(argc, argv);

    std::string_view const url = map["url"];
    std::string_view const request = map["request"];
    fmt::print("url: {}\n", url);
    fmt::print("request: {}\n", request);
}
```
