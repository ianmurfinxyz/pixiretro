project('pixiretro', 'cpp', default_options: ['cpp_std=c++17', 'warning_level=2'])

pxr_src = [
  'source/pxr_bmp.cpp',
  'source/pxr_engine.cpp',
  'source/pxr_rc.cpp',
  'source/pxr_gfx.cpp',
  'source/pxr_input.cpp',
  'source/pxr_sfx.cpp',
  'source/pxr_log.cpp',
  'source/pxr_particle.cpp',
  'source/pxr_wav.cpp',
  'source/pxr_xml.cpp',
  'source/lib/tinyxml2/tinyxml2.cpp'
]

cc = meson.get_compiler('cpp')

projectdir = meson.current_source_dir()
lib_openal_dir = join_paths(projectdir, 'bin/lib/openal')
lib_openal = cc.find_library('openal', dirs: lib_openal_dir)
lib_m = cc.find_library('m', required: true)
lib_glx_mesa = cc.find_library('GLX_mesa', required: true)
lib_sdl2 = dependency('SDL2')

inc_pxr = include_directories('include')

library('pixiretro', 
        pxr_src, 
        dependencies: [lib_m, lib_glx_mesa, lib_sdl2, lib_openal], 
        include_directories: inc_pxr)
