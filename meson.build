project('pixiretro', 'cpp', default_options: ['cpp_std=c++17', 'warning_level=2'])

lib_tinyxml2_src = ['lib/tinyxml2/source/tinyxml2.cpp']
lib_tinyxml2_inc = include_directories('lib/tinyxml2/include')
lib_tinyxml2 = library('tinyxml2', 
                              lib_tinyxml2_src,
                              include_directories: lib_tinyxml2_inc)

pixiretro_src = [
  'source/bmpimage.cpp',
  'source/cutscene.cpp',
  'source/engine.cpp',
  'source/filerc.cpp',
  'source/gfx.cpp',
  'source/input.cpp',
  'source/sfx.cpp',
  'source/log.cpp',
  'source/particles.cpp',
  'source/wavesound.cpp',
  'source/xmlutil.cpp'
]

cc = meson.get_compiler('cpp')

projectdir = meson.current_source_dir()
lib_openal_dir = join_paths(projectdir, 'lib/bin/openal')
lib_openal = cc.find_library('openal', dirs: lib_openal_dir)

lib_m = cc.find_library('m', required: true)
lib_glx_mesa = cc.find_library('GLX_mesa', required: true)

lib_sdl2 = dependency('SDL2')

pixiretro_inc = include_directories('include', 'lib/include')

library('pixiretro', 
        pixiretro_src, 
        link_with: lib_tinyxml2, 
        dependencies: [lib_m, lib_glx_mesa, lib_sdl2, lib_openal], 
        include_directories: pixiretro_inc)
