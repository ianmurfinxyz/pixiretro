#ifndef _PIXIRETRO_SFX_H_
#define _PIXIRETRO_SFX_H_

namespace pxr
{
namespace sfx
{

//
// The relative paths to resource files on disk; save your sound assets to these directories.
//
constexpr const char* RESOURCE_PATH_SOUNDS = "assets/sounds/";

//
// The type of unique keys mapped to sound resources. 
//
using ResourceKey_t = int;

//
// The type of sound resource names.
//
using ResourceName_t = const char*;

//
// Must call before any other function in this module.
//
bool initialize(int deviceid = -1);

//
// Call at program exit.
//
void shutdown();

//
// Load a sound.
//
// Returns the resouce key the sound is mapped to which is needed to later play the sound.
//
ResourceKey_t loadSound(ResourceName_t soundName);

//
// Unload a sound. Frees the memory used by the sound data and frees the resource key.
//
bool unloadSound(ResourceKey_t soundKey);

//
// Plays a sound.
//
void playSound(ResourceKey_t soundKey);

} // namespace sfx
} // namespace pxr

#endif
