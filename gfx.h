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

//----------------------------------------------------------------------------------------------//
// GFX RESOURCES                                                                                //
//----------------------------------------------------------------------------------------------//

struct Sprite
{
  int _rowmin;
  int _colmin;
  int _rowmax;
  int _colmax;
  const BmpImage* _image;
};

// A sprite sheet imposes an ordered structure to a bmpimage; it imposes a regular 2D grid
// (all cells of equal dimensions). Each cell however is accessed via a single index into the
// grid where the indices are arranged like:
// 
//   
//    +---+---+---+---+
//    | 4 | 5 | 6 | 7 |     4x2 sprite sheet
//    +---+---+---+---+
//    | 0 | 1 | 2 | 3 |
//    +---+---+---+---+
//
// This allows the client to care only about the number of sprites in the sheet and the index
// of the sprite they desire.
//
// The intended purpose of this class is for use with 2D animations; a sprite sheet can contain
// a set of animation frames where each successive frame can be accessed with ascending index.
// In this case the client need only know the number of frames.
struct SpriteSheet
{
  SpriteSheet() = default;
  SpriteSheet(const SpriteSheet&) = default;
  SpriteSheet(SpriteSheet&&) = default;
  SpriteSheet& operator=(const SpriteSheet&) = default;
  SpriteSheet& operator=(SpriteSheet&&) = default;

  Sprite getSprite(int spriteNo) const;

  BmpImage _image;
  Vector2i _spriteSize;    // x=width pixels, y=height pixels.
  Vector2i _sheetSize;     // x=width sprites, y=height sprites.
  int _spriteCount;
};

// Directory where this module expects to find sprite resources when loading.
constexpr const char* spritesdir = "assets/sprites/";

// A unique key to identify a gfx resource. If 2 resources use the same key only the first
// encountered will be loaded. It is left to clients of this module to ensure uniqueness. Keys
// must only be unique for each type of resource however. A font and a sprite can use the same
// key without issue.
using ResourceKey_t = int;

// A resource name is the name of the data files for that resource. For example, a sprite called
// 'alien', will be expected to have an associated file on disk called 'alien.bmp'. A font called
// 'space' will be expected to have associated files: 'space.font' and 'space.bmp'. The names
// must not include the directory path to the resource.
using ResourceName_t = const char*;

// A manifest contains a list of resources to load via a 'load*' function.
using ResourceManifest_t = std::vector<std::pair<ResourceKey_t, ResourceName_t>>;

// Resource loading functions can be called before or after gfx module initialisation.

// Loads all sprites listed in the manifest. Upon successful loading sprites can be drawn to 
// a layer via the 'drawSprite' function which takes the ResourceKey_t for the sprite to be
// drawn. Sprites are managed internally by the gfx module, clients of this module need only
// the resource key to use the sprite once loaded.
//
// Returns true upon successfully loading all resources in the manifest, else false.
bool loadSprites(const ResourceManifest_t& manifest);

// Loads all fonts listed in the manifest. Successfully loaded fonts can be used in 'drawText'
// calls. Fonts are managed interally by the gfx module, clients of this module need only the
// resource key to use the font once loaded.
//
// Returns true upon successfully loading all resources in the manifest, else false.
bool loadFonts(const ResourceManifest_t& manifest);

//----------------------------------------------------------------------------------------------//
// GFX DRAWING                                                                                  //
//----------------------------------------------------------------------------------------------//

// Enumeration of all available rendering layers for use in draw calls.
//
// A rendering layer is conceptualised as a virtual screen of fixed resolution independent of
// window size or display resolution. The purpose of these virtual screens is to permit the
// development of display resolution dependent games e.g. space invaders which has a fixed
// world size of 224x256 pixels. Virtual screens allow the game logic to be programmed as if
// the screen has a fixed resolution.
//
// Layers use a 2D cartesian coordinate space with the origin in the bottom-left, y-axis
// ascening up the window and x-axis ascending rightward in the window.
//
//          y
//          ^     [layer coordinate space]
//          |
//  origin  o--> x
//
// The size ratio between a layer pixel and a real display pixel is controlled by the pixel 
// size mode (see modes below).
//
// The color of pixels drawn to a layer is controlled by the color mode.
//
// The position of a layer w.r.t the window is controlled by the position mode (a layer may
// not necessarily fill the entire window).
//
// Layers do not support color blending or alpha transparency. The alpha channel is however used
// as a color key where an alpha = 0 is used to skip a pixel when drawing. This allows layers
// to be actually layered rather than each layer fully obscuring the one below. A value of 0 is
// chosen as 0 is used for full transparency by convention. Thus in image editing software such
// as GIMP fully transparent pixels in the editor will also be so in game.
//
// When rendering layers are rendered to the window via the painters algorithm in the order in 
// which they are declared in this enumeration.
//
enum Layer
{
  LAYER_BACKGROUND,        // Intended for drawing static backgrounds.
  LAYER_STAGE,             // Intended for drawing all dynamic game objects.
  LAYER_UI,                // Intended for drawing the HUD/UI.
  LAYER_ENGINE_STATS,      // Reserved for use by the engine to output performance stats.
  LAYER_COUNT
};

// Modes apply to each rendering layer and can be set independently for each layer.
//
// By default layers use: 
//      ColorMode     = FULL_RGB
//      PxSizeMode = AUTO_MAX
//      PositionMode  = CENTER
//
// With the exception of the LAYER_ENGINE_STATS (which is reserved for use by the engine for
// drawing performance statistics) which has defaults:
//      ColorMode     = FULL_RGB
//      PxSizeMode = AUTO_MIN
//      PositionMode  = BOTTOM_LEFT
//
// DO NOT DRAW TO THE LAYER_ENGINE_STATS! YOU WILL OVERWRITE THE PERFORMANCE STATS.

// The color mode controls the final color of pixels that result from all draw calls.
//
// The modes apply as follows:
//
//      FULL_RGB     - unrestricted colors; colors taken from arguments in draw call (a gfx 
//                     resource argument or a direct color argument).
//
//      YAXIS_BANDED - restricted colors; the color of a pixel is determined by its y-axis
//                     position on the layer being drawn to. The bands set the colors mapped
//                     to each position. Color arguments in draw calls are ignored.
//
//      XAXIS_BANDED - restricted colors; the color of a pixel is determined by its x-axis
//                     position on the layer being drawn to. The bands set the colors mapped
//                     to each position. Color arguments in draw calls are ignored.
//
//      BITMAPS      - all pixels drawn adopt the 'bitmap color' set with a call to 
//                     'setBitmapColor'. Color arguments in draw calls are ignored. The default
//                     color is white.
enum class ColorMode
{
  FULL_RGB,
  YAXIS_BANDED,
  XAXIS_BANDED,
  BITMAPS
};

// The pixel size mode controls the size of the pixels of a layer. Minimum pixel size is 1, the
// maximum size is determined by the opengl implementation used.
//
// The modes apply as follows:
//
//      MANUAL   - pixel size is set manually to a fixed value and does not change when the
//                 window resizes.
//
//      AUTO_MIN - pixel size is automatically set to he minimum size of 1 and does not change
//                 when the window resizes (since it is already at the minimum).
//
//      AUTO_MAX - pixel size is automatically maximised to scale the layer to fit the window,
//                 thus pixel size changes as the window resizes. Pixel sizes are restricted to
//                 integer multiples of the real pixel size of the display.
//
enum class PxSizeMode
{
  MANUAL,
  AUTO_MIN,
  AUTO_MAX
};

// The position mode controls the position of a layer w.r.t the window.
// 
// The modes apply as follows:
//
//      MANUAL       - the layer's origin is at a fixed window coordinate.
//
//      CENTER       - the layer automatically moves to maintain a central position in the 
//                     window as the window resizes.
//
//      TOP_LEFT     - the layer is clamped to the top-left of the window.
//       
//      TOP_RIGHT    - the layer is clamped to the top-right of the window.
//
//      BOTTOM_LEFT  - the layer is clamped to the bottom-left of the window.
//
//      BOTTOM_RIGHT -  the layer is clamped to the bottom-right of the window.
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

// Color bands apply to a single axis (x or y). All pixels with x/y position greater than
// the 'hi' of a band 'i' and less than the 'hi' of the next band 'i+1' will adopt the color
// of the band 'i+1'.
struct ColorBand
{
  ColorBand() : _color{0, 0, 0, 0}, _hi{0}{}
  ColorBand(Color4u color, int hi) : _color{color}, _hi{hi}{}

  Color4u _color;
  int _hi;
};

bool operator<(const ColorBand& lhs, const ColorBand& rhs);
bool operator==(const ColorBand& lhs, const ColorBand& rhs);

// Configuration struct to be used with 'initialize'.
struct Configuration
{
  std::string _windowTitle;
  Vector2i _windowSize;
  Vector2i _layerSize[LAYER_COUNT];    // a size for each layer.
  bool _fullscreen;
};

// Initializes the gfx subsystem. Returns true if success and false if fatal error. Upon
// returning false calls to other module functions have undefined results thus must be called
// prior to any other function.
bool initialize(Configuration config);

// Call to shutdown the module upon app termination.
void shutdown();

// Must be called whenever the window resizes to update layer positions, virtual pixel sizes, 
// the viewport etc.
void onWindowResize(Vector2i windowSize);

// Clears the entire window to a solid color.
void clearWindow(Color4u color);

// Clears a layer such that nothing is drawn for that layer.
void clearLayer(Layer layer);

// Fills a layer with a solid shade, i.e. sets all color channels of all pixels to 'shade' 
// value. If shade == 0 (the alpha color key) this call has the same effect as 'clearLayer'. It 
// is thus not possible to fill a layer pure black. If you need black use RGB:1,1,1 for a very
// close black.
void fastFillLayer(int shade, Layer layer);

// Fills a layer with a solid color, i.e. sets all pixels to said color. This is a slow 
// operation to be used only if you need a specific color, use 'fastFillLayer' or 'clearLayer' 
// for simple clearing ops. It is not recommended this function is used in a tight loop such
// as the mainloop.
void slowFillLayer(Color4u color, Layer layer);

// Draws a sprite to a rendering layer. The resource key must be that of a sprite sheet and the
// spriteNo is the number of the sprite on the sprite sheet to draw.
void drawSprite(Vector2i position, ResourceKey_t key, int spriteNo, Layer layer);
void drawBitmap(Layer layer);
void drawRectangle(Layer layer);
void drawLine(Layer layer);
void drawParticles(Layer layer);
void drawPixel(Vector2i position, Color4u color, Layer layer);
void drawText(Layer layer);

// Called by the engine after calling App::onDraw to present the results of all drawing to the 
// window. It is not necessary to call this function within your app.
void present();

// Sets the color mode for a specific rendering layer. Changes in color mode only effect future
// draw calls; the pixels on the layer are not changed by this call. If setting a color banding
// mode use 'setLayerColorBands' to configure the bands. By default there is a single white band.
// Changing the color mode is a fast operation allowing drawing to be performed in multipe color
// modes.
void setLayerColorMode(ColorMode mode, Layer layer);

// Sets the method of determining the virtual pixel size of a layer. Has immediate effect.
void setLayerPxSizeMode(PxSizeMode mode, Layer layer);

// Sets the method of positioning the layer in the window. Has immediate effect.
void setLayerPositionMode(PositionMode mode, Layer layer);

// Sets the position of a layer. Has no effect if layer is not in position mode MANUAL.
void setLayerPosition(Vector2i position, Layer layer);

// Sets the pixel size of a layer. Has no effect if the layer is not in pixel size mode MANUAL,
// thus this function must be called after setting the layer to MANUAL mode.
void setLayerPixelSize(Layer layer);

// Sets the color bands which apply to draw calls for a layer. Has no effect if the layer is
// not in a color banding mode.
//
// The append flag controls whether the argument bands replace any existing bands or add to
// them. By default existing bands are replaced.
//
// The following rules apply to bands:
// - Bands form an ordered set with elements ordered by ascending 'hi' range value. 
// - If multiple bands have equal hi values, one of the bands will be removed but no guarantees
//   are made as to which one.
// - Bands are only used if the layer is in a color banding mode, use 'setLayerColorMode' to
//   enable a banding mode.
// - Switching a layer between banding and non-banding modes does not change previously set
//   bands.
void setLayerColorBands(std::vector<ColorBand> bands, Layer layer, bool append = false);

// Sets the color used by all draw calls (for all pixels). Has no effect if the the layer is
// not in the BITMAPS color mode. Default color is white.
void setBitmapColor(Color4u color);

} // namespace gfx
} // namespace pxr

#endif

