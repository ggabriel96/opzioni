import os
import re

from conans import ConanFile, Meson
from conans.tools import collect_libs, load


class OpzioniConan(ConanFile):
    name = "opzioni"
    license = "BSL-1.0"
    author = "Gabriel Galli (ggabriel96@hotmail.com)"
    url = "https://github.com/ggabriel96/opzioni"
    description = "A simple command line arguments library for C++"
    topics = (
        "CLI",
        "parser",
        "options",
        "terminal",
        "arguments",
        "parameters",
        "command-line",
        "command-line-parser",
    )

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    generators = "pkg_config"
    requires = "fmt/[>=7.0.3 <8.0.0]"
    build_requires = "catch2/[>=2.13.0 <3.0.0]"
    exports_sources = ["meson.build", "meson_options.txt", "include/*", "src/*"]

    __meson_build_dir = "build"

    def set_version(self):
        content = load(os.path.join(self.recipe_folder, "meson.build"))
        version = re.search("version: '(.+)'", content).group(1)
        self.version = version.strip()

    def build(self):
        meson = Meson(self)
        meson.configure(build_dir=self.__meson_build_dir)
        meson.build()
        meson.test()

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.names["pkg_config"] = self.name
        self.cpp_info.libs = collect_libs(self)
