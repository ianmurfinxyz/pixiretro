cmake_minimum_required(VERSION 3.7)
project(PixiRetro CXX)

set(PXR_SOURCE
	Source/pxr_bmp.cpp
	Source/pxr_collision.cpp
	Source/pxr_engine.cpp
	Source/pxr_gfx.cpp
	Source/pxr_hud.cpp
	Source/pxr_input.cpp
	Source/pxr_log.cpp
	Source/pxr_particle.cpp
	Source/pxr_rand.cpp
	Source/pxr_rc.cpp
	Source/pxr_sfx.cpp
	Source/pxr_wav.cpp
	Source/pxr_xml.cpp)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(PixiRetro ${PXR_SOURCE})
target_compile_features(PixiRetro PRIVATE cxx_std_17)
target_include_directories(PixiRetro PUBLIC Include ${CONAN_INCLUDE_DIRS})
target_link_libraries(PixiRetro ${CONAN_LIBS})
