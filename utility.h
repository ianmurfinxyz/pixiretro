#ifndef _PIXIRETRO_UTILITY_H_
#define _PIXIRETRO_UTILITY_H_

namespace pxr
{

template<typename T>
inline T wrap(T value, T lo, T hi)
{
  return (value < lo) ? hi : (value > hi) ? lo : value;
}

} // namespace pxr

#endif
