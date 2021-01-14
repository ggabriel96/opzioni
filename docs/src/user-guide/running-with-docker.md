# Running with Docker

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
