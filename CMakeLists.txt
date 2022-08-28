cmake_minimum_required(VERSION 3.7)
project(PixiRetro CXX)

set(PXR_SOURCE
	src/pxr_bmp.cpp
	src/pxr_collision.cpp
	src/pxr_engine.cpp
	src/pxr_gfx.cpp
	src/pxr_hud.cpp
	src/pxr_input.cpp
	src/pxr_log.cpp
	src/pxr_particle.cpp
	src/pxr_rand.cpp
	src/pxr_rc.cpp
	src/pxr_sfx.cpp
	src/pxr_wav.cpp
	src/pxr_xml.cpp)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(PixiRetro ${PXR_SOURCE})
target_compile_features(PixiRetro PRIVATE cxx_std_17)
target_include_directories(PixiRetro PUBLIC include ${CONAN_INCLUDE_DIRS})
target_link_libraries(PixiRetro ${CONAN_LIBS})
