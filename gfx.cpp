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
#include "log.h"

using namespace tinyxml2;

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

struct Glyph
{
  int _ascii;
  int _x;
  int _y;
  int _width;
  int _height;
  int _xoffset;
  int _yoffset;
  int _xadvance;
};

struct Font
{
  static constexpr char* FILE_EXTENSION {".fn"};
  static constexpr int ASCII_CHAR_COUNT {95};
  static constexpr int ASCII_CHAR_CHECKSUM {7505}; // sum of ascii codes 32 to 126.

  std::array<Glyph, ASCII_CHAR_COUNT> _glyphs;
  BmpImage _image;
  int _lineHeight;
  int _baseLine;
  int _glyphSpace;
};

static constexpr const char* SPRITESHEET_XML_FILE_EXTENSION {".ss"};

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
static std::unordered_map<ResourceKey_t, Font> fonts;
static SpriteSheet errorSprite;
static Font errorFont;

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

static void initializeScreens(Configuration config)
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

    screen._size = config._layerSize[layer];
    assert(0 < screen._size._x && screen._size._x <= Screen::MAX_WIDTH);
    assert(0 < screen._size._y && screen._size._y <= Screen::MAX_HEIGHT);
    screen._pxCount  = screen._size._x * screen._size._y;
  
    screen._pxColors = new Color4u[screen._pxCount];
    screen._pxPositions = new Vector2i[screen._pxCount];
    
    clearLayer(static_cast<Layer>(layer)); 
    autoAdjustScreen(windowSize, screen);
    
    screen._bands.push_back(ColorBand{colors::white, std::numeric_limits<int>::max()});
    screen._bitmapColor = colors::white;

    std::stringstream ss {};
    ss << "[w:" << screen._size._x << ", h:" << screen._size._y << "] "
       << "mem:" << (screen._pxCount * sizeof(Color4u)) / 1024 << "kib";
    log::log(log::INFO, log::msg_gfx_created_vscreen, ss.str());
  }
}

static void freeScreens()
{
  for(int layer = LAYER_BACKGROUND; layer < LAYER_COUNT; ++layer){
    auto& screen = screens[layer];
    delete[] screen._pxColors;
    delete[] screen._pxPositions;
    screen._pxColors = nullptr;
    screen._pxPositions = nullptr;
  }
}

static void genErrorSprite()
{
  static constexpr int squareSize = 8;

  errorSprite._spriteSize = Vector2i{squareSize, squareSize};
  errorSprite._sheetSize = Vector2i{1, 1};
  errorSprite._spriteCount = 1;
  errorSprite._image.create(errorSprite._spriteSize, colors::red);
}

// Produces a font with all 95 printable ascii characters where all characters are just blank
// red squares.
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

  genErrorSprite();

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

enum AssetID { AID_SPRITE, AID_FONT };

Sprite SpriteSheet::getSprite(int spriteNo) const
{
  // The game/app should make sure this never happens; should be aware of which sprite sheets
  // have what spriteNos.
  assert(0 <= spriteNo && spriteNo < _spriteCount);

  Sprite sprite;
  int row = spriteNo / _sheetSize._x;
  int col = spriteNo % _sheetSize._x;
  sprite._rowmin = row * _spriteSize._y;
  sprite._colmin = col * _spriteSize._x;
  sprite._rowmax = (row + 1) * _spriteSize._y;
  sprite._colmax = (col + 1) * _spriteSize._x;
  sprite._image = &_image;

  return sprite;
}

static void useErrorAsset(ResourceKey_t rkey, AssetID aid)
{
  switch(aid){
    case AID_SPRITE:
      log::log(log::INFO, log::msg_gfx_using_error_sprite);
      sprites.insert(std::make_pair(rkey, errorSprite));
      break;
    case AID_FONT:
      log::log(log::INFO, log::msg_gfx_using_error_font);
      fonts.insert(std::make_pair(rkey, errorFont));
      break;
  }
}

static bool parseXmlDocument(XMLDocument* doc, const std::string& xmlpath, ResourceKey_t rkey, 
                             AssetID aid)
{
  doc->LoadFile(xmlpath.c_str());
  if(doc->Error()){
    log::log(log::ERROR, log::msg_gfx_fail_xml_parse, xmlpath); 
    log::log(log::INFO, log::msg_gfx_tinyxml_error_name, doc->ErrorName());
    log::log(log::INFO, log::msg_gfx_tinyxml_error_desc, doc->ErrorStr());
    useErrorAsset(rkey, aid);
    return false;
  }
  return true;
}

static bool extractChildElement(XMLNode* parent, XMLElement** child, const char* childname,
                                ResourceKey_t rkey, AssetID aid)
{
  *child = parent->FirstChildElement(childname);
  if(*child == 0){
    log::log(log::ERROR, log::msg_gfx_fail_xml_element, childname);
    useErrorAsset(rkey, aid);
    return false;
  }
  return true;
}

static bool extractIntAttribute(XMLElement* element, const char* attribute, int* value, 
                                ResourceKey_t rkey, ResourceName_t rname, XMLDocument* doc, 
                                AssetID aid)
{
  XMLError xmlerror = element->QueryIntAttribute(attribute, value);
  if(xmlerror != XML_SUCCESS){
    log::log(log::ERROR, log::msg_gfx_fail_xml_attribute, std::string{attribute});
    useErrorAsset(rkey, aid);
    return false;
  }
  return true;
}

static bool isValidSpriteSheet(SpriteSheet& sheet)
{
  if((sheet._sheetSize._x * sheet._spriteSize._x) != sheet._image.getWidth())
    return false;

  if((sheet._sheetSize._y * sheet._spriteSize._y) != sheet._image.getHeight())
    return false;

  return true;
}

bool loadSprites(const ResourceManifest_t& manifest)
{
  log::log(log::INFO, log::msg_gfx_loading_sprites);

  for(auto &pair : manifest){
    ResourceKey_t rkey = pair.first;
    ResourceName_t rname = pair.second;

    std::stringstream ss{};
    ss << "key:" << rkey << " name:" << rname;
    log::log(log::INFO, log::msg_gfx_loading_asset, ss.str());

    auto search = sprites.find(rkey);
    if(search != sprites.end()){
      log::log(log::WARN, log::msg_gfx_duplicate_resource_key, std::to_string(rkey));
      log::log(log::INFO, log::msg_gfx_skipping_asset_load, std::string{rname});
      continue;
    }

    SpriteSheet sheet{};

    std::string bmppath{};
    bmppath += spritesdir;
    bmppath += rname;
    bmppath += BmpImage::FILE_EXTENSION;
    if(!sheet._image.load(bmppath)){
      log::log(log::ERROR, log::msg_gfx_fail_load_asset_bmp, std::string{rname});
      useErrorAsset(rkey, AID_SPRITE);
      continue;
    }

    std::string xmlpath {};
    xmlpath += spritesdir;
    xmlpath += rname;
    xmlpath += SPRITESHEET_XML_FILE_EXTENSION;
    XMLDocument doc{};
    if(!parseXmlDocument(&doc, xmlpath, rkey, AID_SPRITE)) continue;

    XMLElement* element {nullptr};

    if(!extractChildElement(&doc, &element, "spritesheet", rkey, AID_SPRITE)) continue;
    if(!extractIntAttribute(element, "rows", &sheet._sheetSize._y, rkey, rname, &doc, AID_SPRITE)) continue;
    if(!extractIntAttribute(element, "cols", &sheet._sheetSize._x, rkey, rname, &doc, AID_SPRITE)) continue;
    if(!extractChildElement(&doc, &element, "sprites", rkey, AID_SPRITE)) continue;
    if(!extractIntAttribute(element, "width", &sheet._spriteSize._x, rkey, rname, &doc, AID_SPRITE)) continue;
    if(!extractIntAttribute(element, "height", &sheet._spriteSize._y, rkey, rname, &doc, AID_SPRITE)) continue;

    sheet._spriteCount = sheet._sheetSize._x * sheet._sheetSize._y;

    if(!isValidSpriteSheet(sheet)){
      log::log(log::ERROR, log::msg_gfx_asset_invalid_xml_bmp_mismatch, std::string{rname});
      log::log(log::INFO, log::msg_gfx_using_error_sprite);
      sprites.insert(std::make_pair(rkey, errorSprite));
      continue;
    }

    sprites.insert(std::make_pair(rkey, std::move(sheet)));
  }
}

bool loadFonts(const ResourceManifest_t& manifest)
{
  log::log(log::INFO, log::msg_gfx_loading_fonts);

  for(auto &pair : manifest){
    ResourceKey_t rkey = pair.first;
    ResourceName_t rname = pair.second;

    std::stringstream ss{};
    ss << "key:" << rkey << " name:" << rname;
    log::log(log::INFO, log::msg_gfx_loading_asset, ss.str());

    auto search = fonts.find(rkey);
    if(search != fonts.end()){
      log::log(log::WARN, log::msg_gfx_duplicate_resource_key, std::to_string(rkey));
      log::log(log::INFO, log::msg_gfx_skipping_asset_load, std::string{rname});
      continue;
    }

    Font font{};

    std::string bmppath{};
    bmppath += fontsdir;
    bmppath += rname;
    bmppath += BmpImage::FILE_EXTENSION;
    if(!font._image.load(bmppath)){
      log::log(log::ERROR, log::msg_gfx_fail_load_asset_bmp, std::string{rname});
      useErrorAsset(rkey, AID_FONT);
      continue;
    }

    std::string xmlpath {};
    xmlpath += fontsdir;
    xmlpath += rname;
    xmlpath += Font::FILE_EXTENSION;
    XMLDocument doc{};
    parseXmlDocument(&doc, xmlpath, rkey, AID_FONT);

    XMLElement* fontElement {nullptr};
    XMLElement* infoElement {nullptr};
    XMLElement* bitmapElement {nullptr};
    XMLElement* charsElement {nullptr};
    XMLElement* charElement {nullptr};

    if(!extractChildElement(&doc, &fontElement, "font", rkey, AID_FONT)) continue;

    if(!extractChildElement(fontElement, &infoElement, "info", rkey, AID_FONT)) continue;
    if(!extractIntAttribute(infoElement, "lineHeight", &font._lineHeight, rkey, rname, &doc, AID_FONT)) continue;
    if(!extractIntAttribute(infoElement, "baseline", &font._baseLine, rkey, rname, &doc, AID_FONT)) continue;
    if(!extractIntAttribute(infoElement, "glyphspace", &font._glyphSpace, rkey, rname, &doc, AID_FONT)) continue;

    Vector2i xmlSize{};
    if(!extractChildElement(fontElement, &bitmapElement, "bitmap", rkey, AID_FONT)) continue;
    if(!extractIntAttribute(bitmapElement, "width", &xmlSize._x, rkey, rname, &doc, AID_FONT)) continue;
    if(!extractIntAttribute(bitmapElement, "height", &xmlSize._y, rkey, rname, &doc, AID_FONT)) continue;

    Vector2i bmpSize = font._image.getSize();

    if(xmlSize != bmpSize){
      std::stringstream ss{};
      ss << "in xml file '" << xmlpath << "' [w:" << xmlSize._x << ", h:" << xmlSize._y << "] : "
         << "in bmp file '" << bmppath << "' [w:" << bmpSize._x << ", h:" << bmpSize._y << "]";
      log::log(log::ERROR, log::msg_gfx_font_size_mismatch, ss.str());
      log::log(log::INFO, log::msg_gfx_asset_invalid_xml_bmp_mismatch);
      useErrorAsset(rkey, AID_FONT);
    }

    int charsCount {0};
    if(!extractChildElement(fontElement, &charsElement, "chars", rkey, AID_FONT)) continue;
    if(!extractIntAttribute(charsElement, "count", &charsCount, rkey, rname, &doc, AID_FONT)) continue;

    if(charsCount != Font::ASCII_CHAR_COUNT){
      log::log(log::ERROR, log::msg_gfx_missing_ascii_glyphs, rname);
      useErrorAsset(rkey, AID_FONT);
      continue;
    }

    int charsRead {0};
    bool isError {false};
    if(!extractChildElement(charsElement, &charElement, "char", rkey, AID_FONT)) continue;
    do{
      Glyph& glyph = font._glyphs[charsRead];
      if(!extractIntAttribute(charElement, "ascii", &glyph._ascii, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "x", &glyph._x, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "y", &glyph._y, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "width", &glyph._width, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "height", &glyph._height, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "xoffset", &glyph._xoffset, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "yoffset", &glyph._yoffset, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      if(!extractIntAttribute(charElement, "xadvance", &glyph._xadvance, rkey, rname, &doc, AID_FONT)) {isError = true; break;}
      ++charsRead;
      charElement = charElement->NextSiblingElement("char");
    }
    while(charElement != 0 && charsRead < Font::ASCII_CHAR_COUNT);

    if(isError)
      continue;

    if(charsRead != Font::ASCII_CHAR_COUNT){
      log::log(log::ERROR, log::msg_gfx_missing_ascii_glyphs, rname);
      useErrorAsset(rkey, AID_FONT);
      continue;
    }

    // chars in the xml file may be listed in any order but we require them in ascending order
    // of ascii value.
    std::sort(font._glyphs.begin(), font._glyphs.end(), [](const Glyph& g0, const Glyph& g1) {
      return g0._ascii < g1._ascii;
    });

    // checksum is used to to test for the condition in which we have the correct number of 
    // glyphs but some are duplicates of the same character.
    int checksum {0};
    for(auto& glyph : font._glyphs)
      checksum += glyph._ascii;

    if(checksum != Font::ASCII_CHAR_CHECKSUM){
      log::log(log::ERROR, log::msg_gfx_font_fail_checksum);
      useErrorAsset(rkey, AID_FONT);
      continue;
    }

    fonts.insert(std::make_pair(rkey, std::move(font)));
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
  memset(screens[layer]._pxColors, alphaKey, screens[layer]._pxCount * sizeof(Color4u));
}

void fastFillLayer(int shade, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  shade = std::max(0, std::min(shade, 255));
  memset(screens[layer]._pxColors, shade, screens[layer]._pxCount * sizeof(Color4u));
}

void slowFillLayer(Color4u color, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  Screen& screen = screens[layer];
  for(int px = 0; px < screen._pxCount; ++px)
    screen._pxColors[px] = color;
}

void drawSprite(Vector2i position, ResourceKey_t spriteKey, int spriteNo, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  auto search = sprites.find(spriteKey);

  // The game/app should ensure this can never happen by ensuring all sprites have
  // been loaded before trying to draw them.
  assert(search != sprites.end());

  Sprite sprite = (*search).second.getSprite(spriteNo);
  const Color4u* const * spritePxs = sprite._image->getPixels();

  // note: spritesheets are validated to ensure the range of rows and cols of all their sprites 
  // lie within the image, thus don't need to check that here.

  int screenRow {0}, screenCol{0};
  for(int spriteRow = sprite._rowmin; spriteRow < sprite._rowmax; ++spriteRow){
    screenRow = position._y + spriteRow;
    if(screenRow < 0) 
      continue;
    if(screenRow >= screen._size._y)
      break;
    for(int spriteCol = sprite._colmin; spriteCol < sprite._colmax; ++spriteCol){
      screenCol = position._x + spriteCol;
      if(screenCol < 0)
        continue;
      if(screenCol >= screen._size._x)
        break;
      screen._pxColors[screenCol + (screenRow * screen._size._x)] = spritePxs[spriteRow][spriteCol]; 
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

void drawText(Vector2i position, const std::string& text, ResourceKey_t fontKey, Layer layer)
{
  assert(LAYER_BACKGROUND <= layer && layer < LAYER_COUNT);
  auto& screen = screens[layer];

  auto search = fonts.find(fontKey);
  assert(search != fonts.end());
  Font& font = (*search).second;
  const Color4u* const* fontPxs = font._image.getPixels();

  for(char c : text){
    assert(' ' <= c && c <= '~');
    const Glyph& glyph = font._glyphs[static_cast<int>(c - ' ')];
    
    int screenRow{0}, screenCol{0};
    for(int glyphRow = 0; glyphRow < glyph._height; ++glyphRow){
      screenRow = position._y + glyphRow + font._baseLine + glyph._yoffset;

      // Drawing top-to-bottom thus if we are beyond the bottom screen border then their may
      // be some more glyph rows that are above it.
      if(screenRow < 0) 
        continue;

      // If we are beyond the top border then all subsequent glyph rows will also be beyond.
      if(screenRow >= screen._size._y)
        break;

      for(int glyphCol = 0; glyphCol < glyph._width; ++glyphCol){
        screenCol = position._x + glyphCol + glyph._xoffset;

        // If we are beyond the left border then their may be some more glyph columns within 
        // the screen.
        if(screenCol < 0)
          continue;

        // Drawing left-to-right thus if screen column is beyond the right-most screen border
        // then all subsequent characters will be too.
        if(screenCol >= screen._size._x)
          return;

        const Color4u& pxColor = fontPxs[glyph._y + glyphRow][glyph._x + glyphCol];

        if(pxColor.getAlpha() == alphaKey)
          continue;
        
        screen._pxColors[screenCol + (screenRow * screen._size._x)] = pxColor;
      }
    }
    position._x += glyph._xadvance + font._glyphSpace;
  }
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
