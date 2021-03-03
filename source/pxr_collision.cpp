#include <cassert>
#include "pxr_collision.h"
#include "pxr_bmp.h"

namespace pxr
{

//
// The collision result instance used to return collision data. This struct is reused in
// every call to avoid repeated memory allocations.
//
static CollisionResult cr;

static void clearResults()
{
  cr._isCollision = false;
  cr._aOverlap = {0, 0, 0, 0};
  cr._bOverlap = {0, 0, 0, 0};
  cr._aPixels.clear();
  cr._bPixels.clear();
}

static void calculateAABBOverlap(const AABB& aBounds, AABB& aOverlap, 
                                 const AABB& bBounds, AABB& bOverlap)
{
  //
  // Overlap w.r.t the space common to both.
  //
  AABB overlap;
  overlap._xmax = std::min(aBounds._xmax, bBounds._xmax);
  overlap._xmin = std::max(aBounds._xmin, bBounds._xmin);
  overlap._ymax = std::min(aBounds._ymax, bBounds._ymax);
  overlap._ymin = std::max(aBounds._ymin, bBounds._ymin);

  //
  // Overlaps w.r.t each local sprite coordinate space.
  //
  aOverlap._xmax = overlap._xmax - aBounds._xmin; 
  aOverlap._xmin = overlap._xmin - aBounds._xmin;
  aOverlap._ymax = overlap._ymax - aBounds._ymin; 
  aOverlap._ymin = overlap._ymin - aBounds._ymin;

  bOverlap._xmax = overlap._xmax - bBounds._xmin; 
  bOverlap._xmin = overlap._xmin - bBounds._xmin;
  bOverlap._ymax = overlap._ymax - bBounds._ymin; 
  bOverlap._ymin = overlap._ymin - bBounds._ymin;

  //
  // The results should be the same overlap region w.r.t two different coordinate spaces.
  //
  assert((aOverlap._xmax - aOverlap._xmin) == (bOverlap._xmax - bOverlap._xmin));
  assert((aOverlap._ymax - aOverlap._ymin) == (bOverlap._ymax - bOverlap._ymin));
}

static void findPixelIntersections(const AABB& aOverlap, 
                                   const gfx::Spritesheet& aSheet,
                                   const gfx::Sprite& aSprite,
                                   const AABB& bOverlap, 
                                   const gfx::Spritesheet& bSheet,
                                   const gfx::Sprite& bSprite,
                                   bool pixelLists)
{

  //
  // aSprite is the subregion of aSheet.
  //
  // aOverlap is the subregion of aSprite overlapping b.
  //
  // aSheetOverlap is the subregion of aSheet overlapping b.
  //
  static AABB aSheetOverlap, bSheetOverlap;

  aSheetOverlap._xmin = aSprite._position._x + aOverlap._xmin;
  aSheetOverlap._xmax = aSprite._position._x + aOverlap._xmax;
  aSheetOverlap._ymin = aSprite._position._y + aOverlap._ymin;
  aSheetOverlap._ymax = aSprite._position._y + aOverlap._ymax;

  bSheetOverlap._xmin = bSprite._position._x + bOverlap._xmin;
  bSheetOverlap._xmax = bSprite._position._x + bOverlap._xmax;
  bSheetOverlap._ymin = bSprite._position._y + bOverlap._ymin;
  bSheetOverlap._ymax = bSprite._position._y + bOverlap._ymax;

  assert(0 <= aSheetOverlap._xmin && aSheetOverlap._xmin < aSheet._image.getWidth());
  assert(0 <= bSheetOverlap._xmin && bSheetOverlap._xmin < bSheet._image.getWidth());
  assert(0 <= aSheetOverlap._xmax && aSheetOverlap._xmax < aSheet._image.getWidth());
  assert(0 <= bSheetOverlap._xmax && bSheetOverlap._xmax < bSheet._image.getWidth());

  assert(0 <= aSheetOverlap._ymin && aSheetOverlap._ymin < aSheet._image.getHeight());
  assert(0 <= bSheetOverlap._ymin && bSheetOverlap._ymin < bSheet._image.getHeight());
  assert(0 <= aSheetOverlap._ymax && aSheetOverlap._ymax < aSheet._image.getHeight());
  assert(0 <= bSheetOverlap._ymax && bSheetOverlap._ymax < bSheet._image.getHeight());

  const gfx::Color4u* const* aPixels = aSheet._image.getPixels();
  const gfx::Color4u* const* bPixels = bSheet._image.getPixels();

  int overlapWidth = aSheetOverlap._xmax - aSheetOverlap._xmin;
  int overlapHeight = aSheetOverlap._ymax - aSheetOverlap._ymin;

  int aPxRow, bPxRow, aPxCol, bPxCol;

  for(int row = 0; row < overlapHeight; ++row){
    for(int col = 0; col < overlapWidth; ++col){
      aPxRow = aSheetOverlap._ymin + row;
      aPxCol = aSheetOverlap._xmin + col;

      bPxRow = bSheetOverlap._ymin + row;
      bPxCol = bSheetOverlap._xmin + col;

      if(aPixels[aPxRow][aPxCol]._a == 0 || bPixels[bPxRow][bPxCol]._a == 0)
        continue;

      cr._aPixels.push_back({aPxCol, aPxRow});
      cr._bPixels.push_back({bPxCol, bPxRow});

      if(!pixelLists)
        break;
    }
  }
}

bool isAABBIntersection(const AABB& a, const AABB& b)
{
  return ((a._xmin <= b._xmax) && (a._xmax >= b._xmin)) 
         && 
         ((a._ymin <= b._ymax) && (a._ymax >= b._ymin));
}

const CollisionResult& isPixelIntersection(const CollisionSubject& a,
                                           const CollisionSubject& b,
                                           bool pixelLists)
{

  const gfx::Spritesheet& aSheet = gfx::getSpritesheet(a._spritesheetKey);
  const gfx::Spritesheet& bSheet = gfx::getSpritesheet(b._spritesheetKey);

  //
  // bottom-left most pixel position of the sprites w.r.t the common space.
  //
  Vector2f aBLPosition, bBLPosition;

  assert(0 <= a._spriteid && a._spriteid < aSheet._sprites.size());
  assert(0 <= b._spriteid && b._spriteid < bSheet._sprites.size());
  
  const gfx::Sprite& aSprite = aSheet._sprites[a._spriteid];
  const gfx::Sprite& bSprite = bSheet._sprites[b._spriteid];

  clearResults();

  //
  // calculate bounds w.r.t the common space.
  //

  aBLPosition = a._position - aSprite._origin;
  bBLPosition = b._position - bSprite._origin;
  
  cr._aBounds = {
    aBLPosition._x,
    aBLPosition._y,
    aBLPosition._x + (aSprite._size._x - 1), 
    aBLPosition._y + (aSprite._size._y - 1)
  };

  cr._bBounds = {
    bBLPosition._x,
    bBLPosition._y,
    bBLPosition._x + (bSprite._size._x - 1), 
    bBLPosition._y + (bSprite._size._y - 1)
  };

  if(!isAABBIntersection(cr._aBounds, cr._bBounds))
    return cr;

  //
  // calculate the local region of each sprite overlapping with the other sprite.
  //

  calculateAABBOverlap(cr._aBounds, cr._aOverlap, cr._bBounds, cr._bOverlap);

  findPixelIntersections(cr._aOverlap, aSheet, aSprite, 
                         cr._bOverlap, bSheet, bSprite, pixelLists);

  assert(cr._aPixels.size() == cr._bPixels.size());

  if(cr._aPixels.size() > 0)
    cr._isCollision = true;

  return cr;
}

} // namespace pxr
