#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <sstream>
#include <cinttypes>
#include <unordered_map>
#include <limits>
#include <cassert>

#include "gfx.h"
#include "math.h"
#include "color.h"
#include "bmpimage.h"
#include "log.h"

namespace pxr
{
namespace gfx
{

bool operator<(const ColorBand& lhs, const ColorBand& rhs){return lhs._hi < rhs._hi;}
bool operator==(const ColorBand& lhs, const ColorBand& rhs){return lhs._hi == rhs._hi;}

// A virtual screen of virtual pixels. Each rendering layer is an instance of a screen.
struct Screen
{
  std::vector<Color4u> _pixelColors;
  std::vector<Vector2i> _pixelPositions;
  std::vector<ColorBand> _bands;
  Vector2i _position;
  Vector2i _screenSize;
  PositionMode _pmode;
  PixelSizeMode _smode;
  ColorMode _cmode;
  Color4u _bitmapColor;
  int _pixelSize;
  int _pixelCount;
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

static constexpr int openglVersionMajor = 3;
static constexpr int openglVersionMinor = 0;
static constexpr int alphakey = 0;

static Configuration config;
static SDL_Window* window;
static SDL_GLContext glContext;
static iRect viewport;
static Vector2i windowSize;
static int minPixelSize;
static int maxPixelSize;
static std::array<Screen, LAYER_COUNT> screens;
static std::unordered_map<ResourceKey_t, Sprite> sprites;
static Sprite errorSprite;

//----------------------------------------------------------------------------------------------//
// GFX RESOURCES                                                                                //
//----------------------------------------------------------------------------------------------//

void genErrorSprite()
{
  static constexpr int squareSize = 8;
  static constexpr int squareArea = squareSize * squareSize;

  errorSprite._size = Vector2i{squareSize, squareSize};
  errorSprite._pixels.resize(squareArea);
  for(auto& pixel : errorSprite._pixels)
    pixel = colors::red;
}

bool loadSprites(const ResourceManifest_t& manifest)
{
  if(errorSprite._pixels.size() == 0)
    genErrorSprite();

  // reuse the same instance to avoid repeated internal allocations.
  BmpImage image {};

  for(auto &pair : manifest){
    ResourceKey_t rkey = pair.first;
    ResourceName_t rname = pair.second;

    auto search = sprites.find(rkey);
    if(search != sprites.end()){
      log::log(log::WARN, log::msg_gfx_duplicate_resource_key, std::to_string(rkey));
      log::log(log::INFO, log::msg_gfx_skipping_asset_load, std::string{rname});
      continue;
    }

    std::string filepath {};
    filepath += spritesdir;
    filepath += rname;
    filepath += BmpImage::FILE_EXTENSION;
    if(!image.load(filepath)){
      log::log(log::ERROR, log::msg_gfx_fail_load_sprite, std::string{rname});
      log::log(log::INFO, log::msg_gfx_using_error_sprite, std::string{});
      sprites.insert(std::make_pair(rkey, errorSprite));
      continue;
    }

    Sprite sprite {};
    sprite._pixels = image.getPixels();
    sprite._size._x = image.getWidth();
    sprite._size._y = image.getHeight();
    sprites.insert(std::make_pair(rkey, std::move(sprite)));
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
  if(screen._smode == PixelSizeMode::AUTO_MAX){
    // note: integer math here thus all division results are implicitly floored as required.
    int pxw = windowSize._x / screen._screenSize._x;  
    int pxh = windowSize._y / screen._screenSize._y;
    screen._pixelSize = std::max(std::min(pxw, pxh), 1);
  }

  // recalculate screen position.
  switch(screen._pmode)
  {
  case PositionMode::MANUAL:
    // no change.
    break;
  case PositionMode::CENTER:
    screen._position._x = std::clamp((windowSize._x - (screen._pixelSize * screen._screenSize._x)) / 2, 0, windowSize._x);
    screen._position._y = std::clamp((windowSize._y - (screen._pixelSize * screen._screenSize._y)) / 2, 0, windowSize._y);
    break;
  case PositionMode::TOP_LEFT:
    screen._position._x = 0;
    screen._position._y = windowSize._y - (screen._pixelSize * screen._screenSize._y);
    break;
  case PositionMode::TOP_RIGHT:
    screen._position._x = windowSize._x - (screen._pixelSize * screen._screenSize._x);
    screen._position._y = windowSize._y - (screen._pixelSize * screen._screenSize._y);
    break;
  case PositionMode::BOTTOM_LEFT:
    screen._position._x = 0;
    screen._position._y = 0;
    break;
  case PositionMode::BOTTOM_RIGHT:
    screen._position._x = windowSize._x - (screen._pixelSize * screen._screenSize._x);
    screen._position._y = 0;
    break;
  }

  // recalculate pixel positions.
  
  // Pixels are drawn as an array of points of _pixelSize diameter. When drawing points in opengl, 
  // the position of the point is taken as the center position. For odd pixel sizes e.g. 7 the 
  // center pixel is simply 3,3 (= floor(7/2)). For even pixel sizes e.g. 8 the center of the 
  // pixel is considered the bottom-left pixel in the top-right quadrant, i.e. 4,4 (= floor(8/2)).
  int pixelCenterOffset = screen._pixelSize / 2;

  for(int row = 0; row < screen._screenSize._y; ++row){
    for(int col = 0; col < screen._screenSize._x; ++col){
      Vector2i& pixelPosition = screen._pixelPositions[col + (row * screen._screenSize._x)];
      pixelPosition._x = screen._position._x + (col * screen._pixelSize) + pixelCenterOffset;
      pixelPosition._y = screen._position._y + (row * screen._pixelSize) + pixelCenterOffset;
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
  pxr::gfx::viewport = viewport;
}

bool initialize(Configuration config)
{
  config = config;

  // create window.
  
  uint32_t flags = SDL_WINDOW_OPENGL;
  if(config._fullscreen){
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    log::log(log::INFO, log::msg_gfx_fullscreen);
  }

  std::stringstream ss {};
  ss << "{w:" << config._windowSize._x << ",h:" << config._windowSize._y << "}";
  log::log(log::INFO, log::msg_gfx_creating_window, std::string{ss.str()});

  window = SDL_CreateWindow(
      config._windowTitle.c_str(), 
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      config._windowSize._x,
      config._windowSize._y,
      flags
  );

  if(window == nullptr){
    log::log(log::FATAL, log::msg_gfx_fail_create_window, std::string{SDL_GetError()});
    return false;
  }

  SDL_GL_GetDrawableSize(window, &windowSize._x, &windowSize._y);
  std::stringstream().swap(ss);
  ss << "{w:" << windowSize._x << ",h:" << windowSize._y << "}";
  log::log(log::INFO, log::msg_gfx_created_window, std::string{ss.str()});

  // create opengl context.

  glContext = SDL_GL_CreateContext(window);
  if(glContext == nullptr){
    log::log(log::FATAL, log::msg_gfx_fail_create_opengl_context, std::string{SDL_GetError()});
    return false;
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, openglVersionMajor) < 0){
    log::log(log::FATAL, log::msg_gfx_fail_set_opengl_attribute, std::string{SDL_GetError()});
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, openglVersionMinor) < 0){
    log::log(log::FATAL, log::msg_gfx_fail_set_opengl_attribute, std::string{SDL_GetError()});
    return false;
  }

  std::string glVersion {reinterpret_cast<const char*>(glGetString(GL_VERSION))};
  log::log(log::INFO, log::msg_gfx_opengl_version, glVersion);

  // set the the initial viewport.

  setViewport(iRect{0, 0, windowSize._x, windowSize._y});

  // setup persistent opengl context state.

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_LESS, 1.f);
  glDisable(GL_BLEND);

  GLfloat params[2];
  glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, params);
  minPixelSize = params[0];
  maxPixelSize = params[1];
  std::stringstream().swap(ss);
  ss << "[min:" << minPixelSize << ",max:" << maxPixelSize << "]";
  log::log(log::INFO, log::msg_gfx_pixel_size_range, std::string{ss.str()});

  // setup rendering layers/screens.
  
  for(int layer = LAYER_BACKGROUND; layer < LAYER_COUNT; ++layer){
    auto& screen = screens[layer];
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
    switch(layer)
    {
    case LAYER_BACKGROUND:
      screen._screenSize = config._backgroundLayerSize;
      break;
    case LAYER_STAGE:
      screen._screenSize = config._stageLayerSize;
      break;
    case LAYER_UI:
      screen._screenSize = config._uiLayerSize;
      break;
    case LAYER_ENGINE_STATS:
      screen._screenSize = config._engineStatsLayerSize;
      break;
    }
    screen._pixelCount  = screen._screenSize._x * screen._screenSize._y;
    assert(screen._pixelCount > 0);
    screen._pixelColors.resize(screen._pixelCount);
    screen._pixelPositions.resize(screen._pixelCount);
    clearLayer(static_cast<Layer>(layer)); 
    autoAdjustScreen(windowSize, screen);
    
    screen._bands.push_back(ColorBand{colors::white, std::numeric_limits<int>::max()});
    screen._bitmapColor = colors::white;
  }

  return true;
}

void shutdown()
{
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
}

void onWindowResize(Vector2i windowSize)
{
  setViewport(iRect{0, 0, windowSize._x, windowSize._y});
  for(auto& screen : screens)
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
  memset(screens[layer]._pixelColors.data(), alphakey, screens[layer]._pixelCount * sizeof(Color4u));
}

void fastFillLayer(int shade, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  shade = std::max(0, std::min(shade, 255));
  memset(screens[layer]._pixelColors.data(), shade, screens[layer]._pixelCount * sizeof(Color4u));
}

void slowFillLayer(Color4u color, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  Screen& screen = screens[layer];
  for(auto& pixelColor : screen._pixelColors)
    pixelColor = color;
}

void drawSprite(Vector2i position, ResourceKey_t spriteKey, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  auto search = sprites.find(spriteKey);

  // The game/app should ensure this can never happen by ensuring all sprites have
  // been loaded before trying to draw them.
  assert(search != sprites.end());

  const Sprite& sprite = (*search).second;

  int spritePixelNo{0}, screenRow{0}, screenCol{0};
  for(int spriteRow = 0; spriteRow < sprite._size._y; ++spriteRow){
    screenRow = position._y + spriteRow;
    if(0 < screenRow && screenRow < screen._screenSize._y){
      for(int spriteCol = 0; spriteCol < sprite._size._x; ++spriteCol){
        screenCol = position._x + spriteCol;
        if(0 < screenCol && screenCol < screen._screenSize._x)
          screen._pixelColors[screenCol + (screenRow * screen._screenSize._x)] = sprite._pixels[spritePixelNo]; 
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
  for(auto& screen : screens){
    glVertexPointer(2, GL_INT, 0, screen._pixelPositions.data());
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, screen._pixelColors.data());
    glPointSize(screen._pixelSize);
    glDrawArrays(GL_POINTS, 0, screen._pixelCount);
  }
  SDL_GL_SwapWindow(window);
}

void setLayerColorMode(ColorMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  screens[layer]._cmode = mode;
}

void setLayerPixelSizeMode(PixelSizeMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];
  screen._smode = mode;

  // setting to manual will keep the current pixel size until it is changed manually with
  // a call to 'setLayerPixelSize'.
  if(mode == PixelSizeMode::MANUAL)
    return;

  if(mode == PixelSizeMode::AUTO_MIN)
    screen._pixelSize = 1;

  autoAdjustScreen(windowSize, screen);
}

void setLayerPositionMode(PositionMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];
  
  // setting to manual will keep the current position until it is changed manually with a 
  // call to 'setLayerPosition'.
  if(mode == PositionMode::MANUAL)
    return;

  autoAdjustScreen(windowSize, screen);
}

void setLayerPosition(Vector2i position, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  // no effect if not in manual mode.
  if(screen._pmode != PositionMode::MANUAL)
    return;

  screen._position = position;
  autoAdjustScreen(windowSize, screen);
}

void setLayerPixelSize(int pixelSize, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  // no effect if not in manual mode.
  if(screen._smode != PixelSizeMode::MANUAL)
    return;

  pixelSize = std::min(std::max(minPixelSize, pixelSize), maxPixelSize);

  screen._pixelSize = pixelSize;
  autoAdjustScreen(windowSize, screen);
}

void setLayerColorBands(std::vector<ColorBand> bands, Layer layer, bool append)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screenBands = screens[layer]._bands;

  if(!append)
    screenBands.clear();

  screenBands.insert(screenBands.end(), screenBands.begin(), bands.end());
  std::sort(screenBands.begin(), screenBands.end());
  screenBands.erase(std::unique(screenBands.begin(), screenBands.end()), screenBands.end());
}

void setBitmapColor(Color4u color, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  // has no effect if not BITMAPS color mode.
  if(screen._cmode != ColorMode::BITMAPS)
    return;

  screen._bitmapColor = color;
}

} // namespace gfx
} // namespace pxr
