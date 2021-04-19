
<p align="center">
  <img src="img/pixiretro_logo.png" atl="pixiretro_engine_logo">
</p>

## What's this?

Pixiretro is a game engine I made to develop small pixel art retro arcade games like Pacman, Space Invaders :space_invader:, Snake :snake: and Donkey Kong. The engine is implemented in C++ using SDL2, SDL2_mixer, Opengl and Tinyxml2. It runs on Linux only.

## Features
- A 2D pixel based software renderer with an opengl backend, which is not a contradiction! (see below)
- An SDL_mixer backed audio module that supports sound effects on multiple channels and music loop sequences.
- Custom file loading (.bmp and .wav)
- Custom lightweight and efficient random number generation using an xorwow generator and a std distribution. This generator maintains significantly less state than the common mersenne twister generator.
- A simple logging system to log info and errors to a log file or to stdout.
- 


## What it doesn't do

- There is no support for advanced hardware rendering techniques such as 2D lighting or shading. However you can write shader functions that are executed upon each pixel by the CPU.
- There is no music streaming for large audio files.

## Renderer

The gfx module maintains a set of virtual screens which you instruct it to create. These screens have fixed (virtual) pixel resolutions set upon creation but are designed so that the size of each (virtual) pixel when renderd to the window (the real screen) can change dynamically, i.e. the scale mapping between virtual pixels and real pixels can change. This allows for the creation of classic arcade games in which the size of the world was a fixed size in pixels (such as space invaders with its world size of 232x256 pixels) which are playable on any size screen, even an ultrawide of 3440x1440 pixels.

The virtual screens support basic transparency (alpha channel masking but not blending) and can be layered. This allows splitting up drawing into layers which can be independently updated. Thus providing a potential performance boost of reducing the load on the (CPU executed) software renderer drawing to the virtual screens; you can split drawing up into things which are updated every frame (such as moving game objects) and those which are static (such as static backgrounds).

Each virtual screen is rendered to the window via a single opengl draw call. Thus the demand on the graphics hardware is low. This is achieved using vertex arrays; each pixel in a virtual screen is represented as a vertex which is drawn as a point (GL_POINT). If your virtual screens are relatively low resolution (say 256x256 pixels) this approach is somewhat efficient. I wouldn't recommend it for high resolution games. The number of pixels, and thus number of vertices, in a virtual screen is the square of its size, thus demand on the GUP will increase exponentially as you increase the size of a virtual screen.

## License
