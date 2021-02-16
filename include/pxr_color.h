#ifndef _PIXIRETRO_GFX_COLOR_H_
#define _PIXIRETRO_GFX_COLOR_H_

#include <algorithm>
#include <cinttypes>

namespace pxr
{
namespace gfx
{

//
// A 4-channel color (RGBA) where each channel is an unsigned 8-bit integer.
//
struct Color4u
{
  constexpr Color4u() : _r{0}, _g{0}, _b{0}, _a{0}{}

  constexpr Color4u(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) :
    _r{r},
    _g{g},
    _b{b},
    _a{a}
  {}

  uint8_t _r;           // !!! DO NOT ADD MEMBER DATA TO THIS CLASS !!!
  uint8_t _g;           //
  uint8_t _b;           // sizeof(Color4u) == 4 bytes naturally aligned; gfx module
  uint8_t _a;           // relies on this being true.
};

//
// A 4-channel color (RGBA) where each channel is a 32-bit float with value within the
// range [0,1].
//
struct Color4f
{
  constexpr Color4f() : _r{0}, _g{0}, _b{0}, _a{0}{}

  constexpr Color4f(float r, float g, float b, float a = 0) :
    _r{r},
    _g{g},
    _b{b},
    _a{a}
  {}

  constexpr Color4f(const Color4u& c) :
    _r{std::clamp(c._r / 255.f, 0.f, 1.f)},
    _g{std::clamp(c._g / 255.f, 0.f, 1.f)},
    _b{std::clamp(c._b / 255.f, 0.f, 1.f)},
    _a{std::clamp(c._a / 255.f, 0.f, 1.f)}
  {}

  const Color4f& operator=(const Color4u& c)
  {
    _r = std::clamp(c._r / 255.f, 0.f, 1.f);
    _g = std::clamp(c._g / 255.f, 0.f, 1.f);
    _b = std::clamp(c._b / 255.f, 0.f, 1.f);
    _a = std::clamp(c._a / 255.f, 0.f, 1.f);
    return *this;
  }

  float _r;
  float _g;
  float _b;
  float _a;
};

//
// Some useful predefined colors.
//
namespace colors
{
  //
  // Pure shades.
  //
  constexpr Color4u white   {255, 255, 255, 255};
  constexpr Color4u black   {0  , 0  , 0  , 255};

  //
  // sRGB primaries.
  //
  constexpr Color4u red     {255, 0  , 0  , 255};
  constexpr Color4u green   {0  , 255, 0  , 255};
  constexpr Color4u blue    {0  , 0  , 255, 255};

  //
  // sRGB secondaries.
  //
  constexpr Color4u cyan    {0  , 255, 255, 255};
  constexpr Color4u magenta {255, 0  , 255, 255};
  constexpr Color4u yellow  {255, 255, 0  , 255};
  
  //
  // greys - more grays: https://en.wikipedia.org/wiki/Shades_of_gray 
  //
  constexpr Color4u gainsboro   {224, 224, 224, 255};
  constexpr Color4u lightgray   {215, 215, 215, 255};
  constexpr Color4u silver      {196, 196, 196, 255};
  constexpr Color4u spanishgray {155, 155, 155, 255};
  constexpr Color4u gray        {131, 131, 131, 255};
  constexpr Color4u dimgray     {107, 107, 107, 255};
  constexpr Color4u davysgray   {87 , 87 , 87 , 255};
  constexpr Color4u jet         {53 , 53 , 53 , 255};

  //
  // pinks - more pinks: https://en.wikipedia.org/wiki/Shades_of_pink
  //
  constexpr Color4u pink        {255, 192, 203, 255};
  constexpr Color4u lightpink   {255, 182, 193, 255};
  constexpr Color4u hotpink     {255, 105, 180, 255};
  constexpr Color4u deeppink    {255, 20 , 147, 255};
  constexpr Color4u barbiepink  {218, 24 , 132, 255};  // couldn't resist! :)
};

} // namespace gfx
} // namespace pxr

#endif
