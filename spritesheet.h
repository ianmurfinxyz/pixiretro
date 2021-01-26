#ifndef _PIXIRETRO_SPRITESHEET_H_
#define _PIXIRETRO_SPRITESHEET_H_

#include "bmpimage.h"
#include "math.h"

namespace pxr
{
namespace gfx
{

struct Sprite
{
  int _rowmin;
  int _colmin;
  int _rowmax;
  int _colmax;
  BmpImage* _image;
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
//
struct SpriteSheet
{
  // These value are coordinated with bmpimage::MAX_WIDTH/HEIGHT and are used to aid validation
  // of sprite sheet xml files and prevent crazy numbers in the xml file causing the loader to
  // attempt excessive or negative allocations or other stuff.
  static constexpr int MIN_SPRITE_WIDTH {8};
  static constexpr int MIN_SPRITE_HEIGHT {8};
  static constexpr int MAX_SHEET_WIDTH {bmpimage::MAX_WIDTH / MIN_SPRITE_WIDTH};
  static constexpr int MAX_SHEET_HEIGHT {bmpimage::MAX_HEIGHT / MIN_SPRITE_HEIGHT};
  static constexpr int MAX_SPRITE_COUNT {MAX_SHEET_WIDTH * MAX_SHEET_HEIGHT};

  SpriteSheet() = default;
  SpriteSheet(const SpriteSheet&) = default;
  SpriteSheet(SpriteSheet&&) = default;
  SpriteSheet& operator=(const SpriteSheet&) default;
  SpriteSheet& operator=(SpriteSheet&&) default;

  Sprite getSprite() const;

  BmpImage _image;
  Vector2i _spriteSize;    // x=width pixels, y=height pixels.
  Vector2i _sheetSize;     // x=width sprites, y=height sprites.
  int _spriteCount;
};


} // namespace gfx
} // namespace pxr

#endif
