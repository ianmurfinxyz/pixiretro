#ifndef _PIXIRETRO_GFX_H_
#define _PIXIRETRO_GFX_H_

#include <string>
#include <cmath>

#include "color.h"
#include "math.h"
#include "bmpimage.h"

namespace pxr
{
namespace gfx
{

//
// The relative paths to resource files on disk; save your assets to these directories.
//
constexpr const char* RESOURCE_PATH_SPRITES = "assets/sprites/";
constexpr const char* RESOURCE_PATH_FONTS = "assets/fonts/";

//
// The file extensions for the resource's xml meta files.
//
constexpr const char* XML_RESOURCE_EXTENSION_SPRITES = ".sprite";
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
  BmpImage _image;
  int _lineHeight;
  int _baseLine;
  int _glyphSpace;
};

//
// A sprite frame is a sub-region of a sprite specified w.r.t a cartesian coordinate space local 
// to the sprite. The sprite space is the same as that of the bmp image.
//
// Each frame within a sprite also has its own cartesian coordinate space which the frame's
// origin is specified relative to. The frame space is thus a subspace of the sprite space and
// is axis aligned and of equal scale to its parent space.
//
//    sy                                 KEY:
//    ^                                  sx = sprite's x axis
//    |      fy                          sy = sprite's y axis
//    |      ^. . . . . .                fx = frame's x axis
//    |      |   ████   .                fy = frame's y axis
//    |      |  ███X██  .
//    |      | ██ ██ ██ .                fxp, fyp = position of frame space w.r.t sprite space.
//    |      |  ██████  .
//    |      o------------> fx           X = position of the frame origin w.r.t frame space.
//    |  (fxp, fyp) 
//    o----------------------> sx
//
// The frame position should be the pixel coordinate of the frame's bottom-left most pixel
// w.r.t to the sprite space (the bmp image).
//
// The frame size is the dimensions of the frame w.r.t either space since both spaces have the
// same scale.
//
// The origin should be a pixel coordinate w.r.t the frame space not the sprite. When drawing a
// frame, the position argument to the draw call is taken as the position of the frame origin.
// Thus if the origin is in the center of the frame then the frame will be drawn centered on the
// position argument.
//
struct SpriteFrame
{
  Vector2i _position;
  Vector2i _size;
  Vector2i _origin;
};

//
// A sprite organises a bitmap image into frames.
//
struct Sprite
{
  BmpImage _image;
  std::vector<SpriteFrame> _frames;
};

//
// The color mode controls the final color of pixels that result from all draw calls.
//
// The modes apply as follows:
//
//      FULL_RGB     - unrestricted colors; colors taken from arguments in draw call (a gfx 
//                     resource argument or a direct color argument).
//
//      YAXIS_BANDED - restricted colors; the color of a pixel is determined by its y-axis
//                     position on the screen being drawn to. The bands set the colors mapped
//                     to each position. Color arguments in draw calls are ignored.
//
//      XAXIS_BANDED - restricted colors; the color of a pixel is determined by its x-axis
//                     position on the screen being drawn to. The bands set the colors mapped
//                     to each position. Color arguments in draw calls are ignored.
//
//      BITMAPS      - all pixels drawn adopt the bitmap color of the screen being drawn to.
//
enum class ColorMode
{
  FULL_RGB,
  YAXIS_BANDED,
  XAXIS_BANDED,
  BITMAPS
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
// Color bands apply to a single axis (x or y). All pixels with x/y position greater than
// the 'hi' of a band 'i' and less than the 'hi' of the next band 'i+1' will adopt the color
// of the band 'i+1' (if drawing in a banded color mode).
//
struct ColorBand
{
  ColorBand() : _color{0, 0, 0, 0}, _hi{0}{}
  ColorBand(Color4u color, int hi) : _color{color}, _hi{hi}{}
  Color4u _color;
  int _hi;
};

//
// The max number of different color bands a screen can use.
//
constexpr int SCREEN_BAND_COUNT = 5;

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
// All screens have 3 modes of operation: position mode, size mode and color mode. For details
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
// devnote: All drawing routines are effectively software routines thus read/write access to 
// the virtual pixels needs to be optimal, thus pixel data is stored via raw pointer arrays.
//
// devnote: The pixel arrays are flattened 1D arrays so they can be passed direct to opengl.
//
struct Screen
{
  std::array<ColorBand, SCREEN_BAND_COUNT> _bands;
  PositionMode _pmode;
  SizeMode _smode;
  ColorMode _cmode;
  Vector2i _position;                // position w.r.t the window.
  Vector2i _manualPosition;          // position w.r.t the window when in manual position mode.
  Vector2i _resolution;
  Color4u _bitmapColor;
  int _pxSize;                       // size of virtual pixels (unit: real pixels).
  int _pxManualSize;                 // size of virtual pixels when in manual size mode.
  int _pxCount;                      // total number of virtual pixels on the screen.
  Color4u* _pxColors;                // accessed [col + (row * width)]
  Vector2i* _pxPositions;            // accessed [col + (row * width)]
  bool _isEnabled;
};

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
// Loads a sprite from RESOURCE_PATH_SPRITES directory in the file system.
//
// The 'name' arg must be the name of the asset files for the sprite. All sprites have 2
// asset files: an xml meta file and a bmp image file. 
//
// The naming format for the asset files is:
//    <name>.<extension>
//
// see XML_RESOURCE_EXTENSION_SPRITES and BmpImage::FILE_EXTENSION for the extensions.
//
//
// Returns the resource key the loaded sprite was mapped to which is needed for the drawing
// routines. Internally sprites are reference counted and thus can be loaded multiple times
// without duplication, each time returning the same key. To actually remove a sprite from
// memory it is necessary to unload a sprite an equal number of times to which it was loaded.
//
ResourceKey_t loadSprite(ResourceName_t name);

//
// Unloads a sprite. The sprite will only be removed from memory if the reference count drops
// to zero.
//
void unloadSprite(ResourceKey_t spriteKey);

//
// Loads a font from RESOURCE_PATH_FONTS directory in the file system.
//
// The 'name' arg must be the name of the asset files for the font. All fonts have 2
// asset files: an xml meta file and a bmp image file. 
//
// The naming format for the asset files is:
//    <name>.<extension>
//
// see XML_RESOURCE_EXTENSION_FONTS and BmpImage::FILE_EXTENSION for the extensions.
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
// Provides access to the frame count of sprites.
//
int getSpriteFrameCount(ResourceKey_t spriteKey);

//
// Clears the entire window to a solid color.
//
void clearWindowColor(Color4f color);

//
// Clears a screen to full transparency.
//
void clearScreenTransparent(int screenid);

//
// Clears a screen with a solid color shade by setting all color channels of all pixels to 
// 'shade' value. 
//
// note: if shade=0 (the alpha color key) this call has the same effect as 'clearScreen'. It is 
// thus not possible to fill a screen pure black; instead use shade=1.
//
void clearScreenShade(int shade, int screenid);

//
// Clears a screen with a solid color. 
//
// note: this is a slow operation to be used only if you really need a specific color. For 
// simple clearing ops use 'fillScreenShade'. 
//
void clearScreenColor(Color4u color, int screenid);

//
// Draw a sprite.
//
void drawSprite(Vector2i position, ResourceKey_t spriteKey, int frameid, int screenid);

//
// Takes a column of pixels from a specific frame of a sprite and draws it with the bottom
// most pixel in the column at position.
//
void drawSpriteColumn(Vector2i position, ResourceKey_t spriteKey, int frameid, int colid, int screenid);

// 
// Draw a text string.
//
void drawText(Vector2i position, const std::string& text, ResourceKey_t fontKey, int screenid);

//
// Draw a rectangle.
//
void drawRectangle(iRect rect, Color4u color, int screenid);

//
// Draw a line.
//
void drawLine(Vector2i p0, Vector2i p1, Color4u color, int screenid);

//
// Draws a single pixel to a screen.
//
void drawPoint(Vector2i position, Color4u color, int screenid);

//
// Issues opengl calls to render results of (software) draw calls and then swaps the buffers.
//
void present();

//
// Changes the color mode of a screen for all future draw calls.
//
void setScreenColorMode(ColorMode mode, int screenid);

//
// Changes the size mode of a screen with immediate effect.
//
void setScreenSizeMode(SizeMode mode, int screenid);

//
// Changes the position mode of a screen with immediate effect.
//
void setScreenPositionMode(PositionMode mode, int screenid);

//
// Changes the manual position of a screen.
//
void setScreenManualPosition(Vector2i position, int screenid);

//
// Changes the manual pixel size of a screen.
//
void setScreenManualPixelSize(int pxSize, int screenid);

//
// Configures one of the color bands of a screen.
//
// note: setting hi to 0 disables the band.
//
// note: asserts 0 <= bandid < SCREEN_BAND_COUNT.
//
void setScreenColorBand(Color4u color, int hi, int bandid, int screenid);

//
// Sets the bitmap color of a screen.
//
void setScreenBitmapColor(Color4u color, int screenid);

//
// Enables a screen so it will be rendered to the window.
//
void enableScreen(int screenid);

//
// Disable a screen so it will not be rendered to the window.
//
void disableScreen(int screenid);

//
// Utility function for calculating the dimensions of the smallest possible bounding box of 
// a text string for a given font. Dimensions are in units of virtual pixels.
//
Vector2i calculateTextSize(const std::string& text, ResourceKey_t fontKey);

//
// Utility to test if a sprite resource key is associated with the error sprite. Allows testing
// if a sprite load failed.
//
bool isErrorSprite(ResourceKey_t spriteKey);

//
// Utility to access the size of sprite frames.
//
Vector2i getSpriteSize(ResourceKey_t spriteKey, int frameid);

} // namespace gfx
} // namespace pxr

#endif

