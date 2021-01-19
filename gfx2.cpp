
#include <vector>
#include <cstring>
#include <cinttypes>

#include "gfx.h"
#include "math.h"
#include "color.h"
#include "bmpimage.h"

namespace pxr
{
namespace gfx
{

// A virtual pixel of a layer/screen. This structure is designed to work with opengl
// function glInterleavedArrays with vertex format GL_C4UB_V2F (opengl 3.0).
struct VPixel
{
  VPixel() : _r{0}, _g{0}, _b{0}, _a{0}, _x{0}, y{0}{}

  uint8_t _r;
  uint8_t _g;
  uint8_t _b;
  uint8_t _a;
  float _x;
  float _y;
};

// A virtual screen of virtual pixels. Each rendering layer is an instance of screen.
struct Screen
{
  std::vector<VPixel> _pixels;
  std::vector<ColorBand> _bands;
  Vector2i _position;
  Vector2i _screenSize;
  PositionMode _pmode;
  SizeMode _smode;
  ColorMode _cmode;
  int _pixelSize;
};

struct Sprite
{
  std::vector<VPixel> _pixels; // flattened 2D array accessed [col + (row * width)]
  Vector2i _size;              // x(num cols) and y(num rows) dimensions of sprite.
};

static constexpr int openglVersionMajor = 3;
static constexpr int openglVersionMinor = 0;
static constexpr int alphakey = 0;

static Config _config;
static SDL_Window* _window;
static SDL_GLContext _glContext;
static iRect _viewport;
static Vector2i _windowSize;
static std::array<Screen, LAYER_COUNT> _screens;
static Color4u _bitmapColor;
static int _minPixelSize;
static int _maxPixelSize;

// Recalculate screen position, pixel sizes, pixel positions etc to a account for a change in 
// window size, display resolution or screen mode attributes.
static void autoAdjustScreen(Vector2i windowSize, Screen& screen)
{
  // recalculate pixel size.
  if(smode == SizeMode::AUTO_MAX){
    // note: integer math here thus all division results are implicitly floored as required.
    int pxw = windowSize._x / _screenSize._x;  
    int pxh = windowSize._y / _screenSize._y;
    _pixelSize = std::max(std::min(pxw, pxh), 1);
  }

  // recalculate screen position.
  switch(_pmode)
  {
  case PositionMode::MANUAL:
    // no change.
    break;
  case PositionMode::CENTER:
    _position._x = std::clamp((windowSize._x - (_pixelSize * _screenSize._x)) / 2, 0, windowSize._x);
    _position._y = std::clamp((windowSize._y - (_pixelSize * _screenSize._y)) / 2, 0, windowSize._y);
    break;
  case PositionMode::TOP_LEFT:
    _position._x = 0;
    _position._y = windowSize._y - (_pixelSize * _screenSize._y);
    break;
  case PositionMode::TOP_RIGHT:
    _position._x = windowSize._x - (_pixelSize * _screenSize._x);
    _position._y = windowSize._y - (_pixelSize * _screenSize._y);
    break;
  case PositionMode::BOTTOM_LEFT:
    _position._x = 0;
    _position._y = 0;
    break;
  case PositionMode::BOTTOM_RIGHT:
    _position._x = windowSize._x - (_pixelSize * _screenSize._x);
    _position._y = 0;
    break;
  }

  // recalculate pixel positions.
  
  // Pixels are drawn as an array of points of _pixelSize diameter. When drawing points in opengl, 
  // the position of the point is taken as the center position. For odd pixel sizes e.g. 7 the 
  // center pixel is simply 3,3 (= floor(7/2)). For even pixel sizes e.g. 8 the center of the 
  // pixel is considered the bottom-left pixel in the top-right quadrant, i.e. 4,4 (= floor(8/2)).
  int pixelCenterOffset = _pixelSize / 2;

  for(int row = 0; row < _screenSize._y; ++row){
    for(int col = 0; col < _screenSize._x; ++col){
      Pixel& pixel = _pixels[col + (row * _screenSize._x)];
      pixel._x = _position._x + (col * _pixelSize) + pixelCenterOffset;
      pixel._y = _position._y + (row * _pixelSize) + pixelCenterOffset;
    }
  }
}

static void setViewport(iRect viewport)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, viewport._w, 0.0, viewport._h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(viewport._x, viewport._y, viewport._w, viewport._h);
  _viewport = viewport;
}

bool initialize(Config config)
{
  _config = config;

  uint32_t flags = SDL_WINDOW_OPENGL;
  if(_config._fullscreen){
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    log::log(log::INFO, log::info_fullscreen_mode);
  }

  std::stringstream ss {};
  ss << "{w:" << _config._windowSize._x << ",h:" << _config._windowSize._y << "}";
  log::log(log::INFO, log::info_creating_window, std::string{ss.str()});

  _window = SDL_CreateWindow(
      _config._windowTitle.c_str(), 
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      _config._windowSize._x,
      _config._windowSize._y,
      flags
  );

  if(_window == nullptr){
    log::log(log::FATAL, log::fail_create_window, std::string{SDL_GetError()});
    return false;
  }

  Vector2i wndsize = getWindowSize();
  std::stringstream().swap(ss);
  ss << "{w:" << wndsize._x << ",h:" << wndsize._y << "}";
  log::log(log::INFO, log::info_created_window, std::string{ss.str()});

  _glContext = SDL_GL_CreateContext(_window);
  if(_glContext == nullptr){
    log::log(log::FATAL, log::fail_create_opengl_context, std::string{SDL_GetError()});
    return false;
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, openglVersionMajor) < 0){
    log::log(log::FATAL, log::fail_set_opengl_attribute, std::string{SDL_GetError()});
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, openglVersionMinor) < 0){
    log::log(log::FATAL, log::fail_set_opengl_attribute, std::string{SDL_GetError()});
    return false;
  }

  std::string glVersion {reinterpret_cast<const char*>(glGetString(GL_VERSION))};
  log::log(log::INFO, log::using_opengl_version, glVersion);

  setViewport(iRect{0, 0, _config._windowSize._x, _config._windowSize._y});

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_LESS, 1.f);
  glDisable(GL_BLEND);

  GLfloat params[2];
  glGetFloatv(gl_aliased_point_size_range, params);
  _minPixelSize = params[0];
  _maxPixelSize = params[1];
  std::stringstream().swap(ss);
  ss << "[min:" << _minPixelSize << ",max:" << _maxPixelSize << "]";
  log::log(log::INFO, log::info_pixel_size_range, std::string{ss.str()});

  return true;
}

void onWindowResize(Vector2i windowSize)
{
  setViewport(windowSize);
  for(auto& screen : _screens)
    autoAdjustScreen(windowSize, screen);
}

void clearWindow(Color4u color)
{
  glClearColor(color.getfRed(), color.getfGreen(), color.getfBlue(), 1.f); 
  glClear(GL_COLOR_BUFFER_BIT);
}

void clearLayer(Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  memset(_screens[layer]._pixels.data(), alphakey, _screens[layer]._pixels.size() * sizeof(VPixel));
}

void fastFillLayer(int shade)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  shade = std::max(0, std::min(shade, 255));
  memset(_screens[layer]._pixels.data(), shade, _screens[layer]._pixels.size() * sizeof(VPixel));
}

void slowFillLayer(Color4u color)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  Screen& screen = _screens[layer];
  for(auto& pixel : screen._pixels){
    pixel._r = color.getRed();
    pixel._g = color.getGreen();
    pixel._b = color.getBlue();
    pixel._a = color.getAlpha();
  }
}

void drawSprite(Layer layer)
{
}

void drawBitmap(Layer layer)
{
}

void drawRectangle(Layer layer)
{
}

void drawLine(Layer layer)
{
}

void drawParticles(Layer layer)
{
}

void drawPixel(Layer layer)
{
}

void drawText(Layer layer)
{
}

void present()
{
  for(auto& screen : _screens){
    glInterleavedArrays(GL_C4UB_V2F, 0, screen._pixels.data());
    glPointSize(screen._pixelSize);
    glDrawArrays(GL_POINTS, 0, screen._pixels.size());
  }
  SDL_GL_SwapWindow(_window);
}

void setLayerColorMode(ColorMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  Screen& screen = _screens[layer];
  screen._cmode = mode;

  if(mode == ColorMode::FULL_RGB)
    return;

  float hi = (mode == ColorMode::YAXIS_BANDED) ? screen._screenSize._y : screen._screenSize._x;
  screen._bands.clear();
  screen._bands.push_back(ColorBand{colors:white, 0, hi});
}

void setLayerPixelSizeMode(PixelSizeMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
   
}

} // namespace gfx
} // namespace pxr
