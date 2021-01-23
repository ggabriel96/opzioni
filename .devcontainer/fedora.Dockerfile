FROM registry.fedoraproject.org/f33/fedora-toolbox:33
SHELL [ "/bin/bash", "-euo", "pipefail", "-c" ]

RUN rpm --import https://packages.microsoft.com/keys/microsoft.asc && \
    echo $'[code]\n\
name=Visual Studio Code\n\
baseurl=https://packages.microsoft.com/yumrepos/vscode\n\
enabled=1\n\
gpgcheck=1\n\
gpgkey=https://packages.microsoft.com/keys/microsoft.asc\n' > /etc/yum.repos.d/vscode.repo

# libX11-xcb is for VS Code
RUN dnf install clang clang-tools-extra code fish flatpak gcc-c++ libX11-xcb make -y

RUN pip install conan meson ninja
