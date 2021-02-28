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
// Internally sounds are reference counted and so can be loaded multiple times without 
// duplication, each time returning the same key. To actually remove sounds from memory it is
// necessary to unload a sound an equal number of times to which it was loaded.
// 
// If a sounds cannot be loaded an error will be logged to the log and the resource key of
// the error sound will be returned.
//
// Returned resource keys are always positive.
//
ResourceKey_t loadSound(ResourceName_t soundName);

//
// Unload a sound, freeing the memory used by the sound data. The sound will only be removed
// from memory if the reference count drops to zero.
//
void unloadSound(ResourceKey_t soundKey);

//
// Plays a sound.
//
void playSound(ResourceKey_t soundKey, bool loop = false);

//
// Stop a sound playing. 
//
// note: there is no way to distinguish between 'instances' of a playing sound; this function 
// will stop playing all instances of the sound with this sound key.
//
void stopSound(ResourceKey_t soundKey);

} // namespace sfx
} // namespace pxr

#endif
