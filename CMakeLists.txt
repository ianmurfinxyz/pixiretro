cmake_minimum_required(VERSION 3.15)
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

add_library(PixiRetro ${PXR_SOURCE})
target_include_directories(PixiRetro PUBLIC Include)

set(PXR_INCLUDE
	Include/pxr_bmp.h
	Include/pxr_collision.h
	Include/pxr_engine.h
	Include/pxr_gfx.h
	Include/pxr_hud.h
	Include/pxr_input.h
	Include/pxr_log.h
	Include/pxr_particle.h
	Include/pxr_rand.h
	Include/pxr_rc.h
	Include/pxr_sfx.h
	Include/pxr_wav.h
	Include/pxr_rect.h
	Include/pxr_mathutil.h
	Include/pxr_game.h
	Include/pxr_color.h
	Include/pxr_vec.h
	Include/pxr_xml.h)

set_target_properties(PixiRetro PROPERTIES PUBLIC_HEADER ${PXR_INCLUDE})

add_definitions("-std=c++11")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

target_link_libraries(PixiRetro ${CONAN_LIBS})

install(TARGETS PixiRetro DESTINATION "./Target"
	PUBLIC_HEADER DESTINATION Include
	RUNTIME DESTINATION Bin
	ARCHIVE DESTINATION Lib
	LIBRARY DESTINATION Lib)
	
