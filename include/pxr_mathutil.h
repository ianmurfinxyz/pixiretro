#ifndef _PIXIRETRO_MATH_UTIL_H_
#define _PIXIRETRO_MATH_UTIL_H_

namespace pxr
{

template<typename T>
inline T lerp(T v0, T v1, T t){
  return (1 - t) * v0 + t * v1;
}

template<typename T>
inline T wrap(T value, T lo, T hi)
{
  return (value < lo) ? hi : (value > hi) ? lo : value;
}

} // namespace pxr

#endif
