#ifndef _PIXIRETRO_SFX_H_
#define _PIXIRETRO_SFX_H_

#include <SDL_audio.h>
#include <limits>

namespace pxr
{
namespace sfx
{

//
// The relative paths to resource files on disk; save your sound assets to these directories.
//
constexpr const char* RESOURCE_PATH_SOUNDS = "assets/sounds/";
constexpr const char* RESOURCE_PATH_MUSIC = "assets/music/";

//
// The type of unique keys mapped to sound resources. 
//
using ResourceKey_t = int32_t;

//
// The type of sound resource names.
//
using ResourceName_t = const char*;

//
// The type of unique keys which identify sound channels.
//
using SoundChannel_t = int;

//
// Returned upon failure to load a music file. This key is safe to pass to music functions
// and will simply cause those functions to be no-ops. It will not be returned upon failure 
// to load sound effects, instead an error key is returned in that case.
//
static constexpr ResourceKey_t nullResourceKey {-1};

//
// The key returned upon failure to load sound resources. Will link to a generated sine tone
// which will play in place of any unloaded sounds.
//
extern ResourceKey_t errorSoundKey;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GENERAL       
//////////////////////////////////////////////////////////////////////////////////////////////////

enum OutputMode 
{ 
  MONO   = 1, 
  STEREO = 2 
};

//
// See SDL_audio.h for more details of audio sample formats.
//
// note: This module only supports little-endian integer output formats (designed for x86).
// 
// note: U16 == U16LSB
//       U32 == U32LSB
//       S32 == S32LSB
// i.e. they are equivilent.
//
enum SampleFormat : uint16_t
{
  SAMPLE_FORMAT_U8     = AUDIO_U8,     // Unsigned 8-bit samples.
  SAMPLE_FORMAT_S8     = AUDIO_S8,     // Signed 8-bit samples.
  SAMPLE_FORMAT_U16LSB = AUDIO_U16LSB, // Unsigned 16-bit samples little-endian.
  SAMPLE_FORMAT_S16LSB = AUDIO_S16LSB, // Signed 16-bit samples little-endian.
  SAMPLE_FORMAT_U16    = AUDIO_U16,    // Unsigned 16-bit samples little endian.
  SAMPLE_FORMAT_S16    = AUDIO_S16,    // Signed 16-bit samples little endian.
  SAMPLE_FORMAT_S32LSB = AUDIO_S32LSB, // Signed 32-bit samples little endian.
  SAMPLE_FORMAT_S32    = AUDIO_S32,    // Signed 32-bit samples little endian.
};

static constexpr int DEFAULT_SAMPLING_FREQ_HZ {22050               };
static constexpr int DEFAULT_SAMPLE_FORMAT    {SAMPLE_FORMAT_S16LSB};
static constexpr int DEFAULT_CHUNK_SIZE       {4096                };
static constexpr int DEFAULT_NUM_MIX_CHANNELS {16                  };

struct SFXConfiguration
{
  int      _samplingFreq_hz {DEFAULT_SAMPLING_FREQ_HZ};
  uint16_t _sampleFormat    {DEFAULT_SAMPLE_FORMAT   };
  int      _outputMode      {OutputMode::MONO        };
  int      _chunkSize       {DEFAULT_CHUNK_SIZE      };
  int      _numMixChannels  {DEFAULT_NUM_MIX_CHANNELS};
};

//
// Must call before any other function in this module.
//
bool initialize(SFXConfiguration sfxconf = SFXConfiguration{});

//
// Call at program exit.
//
void shutdown();

//
// Called by the engine every tick to service the module, e.g. to unload any sounds in the
// unload queue.
//
void onUpdate(float dt);

//////////////////////////////////////////////////////////////////////////////////////////////////
// SOUND EFFECTS
//////////////////////////////////////////////////////////////////////////////////////////////////

//
// Helper definition for use with play functions (both sounds and music).
//
static constexpr int INFINITE_LOOPS {-1};
static constexpr int NO_LOOPS {0};

//
// Helper definition for use with functions which pause/stop/resume sound channel playback.
//
static constexpr int ALL_CHANNELS {-1};

//
// Returned from play functions on errors. It is safe to pass this value to any function which
// modifies a channel, e.g. stopChannel, pauseChannel etc.
//
static constexpr int NULL_CHANNEL {-2};

//
// Helper definition for use with volume functions (both sounds and music).
//
static constexpr int MIN_VOLUME {0};
static constexpr int MAX_VOLUME {128};

//
// Can be used with play functions to play the error sound.
//
extern ResourceKey_t errorSoundKey;

//
// Load a WAV sound.
//
// Returns the resouce key the sound is mapped to which is needed to later play the sound.
//
// Internally sounds are reference counted and so can be loaded multiple times without 
// duplication, each time returning the same key. To actually remove sounds from memory
// it is necessary to unload a sound an equal number of times to which it was loaded.
// 
// If a sound cannot be loaded an error will be logged to the log and the resource key of
// the error sound will be returned.
//
// Returned resource keys are always positive.
//
ResourceKey_t loadSoundWAV(ResourceName_t soundName);

//
// Adds a sound to the queue of sounds waiting to be unloaded. Sounds in the queue are unloaded
// once all channels have stopped using it. A call to this function will only actually queue a 
// sound if its reference count drops to 0. Thus it is necessary to unload a sound an equal
// number of times to which it was loaded.
//
void queueUnloadSound(ResourceKey_t soundKey);

//
// Play a sound. These functions return the channel the sound is playing on which can be used to
// manipulate the playback.
//
SoundChannel_t playSound(ResourceKey_t soundKey, int loops = NO_LOOPS);
SoundChannel_t playSoundTimed(ResourceKey_t soundKey, int loops, int playDuration_ms);
SoundChannel_t playSoundFadeIn(ResourceKey_t soundKey, int loops, int fadeDuration_ms);
SoundChannel_t playSoundFadeInTimed(ResourceKey_t soundKey, int loops, int fadeDuration_ms, int playDuration_ms);

//
// Pause/resume/stop sound channels.
//
void stopChannel(SoundChannel_t channel);
void stopChannelTimed(SoundChannel_t channel, int durationUntilStop_ms);
void stopChannelFadeOut(SoundChannel_t channel, int fadeDuration_ms);
void pauseChannel(SoundChannel_t channel);
void resumeChannel(SoundChannel_t channel);

//
// Passing in ALL_CHANNELS or NULL_CHANNEL will always return false. 
//
bool isChannelPlaying(SoundChannel_t channel);
bool isChannelPaused(SoundChannel_t channel);

//
// Passing ALL_CHANNELS will set the volume for all channels as would be expected. Volumes
// outside the range of valid volumes will be clamped.
//
void setChannelVolume(SoundChannel_t channel, int volume);

//
// Passing in ALL_CHANNELS will return the average volume. Passing in NULL_CHANNEL will always
// return 0.
//
int getChannelVolume(SoundChannel_t channel);

int getMusicVolume();


//////////////////////////////////////////////////////////////////////////////////////////////////
// MUSIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr float PLAY_MUSIC_FOREVER {std::numeric_limits<float>::max()};

struct MusicSequenceNode
{
  ResourceKey_t _musicKey;
  int _fadeInDuration_ms;
  int _playDuration_ms;
  int _fadeOutDuration_ms;
};

using MusicSequence_t = std::vector<MusicSequenceNode>;

ResourceKey_t loadMusicWAV(ResourceName_t musicName);
void queueUnloadMusic(ResourceKey_t musicKey);
void playMusic(MusicSequence_t sequence, bool loop = true);
void stopMusic();
void pauseMusic();
void resumeMusic();
bool isMusicPlaying();
bool isMusicPaused();
bool isMusicFadingIn();
bool isMusicFadingOut();
void setMusicVolume(int volume);

} // namespace sfx
} // namespace pxr

#endif
