import os

from conans import ConanFile, Meson, tools


class OpzioniTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "pkg_config"

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()

    def test(self):
        if not tools.cross_building(self):
            self.run(f".{os.sep}main Unknown")
