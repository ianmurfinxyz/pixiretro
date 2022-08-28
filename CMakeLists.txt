cmake_minimum_required(VERSION 3.7)
project(pixiretro CXX)

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

add_library(pixiretro ${PXR_SOURCE})
target_compile_features(pixiretro PRIVATE cxx_std_17)
target_include_directories(pixiretro PUBLIC include ${CONAN_INCLUDE_DIRS})
target_link_directories(pixiretro PUBLIC ${CONAN_LIB_DIRS})
target_link_libraries(pixiretro ${CONAN_LIBS})
