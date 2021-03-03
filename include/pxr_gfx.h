#ifndef _PIXIRETRO_GFX_H_
#define _PIXIRETRO_GFX_H_

#include <string>
#include <cmath>

#include "pxr_color.h"
#include "pxr_vec.h"
#include "pxr_rect.h"
#include "pxr_bmp.h"

namespace pxr
{
namespace gfx
{

//
// The relative paths to resource files on disk; save your assets to these directories.
//
constexpr const char* RESOURCE_PATH_SPRITESHEETS = "assets/spritesheets/";
constexpr const char* RESOURCE_PATH_FONTS = "assets/fonts/";

//
// The file extensions for the resource's xml meta files.
//
constexpr const char* XML_RESOURCE_EXTENSION_SPRITESHEETS = ".spritesheet";
constexpr const char* XML_RESOURCE_EXTENSION_FONTS = ".font";

//
// A unique key to identify a gfx resource for use in draw calls.
//
using ResourceKey_t = int;

// 
// The name of a gfx resource used to find the resource's files on disk.
//
using ResourceName_t = const char*;

// 
// Total number of printable ASCII characters. 
//
constexpr int ASCII_CHAR_COUNT = 95;

//
// Sum of all printable ASCII character codes from 32 (!) to 126 (~) inclusive.
//
constexpr int ASCII_CHAR_CHECKSUM = 7505;

//
// A font glyph.
//
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

// 
// An ASCII bitmap font.
//
struct Font
{
  std::array<Glyph, ASCII_CHAR_COUNT> _glyphs;
  io::Bmp _image;
  int _lineHeight;
  int _baseLine;
  int _glyphSpace;
};

//
// A sprite is a sub-region of a spritesheet specified w.r.t a cartesian coordinate space local 
// to the spritesheet. The spritesheet space is the same as that of the bmp image where the
// origin is in the bottom-left corner.
//
// Each sprite within a spritesheet also has its own cartesian coordinate space which the sprites's
// origin is specified relative to. The sprite space is thus a subspace of the sprite space and
// is axis aligned and of equal scale to its parent space.
//
//    ssy                                KEY:
//    ^                                  ssx = spritesheet's x axis
//    |      sy                          ssy = spritesheet's y axis
//    |      ^. . . . . .                sx = sprite's x axis
//    |      |   ████   .                sy = sprite's y axis
//    |      |  ███X██  .
//    |      | ██ ██ ██ .                sxp, syp = position of sprite space w.r.t spritesheet space.
//    |      |  ██████  .
//    |      o------------> sx           X = position of the sprite origin w.r.t sprite space.
//    |  (sxp, syp) 
//    o----------------------> ssx
//
// The sprite position should be the pixel coordinate of the sprite's bottom-left most pixel
// w.r.t to the spritesheet space (the bmp image).
//
// The sprite size is the dimensions of the sprite w.r.t either space since both spaces have the
// same scale.
//
// The origin should be a pixel coordinate w.r.t the sprite space not the spritesheet. When drawing a
// sprite, the position argument to the draw call is taken as the position of the sprite origin.
// Thus if the origin is in the center of the sprite then the sprite will be drawn centered on the
// position argument.
//
struct Sprite
{
  Vector2i _position;
  Vector2i _size;
  Vector2i _origin;
};

//
// The type of sprite ids required in draw calls. Primarily used to improve code readability.
//
using SpriteId_t = int;

//
// A spritesheet organises a bitmap image into sprites.
//
struct Spritesheet
{
  io::Bmp _image;
  std::vector<Sprite> _sprites;
};

//
// The pixel mode sets whether to use a pixel shader in draw calls.
//
// The modes apply as follows:
//
//      NO_SHADER - the default. Pixels colors are taken direct from draw call args (either
//                  an explicit color arg or the gfx resource).
//
//      SHADER    - Pixel colors are fed into a user provided shader function along with the
//                  pixel coordinate. The output color is then drawn to the screen.
//
enum class PixelMode
{
  NO_SHADER,
  SHADER
};

//
// The size mode controls the size of the pixels of a screen. Minimum pixel size is 1, the
// maximum size is determined by the opengl implementation used (max is printed to the log
// upon gfx initialization for convenience).
//
// The modes apply as follows:
//
//      MANUAL   - pixel size is set manually to a fixed value and does not change when the
//                 window resizes.
//
//      AUTO_MIN - pixel size is automatically set to he minimum size of 1 and does not change
//                 when the window resizes (since it is already at the minimum).
//
//      AUTO_MAX - pixel size is automatically maximised to scale the screen to fit the window,
//                 thus pixel size changes as the window resizes. Pixel sizes are restricted to
//                 integer multiples of the real pixel size of the display.
//
enum class SizeMode
{
  MANUAL,
  AUTO_MIN,
  AUTO_MAX
};

//
// The position mode controls the position of a screen w.r.t the window.
// 
// The modes apply as follows:
//
//      MANUAL       - the screen's origin is at a fixed window coordinate.
//
//      CENTER       - the screen automatically moves to maintain a central position in the 
//                     window as the window resizes.
//
//      TOP_LEFT     - the screen is clamped to the top-left of the window.
//       
//      TOP_RIGHT    - the screen is clamped to the top-right of the window.
//
//      BOTTOM_LEFT  - the screen is clamped to the bottom-left of the window.
//
//      BOTTOM_RIGHT -  the screen is clamped to the bottom-right of the window.
//
enum class PositionMode
{
  MANUAL,
  CENTER,
  TOP_LEFT,
  TOP_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_RIGHT
};

//
// The signiture of pixel shader functions to be set by the user if using PixelMode::SHADER.
//
// The arguments to the shader are:
//
//    inColor - the color sampled from the gfx resource or taken the color argument to the
//              draw call.
//
//    pxx     - the x-axis position of the current pixel being drawn w.r.t the virtual screen 
//              coordinate space.
//
//    pxy     - the y-axis position of the current pixel being drawn w.r.t the virtual screen 
//              coordinate space.
//
// The shader returns a color value it has calculated. The returned color value will replace
// the input color for the pixel being drawn to the screen at position [pxx, pxy].
//
using PXShader_t = Color4u (*)(Color4u inColor, int pxx, int pxy);

//
// A virtual screen of virtual pixels used to create a layer of abstraction from the display
// allowing extra properties to be added to the screen such as a fixed resolution independent
// of window size or display resolution.
//
// Screens use a 2D cartesian coordinate space with the origin in the bottom-left, y-axis
// ascending north and x-axis ascending east.
//
//          y
//          ^     [screen coordinate space]
//          |
//  origin  o---> x
//
// All screens have 3 modes of operation: position mode, size mode and pixel mode. For details
// of the modes see the modes enumerations above.
//
// Screens do not support any color blending however they do support a color key of sorts to 
// allow pixels in draw calls to be omitted; any pixel to be drawn with an alpha=0 will be 
// skipped. Note that this allows fully transparent pixels drawn in an image editor like GIMP
// to be omitted when drawing.
//
// Further screens can be stacked on top of one another. The draw order (stack order) is 
// determined by the order in which the screens were created; first created first draw. Any
// transparent pixels in a screen will allow the corresponding pixel of any screens lower in the 
// stacking order to show through.
//
struct Screen
{
  PXShader_t   _pxShader;
  PositionMode _pmode;
  SizeMode     _smode;
  PixelMode    _xmode;
  Vector2i     _position;        // position w.r.t window space.
  Vector2i     _manualPosition;  // position w.r.t window space when in manual position mode.
  Vector2i     _resolution;      // size/dimensions of the virtual screen.
  int          _pxSize;          // size of virtual pixels (unit: real pixels).
  int          _pxManualSize;    // size of virtual pixels when in manual size mode.
  int          _pxCount;         // total number of virtual pixels on the screen.
  Color4u*     _pxColors;        // accessed [col + (row * width)]
  Vector2i*    _pxPositions;     // accessed [col + (row * width)]
  bool         _isEnabled;       // enable/disable drawing this screen to the window.
};

//
// The type of screen ids. Primarly used to improve code readability.
//
using ScreenId_t = int;

//
// Initializes the gfx subsystem. Returns true if success and false if fatal error.
//
bool initialize(std::string windowTitle, Vector2i windowSize, bool fullscreen);

//
// Call to shutdown the module upon app termination.
//
void shutdown();

//
// Creates a new virtual screen which can be drawn to via a draw call. 
//
// By default screens are created with modes set as: 
//
//      ColorMode    = FULL_RGB
//      SizeMode     = AUTO_MAX
//      PositionMode = CENTER
//
// Returns the integer id of the screen for use with draw calls. Internally screens are stored
// in an array thus returned ids start at 0 and increase by 1 with each new screen created.
//
int createScreen(Vector2i resolution);

//
// Must be called whenever the window resizes to update the screens.
//
void onWindowResize(Vector2i windowSize);

//
// Loads a spritesheet from RESOURCE_PATH_SPRITESHEET directory in the file system.
//
// The 'name' arg must be the name of the asset files for the spritesheet. All spritesheets have 
// 2 asset files: an xml meta file and a bmp image file. 
//
// The naming format for the asset files is:
//    <name>.<extension>
//
// see XML_RESOURCE_EXTENSION_SPRITESHEET and Bmp::FILE_EXTENSION for the extensions.
//
//
// Returns the resource key the loaded spritesheet was mapped to which is needed for the drawing
// routines. Internally spritesheets are reference counted and thus can be loaded multiple times
// without duplication, each time returning the same key. To actually remove a spritesheet from
// memory it is necessary to unload a spritesheet an equal number of times to which it was loaded.
//
ResourceKey_t loadSpritesheet(ResourceName_t name);

//
// Unloads a spritesheet. The spritesheet will only be removed from memory if the reference 
// count drops to zero.
//
void unloadSpritesheet(ResourceKey_t sheetKey);

//
// Loads a font from RESOURCE_PATH_FONTS directory in the file system.
//
// The 'name' arg must be the name of the asset files for the font. All fonts have 2
// asset files: an xml meta file and a bmp image file. 
//
// The naming format for the asset files is:
//    <name>.<extension>
//
// see XML_RESOURCE_EXTENSION_FONTS and Bmp::FILE_EXTENSION for the extensions.
//
// Returns the resource key the loaded font was mapped to which is needed for the drawing
// routines. Internally fonts are reference counted and thus can be loaded multiple times
// without duplication, each time returning the same key. To actually remove a fonts from
// memory it is necessary to unload a fonts an equal number of times to which it was loaded.
//
ResourceKey_t loadFont(ResourceName_t name);

//
// Unloads a font. The font will only be removed from memory if the reference count drops 
// to zero.
//
void unloadFont(ResourceKey_t fontKey);

//
// Provides access to the sprite count of a spritesheet.
//
int getSpriteCount(ResourceKey_t sheetKey);

//
// Clears the entire window to a solid color.
//
void clearWindowColor(Color4f color);

//
// Clears a screen to full transparency.
//
void clearScreenTransparent(ScreenId_t screenid);

//
// Clears a screen with a solid color shade by setting all color channels of all pixels to 
// 'shade' value. 
//
// note: if shade=0 (the alpha color key) this call has the same effect as 'clearScreen'. It is 
// thus not possible to fill a screen pure black; instead use shade=1.
//
void clearScreenShade(int shade, ScreenId_t screenid);

//
// Clears a screen with a solid color. 
//
// note: this is a slow operation to be used only if you really need a specific color. For 
// simple clearing ops use 'fillScreenShade'. 
//
void clearScreenColor(Color4u color, ScreenId_t screenid);

//
// Draw a sprite of a spritesheet.
//
void drawSprite(Vector2i position, ResourceKey_t sheetKey, SpriteId_t spriteid, ScreenId_t screenid, 
                bool mirrorX = false, bool mirrorY = false);

//
// Takes a column of pixels from a specific sprite of a spritesheet and draws it with the bottom
// most pixel in the column at position.
//
void drawSpriteColumn(Vector2i position, ResourceKey_t sheetKey, SpriteId_t spriteid, int colid, ScreenId_t screenid);

// 
// Draw a text string.
//
void drawText(Vector2i position, const std::string& text, ResourceKey_t fontKey, ScreenId_t screenid);

//
// Draw a border rectangle, i.e draw only the outline. This function clamps the rectangle to 
// within the screen boundary.
//
void drawBorderRectangle(iRect rect, Color4u color, ScreenId_t screenid);

//
// Draw a fill rectangle. This function clamps the rectangle to within the screen boundary.
//
void drawFillRectangle(iRect rect, Color4u color, ScreenId_t screenid);

//
// Draw a line.
//
void drawLine(Vector2i p0, Vector2i p1, Color4u color, ScreenId_t screenid);

//
// Draws a single pixel to a screen.
//
void drawPoint(Vector2i position, Color4u color, ScreenId_t screenid);

//
// Issues opengl calls to render results of (software) draw calls and then swaps the buffers.
//
void present();

//
// Changes the pixel mode of a screen for all future draw calls.
//
void setScreenPixelMode(PixelMode mode, ScreenId_t screenid);

//
// Changes the size mode of a screen with immediate effect.
//
void setScreenSizeMode(SizeMode mode, ScreenId_t screenid);

//
// Changes the position mode of a screen with immediate effect.
//
void setScreenPositionMode(PositionMode mode, ScreenId_t screenid);

//
// Changes the manual position of a screen.
//
void setScreenManualPosition(Vector2i position, ScreenId_t screenid);

//
// Changes the manual pixel size of a screen.
//
void setScreenManualPixelSize(int pxSize, ScreenId_t screenid);

//
// Sets the pixel shader function to use for a particular screen. This function will only be 
// used if the screen is in PixelMode::SHADER.
//
void setPixelShader(PXShader_t shader, ScreenId_t screenid);

//
// Enables a screen so it will be rendered to the window.
//
void enableScreen(ScreenId_t screenid);

//
// Disable a screen so it will not be rendered to the window.
//
void disableScreen(ScreenId_t screenid);

//
// Utility function for calculating the dimensions of the smallest possible bounding box of 
// a text string for a given font. Dimensions are in units of virtual pixels.
//
Vector2i calculateTextSize(const std::string& text, ResourceKey_t fontKey);

//
// Utility to test if a spritesheet resource key is associated with the error spritesheet. Allows 
// testing if a spritesheet load failed.
//
bool isErrorSpritesheet(ResourceKey_t sheetKey);

//
// Utility to access the size of a spritesheet.
//
Vector2i getSpritesheetSize(ResourceKey_t sheetKey);

//
// Utility to access the size of a sprite within a spritesheet.
//
Vector2i getSpriteSize(ResourceKey_t sheetKey, SpriteId_t spriteid);

//
// Provides read only access to internally stored spritesheets.
//
const Spritesheet& getSpritesheet(ResourceKey_t sheetKey);

} // namespace gfx
} // namespace pxr

#endif

