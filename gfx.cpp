#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <sstream>
#include <cinttypes>
#include <limits>
#include <cassert>

//#include <iostream>
#include <chrono>

#include "xmlutil.h"

#include "gfx.h"
#include "math.h"
#include "color.h"
#include "bmpimage.h"
#include "log.h"

using namespace tinyxml2;

namespace pxr
{
namespace gfx
{

static constexpr int MIN_OPENGL_VERSION_MAJOR = 2;
static constexpr int MIN_OPENGL_VERSION_MINOR = 1;
static constexpr int DEF_OPENGL_VERSION_MAJOR = 3;
static constexpr int DEF_OPENGL_VERSION_MINOR = 0;

static constexpr int ALPHA_KEY = 0;

static std::string windowTitle;
static Vector2i windowSize;
static bool fullscreen;
static int minPixelSize;
static int maxPixelSize;
static SDL_Window* window;
static SDL_GLContext glContext;
static iRect viewport;
static std::vector<Screen> screens;
static std::vector<Sprite> sprites;
static std::vector<Font> fonts;
static Sprite errorSprite;
static Font errorFont;

static bool operator<(const ColorBand& lhs, const ColorBand& rhs)
{
  return lhs._hi < rhs._hi;
}

static bool operator==(const ColorBand& lhs, const ColorBand& rhs)
{
  return lhs._hi == rhs._hi;
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

// 
// Generates a red sqaure sprite with the (single) frame's origin in the bottom-left.
//
static void genErrorSprite()
{
  static constexpr int squareSize = 8;

  SpriteFrame frame{};
  frame._position = Vector2i{0, 0};
  frame._size = Vector2i{squareSize, squareSize};
  frame._origin = Vector2i{0, 0};

  errorSprite._image.create(frame._size, colors::red);
  errorSprite._frames.push_back(frame);
}

//
// Generates an 8px font with all 95 printable ascii characters where all characters are just 
// blank red squares.
//
static void genErrorFont()
{
  errorFont._lineHeight = 8;
  errorFont._baseLine = 1;
  errorFont._glyphSpace = 0;
  errorFont._image.create(Vector2i{256, 24}, colors::red);
  for(auto& glyph : errorFont._glyphs){
    glyph._x = 0;
    glyph._y = 0;
    glyph._width = 6;
    glyph._height = 6;
    glyph._xoffset = 1;
    glyph._yoffset = 0;
    glyph._xadvance = 8;
  }
}

bool initialize(std::string windowTitle_, Vector2i windowSize_, bool fullscreen_)
{
  log::log(log::INFO, log::msg_gfx_initializing);

  windowSize = windowSize_;
  windowTitle = windowTitle_;
  fullscreen = fullscreen_;

  uint32_t flags = SDL_WINDOW_OPENGL;
  if(fullscreen){
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    log::log(log::INFO, log::msg_gfx_fullscreen);
  }

  std::stringstream ss {};
  ss << "{w:" << windowSize._x << ",h:" << windowSize._y << "}";
  log::log(log::INFO, log::msg_gfx_creating_window, std::string{ss.str()});

  window = SDL_CreateWindow(
      windowTitle.c_str(), 
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      windowSize._x,
      windowSize._y,
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

  glContext = SDL_GL_CreateContext(window);
  if(glContext == nullptr){
    log::log(log::FATAL, log::msg_gfx_fail_create_opengl_context, std::string{SDL_GetError()});
    return false;
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, DEF_OPENGL_VERSION_MAJOR) < 0){
    log::log(log::FATAL, log::msg_gfx_fail_set_opengl_attribute, std::string{SDL_GetError()});
    return false;
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, DEF_OPENGL_VERSION_MINOR) < 0){
    log::log(log::FATAL, log::msg_gfx_fail_set_opengl_attribute, std::string{SDL_GetError()});
    return false;
  }

  std::string glVersion {reinterpret_cast<const char*>(glGetString(GL_VERSION))};
  log::log(log::INFO, log::msg_gfx_opengl_version, glVersion);

  // TODO: extract version from string and check it meets min requirement.

  const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  log::log(log::INFO, log::msg_gfx_opengl_renderer, glRenderer);

  const char* glVendor {reinterpret_cast<const char*>(glGetString(GL_VENDOR))};
  log::log(log::INFO, log::msg_gfx_opengl_vendor, glVendor);

  GLfloat params[2];
  glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, params);
  minPixelSize = params[0];
  maxPixelSize = params[1];
  std::stringstream().swap(ss);
  ss << "[min:" << minPixelSize << ",max:" << maxPixelSize << "]";
  log::log(log::INFO, log::msg_gfx_pixel_size_range, std::string{ss.str()});

  setViewport(iRect{0, 0, windowSize._x, windowSize._y});

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.f);

  genErrorSprite();
  genErrorFont();

  return true;
}

static void freeScreens()
{
  for(auto& screen : screens){
    delete[] screen._pxColors;
    delete[] screen._pxPositions;
    screen._pxColors = nullptr;
    screen._pxPositions = nullptr;
  }
}

void shutdown()
{
  freeScreens();
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
}

//
// Recalculates screen position, pixel size, pixel positions etc to a account for a change in 
// window size, display resolution or screen mode attributes.
//
static void autoAdjustScreen(Vector2i windowSize, Screen& screen)
{
  if(screen._smode == SizeMode::AUTO_MIN){
    screen._pxSize = 1;
  }
  else if(screen._smode == SizeMode::AUTO_MAX){
    int pxw = windowSize._x / screen._resolution._x;   // note: int division thus implicit floor.
    int pxh = windowSize._y / screen._resolution._y;
    screen._pxSize = std::max(std::min(pxw, pxh), 1);
  }
  else if(screen._smode == SizeMode::MANUAL)
    screen._pxSize = screen._pxManualSize;

  switch(screen._pmode)
  {
  case PositionMode::MANUAL:
    screen._position = screen._manualPosition;
    break;
  case PositionMode::CENTER:
    screen._position._x = std::clamp((windowSize._x - (screen._pxSize * screen._resolution._x)) / 2, 0, windowSize._x);
    screen._position._y = std::clamp((windowSize._y - (screen._pxSize * screen._resolution._y)) / 2, 0, windowSize._y);
    break;
  case PositionMode::TOP_LEFT:
    screen._position._x = 0;
    screen._position._y = windowSize._y - (screen._pxSize * screen._resolution._y);
    break;
  case PositionMode::TOP_RIGHT:
    screen._position._x = windowSize._x - (screen._pxSize * screen._resolution._x);
    screen._position._y = windowSize._y - (screen._pxSize * screen._resolution._y);
    break;
  case PositionMode::BOTTOM_LEFT:
    screen._position._x = 0;
    screen._position._y = 0;
    break;
  case PositionMode::BOTTOM_RIGHT:
    screen._position._x = windowSize._x - (screen._pxSize * screen._resolution._x);
    screen._position._y = 0;
    break;
  }

  //
  // Pixels are drawn as an array of points of _pxSize diameter. When drawing points in opengl, 
  // the position of the point is taken as the center position. For odd pixel sizes e.g. 7 the 
  // center pixel is simply 3,3 (= floor(7/2)). For even pixel sizes e.g. 8 the center of the 
  // pixel is considered the bottom-left pixel in the top-right quadrant, i.e. 4,4 (= floor(8/2)).
  //
  int pixelCenterOffset = screen._pxSize / 2;

  for(int row = 0; row < screen._resolution._y; ++row){
    for(int col = 0; col < screen._resolution._x; ++col){
      Vector2i& pxPosition = screen._pxPositions[col + (row * screen._resolution._x)];
      pxPosition._x = screen._position._x + (col * screen._pxSize) + pixelCenterOffset;
      pxPosition._y = screen._position._y + (row * screen._pxSize) + pixelCenterOffset;
    }
  }
}

int createScreen(Vector2i resolution)
{
  assert(resolution._x > 0 && resolution._y > 0);

  screens.push_back(Screen{});
  ResourceKey_t screenid = screens.size() - 1;

  auto& screen = screens.back();

  for(auto& band : screen._bands){
    band._color = colors::white;
    band._hi = std::numeric_limits<int>::max();
  }

  screen._pmode = PositionMode::CENTER;
  screen._smode = SizeMode::AUTO_MAX;
  screen._cmode = ColorMode::FULL_RGB;
  screen._position = Vector2i{0, 0};
  screen._manualPosition = Vector2i{0, 0};
  screen._resolution = resolution;
  screen._bitmapColor = colors::white;
  screen._pxManualSize = 1;
  screen._pxCount = screen._resolution._x * screen._resolution._y;
  screen._pxColors = new Color4u[screen._pxCount];
  screen._pxPositions = new Vector2i[screen._pxCount];
  screen._isEnabled = true;

  clearScreenTransparent(screenid); 
  autoAdjustScreen(windowSize, screen);

  int memkib = ((screen._pxCount * sizeof(Color4u)) + (screen._pxCount * sizeof(Vector2i))) / 1024;

  std::stringstream ss {};
  ss << "resolution:" << resolution._x << "x" << resolution._y << "vpx mem:" << memkib << "kib";
  log::log(log::INFO, log::msg_gfx_created_vscreen, ss.str());

  return screenid;
}

static ResourceKey_t useErrorSprite()
{
  log::log(log::INFO, log::msg_gfx_using_error_sprite);
  sprites.push_back(errorSprite);
  return sprites.size() - 1;
}

static ResourceKey_t useErrorFont()
{
  log::log(log::INFO, log::msg_gfx_using_error_font);
  fonts.push_back(errorFont);
  return fonts.size() - 1;
}

ResourceKey_t loadSprite(ResourceName_t name)
{
  log::log(log::INFO, log::msg_gfx_loading_sprite, name);

  Sprite sprite{};

  std::string bmppath{};
  bmppath += RESOURCE_PATH_SPRITES;
  bmppath += name;
  bmppath += BmpImage::FILE_EXTENSION;
  if(!sprite._image.load(bmppath)){
    log::log(log::ERROR, log::msg_gfx_fail_load_asset_bmp, name);
    return useErrorSprite();
  }

  std::string xmlpath {};
  xmlpath += RESOURCE_PATH_SPRITES;
  xmlpath += name;
  xmlpath += XML_RESOURCE_EXTENSION_SPRITES;
  XMLDocument doc{};
  if(!parseXmlDocument(&doc, xmlpath)) 
    return useErrorSprite();

  XMLElement* xmlsprite{nullptr};
  XMLElement* xmlframe{nullptr};

  int err{0};
  if(!extractChildElement(&doc, &xmlsprite, "sprite")) return useErrorSprite();
  if(!extractChildElement(xmlsprite, &xmlframe, "frame")) return useErrorSprite();
  do{
    SpriteFrame frame{};
    if(!extractIntAttribute(xmlframe, "x", &frame._position._x)){++err; break;}
    if(!extractIntAttribute(xmlframe, "y", &frame._position._y)){++err; break;}
    if(!extractIntAttribute(xmlframe, "w", &frame._size._x)){++err; break;}
    if(!extractIntAttribute(xmlframe, "h", &frame._size._y)){++err; break;}
    if(!extractIntAttribute(xmlframe, "ox", &frame._origin._x)){++err; break;}
    if(!extractIntAttribute(xmlframe, "oy", &frame._origin._y)){++err; break;}
    sprite._frames.push_back(frame);
    xmlframe = xmlframe->NextSiblingElement("frame");
  }
  while(xmlframe != 0);
  if(err) return useErrorSprite();

  // 
  // Validate all frames to avoid segfaults.
  //
  err = 0;
  Vector2i bmpSize = sprite._image.getSize();
  for(auto& frame : sprite._frames){
    if(frame._position._x < 0 || frame._position._y < 0){++err; break;}
    if(frame._size._x < 0 || frame._size._y < 0){++err; break;}
    if(frame._origin._x < 0 || frame._origin._y < 0){++err; break;}
    if(frame._origin._x >= frame._size._x || frame._origin._y >= frame._size._y){++err; break;}
    if(frame._position._x + frame._size._x > bmpSize._x){++err; break;}
    if(frame._position._y + frame._size._y > bmpSize._y){++err; break;}
  }

  if(err){
    log::log(log::ERROR, log::msg_gfx_sprite_invalid_xml_bmp_mismatch, name);
    return useErrorSprite();
  }

  log::log(log::INFO, log::msg_gfx_loading_sprite_success);

  sprites.push_back(std::move(sprite));
  return sprites.size() - 1;
}

ResourceKey_t loadFont(ResourceName_t name)
{
  log::log(log::INFO, log::msg_gfx_loading_font, name);

  Font font{};

  std::string bmppath{};
  bmppath += RESOURCE_PATH_FONTS;
  bmppath += name;
  bmppath += BmpImage::FILE_EXTENSION;
  if(!font._image.load(bmppath)){
    log::log(log::ERROR, log::msg_gfx_fail_load_asset_bmp, name);
    return useErrorFont();
  }

  std::string xmlpath {};
  xmlpath += RESOURCE_PATH_FONTS;
  xmlpath += name;
  xmlpath += XML_RESOURCE_EXTENSION_FONTS;
  XMLDocument doc{};
  if(!parseXmlDocument(&doc, xmlpath))
    return useErrorFont();

  XMLElement* xmlfont{nullptr};
  XMLElement* xmlcommon{nullptr};
  XMLElement* xmlchars{nullptr};
  XMLElement* xmlchar{nullptr};

  if(!extractChildElement(&doc, &xmlfont, "font")) return useErrorFont();
  if(!extractChildElement(xmlfont, &xmlcommon, "common")) return useErrorFont();
  if(!extractIntAttribute(xmlcommon, "lineHeight", &font._lineHeight)) return useErrorFont();
  if(!extractIntAttribute(xmlcommon, "baseline", &font._baseLine)) return useErrorFont();
  if(!extractIntAttribute(xmlcommon, "glyphspace", &font._glyphSpace)) return useErrorFont();

  int charsCount {0};
  if(!extractChildElement(xmlfont, &xmlchars, "chars")) return useErrorFont();
  if(!extractIntAttribute(xmlchars, "count", &charsCount)) return useErrorFont();

  if(charsCount != ASCII_CHAR_COUNT){
    log::log(log::ERROR, log::msg_gfx_missing_ascii_glyphs, name);
    return useErrorFont();
  }

  int charsRead{0}, err{0};
  if(!extractChildElement(xmlchars, &xmlchar, "char")) return useErrorFont();
  do{
    Glyph& glyph = font._glyphs[charsRead];
    if(!extractIntAttribute(xmlchar, "ascii", &glyph._ascii)){++err; break;}
    if(!extractIntAttribute(xmlchar, "x", &glyph._x)){++err; break;}
    if(!extractIntAttribute(xmlchar, "y", &glyph._y)){++err; break;}
    if(!extractIntAttribute(xmlchar, "width", &glyph._width)){++err; break;}
    if(!extractIntAttribute(xmlchar, "height", &glyph._height)){++err; break;}
    if(!extractIntAttribute(xmlchar, "xoffset", &glyph._xoffset)){++err; break;}
    if(!extractIntAttribute(xmlchar, "yoffset", &glyph._yoffset)){++err; break;}
    if(!extractIntAttribute(xmlchar, "xadvance", &glyph._xadvance)){++err; break;}
    ++charsRead;
    xmlchar = xmlchar->NextSiblingElement("char");
  }
  while(xmlchar != 0 && charsRead < ASCII_CHAR_COUNT);
  if(err) return useErrorFont();

  std::sort(font._glyphs.begin(), font._glyphs.end(), [](const Glyph& g0, const Glyph& g1) {
    return g0._ascii < g1._ascii;
  });

  if(charsRead != ASCII_CHAR_COUNT){
    log::log(log::ERROR, log::msg_gfx_missing_ascii_glyphs, name);
    return useErrorFont();
  }

  // 
  // Validate all glyphs to avoid segfaults.
  //
  err = 0;
  Vector2i bmpSize = font._image.getSize();
  for(auto& glyph : font._glyphs){
    if(glyph._ascii < 32 || glyph._ascii > 126){++err; break;}
    if(glyph._x < 0 || glyph._y < 0){++err; break;}
    if(glyph._width < 0 || glyph._height < 0){++err; break;}
    if(glyph._x + glyph._width > bmpSize._x){++err; break;}
    if(glyph._y + glyph._height > bmpSize._y){++err; break;}
  }

  if(err){
    log::log(log::ERROR, log::msg_gfx_font_invalid_xml_bmp_mismatch);
    return useErrorFont();
  }

  //
  // checksum is used to to test for the condition in which we have the correct number of 
  // glyphs but some are duplicates of the same character.
  //
  int checksum {0};
  for(auto& glyph : font._glyphs){
    checksum += glyph._ascii;
  }
  if(checksum != ASCII_CHAR_CHECKSUM){
    log::log(log::ERROR, log::msg_gfx_font_fail_checksum);
    return useErrorFont();
  }

  log::log(log::INFO, log::msg_gfx_loading_font_success);

  fonts.push_back(std::move(font));
  return fonts.size() - 1;
}

int getSpriteFrameCount(ResourceKey_t spriteKey)
{
  assert(0 <= spriteKey && spriteKey < sprites.size());
  return sprites[spriteKey]._frames.size();
}

void onWindowResize(Vector2i windowSize)
{
  setViewport(iRect{0, 0, windowSize._x, windowSize._y});
  for(auto& screen : screens)
    autoAdjustScreen(windowSize, screen);
}

void clearWindowColor(Color4f color)
{
  glClearColor(color._r, color._g, color._b, color._a); 
  glClear(GL_COLOR_BUFFER_BIT);
}

void clearScreenTransparent(int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  memset(screens[screenid]._pxColors, ALPHA_KEY, screens[screenid]._pxCount * sizeof(Color4u));
}

void clearScreenShade(int shade, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  shade = std::max(0, std::min(shade, 255));
  memset(screens[screenid]._pxColors, shade, screens[screenid]._pxCount * sizeof(Color4u));
}

void clearScreenColor(Color4u color, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  Screen& screen = screens[screenid];
  for(int px = 0; px < screen._pxCount; ++px)
    screen._pxColors[px] = color;
}

void drawSprite(Vector2i position, ResourceKey_t spriteKey, int frameid, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];

  assert(0 <= spriteKey && spriteKey < sprites.size());
  auto& sprite = sprites[spriteKey];
  const Color4u* const * spritePxs = sprite._image.getPixels();

  assert(0 <= frameid);
  frameid = frameid < sprite._frames.size() ? frameid : 0; // may be an error sprite with 1 frame.
  auto& frame = sprite._frames[frameid];

  int screenRow {0}, screenCol{0}, screenRowOffset{0}, screenRowBase{0}, screenColBase{0};
  screenRowBase = position._y + frame._origin._y;
  screenColBase = position._x + frame._origin._x;
  for(int frameRow = 0; frameRow < frame._size._y; ++frameRow){
    screenRow = screenRowBase + frameRow;
    if(screenRow < 0) continue;
    if(screenRow >= screen._resolution._y) break;
    screenRowOffset = screenRow * screen._resolution._x;
    for(int frameCol = 0; frameCol < frame._size._x; ++frameCol){
      screenCol = screenColBase + frameCol;
      if(screenCol < 0) continue;
      if(screenCol >= screen._resolution._x) break;
      const Color4u& pxColor = spritePxs[frame._position._y + frameRow][frame._position._x + frameCol];
      if(pxColor._a == ALPHA_KEY) continue;
      if(screen._cmode == ColorMode::FULL_RGB){
        screen._pxColors[screenCol + screenRowOffset] = pxColor;
      }
      else if(screen._cmode == ColorMode::YAXIS_BANDED){
        int bandid{0};
        while(screenRow > screen._bands[bandid]._hi && bandid < SCREEN_BAND_COUNT)
          ++bandid;
        screen._pxColors[screenCol + screenRowOffset] = screen._bands[bandid]._color;
      }
      else if(screen._cmode == ColorMode::XAXIS_BANDED){
        int bandid{0};
        while(screenCol > screen._bands[bandid]._hi && bandid < SCREEN_BAND_COUNT)
          ++bandid;
        screen._pxColors[screenCol + screenRowOffset] = screen._bands[bandid]._color;
      }
      else{
        screen._pxColors[screenCol + screenRowOffset] = screen._bitmapColor;
      }
    }
  }
}

void drawRectangle(iRect rect, Color4u color, int screenid)
{
}

void drawLine(Vector2i p0, Vector2i p1, Color4u color, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];

  p0._x = std::clamp(p0._x, 0, screen._resolution._x - 1);
  p1._x = std::clamp(p1._x, 0, screen._resolution._x - 1);
  p0._y = std::clamp(p0._y, 0, screen._resolution._y - 1);
  p1._y = std::clamp(p1._y, 0, screen._resolution._y - 1);

  //
  // constants in line equation y=mx+c
  //
  int dx = static_cast<float>(p1._x - p0._x);
  int dy = static_cast<float>(p1._y - p0._y);

  if(dx == 0 && dy == 0)
    return;

  int ymin, ymax;
  if(p0._y < p1._y){
    ymin = p0._y;
    ymax = p1._y;
  }
  else{
    ymin = p1._y;
    ymax = p0._y;
  }

  int xmin, xmax;
  if(p0._x < p1._x){
    xmin = p0._x;
    xmax = p1._x;
  }
  else{
    xmin = p1._x;
    xmax = p0._x;
  }

  if(dx == 0)
    for(int y = ymin; y < ymax; ++y)
      screen._pxColors[xmin + (y * screen._resolution._x)] = color;

  else if(dy == 0)
    for(int x = xmin; x < xmax; ++x)
      screen._pxColors[x + (ymin * screen._resolution._x)] = color;

  else{
    float m = static_cast<float>(dy) / dx;
    for(int x = xmin; x <= xmax; ++x){
      int y = (m * x) + ymin;
      screen._pxColors[x + (y * screen._resolution._x)] = color;
    }
  }
}

void drawText(Vector2i position, const std::string& text, ResourceKey_t fontKey, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];

  assert(0 <= fontKey && fontKey < fonts.size());
  auto& font = fonts[fontKey];
  const Color4u* const* fontPxs = font._image.getPixels();

  int baseLineY = position._y + font._baseLine;
  for(char c : text){
    if(c == '\n'){
      //baseLineY -= font._lineHeight; TODO ignore for now.
      continue;
    }
    assert(' ' <= c && c <= '~');
    const Glyph& glyph = font._glyphs[static_cast<int>(c - ' ')];
    int screenRow{0}, screenCol{0}, screenRowBase{0}, screenRowOffset {0};
    screenRowBase = baseLineY + glyph._yoffset;
    for(int glyphRow = 0; glyphRow < glyph._height; ++glyphRow){
      screenRow = screenRowBase + glyphRow;
      if(screenRow < 0) continue;
      if(screenRow >= screen._resolution._y) break;
      screenRowOffset = screenRow * screen._resolution._x;
      for(int glyphCol = 0; glyphCol < glyph._width; ++glyphCol){
        screenCol = position._x + glyphCol + glyph._xoffset;
        if(screenCol < 0) continue;
        if(screenCol >= screen._resolution._x) return;
        const Color4u& pxColor = fontPxs[glyph._y + glyphRow][glyph._x + glyphCol];
        if(pxColor._a == ALPHA_KEY) continue;
        if(screen._cmode == ColorMode::FULL_RGB){
          screen._pxColors[screenCol + screenRowOffset] = pxColor;
        }
        else if(screen._cmode == ColorMode::YAXIS_BANDED){
          int bandid{0};
          while(screenRow > screen._bands[bandid]._hi && bandid < SCREEN_BAND_COUNT)
            ++bandid;
          screen._pxColors[screenCol + screenRowOffset] = screen._bands[bandid]._color;
        }
        else if(screen._cmode == ColorMode::XAXIS_BANDED){
          int bandid{0};
          while(screenCol > screen._bands[bandid]._hi && bandid < SCREEN_BAND_COUNT)
            ++bandid;
          screen._pxColors[screenCol + screenRowOffset] = screen._bands[bandid]._color;
        }
        else{
          screen._pxColors[screenCol + screenRowOffset] = screen._bitmapColor;
        }
      }
    }
    position._x += glyph._xadvance + font._glyphSpace;
  }
}

void present()
{
  //--------------------------------------------------------------------------------
  //
  // TODO TEMP
  //
  //
  //auto now0 = std::chrono::high_resolution_clock::now();
  //
  //
  //--------------------------------------------------------------------------------

  for(auto& screen : screens){
    if(!screen._isEnabled) continue;
    glVertexPointer(2, GL_INT, 0, screen._pxPositions);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, screen._pxColors);
    glPointSize(screen._pxSize);
    glDrawArrays(GL_POINTS, 0, screen._pxCount);
  }

  SDL_GL_SwapWindow(window);

  //--------------------------------------------------------------------------------
  //
  // TODO TEMP
  //
  //
  //auto now1 = std::chrono::high_resolution_clock::now();
  //auto dt = std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0);
  //std::cout << "present time: " << dt.count() << "us" << std::endl;
  //
  //
  //--------------------------------------------------------------------------------
}

void setScreenColorMode(ColorMode mode, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  screens[screenid]._cmode = mode;
}

void setScreenSizeMode(SizeMode mode, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];
  screen._smode = mode;
  autoAdjustScreen(windowSize, screen);
}

void setScreenPositionMode(PositionMode mode, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];
  screen._pmode = mode;
  autoAdjustScreen(windowSize, screen);
}

void setScreenManualPosition(Vector2i position, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];
  screen._manualPosition = position;
  if(screen._pmode == PositionMode::MANUAL)
    autoAdjustScreen(windowSize, screen);
}

void setScreenManualPixelSize(int pxSize, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];
  screen._pxManualSize = std::min(std::max(minPixelSize, pxSize), maxPixelSize);
  if(screen._smode == SizeMode::MANUAL)
    autoAdjustScreen(windowSize, screen);
}

void setScreenColorBand(Color4u color, int hi, int bandid, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  auto& screen = screens[screenid];

  assert(0 <= bandid && bandid < SCREEN_BAND_COUNT);
  auto& band = screen._bands[bandid];

  band._color = color;
  band._hi = hi != 0 ? hi : std::numeric_limits<int>::max();

  std::sort(screen._bands.begin(), screen._bands.end());
}

void setBitmapColor(Color4u color, int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  screens[screenid]._bitmapColor = color;
}

void enableScreen(int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  screens[screenid]._isEnabled = true;
}

void disableScreen(int screenid)
{
  assert(0 <= screenid && screenid < screens.size());
  screens[screenid]._isEnabled = false;
}

Vector2i calculateTextSize(const std::string& text, ResourceKey_t fontKey)
{
  Vector2i size{0, 0};

  assert(0 <= fontKey && fontKey < fonts.size());
  auto& font = fonts[fontKey];

  for(char c : text){
    if(c == '\n') continue;
    assert(' ' <= c && c <= '~');
    const Glyph& glyph = font._glyphs[static_cast<int>(c - ' ')];
    size._x += glyph._xadvance + font._glyphSpace;
    size._y = size._y < glyph._height ? glyph._height : size._y;
  }
  return size;
}

} // namespace gfx
} // namespace pxr
