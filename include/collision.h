#ifndef _PIXIRETRO_COLLISION_H_
#define _PIXIRETRO_COLLISION_H_

namespace pxr
{

// COLLISION RESULTS
//
// The following is an explanation of the data returned from a collision test between two bitmaps,
// say bitmap A and bitmap B.
//
// To test for pixel perfect collisions between two bitmaps, each bitmap is considered to be
// bounded by an axis-aligned bounding box (AABB) calculated from the positions of the bitmaps
// and their dimensions. Two overlap AABBs are calculated from any intersection, one for each
// bitmap. The overlaps represent the local overlap (i.e. coordinates w.r.t the bitmap coordinates,
// see class Bitmap) of each bitmap, illustrated in figure 1.
//
//               Wa=20                     KEY
//           +----------+                  ===
//           |          |                  Pn = position of bitmap N
//     Ha=20 |          |                  Wn = width of bitmap N
//           |     +----|-----+            Hn = height of bitmap N
//           | A   | S  |     |
// Pa(20,20) o-----|----+     | Hb=20      S = overlap region of bitmaps A and B.
//                 |          |
//                 | B        |            There is only a single overlap region S for any collision.
//       Pb(30,10) o----------+            However this region can be expressed w.r.t the coordinate
//                     Wb=20               space of each bitmap.
//    
//     y                                   Both expressions will be returned. In this example we
//     ^                                   will have the result:
//     |  screen                                          left, right, top, bottom
//     |   axes                               aOverlap = {10  , 20   , 10 , 0 }
//     o-----> x                              bOverlap = {0   , 10   , 20 , 10}
//
//                                        Note that the overlap S w.r.t the screen would be:
//                                             Overlap = {30  , 40   , 30 , 20}
//                            [ figure 1]
//
//  Further, lists of all pixel intersections can also be returned. Intersecting pixels are returned
//  as two lists: the set of all pixels in bitmap A which intersect a pixel in bitmap B (aPixels)
//  and the set of all pixels in bitmap B which intersect a pixel in bitmap A (bPixels). The
//  pixels in the lists are expressed in coordinates w.r.t their bitmaps coordinate space. Note 
//  pixels are returned as 2D vectors where [x,y] = [col][row]. 
//
//  USAGE NOTES
//
//  The collision data returned from the collision test function is stored internally and returned
//  via constant reference to avoid allocating memory for each and every test. Consequently the
//  data returned persists only inbetween calls to the test function and is overwritten by 
//  subsequent calls. If you need persistence then copy construct the returned Collision struct.
//
//  Not every usage requires all collision data thus some collision data is optional where skipping
//  the collection of such data can provide performance benefits, notably the pixel lists. If a 
//  pixel list is not required the test resolution can shortcut with a positive result upon 
//  detecting the first pixel intersection. By default lists are generated.

// AABB:
//              +-------x (xmax, ymax)       y
//              |       |                    ^  screen
//              | AABB  |                    |   axes
//              |       |                    |   
// (xmin, ymin) o-------+                    o------> x
struct AABB
{
  int32_t _xmin;
  int32_t _ymin;
  int32_t _xmax;
  int32_t _ymax;
};

struct Collision
{
  bool _isCollision;
  AABB _aBounds;
  AABB _bBounds;
  AABB _aOverlap;
  AABB _bOverlap;
  std::vector<Vector2i> _aPixels;
  std::vector<Vector2i> _bPixels;
};

const Collision& testCollision(Vector2i aPosition, const Bitmap& aBitmap, 
                               Vector2i bPosition, const Bitmap& bBitmap, bool pixelLists = true);

} // namespace pxr

#endif
