import os
import re
from typing import List

from conans import ConanFile, Meson
from conans.tools import collect_libs, load, mkdir


class OpzioniConan(ConanFile):
    name = "opzioni"
    license = "MIT"
    author = "Gabriel Galli (ggabriel96@hotmail.com)"
    url = "https://github.com/ggabriel96/opzioni"
    description = "A simple command line arguments library for C++"
    topics = "CLI", "terminal", "options", "arguments", "parameters"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

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
        if self.source_folder == self.build_folder:
            self.source_folder = os.path.join(self.source_folder, "src")
        meson = Meson(self)
        meson.configure(
            source_folder=self.source_folder, build_folder=self.build_folder,
        )
        meson.build()

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.names["pkg_config"] = "opzioni"
        self.cpp_info.libs = collect_libs(self)
