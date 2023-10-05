from conan import ConanFile
from conan.tools.files import load
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
import os, shutil, json

class Recipe(ConanFile):
    def __init__(self, display_name):
        super().__init__(display_name)

        # Read metadata from file
        package_json = json.loads(load(self, os.path.join(self.recipe_folder, "package.json")))
        
        self.CMAKE_PROJECT_NAME = package_json['name']
        self.name = self.CMAKE_PROJECT_NAME.lower()
        
        self.version = package_json['version']
        self.url = package_json['homepage']
        self.description = package_json['description']
    
    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    
    # Export files
    exports = "package.json"
    exports_sources = "package.json", "CMakeLists.txt", "include/*", "test/*"
    no_copy_source = True
    
    def requirements(self):
        self.test_requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)
        
    def generate(self):
        # Import dependencies 
        deps = CMakeDeps(self)
        deps.generate()

        # Generate toolchain with conan configurations for VS
        tc = CMakeToolchain(self, generator="Ninja Multi-Config")

        # Do not generate user presets due to unsupported schema in VS2019
        tc.user_presets_path = None 

        # Support older versions of the JSON schema
        tc.cache_variables["CMAKE_TOOLCHAIN_FILE"] = os.path.join(self.generators_folder, tc.filename)
        tc.cache_variables["CMAKE_INSTALL_PREFIX"] = "${sourceDir}/out/install"

        # Generate the CMake
        tc.generate()

        # Link the generated presets to the root
        presets_gen = os.path.join(self.generators_folder, "CMakePresets.json")
        presets_usr = os.path.join(self.source_folder, "CMakeUserPresets.json")

        shutil.copyfile(src=presets_gen, dst=presets_usr)
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()
        
    def package(self):
        cmake = CMake(self)
        cmake.install()
        
    def package_id(self):
        self.info.clear()
        
    def package_info(self):
        # For header-only packages, libdirs and bindirs are not used
        # so it's necessary to set those as empty.
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []