#ifndef _PIXIRETRO_SFX_H_
#define _PIXIRETRO_SFX_H_

namespace pxr
{
namespace sfx
{

constexpr const char* RESOURCE_PATH_SOUNDS = "assets/sounds/";

using ResourceKey_t = int;
using ResourceName_t = const char*;

//
//
//
bool initialize(int deviceid = -1);

//
//
//
void shutdown();

//
//
//
ResourceKey_t loadSound(ResourceName_t soundName);

//
//
//
bool unloadSound(ResourceKey_t soundKey);

//
//
//
void playSound(ResourceKey_t soundKey);

} // namespace sfx
} // namespace pxr

#endif
