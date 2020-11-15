FROM conanio/gcc10:latest

USER root
RUN DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get -y install --no-install-recommends neovim ssh zsh gdb
RUN usermod -s $(which zsh) conan

USER conan
RUN pip install meson
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended
