from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class MTBHParticleSim(ConanFile):
    name = "multithreaded-barnes-hut-particle=sim"
    version = "0.1"
    package_type = "application"

    license = "Unlicense"
    author = "N. Sanchirico <lnk2past@gmail.com>"
    description = "A Multithreaded implementation of the Barnes-Hut Approximation"

    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*"

    def requirements(self):
        self.requires("nlohmann_json/3.12.0", force=True)
        self.requires("pybind11/3.0.1", force=True)
        self.requires("pybind11_json/0.2.13", force=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    

    
