#ifndef _PIXIRETRO_COLLISION_H_
#define _PIXIRETRO_COLLISION_H_

#include "pxr_gfx.h"
#include "pxr_vec.h"

namespace pxr
{

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// PIXIRETRO COLLISION DETECTION
//
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// This module handles both AABB collision tests and pixel perfect collision tests between two
// sprites. Pixels within two sprites are considered colliding if they occupy the same position
// within a common screen space and their alpha components do not equal 0. Just as the gfx
// module considers all pixels with alpha = 0 as being fully transparent, this module considers
// all such pixels as occupying no space, and thus being incapable of collision.
//
// To test for pixel perfect collisions between two sprites, each sprite is considered to be
// bounded by an axis-aligned bounding box (AABB) calculated from the positions of the sprites,
// w.r.t the common coordinate space within which the sprites may be colliding, and their 
// dimensions. 
//
// Two overlap AABBs are calculated from any intersection; one for each sprite. The overlaps 
// represent the local overlap of each sprite (i.e. coordinates w.r.t the local space of their 
// respective sprite and not the common space) illustrated in figure 1.
//
//
//               Wa=20                     KEY
//           +----------+                  ===
//           |          |                  Pn = position of sprite N
//     Ha=20 |          |                  Wn = width of sprite N
//           |     +----|-----+            Hn = height of sprite N
//           | A   | S  |     |
// Pa(20,20) o-----|----+     | Hb=20      S = overlap region of sprites A and B.
//                 |          |
//                 | B        |            There is only a single overlap region S for any
//       Pb(30,10) o----------+            collision. However this region can be expressed 
//                     Wb=20               w.r.t the coordinate space of each sprite.
//    
//                                         Both expressions will be returned. In this example
//     y                                   we will have the result:
//     ^
//     |  screen                                          left, right, top, bottom
//     |   axes                               aOverlap = {10  , 20   , 10 , 0 }
//     o-----> x                              bOverlap = {0   , 10   , 20 , 10}
//
//                                         Note that the overlap S w.r.t the screen would be:
//                                             Overlap = {30  , 40   , 30 , 20}
//
// [figure 1 : Two sprites, sprite A and B, overlapping in a common coordinate space.]
//
// Further, lists of all pixel intersections can also be returned. Intersecting pixels are returned
// as two lists: the set of all pixels in sprite A which intersect a pixel in sprite B (aPixels)
// and the set of all pixels in sprite B which intersect a pixel in sprite A (bPixels). The
// pixels in the lists are expressed in coordinates w.r.t their sprites coordinate space. Note 
// pixels are returned as 2D vectors where [x,y] = [col][row]. 
//
// MODULE USAGE
//
// The collision data returned from the pixel intersection function is stored internally and 
// returned via constant reference to avoid allocating memory for each and every test. 
// Consequently the data returned persists only between calls to the test function; a subsequent
// call will overwrite the data of the prior call. If you need persistence then copy construct 
// the returned Collision struct.
//
// Since not every usage requires all collision data some collision data is made optional where 
// skipping the collection of such data can provide performance benefits, notably the pixel 
// lists. If a pixel list is not required the test resolution can shortcut with a positive 
// result upon detecting the first pixel intersection. By default lists are not generated.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

//
// An axis-aligned bounding box (AABB).
//
//              +-------x (xmax, ymax)       y
//              |       |                    ^  screen
//              | AABB  |                    |   axes
//              |       |                    |   
// (xmin, ymin) o-------+                    o------> x
//
struct AABB
{
  int32_t _xmin;
  int32_t _ymin;
  int32_t _xmax;
  int32_t _ymax;
};

//
// Collision results.
//
struct CollisionResult
{
  bool _isCollision;
  AABB _aBounds;
  AABB _bBounds;
  AABB _aOverlap;
  AABB _bOverlap;
  std::vector<Vector2i> _aPixels;       // x,y = col,row
  std::vector<Vector2i> _bPixels;
};

//
// A potentially colliding object.
//
//
struct CollisionSubject
{
  Vector2i _position;
  gfx::ResourceKey_t _spritesheetKey;
  gfx::SpriteId_t _spriteid;
};

//
// Basic AABB intersection test.
//
bool isAABBIntersection(const AABB& a, const AABB& b);

//
// Pixel perfect collision test.
//
// Note: providing invalid subject data (an invalid spritesheet key or spriteid) will terminate
// the program (via strippable assert).
//
const CollisionResult& isPixelIntersection(const CollisionSubject& a,
                                           const CollisionSubject& b,
                                           bool pixelLists = false);

} // namespace pxr

#endif
