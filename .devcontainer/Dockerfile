FROM conanio/gcc10:latest

USER root
RUN DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get -y install --no-install-recommends ssh

USER conan
RUN pip install meson
