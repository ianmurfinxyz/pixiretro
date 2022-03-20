#include <string>
#include <unordered_map>
#include <cmath>
#include <cassert>
#include <vector>
#include <algorithm>
#include <SDL_mixer.h>
#include "pxr_sfx.h"
#include "pxr_log.h"
#include "pxr_wav.h"

#include <iostream>

namespace pxr
{
namespace sfx
{

/////////////////////////////////////////////////////////////////////////////////////////////////
// MODULE DATA
/////////////////////////////////////////////////////////////////////////////////////////////////

struct SoundResource
{
  std::string _name = "";
  Mix_Chunk* _chunk = nullptr;
  int _referenceCount = 0;
};

struct MusicResource
{
  std::string _name = "";
  Mix_Music* _music = nullptr;
  int _referenceCount = 0;
};

class MusicSequencePlayer
{
public:
  enum State { STOPPED, PAUSED, PLAYING, FADING_OUT };
  MusicSequencePlayer();
  void onUpdate(float dt);
  void play(MusicSequence_t sequence, bool loop);
  void stop();
  void pause();
  void resume();
  State getState() const {return _state;} 
  bool isUsingMusicResource(ResourceKey_t musicKey);
private:
  void playNode(const MusicSequenceNode* node);
  void stopNode(const MusicSequenceNode* node);
  void onPlayingUpdate(float dt);
  void onFadingOutUpdate(float dt);
private:
  State _state;
  MusicSequence_t _sequence;
  int _currentNode;
  float _musicClock_s;
  bool _isLooping;
};

static MusicSequencePlayer musicSequencePlayer;

//
// Nyquist-Shannon sampling theorem states sampling frequency should be atleast twice 
// that of largest wave frequency. Thus do not make the wave freq > half sampling frequency.
//
static constexpr int errorSoundFreq_hz {200};
static constexpr float errorSoundDuration_s {0.5f};
static constexpr int errorSoundVolume {MAX_VOLUME};
static ResourceName_t errorSoundName {"sfxerror"};
ResourceKey_t errorSoundKey {0};

//
// The set of all loaded sounds accessed via their resource key.
//
static ResourceKey_t nextResourceKey {0};
static std::unordered_map<ResourceKey_t, SoundResource> sounds;

//
// The set of all loaded music accessed via their resource key.
//
static std::unordered_map<ResourceKey_t, MusicResource> music;

//
// Music volume is updated in the update function at the next point in which the update will
// succeed; volume cannot be set during fade effects.
//
static int musicVolume;
static bool hasMusicVolumeChanged {false};

//
// The configuration this module was initialized with.
//
SFXConfiguration sfxconfiguration;

//
// Maintains data on which channel is playing which sound. Channel ids range from 0 up to
// sfxconfiguration._numMixChannels - 1.
//
static std::vector<ResourceKey_t> channelPlayback;

//
// An array of current volumes for all mix channels. Stored here because in SDL_Mixer there 
// appears to be no way to get the volume of a channel without setting it.
//
static std::vector<int> channelVolume;

//
// The set of all sounds waiting to be unloaded once all running instances have stopped
// playback.
//
static std::vector<ResourceKey_t> soundUnloadQueue;
static std::vector<ResourceKey_t> musicUnloadQueue;

/////////////////////////////////////////////////////////////////////////////////////////////////
// SOUND FUNCTIONS 
/////////////////////////////////////////////////////////////////////////////////////////////////

void onChannelFinished(int channel)
{
  assert(0 <= channel && channel < sfxconfiguration._numMixChannels);
  channelPlayback[channel] = nullResourceKey;
}

//
// Generates a short sinusoidal beep.
//
template<typename T>
Mix_Chunk* generateSineBeep(int waveFreq_hz, float waveDuration_s)
{
  static_assert(std::numeric_limits<T>::is_integer);

  float waveFreq_rad_per_s = waveFreq_hz * 2.f * M_PI;
  int sampleCount = sfxconfiguration._samplingFreq_hz * waveDuration_s;
  float samplePeriod_s = 1.f / sfxconfiguration._samplingFreq_hz;
  T* pcm = new T[sampleCount];
  for(int s = 0; s < sampleCount; ++s){
    float sf = sinf(waveFreq_rad_per_s * (s * samplePeriod_s));   // wave equation = sin(wt)
    if(std::numeric_limits<T>::is_signed){
      if(sf < 0.f)  pcm[s] = static_cast<T>(sf * std::numeric_limits<T>::min());
      if(sf >= 0.f) pcm[s] = static_cast<T>(sf * std::numeric_limits<T>::max());
    }
    else{
      pcm[s] = static_cast<T>((sf + 1.f) * (std::numeric_limits<T>::max() / 2));
    }
  }

  Mix_Chunk* chunk = new Mix_Chunk();
  chunk->allocated = 1;
  chunk->abuf = reinterpret_cast<uint8_t*>(pcm);
  chunk->alen = sampleCount * sizeof(T);
  chunk->volume = errorSoundVolume;
  return chunk;
}

static void generateErrorSound(SampleFormat format)
{
  Mix_Chunk* chunk {nullptr};
  switch(format){
    case SAMPLE_FORMAT_U8     : {chunk = generateSineBeep<uint8_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S8     : {chunk = generateSineBeep<int8_t  >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_U16LSB : {chunk = generateSineBeep<uint16_t>(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S16LSB : {chunk = generateSineBeep<int16_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    case SAMPLE_FORMAT_S32LSB : {chunk = generateSineBeep<int32_t >(errorSoundFreq_hz, errorSoundDuration_s); break;}
    default: assert(0);
  }
  SoundResource resource {};
  resource._name = errorSoundName;
  resource._chunk = chunk;
  resource._referenceCount = 0;
  errorSoundKey = nextResourceKey++;
  sounds.emplace(std::make_pair(errorSoundKey, resource));
}

static void freeErrorSound()
{
  auto search = sounds.find(errorSoundKey);
  assert(search != sounds.end());
  auto& resource = search->second;
  delete[] resource._chunk->abuf;
  delete resource._chunk;
  resource._chunk = nullptr;
  sounds.erase(search);
}

static bool unloadSound(ResourceKey_t soundKey)
{
  assert(soundKey != errorSoundKey);
  auto search = sounds.find(soundKey);
  if(search == sounds.end()){
    log::log(log::LVL_WARN, log::msg_sfx_unloading_nonexistent_sound, std::to_string(soundKey));
  }
  else{
    search->second._referenceCount--;
    if(search->second._referenceCount <= 0){
      Mix_FreeChunk(search->second._chunk);
      sounds.erase(search);
      log::log(log::LVL_INFO, log::msg_sfx_sound_unloaded, std::to_string(soundKey));
    }
  }
  return true;
}

static bool isChannelPlayingSound(ResourceKey_t soundKey)
{
  return std::find(channelPlayback.begin(), channelPlayback.end(), soundKey) != channelPlayback.end();
}

static void unloadUnusedSounds()
{
  if(soundUnloadQueue.size() == 0) return;
  soundUnloadQueue.erase(std::remove_if(soundUnloadQueue.begin(), soundUnloadQueue.end(), [](ResourceKey_t soundKey){
    return !isChannelPlayingSound(soundKey) && unloadSound(soundKey);
  }));
}

static ResourceKey_t returnErrorSound()
{
  auto search = sounds.find(errorSoundKey);
  assert(search != sounds.end());
  search->second._referenceCount++;
  log::log(log::LVL_INFO, log::msg_sfx_error_sound_usage, std::to_string(search->second._referenceCount));
  return errorSoundKey;
}

ResourceKey_t loadSoundWAV(ResourceName_t soundName)
{
  log::log(log::LVL_INFO, log::msg_sfx_loading_sound, soundName);

  for(auto& pair : sounds){
    if(pair.second._name == soundName){
      pair.second._referenceCount++;
      std::string addendum {"reference count="};
      addendum += std::to_string(pair.second._referenceCount);
      log::log(log::LVL_INFO, log::msg_sfx_sound_already_loaded, addendum);
      return pair.first;
    }
  }

  SoundResource resource {};
  std::string wavpath {};
  wavpath += RESOURCE_PATH_SOUNDS;
  wavpath += soundName;
  wavpath += io::Wav::FILE_EXTENSION;
  resource._chunk = Mix_LoadWAV(wavpath.c_str());
  if(resource._chunk == nullptr){
    log::log(log::LVL_ERROR, log::msg_sfx_fail_load_sound, wavpath + " : " + Mix_GetError());
    log::log(log::LVL_INFO, log::msg_sfx_using_error_sound, wavpath);
    return returnErrorSound();
  }
  resource._name = soundName;
  resource._referenceCount = 1;

  ResourceKey_t newKey = nextResourceKey++;
  sounds.emplace(std::make_pair(newKey, resource));

  std::string addendum{};
  addendum += "[name:key]=[";
  addendum += soundName;
  addendum += ":";
  addendum += std::to_string(newKey);
  addendum += "]";
  log::log(log::LVL_INFO, log::msg_sfx_load_sound_success, addendum);

  return newKey;
}

void queueUnloadSound(ResourceKey_t soundKey)
{
  assert(soundKey != errorSoundKey);
  soundUnloadQueue.push_back(soundKey);
}

static Mix_Chunk* findChunk(ResourceKey_t soundKey)
{
  auto search = sounds.find(soundKey);
  if(search == sounds.end()){
    log::log(log::LVL_WARN, log::msg_sfx_playing_nonexistent_sound, std::to_string(soundKey));
    return nullptr;
  }
  return search->second._chunk;
}

static SoundChannel_t onSoundPlayError(ResourceKey_t soundKey)
{
  std::string addendum{};
  addendum += std::to_string(soundKey);
  addendum += " : ";
  addendum += Mix_GetError();
  log::log(log::LVL_WARN, log::msg_sfx_fail_play_sound, addendum);
  return NULL_CHANNEL;
}

SoundChannel_t playSound(ResourceKey_t soundKey, int loops)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_PlayChannel(-1, chunk, loops);
  if(channel == -1) return onSoundPlayError(soundKey);
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

SoundChannel_t playSoundTimed(ResourceKey_t soundKey, int loops, int playDuration_ms)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_PlayChannelTimed(-1, chunk, loops, playDuration_ms);
  if(channel == -1) return onSoundPlayError(soundKey);
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

SoundChannel_t playSoundFadeIn(ResourceKey_t soundKey, int loops, int fadeDuration_ms)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_FadeInChannel(-1, chunk, loops, fadeDuration_ms);
  if(channel == -1) return onSoundPlayError(soundKey);
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

SoundChannel_t playSoundFadeInTimed(ResourceKey_t soundKey, int loops, int fadeDuration_ms, int playDuration_ms)
{
  auto* chunk = findChunk(soundKey);
  if(chunk == nullptr) return NULL_CHANNEL;
  SoundChannel_t channel = Mix_FadeInChannelTimed(-1, chunk, loops, fadeDuration_ms, playDuration_ms);
  if(channel == -1) return onSoundPlayError(soundKey);
  assert(channelPlayback[channel] == nullResourceKey);
  channelPlayback[channel] = soundKey;
  return channel;
}

void stopChannel(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_HaltChannel(channel);
}

void stopChannelTimed(SoundChannel_t channel, int durationUntilStop_ms)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_ExpireChannel(channel, durationUntilStop_ms);
}

void stopChannelFadeOut(SoundChannel_t channel, int fadeDuration_ms)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_FadeOutChannel(channel, fadeDuration_ms);
}

void pauseChannel(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_Pause(channel);
}

void resumeChannel(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  Mix_Resume(channel);
}

bool isChannelPlaying(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return false;
  if(channel == ALL_CHANNELS) return false;
  assert(0 <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  return Mix_Playing(channel) == 1;
}

bool isChannelPaused(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return false;
  if(channel == ALL_CHANNELS) return false;
  assert(0 <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  return Mix_Paused(channel) == 1;
}

void setChannelVolume(SoundChannel_t channel, int volume)
{
  if(channel == NULL_CHANNEL) return;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  int vol = std::clamp(volume, MIN_VOLUME, MAX_VOLUME);
  channelVolume[channel] = Mix_Volume(channel, vol); 
}

int getChannelVolume(SoundChannel_t channel)
{
  if(channel == NULL_CHANNEL) return 0;
  assert(ALL_CHANNELS <= channel && channel <= sfxconfiguration._numMixChannels - 1);
  return channelVolume[channel];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MUSIC FUNCTIONS 
/////////////////////////////////////////////////////////////////////////////////////////////////

static Mix_Music* findMusic(ResourceKey_t musicKey)
{
  if(musicKey == nullResourceKey){
    log::log(log::LVL_WARN, log::msg_sfx_playing_nonexistent_music, std::to_string(musicKey));
    return nullptr;
  }
  auto search = music.find(musicKey);
  if(search == music.end()){
    log::log(log::LVL_WARN, log::msg_sfx_playing_nonexistent_music, std::to_string(musicKey));
    return nullptr;
  }
  return search->second._music;
}

static void onMusicPlayError(ResourceKey_t musicKey)
{
  std::string addendum{};
  addendum += std::to_string(musicKey);
  addendum += " : ";
  addendum += Mix_GetError();
  log::log(log::LVL_WARN, log::msg_sfx_fail_play_music, addendum);
}

static void playMusic__(ResourceKey_t musicKey, int loops)
{
  Mix_HaltMusic(); // to prevent the mixer play function blocking.
  Mix_Music* music = findMusic(musicKey);
  if(music == nullptr) return;
  if(Mix_PlayMusic(music, loops) != 0) return onMusicPlayError(musicKey);
}

static void playMusicFadeIn__(ResourceKey_t musicKey, int loops, int fadeDuration_ms)
{
  Mix_HaltMusic(); // to prevent the mixer play function blocking.
  Mix_Music* music = findMusic(musicKey);
  if(music == nullptr) return;
  if(Mix_FadeInMusic(music, loops, fadeDuration_ms) != 0) return onMusicPlayError(musicKey);
}

static void stopMusic__()
{
  Mix_HaltMusic();
}

static void stopMusicFadeOut__(int fadeDuration_ms)
{
  Mix_FadeOutMusic(fadeDuration_ms);
}

static void pauseMusic__()
{
  Mix_PauseMusic();
}

static void resumeMusic__()
{
  Mix_ResumeMusic();
}

MusicSequencePlayer::MusicSequencePlayer() :
  _state{STOPPED},
  _sequence{},
  _currentNode{0},
  _musicClock_s{0.f},
  _isLooping{false}
{}

void MusicSequencePlayer::onUpdate(float dt)
{
  // note: requires a fading out state here because the SDL_mixer play functions will block
  // if there is currently music fading out, sleeping until the fade out is complete so it can
  // start playing the new music. To prevent blocking I wait for the fade out myself without
  // blocking.

  switch(_state){
    case PLAYING: 
      onPlayingUpdate(dt); 
      break;
    case FADING_OUT: 
      onFadingOutUpdate(dt); 
      break;
    default:
      break;
  }
}

void MusicSequencePlayer::play(MusicSequence_t sequence, bool loop)
{
  stop();
  _sequence = std::move(sequence);
  _currentNode = 0;
  _musicClock_s = 0.f;
  _isLooping = loop;
  if(_sequence.size() == 0){ 
    _state = STOPPED;
    return;
  }
  playNode(&_sequence[_currentNode]);
}

void MusicSequencePlayer::stop()
{
  if(_state != STOPPED){
    _musicClock_s = 0.f;
    _currentNode = 0;
    _state = STOPPED;
    _sequence.clear();
    stopMusic__();
  }
}

void MusicSequencePlayer::pause()
{
  if(_state == PLAYING){
    _state = PAUSED;
    pauseMusic__();
  }
}

void MusicSequencePlayer::resume()
{
  if(_state == PAUSED){
    _state = PLAYING;
    resumeMusic__();
  }
}

bool MusicSequencePlayer::isUsingMusicResource(ResourceKey_t musicKey)
{
  return music.find(musicKey) != music.end();
}

void MusicSequencePlayer::playNode(const MusicSequenceNode* node)
{
  if(node->_fadeInDuration_ms > 0.f)
    playMusicFadeIn__(node->_musicKey, INFINITE_LOOPS, node->_fadeInDuration_ms);
  else
    playMusic__(node->_musicKey, INFINITE_LOOPS);
  _state = PLAYING;
}

void MusicSequencePlayer::stopNode(const MusicSequenceNode* node)
{
  if(node->_fadeOutDuration_ms > 0.f) 
    stopMusicFadeOut__(node->_fadeOutDuration_ms);
  else 
    stopMusic__();
  _state = FADING_OUT;
}

void MusicSequencePlayer::onPlayingUpdate(float dt)
{
  assert(0 <= _currentNode && _currentNode < _sequence.size());
  const MusicSequenceNode* node {nullptr};
  node = &_sequence[_currentNode];
  _musicClock_s += dt;
  if(_musicClock_s > ((node->_playDuration_ms - node->_fadeOutDuration_ms) / 1000.f)){
    stopNode(node);
    _musicClock_s = 0.f;
  }
}

void MusicSequencePlayer::onFadingOutUpdate(float dt)
{
  assert(0 <= _currentNode && _currentNode < _sequence.size());
  const MusicSequenceNode* node {nullptr};
  node = &_sequence[_currentNode];
  _musicClock_s += dt;
  if(_musicClock_s > ((node->_fadeOutDuration_ms / 1000.f))){
    ++_currentNode;
    if(_currentNode >= _sequence.size()){
      if(!_isLooping) return stop();
      _currentNode = 0;
    }
    node = &_sequence[_currentNode];
    playNode(node);
    _musicClock_s = 0.f;
  }
}

ResourceKey_t loadMusicWAV(ResourceName_t musicName)
{
  log::log(log::LVL_INFO, log::msg_sfx_loading_music, musicName);

  for(auto& pair : music){
    if(pair.second._name == musicName){
      pair.second._referenceCount++;
      std::string addendum {"reference count="};
      addendum += std::to_string(pair.second._referenceCount);
      log::log(log::LVL_INFO, log::msg_sfx_music_already_loaded, addendum);
      return pair.first;
    }
  }

  MusicResource resource {};
  std::string wavpath {};
  wavpath += RESOURCE_PATH_MUSIC;
  wavpath += musicName;
  wavpath += io::Wav::FILE_EXTENSION;
  resource._music = Mix_LoadMUS(wavpath.c_str());
  if(resource._music == nullptr){
    log::log(log::LVL_ERROR, log::msg_sfx_fail_load_music, wavpath + " : " + Mix_GetError());
    log::log(log::LVL_WARN, log::msg_sfx_no_error_music);
    return nullResourceKey;
  }
  resource._name = musicName;
  resource._referenceCount = 1;

  ResourceKey_t newKey = nextResourceKey++;
  music.emplace(std::make_pair(newKey, resource));

  std::string addendum{};
  addendum += "[name:key]=[";
  addendum += musicName;
  addendum += ":";
  addendum += std::to_string(newKey);
  addendum += "]";
  log::log(log::LVL_INFO, log::msg_sfx_load_music_success, addendum);

  return newKey;
}

static bool unloadMusic(ResourceKey_t musicKey)
{
  auto search = music.find(musicKey);
  if(search == music.end()){
    log::log(log::LVL_WARN, log::msg_sfx_unloading_nonexistent_music, std::to_string(musicKey));
  }
  else{
    search->second._referenceCount--;
    if(search->second._referenceCount <= 0){
      Mix_FreeMusic(search->second._music);
      music.erase(search);
      log::log(log::LVL_INFO, log::msg_sfx_music_unloaded, std::to_string(musicKey));
    }
  }
  return true;
}

static void unloadUnusedMusic()
{
  if(musicUnloadQueue.size() == 0) return;
  musicUnloadQueue.erase(std::remove_if(musicUnloadQueue.begin(), musicUnloadQueue.end(), [](ResourceKey_t musicKey){
    return !musicSequencePlayer.isUsingMusicResource(musicKey) && unloadMusic(musicKey);
  }));
}

void queueUnloadMusic(ResourceKey_t musicKey)
{
  musicUnloadQueue.push_back(musicKey);
}

void playMusic(MusicSequence_t sequence, bool loop)
{
  musicSequencePlayer.play(std::move(sequence), loop);
}

void stopMusic()
{
  musicSequencePlayer.stop();
}

void pauseMusic()
{
  musicSequencePlayer.pause();
}

void resumeMusic()
{
  musicSequencePlayer.resume();
}

bool isMusicPlaying()
{
  return Mix_PlayingMusic() == 1;
}

bool isMusicPaused()
{
  return Mix_PausedMusic() == 1;
}

bool isMusicFadingIn()
{
  return Mix_FadingMusic() == MIX_FADING_IN;
}

bool isMusicFadingOut()
{
  return Mix_FadingMusic() == MIX_FADING_OUT;
}

void setMusicVolume(int volume)
{
  int vol = std::clamp(volume, MIN_VOLUME, MAX_VOLUME);
  if(vol != musicVolume){
    musicVolume = vol;
    hasMusicVolumeChanged = true;
  }
}

int getMusicVolume(int volume)
{
  return musicVolume;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// GENERAL FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////

static void logSpec()
{
  const SDL_version* version = Mix_Linked_Version();
  log::log(log::LVL_INFO, "SDL_Mixer Version:");
  log::log(log::LVL_INFO, "major:", std::to_string(version->major));
  log::log(log::LVL_INFO, "minor:", std::to_string(version->minor));
  log::log(log::LVL_INFO, "patch:", std::to_string(version->patch));

  int freq, channels; uint16_t format;
  if(!Mix_QuerySpec(&freq, &format, &channels)){
    log::log(log::LVL_WARN, log::msg_sfx_fail_query_spec, std::string{Mix_GetError()});
    return;
  }
  
  const char* formatString {nullptr};
  switch(format){
    case SAMPLE_FORMAT_U8    : {formatString = "U8";     break;}
    case SAMPLE_FORMAT_S8    : {formatString = "S8";     break;}
    case SAMPLE_FORMAT_U16LSB: {formatString = "U16LSB"; break;}
    case SAMPLE_FORMAT_S16LSB: {formatString = "S16LSB"; break;}
    case SAMPLE_FORMAT_S32LSB: {formatString = "S32LSB"; break;}
    default                  : {formatString = "unknown format";}
  }

  const char* modeString {nullptr};
  switch(channels){
    case OutputMode::MONO:   {modeString = "mono";   break;}
    case OutputMode::STEREO: {modeString = "stereo"; break;}
    default:                 {modeString = "unknown mode";}
  }

  log::log(log::LVL_INFO, "SDL_Mixer Audio Device Spec: ");
  log::log(log::LVL_INFO, "sample frequency: ", std::to_string(freq));
  log::log(log::LVL_INFO, "sample format: ", formatString);
  log::log(log::LVL_INFO, "output mode: ", modeString);
}

bool initialize(SFXConfiguration sfxconf)
{
  assert(!(SDL_AUDIO_ISFLOAT(sfxconf._sampleFormat)));
  log::log(log::LVL_INFO, log::msg_sfx_initializing);
  sfxconfiguration = sfxconf;
  int result = Mix_OpenAudio(
    sfxconf._samplingFreq_hz, 
    sfxconf._sampleFormat, 
    sfxconf._outputMode, 
    sfxconf._chunkSize
  );
  if(result != 0){
    log::log(log::LVL_ERROR, log::msg_sfx_fail_open_audio, std::string{Mix_GetError()});
    return false;
  }
  Mix_AllocateChannels(sfxconf._numMixChannels);
  Mix_ChannelFinished(&onChannelFinished);
  channelPlayback.resize(sfxconf._numMixChannels, nullResourceKey);
  channelPlayback.shrink_to_fit();
  channelVolume.resize(sfxconf._numMixChannels, MAX_VOLUME);
  channelVolume.shrink_to_fit();
  generateErrorSound(static_cast<SampleFormat>(sfxconf._sampleFormat));
  logSpec();
  return true;
}

void shutdown()
{
  stopChannel(ALL_CHANNELS);
  freeErrorSound();
  for(auto& pair : sounds)
    Mix_FreeChunk(pair.second._chunk);
  sounds.clear();
  Mix_CloseAudio();
}

void onUpdate(float dt)
{
  unloadUnusedSounds();
  unloadUnusedMusic();

  if(hasMusicVolumeChanged && Mix_FadingMusic() == MIX_NO_FADING){
    Mix_VolumeMusic(musicVolume);
    hasMusicVolumeChanged = false;
  }

  musicSequencePlayer.onUpdate(dt);
}

} // namespace sfx
} // namespace pxr
