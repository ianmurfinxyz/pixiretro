#ifndef _PIXIRETRO_COLOR_H_
#define _PIXIRETRO_COLOR_H_

#include <algorithm>
#include <cinttypes>

namespace pxr
{

// 32-bit RGBA color.
class Color4u
{
private:
  constexpr static float f_lo {0.f};
  constexpr static float f_hi {1.f};

public:
  constexpr Color4u() : _r{0}, _g{0}, _b{0}, _a{0}{}

  constexpr Color4u(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) :
    _r{r},
    _g{g},
    _b{b},
    _a{a}
  {}

  Color4u(const Color4u&) = default;
  Color4u(Color4u&&) = default;
  Color4u& operator=(const Color4u&) = default;
  Color4u& operator=(Color4u&&) = default;

  void setRed(uint8_t r){_r = r;}
  void setGreen(uint8_t g){_g = g;}
  void setBlue(uint8_t b){_b = b;}
  void setAlpha(uint8_t a){_a = a;}

  uint8_t getRed() const {return _r;}
  uint8_t getGreen() const {return _g;}
  uint8_t getBlue() const {return _b;}
  uint8_t getAlpha() const {return _a;}

  // clamp to cut-off float math errors.
  float getfRed() const {return std::clamp(_r / 255.f, f_lo, f_hi);}    
  float getfGreen() const {return std::clamp(_g / 255.f, f_lo, f_hi);}
  float getfBlue() const {return std::clamp(_b / 255.f, f_lo, f_hi);}
  float getfAlpha() const {return std::clamp(_a / 255.f, f_lo, f_hi);}

private:
  uint8_t _r;
  uint8_t _g;
  uint8_t _b;
  uint8_t _a;
};

namespace colors
{
  constexpr Color4u white {255, 255, 255, 255};
  constexpr Color4u black {0, 0, 0, 255};
  constexpr Color4u red {255, 0, 0, 255};
  constexpr Color4u green {0, 255, 0, 255};
  constexpr Color4u blue {0, 0, 255, 255};
  constexpr Color4u cyan {0, 255, 255, 255};
  constexpr Color4u magenta {255, 0, 255, 255};
  constexpr Color4u yellow {255, 255, 0, 255};
  
  // greys - more grays: https://en.wikipedia.org/wiki/Shades_of_gray 
  
  constexpr Color4u gainsboro {224, 224, 224, 255};
  constexpr Color4u lightgray {215, 215, 215, 255};
  constexpr Color4u silver {196, 196, 196, 255};
  constexpr Color4u spanishgray {155, 155, 155, 255};
  constexpr Color4u gray {131, 131, 131, 255};
  constexpr Color4u dimgray {107, 107, 107, 255};
  constexpr Color4u davysgray {87, 87, 87, 255};
  constexpr Color4u jet {53, 53, 53, 255};
};

} // namespace pxr

#endif
