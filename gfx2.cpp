
#include <vector>
#include <array>
#include <cstring>
#include <cinttypes>
#include <unordered_map>
#include <numerics>

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
  Color4u _bitmapColor;
  int _pixelSize;
};

struct Sprite
{
  Sprite() = default;
  Sprite(const Sprite&) = default;
  Sprite(Sprite&&) = default;
  Sprite& operator=(const Sprite&) = default;
  Sprite& operator=(Sprite&&) = default;

  std::vector<Color4u> _pixels; // flattened 2D array accessed [col + (row * width)]
  Vector2i _size;               // x(num cols) and y(num rows) dimensions of sprite.
};

static constexpr const char* bmp

static constexpr int openglVersionMajor = 3;
static constexpr int openglVersionMinor = 0;
static constexpr int alphakey = 0;

static Config _config;
static SDL_Window* _window;
static SDL_GLContext _glContext;
static iRect _viewport;
static Vector2i _windowSize;
static Color4u _bitmapColor;
static int _minPixelSize;
static int _maxPixelSize;
static std::array<Screen, LAYER_COUNT> _screens;
static std::unordered_map<ResourceKey_t, Sprite> _sprites;
static Sprite _errorSprite;

//----------------------------------------------------------------------------------------------//
// GFX RESOURCES                                                                                //
//----------------------------------------------------------------------------------------------//

void genErrorSprite()
{
  static constexpr int squareSize = 8;
  static constexpr int squareArea = squareSize * squareSize;

  _errorSprite._size = Vector2i{squareSize, squareSize};
  _errorSprite._pixels.resize(squareArea);
  for(auto& pixel : _errorSprite._pixels)
    pixel = colors::red;
}

bool loadSprites(const ResourceManifest_t& manifest)
{
  if(_errorSprite._pixels.size() == 0)
    genErrorSprite();

  // reuse the same instance to avoid repeated internal allocations.
  BmpImage image {};

  for(auto &pair : manifest){
    ResourceKey_t rkey = pair.first;
    ResourceName_t rname = pair.second;

    auto search = _sprites.find(rkey);
    if(search != _sprites.end()){
      log::log(log::WARN, log::msg_duplicate_resource_key, to_string(rkey));
      log::log(log::INFO, log::msg_skipping_asset_load, std::string{rname});
      continue;
    }

    std::string filepath {};
    filepath += spritesdir;
    filepath += rname;
    filepath += BmpImage::FILE_EXTENSION;
    if(!image.load(filepath)){
      log::log(log::ERROR, log::msg_failed_load_sprite, std::string{rname});
      log::log(log::INFO, log::msg_using_error_sprite, std::string{});
      _sprites.insert(std::make_pair(rkey, _errorSprite));
      continue;
    }

    Sprite sprite {};
    sprite._pixels = image.getPixels();
    sprite._size._x = image.getWidth();
    sprite._size._y = image.getHeight();
    _sprites.insert(std::make_pair(rkey, std::move(sprite)));
  }
}

bool loadFonts(const ResourceManifest_t& manifest)
{

}

//----------------------------------------------------------------------------------------------//
// GFX DRAWING                                                                                  //
//----------------------------------------------------------------------------------------------//

// Recalculate screen position, pixel sizes, pixel positions etc to a account for a change in 
// window size, display resolution or screen mode attributes.
static void autoAdjustScreen(Vector2i windowSize, Screen& screen)
{
  // recalculate pixel size.
  if(screen._smode == SizeMode::AUTO_MAX){
    // note: integer math here thus all division results are implicitly floored as required.
    int pxw = windowSize._x / _screenSize._x;  
    int pxh = windowSize._y / _screenSize._y;
    screen._pixelSize = std::max(std::min(pxw, pxh), 1);
  }

  // recalculate screen position.
  switch(screen._pmode)
  {
  case PositionMode::MANUAL:
    // no change.
    break;
  case PositionMode::CENTER:
    screen._position._x = std::clamp((windowSize._x - (_pixelSize * _screenSize._x)) / 2, 0, windowSize._x);
    screen._position._y = std::clamp((windowSize._y - (_pixelSize * _screenSize._y)) / 2, 0, windowSize._y);
    break;
  case PositionMode::TOP_LEFT:
    screen._position._x = 0;
    screen._position._y = windowSize._y - (_pixelSize * _screenSize._y);
    break;
  case PositionMode::TOP_RIGHT:
    screen._position._x = windowSize._x - (_pixelSize * _screenSize._x);
    screen._position._y = windowSize._y - (_pixelSize * _screenSize._y);
    break;
  case PositionMode::BOTTOM_LEFT:
    screen._position._x = 0;
    screen._position._y = 0;
    break;
  case PositionMode::BOTTOM_RIGHT:
    screen._position._x = windowSize._x - (_pixelSize * _screenSize._x);
    screen._position._y = 0;
    break;
  }

  // recalculate pixel positions.
  
  // Pixels are drawn as an array of points of _pixelSize diameter. When drawing points in opengl, 
  // the position of the point is taken as the center position. For odd pixel sizes e.g. 7 the 
  // center pixel is simply 3,3 (= floor(7/2)). For even pixel sizes e.g. 8 the center of the 
  // pixel is considered the bottom-left pixel in the top-right quadrant, i.e. 4,4 (= floor(8/2)).
  int pixelCenterOffset = _pixelSize / 2;

  for(int row = 0; row < screen._screenSize._y; ++row){
    for(int col = 0; col < screen._screenSize._x; ++col){
      Pixel& pixel = screen._pixels[col + (row * screen._screenSize._x)];
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

  // create window.
  
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

  SDL_GL_GetDrawableSize(_window, &_windowSize._x, &_windowSize._y);
  std::stringstream().swap(ss);
  ss << "{w:" << _windowSize._x << ",h:" << _windowSize._y << "}";
  log::log(log::INFO, log::info_created_window, std::string{ss.str()});

  // create opengl context.

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

  // set the the initial viewport.

  setViewport(iRect{0, 0, _windowSize._x, _windowSize._y});

  // setup persistent opengl context state.

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

  // setup rendering layers/screens.
  
  for(int layer = LAYER_BACKGROUND; layer < LAYER_COUNT; ++layer){
    auto& screen = _screens[layer];
    if(layer == LAYER_ENGINE_STATS){
      screen._pmode = PositionMode::BOTTOM_LEFT;
      screen._smode = PixelSizeMode::AUTO_MIN;
      screen._cmode = ColorMode::FULL_RGB;
    }
    else{
      screen._pmode = PositionMode::CENTER;
      screen._smode = PixelSizeMode::AUTO_MAX;
      screen._cmode = ColorMode::FULL_RGB;
    }
    Vector2i screenSize {};
    switch(layer)
    {
    case LAYER_BACKGROUND:
      screenSize = _config._backgroundLayerSize;
      break;
    case LAYER_STAGE:
      screenSize = _config._stageLayerSize;
      break;
    case LAYER_UI:
      screenSize = _config._uiLayerSize;
      break;
    case LAYER_ENGINE_STATS:
      screenSize = _config._engineStatsSize;
      break;
    }
    int numPixels = screenSize._x * screenSize._y;
    assert(numPixels > 0);
    screen._pixels.resize(numPixels);
    autoAdjustScreen(_windowSize, screen)

    screen._bands.push_back(ColorBand{colors::white, 0, std::numeric_limits<int>::max()});
    screen._bitmapsColor = colors::white;
  }

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

void drawSprite(Vector2i position, ResourceKey_t spriteKey, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = _screens[layer];

  auto search = _sprites.find(spriteKey);

  // The game/app should ensure this can never happen by ensuring all sprites have
  // been loaded before trying to draw them.
  assert(search ! _sprites.end());

  const Sprite& sprite = search.second;

  int spritePixelNo{0}, screenRow{0}, screenCol{0};
  for(int spriteRow = 0; spriteRow < sprite._size._y; ++spriteRow){
    screenRow = position._y + spriteRow;
    if(0 < screenRow && screenRow < screen._screenSize._y){
      for(int spriteCol = 0; spriteCol < sprite._size._x; ++spriteCol){
        screenCol = position._x + spriteCol;
        if(0 < screenCol && screenCol < screen._screenSize._x)
          screen._pixels[screenCol + (screenRow * screen._screenSize._x)] = sprite._pixels[spritePixelNo]; 
      }
    }
    ++spritePixelNo;
  }
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
  _screens[layer]._cmode = mode;
}

void setLayerPixelSizeMode(PixelSizeMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = _screens[layer];
  screen._smode = mode;

  // setting to manual will keep the current pixel size until it is changed manually with
  // a call to 'setLayerPixelSize'.
  if(mode == PixelSizeMode::MANUAL)
    return;

  if(mode == PixelSizeMode::AUTO_MIN)
    screen._pixelSize = 1;

  autoAdjustScreen(_windowSize, screen);
}

void setLayerPositionMode(PositionMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = _screens[layer];
  
  // setting to manual will keep the current position until it is changed manually with a 
  // call to 'setLayerPosition'.
  if(mode == PositionMode::MANUAL)
    return;

  autoAdjustScreen(_windowSize, screen);
}

void setLayerPosition(Vector2i position, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = _screens[layer];

  // no effect if not in manual mode.
  if(screen._pmode != PositionMode::MANUAL)
    return;

  screen._position = position;
  autoAdjustScreen(_windowSize, screen);
}

void setLayerPixelSize(int pixelSize, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = _screens[layer];

  // no effect if not in manual mode.
  if(screen._smode != PixelSizeMode::MANUAL)
    return;

  screen._pixelSize = pixelSize;
  autoAdjustScreen(_windowSize, screen);
}

void setLayerColorBands(std::vector<ColorBand> bands, Layer layer, bool append)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screenBands = _screens[layer]._bands;

  if(!appends)
    screenBands.clear();

  screenBands.insert(screenBands.end(), screenBands.begin(), bands.end());
  std::sort(screenBands.begin(), screenBands.end());
  screenBands.erase(std::unique(screenBands.begin(), screenBands.end()), screenBands.end());
}

void setBitmapColor(Color4u color, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = _screens[layer];

  // has no effect if not BITMAPS color mode.
  if(screen._cmode != ColorMode::BITMAPS)
    return;

  screen._bitmapColor = color;
}

} // namespace gfx
} // namespace pxr
