from conans import ConanFile, CMake, tools
import os, shutil

class PixiRetroConan(ConanFile):
	name = "pixiretro"
	version = "0.9.0"
	license = "MIT"
	author = "Ian Murfin - github.com/ianmurfinxyz - ianmurfin@protonmail.com"
	url = "https://github.com/ianmurfinxyz/pixiretro_engine"
	description = "A small library for making 2D arcade games."
	topics = ("Game Framework", "2D Games", "Arcade Games")
	settings = "os", "compiler", "build_type", "arch"
	options = {"shared": [True, False], "fPIC": [True, False]}
	default_options = {"shared": False, "fPIC": True}
	build_subfolder = "build"
	source_subfolder = "source"
	user = "ianmurfinxyz"
	channel = "stable"
	generators = "cmake"
	exports_sources = ["CMakeLists.txt", "Source/*", "Include/*"]

	def config_options(self):
		if self.settings.os == "Windows":
			del self.options.fPIC

	def requirements(self):
		self.requires("tinyxml2/9.0.0")
		self.requires("opengl/system")
		self.requires("sdl/2.0.20")
		self.requires("sdl_image/2.0.5")
		self.requires("sdl_mixer/2.0.4")
		self.requires("sdl_ttf/2.0.18")

	def build(self):
		cmake = CMake(self)
		cmake.configure(build_folder=self.build_subfolder)
		cmake.build()

	def package(self):
		self.copy("*.h", dst="include", src=self.source_subfolder)
		self.copy("*.lib", dst="lib", keep_path=False)
		self.copy("*.a", dst="lib", keep_path=False)
		self.copy("*.exp", dst="lib", keep_path=False)
		self.copy("*.dll", dst="bin",keep_path=False)
		self.copy("*.so", dst="bin", keep_path=False)
		self.copy("*.pdb", dst="bin", keep_path=False)

	def package_info(self):
		self.cpp_info.includedirs = ['include']
		
		build_type = self.settings.get_safe("build_type", default="Release")
		postfix = "d" if build_type == "Debug" else ""
		
		if self.settings.os == "Windows":
			static = "-static" if self.options.shared else ""
			self.cpp_info.libs = [
				f"PixiRetro{static}{postfix}.lib"
			]
		elif self.settings.os == "Linux":
			extension = "so" if self.options.shared else "a"
			self.cpp_info.libs = [
				f"PixiRetro{static}{postfix}.{extension}"
			]
		
		self.cpp_info.libdirs = ['lib']
		self.cpp_info.bindirs = ['bin']
