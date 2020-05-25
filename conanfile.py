import os
import re
from typing import List

from conans import ConanFile, Meson
from conans.tools import load, mkdir


class OpzioniConan(ConanFile):
    name = "opzioni"
    license = "MIT"
    author = "Gabriel Galli (ggabriel96@hotmail.com)"
    url = "https://github.com/ggabriel96/opzioni"
    description = "A simple command line arguments library for C++"
    topics = "CLI", "terminal", "options", "arguments", "parameters"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "build_examples": [True, False]}
    default_options = {"shared": False, "build_examples": False}

    source_folder = "src"
    build_folder = "build"
    generators = "pkg_config"
    exports_sources = "include/*", f"{source_folder}/*"
    build_requires = "fmt/6.2.1"

    def set_version(self):
        content = load(
            os.path.join(self.recipe_folder, self.source_folder, "meson.build")
        )
        version = re.search("version: '(.+)'", content).group(1)
        self.version = version.strip()

    def build(self):
        if not self.develop:
            self.source_folder = os.path.join(self.source_folder, "src")
        meson_options = {"examples": self.options.build_examples}
        meson = Meson(self)
        meson.configure(
            defs=meson_options,
            source_folder=self.source_folder,
            build_folder=self.build_folder,
        )
        meson.build()

    # def package(self):
    #     self.copy("*.hpp", dst="include", src="src")
    #     self.copy("*.lib", dst="lib", keep_path=False)
    #     self.copy("*.dll", dst="bin", keep_path=False)
    #     self.copy("*.dylib*", dst="lib", keep_path=False)
    #     self.copy("*.so", dst="lib", keep_path=False)
    #     self.copy("*.a", dst="lib", keep_path=False)

    # def package_info(self):
    #     self.cpp_info.libs = ["opzioni"]
