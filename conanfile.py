from conan import ConanFile
from conan.tools.cmake import cmake_layout


class HelloConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    # requires = "fmt/12.0.0"
    # default_options = {"fmt/*:shared": True}

    def layout(self):
        cmake_layout(self)
