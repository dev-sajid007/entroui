from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class EntroUIConan(ConanFile):
    name = "entroui"
    version = "1.0.0"
    license = "MIT"
    author = "EntroUI Contributors"
    description = "A lightweight C++20 declarative desktop UI framework"
    topics = ("ui", "desktop", "sdl3", "cpp20")
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "with_sdl": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "with_sdl": True,
        "with_tests": True,
        "spdlog/*:shared": False,
    }

    def requirements(self):
        self.requires("spdlog/1.17.0")
        if self.options.with_tests:
            self.requires("gtest/1.17.0")

    def layout(self):
        self.folders.source = "."
        self.folders.build = "."
        self.folders.generators = "generators"

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["ENTROUI_BUILD_SDL"] = bool(self.options.with_sdl)
        toolchain.variables["ENTROUI_BUILD_TESTS"] = bool(self.options.with_tests)
        toolchain.generate()
        deps = CMakeDeps(self)
        deps.generate()
