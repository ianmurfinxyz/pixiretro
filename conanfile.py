from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class PixiRetroConan(ConanFile):
	name = "PixiRetro"
	version = "0.9.0"
	
	# optional metadata
	license = "MIT"
	author = "Ian Murfin - github.com/ianmurfinxyz - ianmurfin@protonmail.com"
	url = "<Package recipe repository url here, for issues about the package>"
	description = "A small framework for making 2D arcade games."
	topics = ("Game Framework", "2D Games", "Arcade Games")
	
	# Binary configuration
	settings = "os", "compiler", "build_type", "arch"
	options = {"shared": [True, False], "fPIC": [True, False]}
	default_options = {"shared": False, "fPIC": True}
	
	scm = {
		"type": "git",
		"subfolder": "Source",
		"url": "auto",
		"revision": "auto"
	}
	
	def config_options(self):
		if self.settings.os == "Windows":
			del self.options.fPIC
			
	def layout(self):
        cmake_layout(self)
	
	def requirements(self):
		self.requires("tinyxml2/9.0.0")
		self.requires("sdl/2.0.20")
		self.requires("sdl_image/2.0.5")
		
	def generate(self):
		tc = CMakeToolchain(self)
		tc.generate()
	
	def build(self):
		cmake = CMake(self)
		cmake.configure()
		cmake.build()
	
	def package(self):
		cmake = CMake(self)
		cmake.install()
	
	def package_info(self):
		self.cpp_info.libs = ["PixiRetro"]
