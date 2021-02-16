#include "collision.h"

namespace pxr
{

static bool isAABBIntersection(const AABB& a, const AABB& b)
{
  return ((a._xmin <= b._xmax) && (a._xmax >= b._xmin)) && ((a._ymin <= b._ymax) && (a._ymax >= b._ymin));
}

static void calculateAABBOverlap(const AABB& aBounds, const AABB& bBounds, 
                                 AABB& aOverlap, AABB& bOverlap)
{
  // Overlap w.r.t screen space which is common to both.
  AABB overlap;
  overlap._xmax = std::min(aBounds._xmax, bBounds._xmax);
  overlap._xmin = std::max(aBounds._xmin, bBounds._xmin);
  overlap._ymax = std::min(aBounds._ymax, bBounds._ymax);
  overlap._ymin = std::max(aBounds._ymin, bBounds._ymin);

  // Overlaps w.r.t each local bitmap coordinate space.
  aOverlap._xmax = overlap._xmax - aBounds._xmin; 
  aOverlap._xmin = overlap._xmin - aBounds._xmin;
  aOverlap._ymax = overlap._ymax - aBounds._ymin; 
  aOverlap._ymin = overlap._ymin - aBounds._ymin;

  bOverlap._xmax = overlap._xmax - bBounds._xmin; 
  bOverlap._xmin = overlap._xmin - bBounds._xmin;
  bOverlap._ymax = overlap._ymax - bBounds._ymin; 
  bOverlap._ymin = overlap._ymin - bBounds._ymin;

  // The results should be the same overlap region w.r.t two different coordinate spaces.
  assert((aOverlap._xmax - aOverlap._xmin) == (bOverlap._xmax - bOverlap._xmin));
  assert((aOverlap._ymax - aOverlap._ymin) == (bOverlap._ymax - bOverlap._ymin));
}

static void findPixelIntersectionSets(const AABB& aOverlap, const Bitmap& aBitmap, 
                                      const AABB& bOverlap, const Bitmap& bBitmap,
                                      std::vector<Vector2i>& aPixels, std::vector<Vector2i>& bPixels,
                                      bool pixelLists)
{
  int32_t overlapWidth = aOverlap._xmax - aOverlap._xmin;
  int32_t overlapHeight = aOverlap._ymax - aOverlap._ymin;

  int32_t aBitRow, bBitRow, aBitCol, bBitCol, aBitValue, bBitValue;

  for(int32_t row = 0; row < overlapHeight; ++row){
    for(int32_t col = 0; col < overlapWidth; ++col){
      aBitRow = aOverlap._ymin + row;
      aBitCol = aOverlap._xmin + col;

      bBitRow = bOverlap._ymin + row;
      bBitCol = bOverlap._xmin + col;

      aBitValue = aBitmap.getBit(aBitRow, aBitCol);
      bBitValue = bBitmap.getBit(bBitRow, bBitCol);

      if(aBitValue == 0 || bBitValue == 0)
        continue;

      aPixels.push_back({aBitCol, aBitRow});
      bPixels.push_back({bBitCol, bBitRow});

      if(!pixelLists)
        return;
    }
  }
}

const Collision& testCollision(Vector2i aPosition, const Bitmap& aBitmap, 
                               Vector2i bPosition, const Bitmap& bBitmap, bool pixelLists)
{
  static Collision c;

  c._isCollision = false;
  c._aOverlap = {0, 0, 0, 0};
  c._bOverlap = {0, 0, 0, 0};
  c._aPixels.clear();
  c._bPixels.clear();

  c._aBounds = {aPosition._x, aPosition._y, 
                aPosition._x + aBitmap.getWidth(), aPosition._y + aBitmap.getHeight()};
  c._bBounds = {bPosition._x, bPosition._y, 
                bPosition._x + bBitmap.getWidth(), bPosition._y + bBitmap.getHeight()};

  if(!isAABBIntersection(c._aBounds, c._bBounds))
    return c;

  calculateAABBOverlap(c._aBounds, c._bBounds, c._aOverlap, c._bOverlap);

  findPixelIntersectionSets(c._aOverlap, aBitmap, 
                            c._bOverlap, bBitmap, 
                            c._aPixels, c._bPixels, pixelLists);

  assert(c._aPixels.size() == c._bPixels.size());

  if(c._aPixels.size() != 0)
    c._isCollision = true;  

  return c;
}

} // namespace pxr
