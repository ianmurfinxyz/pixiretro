cmake_minimum_required(VERSION 3.15)
project(PixiRetro CXX)

add_definitions("-std=c++11")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

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

add_library(PixiRetro ${PXR_SOURCE})
target_include_directories(PixiRetro PUBLIC Include)
target_link_libraries(PixiRetro ${CONAN_LIBS})

	
