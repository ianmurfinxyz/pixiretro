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

#include "tinyxml2.h"  // TODO move to a lib dir

#include "gfx.h"
#include "math.h"
#include "color.h"
#include "bmpimage.h"
#include "spritesheet.h"
#include "log.h"

namespace pxr
{
namespace gfx
{

bool operator<(const ColorBand& lhs, const ColorBand& rhs){return lhs._hi < rhs._hi;}
bool operator==(const ColorBand& lhs, const ColorBand& rhs){return lhs._hi == rhs._hi;}

// A virtual screen of virtual pixels. Each rendering layer is an instance of a screen.
//
// note: pixel data is stored via raw pointers to optimise rendering performance since pixels
// must be read and written potentially tens of thousands of times in each draw tick 
// (drawing routines are essentially software rending routines with only the results being
// rendered via the GPU).
//
// note: pixel data is stored as a flattened 2D so it can be accessed directly by opengl.
struct Screen
{
  static constexpr int MAX_WIDTH {800};   // Limit virtual screen resolution to SVGA.
  static constexpr int MAX_HEIGHT {600};

  Color4u* _pxColors;              // flattened 2D arrays accessed [col + (row * width)]
  Vector2i* _pxPositions;
  std::vector<ColorBand> _bands;
  Vector2i _position;              // position w.r.t the window.
  Vector2i _size;                  // x=width pixels, y=height pixels.
  PositionMode _pmode;
  PxSizeMode _smode;
  ColorMode _cmode;
  Color4u _bitmapColor;
  int _pxSize;
  int _pxCount;
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

static constexpr int SPRITESHEET_XML_FILE_EXTENSION {".ss"};

static constexpr int openglVersionMajor = 3;
static constexpr int openglVersionMinor = 0;
static constexpr int alphaKey = 0;

static Configuration config;
static SDL_Window* window;
static SDL_GLContext glContext;
static iRect viewport;
static Vector2i windowSize;
static int minPixelSize;
static int maxPixelSize;
static std::array<Screen, LAYER_COUNT> screens;
static std::unordered_map<ResourceKey_t, SpriteSheet> sprites;
static SpriteSheet errorSprite;

//----------------------------------------------------------------------------------------------//
// GFX SETUP                                                                                    //
//----------------------------------------------------------------------------------------------//

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

// Recalculate screen position, pixel sizes, pixel positions etc to a account for a change in 
// window size, display resolution or screen mode attributes.
static void autoAdjustScreen(Vector2i windowSize, Screen& screen)
{
  // recalculate pixel size.
  if(screen._smode == PxSizeMode::AUTO_MAX){
    // note: integer math here thus all division results are implicitly floored as required.
    int pxw = windowSize._x / screen._size._x;  
    int pxh = windowSize._y / screen._size._y;
    screen._pxSize = std::max(std::min(pxw, pxh), 1);
  }

  // recalculate screen position.
  switch(screen._pmode)
  {
  case PositionMode::MANUAL:
    // no change.
    break;
  case PositionMode::CENTER:
    screen._position._x = std::clamp((windowSize._x - (screen._pxSize * screen._size._x)) / 2, 0, windowSize._x);
    screen._position._y = std::clamp((windowSize._y - (screen._pxSize * screen._size._y)) / 2, 0, windowSize._y);
    break;
  case PositionMode::TOP_LEFT:
    screen._position._x = 0;
    screen._position._y = windowSize._y - (screen._pxSize * screen._size._y);
    break;
  case PositionMode::TOP_RIGHT:
    screen._position._x = windowSize._x - (screen._pxSize * screen._size._x);
    screen._position._y = windowSize._y - (screen._pxSize * screen._size._y);
    break;
  case PositionMode::BOTTOM_LEFT:
    screen._position._x = 0;
    screen._position._y = 0;
    break;
  case PositionMode::BOTTOM_RIGHT:
    screen._position._x = windowSize._x - (screen._pxSize * screen._size._x);
    screen._position._y = 0;
    break;
  }

  // recalculate pixel positions.
  
  // Pixels are drawn as an array of points of _pxSize diameter. When drawing points in opengl, 
  // the position of the point is taken as the center position. For odd pixel sizes e.g. 7 the 
  // center pixel is simply 3,3 (= floor(7/2)). For even pixel sizes e.g. 8 the center of the 
  // pixel is considered the bottom-left pixel in the top-right quadrant, i.e. 4,4 (= floor(8/2)).
  int pixelCenterOffset = screen._pxSize / 2;

  for(int row = 0; row < screen._size._y; ++row){
    for(int col = 0; col < screen._size._x; ++col){
      Vector2i& pxPosition = screen._pxPositions[col + (row * screen._size._x)];
      pxPosition._x = screen._position._x + (col * screen._pxSize) + pixelCenterOffset;
      pxPosition._y = screen._position._y + (row * screen._pxSize) + pixelCenterOffset;
    }
  }
}

void initializeScreens(Configuration config)
{
  for(int layer = LAYER_BACKGROUND; layer < LAYER_COUNT; ++layer){
    auto& screen = screens[layer];
    if(layer == LAYER_ENGINE_STATS){
      screen._pmode = PositionMode::BOTTOM_LEFT;
      screen._smode = PxSizeMode::AUTO_MIN;
      screen._cmode = ColorMode::FULL_RGB;
    }
    else{
      screen._pmode = PositionMode::CENTER;
      screen._smode = PxSizeMode::AUTO_MAX;
      screen._cmode = ColorMode::FULL_RGB;
    }

    screen._size = config._screenSize[layer];
    assert(0 < screen._size._x && screen._size._x <= screen::MAX_WIDTH);
    assert(0 < screen._size._y && screen._size._y <= screen::MAX_HEIGHT);
    screen._pxCount  = screen._size._x * screen._size._y;
  
    screen._pxColors = new Color4u[screen._pxCount];
    screen._pxPositions = new Vector2i[screen._pxCount];
    
    clearLayer(static_cast<Layer>(layer)); 
    autoAdjustScreen(windowSize, screen);
    
    screen._bands.push_back(ColorBand{colors::white, std::numeric_limits<int>::max()});
    screen._bitmapColor = colors::white;

    std::sstream ss {};
    ss << "[w:" << screen._size._x << ", h:" << screen._size._y << "] "
       << "mem:" << (screen._pxCount * sizeof Color4u) / 1024 << "kib";
    log::log(log::INFO, log::msg_gfx_created_vscreen, ss.str());
  }
}

void freeScreens()
{
  for(int layer = LAYER_BACKGROUND; layer < LAYER_COUNT; ++layer){
    auto& screen = screens[layer];
    delete[] screen._pxColors;
    delete[] screen._pxPositions;
    screen._pxColors = nullptr;
    screen._pxPositions = nullptr;
  }
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
  glAlphaFunc(GL_GREATER, 0.f);
  glDisable(GL_BLEND);

  GLfloat params[2];
  glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, params);
  minPixelSize = params[0];
  maxPixelSize = params[1];
  std::stringstream().swap(ss);
  ss << "[min:" << minPixelSize << ",max:" << maxPixelSize << "]";
  log::log(log::INFO, log::msg_gfx_pixel_size_range, std::string{ss.str()});

  initializeScreens(config);  

  return true;
}

void shutdown()
{
  freeScreens();
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
}

//----------------------------------------------------------------------------------------------//
// GFX RESOURCES                                                                                //
//----------------------------------------------------------------------------------------------//

static void genErrorSprite()
{
  static constexpr int squareSize = 8;

  errorSprite._spriteSize = Vector2i{squareSize, squareSize};
  errorSprite._sheetSize = Vector2i{1, 1};
  errorSprite._spriteCount = 1;
  errorSprite._image.create(errorSprite._spriteSize, colors::red);
}

static bool extractIntAttribute(XMLElement* element, const char* attribute, int& value, 
                                ResourceName_t rname)
{
  XMLError xmlerror = elem->QueryIntAttribute(attribute, value);
  if(xmlerror != XML_SUCCESS){
    log::log(log::ERROR, log::msg_gfx_fail_xml_attribute, std::string{"cols"});
    log::log(log::INFO, log::msg_gfx_tinyxml_error_name, doc.ErrorName());
    log::log(log::INFO, log::msg_gfx_tinyxml_error_desc, doc.ErrorStr());
    log::log(log::INFO, log::msg_gfx_skipping_asset_load, std::string{rname});
    return false;
  }
  return true;
}

static bool isValidSpriteSheet(SpriteSheet& sheet)
{
  if((sheet._sheetSize._x * sheet._spriteSize._x) != _image.getWidth())
    return false;

  if((sheet._sheetSize._y * sheet._spriteSize._y) != _image.getHeight())
    return false;

  return true;
}

bool loadSprites(const ResourceManifest_t& manifest)
{
  log::log(log::INFO, log::msg_gfx_loading_sprites);

  if(errorSprite._pixels.size() == 0)
    genErrorSprite();

  for(auto &pair : manifest){
    ResourceKey_t rkey = pair.first;
    ResourceName_t rname = pair.second;

    std::stringstream ss{};
    ss << "key:" << rkey << " name:" << rname;
    log::log(log::INFO, log::msg_gfx_loading_sprite, ss.str());

    auto search = sprites.find(rkey);
    if(search != sprites.end()){
      log::log(log::WARN, log::msg_gfx_duplicate_resource_key, std::to_string(rkey));
      log::log(log::INFO, log::msg_gfx_skipping_asset_load, std::string{rname});
      continue;
    }

    std::string bmppath {};
    bmppath += spritesdir;
    bmppath += rname;
    bmppath += BmpImage::FILE_EXTENSION;
    if(!sheet._image.load(bmppath)){
      log::log(log::ERROR, log::msg_gfx_fail_load_sprite, std::string{rname});
      log::log(log::INFO, log::msg_gfx_using_error_sprite, std::string{});
      sprites.insert(std::make_pair(rkey, errorSprite));
      continue;
    }

    std::string xmlpath {};
    xmlpath += spritesdir;
    xmlpath += rname;
    xmlpath += SPRITESHEET_XML_FILE_EXTENSION;
    XMLDocument doc{};
    doc.LoadFile(xmlpath.str());
    if(doc.Error()){
      log::log(log::ERROR, log::msg_gfx_fail_xml_parse, xmlpath); 
      log::log(log::INFO, log::msg_gfx_tinyxml_error_name, doc.ErrorName());
      log::log(log::INFO, log::msg_gfx_tinyxml_error_desc, doc.ErrorStr());
      log::log(log::INFO, log::msg_gfx_skipping_asset_load, std::string{rname});
      continue;
    }

    SpriteSheet sheet {};

    XMLElement* element = doc.FirstChildElement("spritesheet");
    if(!extractIntAttribute(element, "rows", sheet._sheetSize._y, rname)) continue;
    if(!extractIntAttribute(element, "cols", sheet._sheetSize._x, rname)) continue;

    element = doc.FirstChildElement("sprites");
    if(!extractIntAttribute(element, "width", sheet._spriteSize._x, rname)) continue;
    if(!extractIntAttribute(element, "height", sheet._spriteSize._y, rname)) continue;

    sheet._spriteCount = sheet._sheetSize._x * sheet._sheetSize._y;

    if(!isValidSpriteSheet(sheet)){
      log::log(log::ERROR, log::msg_gfx_sheet_invalid, std::string{rname});
      log::log(log::INFO, log::msg_gfx_using_error_sprite, std::string{});
      sprites.insert(std::make_pair(rkey, errorSprite));
      continue;
    }

    sprites.insert(std::make_pair(rkey, std::move(sheet)));
  }
}

bool loadFonts(const ResourceManifest_t& manifest)
{
  BmpImage image {};
  for(auto &pair : manifest){
    ResourceKey_t rkey = pair.first;
    ResourceName_t rname = pair.second;

    std::string
   
    XMLDocument doc {}; 
    doc.load(
  }
}

//----------------------------------------------------------------------------------------------//
// GFX DRAWING                                                                                  //
//----------------------------------------------------------------------------------------------//

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
  memset(screens[layer]._pixelColors.data(), alphaKey, screens[layer]._pxCount * sizeof(Color4u));
}

void fastFillLayer(int shade, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  shade = std::max(0, std::min(shade, 255));
  memset(screens[layer]._pixelColors.data(), shade, screens[layer]._pxCount * sizeof(Color4u));
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
    if(0 < screenRow && screenRow < screen._size._y){
      for(int spriteCol = 0; spriteCol < sprite._size._x; ++spriteCol){
        screenCol = position._x + spriteCol;
        if(0 < screenCol && screenCol < screen._size._x){
          screen._pixelColors[screenCol + (screenRow * screen._size._x)] = sprite._pixels[spritePixelNo]; 
          ++spritePixelNo;
        }
      }
    }
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
    glVertexPointer(2, GL_INT, 0, screen._pxPositions);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, screen._pxColors);
    glPointSize(screen._pxSize);
    glDrawArrays(GL_POINTS, 0, screen._pxCount);
  }
  SDL_GL_SwapWindow(window);
}

void setLayerColorMode(ColorMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  screens[layer]._cmode = mode;
}

void setLayerPxSizeMode(PxSizeMode mode, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];
  screen._smode = mode;

  // setting to manual will keep the current pixel size until it is changed manually with
  // a call to 'setLayerPixelSize'.
  if(mode == PxSizeMode::MANUAL)
    return;

  if(mode == PxSizeMode::AUTO_MIN)
    screen._pxSize = 1;

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

void setLayerPixelSize(int pxSize, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  // no effect if not in manual mode.
  if(screen._smode != PxSizeMode::MANUAL)
    return;

  pxSize = std::min(std::max(minPixelSize, pxSize), maxPixelSize);

  screen._pxSize = pxSize;
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
