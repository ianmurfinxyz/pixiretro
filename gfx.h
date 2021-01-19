#ifndef _GFX_H_
#define _GFX_H_

#include <memory>
#include "color.h"

namespace pxr
{

class Renderer
{
public:
  struct Config
  {
    std::string _windowTitle;
    int32_t _windowWidth;
    int32_t _windowHeight;
    int32_t _openglVersionMajor;
    int32_t _openglVersionMinor;
    bool _fullscreen;
  };
  
public:
  Renderer(const Config& config);
  Renderer(const Renderer&) = delete;
  Renderer* operator=(const Renderer&) = delete;
  ~Renderer();
  void setViewport(iRect viewport);
  void blitText(Vector2f position, const std::string& text, const Font& font, const Color3f& color);
  void blitBitmap(Vector2f position, const Bitmap& bitmap, const Color3f& color);
  void drawBorderRect(const iRect& rect, const Color3f& background, const Color& borderColor, int32_t borderWidth = 1);
  void clearWindow(const Color3f& color);
  void clearViewport(const Color3f& color);
  void show();
  Vector2i getWindowSize() const;

private:
  SDL_Window* _window;
  SDL_GLContext _glContext;
  Config _config;
  iRect _viewport;
};

//----------------------------------------------------------------------------------------------//
// RENDERER                                                                                     //
//----------------------------------------------------------------------------------------------//

class Renderer
{
public:
  struct Config
  {
    std::string _windowTitle;
    int32_t _windowWidth;
    int32_t _windowHeight;
  };

public:
  Renderer(const Config& config);
  Renderer(const Renderer&) = delete;
  Renderer* operator=(const Renderer&) = delete;
  ~Renderer();
  void setViewport(iRect viewport);
  void clearWindow(const Color4& color);
  void clearViewport(const Color4& color);
  void drawPixelArray(int first, int count, void* pixels, int pixelSize);
  void show();
  Vector2i getWindowSize() const;

  int queryMaxPixelSize();

private:
  static constexpr int openglVersionMajor = 2;
  static constexpr int openglVersionMinor = 1;

private:
  SDL_Window* _window;
  SDL_GLContext _glContext;
  Config _config;
  iRect _viewport;
};

//----------------------------------------------------------------------------------------------//
// SCREEN                                                                                       //
//----------------------------------------------------------------------------------------------//

// A virtual screen with resolution independent of display resolution and window size.
//
// Pixels on the screen are arranged on a coordinate system with the origin in the bottom left
// most corner, rows ascending north and columns ascending east as shown below.
//
//         row
//          ^
//          |
//          |
//   origin o----> col
//
// Although the resolution of the virtual screen is fixed, the size of each virtual pixel on 
// the real screen is variable and controlled by the size mode. The size of virtual pixels are 
// always integer multiples of real pixels however. It is thus posible the virtual screen does
// not occupy all space on the real screen (in the window). The position mode controls the
// position of the virtual screen in the window and thus also the position of any empty space.
// A screen can be clamped to a corner of the window or centrally aligned. When centrally
// aligned empty space will be distibuted evenly around the virtual screen. Pixel sizes cannot
// be smaller than 1 and have a limit defined by the opengl implementation used.
//
// Screens have two color modes: full-color and y-banded. In full-color mode pixel colors are 
// set via the drawing functions, or taken from the gfx resource (sprites, bitmaps etc). In 
// y-banded mode the color of a pixel is determined by its y-axis position which determines the
// color band in which the pixel resides. This mode is designed specifically to create the 
// drawing effect seen in classic space invaders (part I and II).
//
// The Screen is designed to be used within a stack of screens where each member represents
// a drawing layer. The layers are then drawn in their stacking order. Thus screens do not 
// support transparency on the screen level but do support it on the stack level. Any pixel 
// drawn to a screen is considered fully transparent if its alpha==255 and fully opaque if its
// alpha<255 on the stack level. So for example if there 2 screens on a stack, the bottom screen
// shows a fully opaque background and the top screen has only pixels with alpha==255, when the
// stack is drawn only the bottom screen will contribute to the final image. If the top screen 
// was fully transparent except for a red square in a corner. The final image will show the
// background with a red square in a corner. However drawing to the same place on the same 
// screen will not perform any alpha blending. All drawing to a screen overwrites all pixels
// that occupy that space.
class Screen
{
public:
  // Pixels fall inside a color band i if their y-axis position is greater than the end of
  // the band i-1 and less than or equal to the end of the band i. Color bands form an ordered 
  // set with bands ascending from lower y-axis ranges to higher.
  struct ColorBand
  {
    int _positionEnd; 
    Color4 color;
  };

public:
  Screen(Vector2i windowSize, Vector2i screenSize);
  ~Screen() = default;

  Screen(const Screen&) = default;
  Screen(Screen&&) = default;
  Screen& operator=(const Screen&) = default;
  Screen& operator=(Screen&&) = default;

  // slow clear but allows any color.
  void clearColor(const Color4& color);

  // fast clear but only allows grays or black (as all color channels equal). Value is
  // clamped to range [0, 255] inclusive. Clearing to a value==255 (i.e. white) is equivilent 
  // to calling 'clearTransparent'.
  void clearShade(int value);

  // clears all pixels to full transparency.
  void clearTransparent();

  void drawPixel(int x, int y, const Color4& color);
  void drawSprite(int x, int y, const Sprite& sprite);
  //void drawBitmap(int x, int y, const Bitmap& bitmap);
  //void drawBitmap(int x, int y, const Bitmap& bitmap);
  //void drawText(int x, int y, std::string text, const Font& font);

  // must be called when the window size changes so the screen can update its
  // position and pixel size.
  void onWindowResize(Vector2i windowSize);

  // render the virual screen on the real screen.
  void render();

  void useManualPositioning(Vector2i screenPosition);
  void useCenterPositioning();
  void useTopLeftPositioning();
  void useTopRightPositioning();
  void useBottomLeftPositioning();
  void useBottomRightPositioning();

  // these functions do not change any current pixels on the screen. Once a change in the
  // color mode is made only newly drawn pixels will be effected.
  void useBandedColors(std::vector<ColorBands> bands);
  void useFullColors();

  void useManualSize(int pixelSize);
  void useAutoSize();

private:
  enum class PositionMode
  {
    MANUAL,              // manually set screen position w.r.t the window.
    CENTER,              // center align the screen in the window.
    TOP_LEFT,            // clamp screen to the top-left of the window.
    TOP_RIGHT,           // clamp screen to the top-right of the window.
    BOTTOM_LEFT,         // clamp screen to the bottom-left of the window.
    BOTTOM_RIGHT         // clamp screen to the bottom-right of the window.
  };

  enum class SizeMode
  {
    MANUAL,              // manually assign pixel size.
    AUTO_MAX             // maximise pixel size to fit window. 
  };

  enum class ColorMode
  {
    FULL_COLOR,          // Pixels can take any color.
    Y_BANDED             // The y-axis is split into color bands; pixel color dependent on position.
  };

  // 12 byte pixels designed to work with glInterleavedArrays format GL_C4UB_V2F (opengl 3.0).
  struct Pixel
  {
    Pixel() : _color{}, _x{0}, _y{0}{}

    Color4u _color;
    float _x;
    float _y;
  };

private:
  std::vector<Pixel> _pixels;     // flattened 2D array accessed (col + (row * width)).
  std::vector<ColorBand> _bands;  // ordered set of color bands.
  Vector2i _position;             // position of virtual screen w.r.t the window.
  Vector2i _screenSize;           // width,height of screen in pixels.
  Vector2i _windowSize;           // copy of current window size.
  PositionMode _pmode;
  ScaleMode _smode;
  ColorMode _cmode;
  int _pixelSize;
};

namespace subsys
{
  extern std::unique_ptr<Renderer> renderer;
}

} // namespace pxr

#endif
