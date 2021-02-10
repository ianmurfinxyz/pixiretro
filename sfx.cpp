#include <string>
#include <sstream>
#include <array>
#include <map>
#include <cmath>
#include <cassert>
#include <vector>
#include <algorithm>
#include "sfx.h"
#include "log.h"
#include "al.h"
#include "alc.h"
#include "wavesound.h"

namespace pxr
{
namespace sfx
{

//
// OpenAL Assert Macro.
//
// Many openAL calls can only fail upon encountering programming errors such as passing 
// invalid ALenum values. Thus these errors should never happen. 
//
// note: AL_OUT_OF_MEMORY errors may also occur; these can also be asserted since if the
// program runs out of memory it should crash and likely will anyway.
//
#define alas(FUNCTION_CALL)\
FUNCTION_CALL;\
assert(alGetError() == AL_NO_ERROR);

//
// Other openAL calls may fail due to unpredictable run-time errors. These errors should be 
// reported but will be left unhandled.
//
#define alErrorCheck(FUNCTION_CALL, ON_ERROR_STATEMENT)\
{\
  ALenum error = alGetError();\
  if(error != AL_NO_ERROR){\
    log::log(log::ERROR, log::msg_sfx_openal_error, alGetString(error));\
    log::log(log::ERROR, log::msg_sfx_openal_call, std::string{#FUNCTION_CALL});\
    ON_ERROR_STATEMENT;\
  }\
}

//
// OpenAL Error Check Macro.
//
#define alec(FUNCTION_CALL, ON_ERROR_STATEMENT)\
FUNCTION_CALL;\
alErrorCheck(FUNCTION_CALL, ON_ERROR_STATEMENT)\

/////////////////////////////////////////////////////////////////////////////////////////////////
// MODULE DATA
/////////////////////////////////////////////////////////////////////////////////////////////////

using SoundBufferKey_t = ALuint;
using SoundSourceKey_t = ALuint;

static ALCdevice* sfxDevice {nullptr};
static ALCcontext* sfxContext {nullptr};

static constexpr int SOUND_SOURCE_COUNT {16};
static std::array<std::pair<SoundSourceKey_t, ResourceKey_t>, SOUND_SOURCE_COUNT> soundSources;

ResourceKey_t nextSoundKey {0};
static std::map<ResourceKey_t, SoundBufferKey_t> soundBuffers;
SoundBufferKey_t errorSoundBuffer;

/////////////////////////////////////////////////////////////////////////////////////////////////
// MODULE FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////

//
// Generates a short sinusoidal beep.
//
static void genErrorSound()
{
  static constexpr int sampleFreqHz {44100};
  static constexpr int sampleCount {sampleFreqHz / 2};
  static constexpr float samplePeriodSec {1.f / sampleFreqHz};

  //
  // Nyquist-Shannon sampling theorem states sampling frequency should be atleast twice 
  // that of largest wave frequency.
  //
  // 0.1 means sample freq is x10 the wav freq and wave freq is 4410 which is an absolutely
  // horrid high pitch tone...perfect! :)
  //
  static constexpr float waveToSampleFreqRatio {0.1};
  static constexpr float waveFreqRadPSec {(sampleFreqHz * waveToSampleFreqRatio) * (2.f * M_PI)};

  unsigned char* pcm = new unsigned char[sampleCount];
  for(int s = 0; s < sampleCount; ++s){
    //
    // wave equation = sin(wt)
    //
    float sf = sinf(waveFreqRadPSec * (s * samplePeriodSec));

    //
    // sinf returns value within range [-1, +1]; need to convert to within range [0, 255] for
    // mono 8-bit samples.
    //
    sf = std::clamp((sf + 1) * 127.5f, 0.f, 255.f);

    pcm[s] = static_cast<unsigned char>(sf);
  }

  alas(alGenBuffers(1, &errorSoundBuffer));
  alas(alBufferData(errorSoundBuffer, AL_FORMAT_MONO8, reinterpret_cast<void*>(pcm), sampleCount, sampleFreqHz));
  delete[] pcm;
}

bool initialize(int deviceid)
{
  log::log(log::INFO, log::msg_sfx_initializing);

  //
  // Create openAL sound device.
  //
  
  std::vector<std::string> deviceNames;
  const ALCchar* names = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
  do{
    deviceNames.push_back(names);
    names += deviceNames.back().size() + 1;
  }
  while(*(names + 1) != '\0');

  //
  // TODO - CHECK: this code assigns device ids based on the order they are listed in the device
  // strings list. Does this order change? If it does then this code is flawed and will not
  // work.
  //
  
  log::log(log::INFO, log::msg_sfx_listing_devices);
  for(int i = 0; i < deviceNames.size(); ++i){
    std::stringstream ss {};
    ss << "[" << i << "] : " << deviceNames[i];
    log::log(log::INFO, log::msg_sfx_device, ss.str());
  }
  log::log(log::INFO, log::msg_sfx_set_device_info);

  const ALCchar* defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
  log::log(log::INFO, log::msg_sfx_default_device, defaultDeviceName);

  const ALCchar* deviceName {nullptr};
  if(deviceid > 0){
    if(deviceid > deviceNames.size()){
      log::log(log::ERROR, log::msg_sfx_invalid_deviceid, std::to_string(deviceid));
    }
    else{
      deviceName = deviceNames[deviceid].c_str();
    }
  }

  log::log(log::INFO, log::msg_sfx_creating_device, deviceName ? deviceName : std::string{"default"});

  sfxDevice = alcOpenDevice(deviceName);
  if(!sfxDevice){
    log::log(log::ERROR, log::msg_sfx_fail_create_device);
    return false;
  }

  //
  // Create openAL context.
  //

  sfxContext = alcCreateContext(sfxDevice, nullptr);
  if(!sfxContext){
    log::log(log::ERROR, log::msg_sfx_fail_create_context);
    alcCloseDevice(sfxDevice);
    return false;
  }

  alcMakeContextCurrent(sfxContext);

  //
  // Setup listener.
  //

  alas(alListenerf(AL_GAIN, 1.f));

  float vecs[6] {0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  alas(alListenerfv(AL_POSITION, vecs));
  alas(alListenerfv(AL_VELOCITY, vecs));
  alas(alListenerfv(AL_ORIENTATION, vecs));

  //
  // setup sources.
  //

  for(auto& source : soundSources){
    alec(alGenSources(1, &source.first), shutdown(); shutdown(); return false;);
    source.second = -1;
    alas(alSourcef(source.first, AL_PITCH, 1.f));
    alas(alSourcef(source.first, AL_GAIN, 1.f));
    alas(alSource3f(source.first, AL_POSITION, 0.f, 0.f, 0.f));
    alas(alSource3f(source.first, AL_VELOCITY, 0.f, 0.f, 0.f));
    alas(alSourcei(source.first, AL_LOOPING, AL_TRUE));
  }

  genErrorSound();

  return true;
}

void shutdown()
{
  for(auto source : soundSources)
    if(alIsSource(source.first))
      alas(alSourceStop(source.first));

  for(auto& [key, value] : soundBuffers){
    if(alIsBuffer(value)){
      alas(alDeleteBuffers(1, &value));
    }
  }

  alcMakeContextCurrent(nullptr);
  alcDestroyContext(sfxContext); 
  alcCloseDevice(sfxDevice);
}

static ResourceKey_t useErrorSound()
{
  ResourceKey_t newKey {0};
  log::log(log::INFO, log::msg_sfx_using_error_sound);
  newKey = nextSoundKey++;
  soundBuffers.emplace(std::make_pair(newKey, errorSoundBuffer));
  return newKey; 
}

ResourceKey_t loadSound(ResourceName_t soundName)
{
  WaveSound wav {};

  std::string wavpath {};
  wavpath += RESOURCE_PATH_SOUNDS;
  wavpath += soundName;
  wavpath += WaveSound::FILE_EXTENSION;
  if(!wav.load(wavpath))
    return useErrorSound();

  SoundBufferKey_t buffer {0};
  alas(alGenBuffers(1, &buffer));

  int sampleBits = wav.getBitsPerSample();
  int nChannels = wav.getNumChannels();
  ALenum format = (nChannels == 1) ? 
                  (sampleBits == 8) ? AL_FORMAT_MONO8   : AL_FORMAT_MONO16 :
                  (sampleBits == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;

  alec(alBufferData(buffer, 
                    format, 
                    wav.getSampleData(), 
                    wav.getSampleDataSize(), 
                    wav.getSampleRate()),
       return useErrorSound()
  );

  ResourceKey_t newKey = nextSoundKey++;
  soundBuffers.emplace(std::make_pair(newKey, buffer));
  return newKey;
}

void unloadSound(ResourceKey_t soundKey)
{
  stopSound(soundKey);
  auto search = soundBuffers.find(soundKey);
  if(search != soundBuffers.end()){
    alec(alDeleteBuffers(1, &search->second), 0);
    soundBuffers.erase(search);
  }
}

void playSound(ResourceKey_t soundKey, bool loop)
{
  auto search = soundBuffers.find(soundKey);
  if(search == soundBuffers.end()){
    log::log(log::WARN, log::msg_sfx_missing_sound, std::to_string(soundKey));
    return;
  }
  SoundBufferKey_t buffer = search->second;

  for(auto source : soundSources){
    ALint state;
    alas(alGetSourcei(source.first, AL_SOURCE_STATE, &state));
    if(state != AL_PLAYING){
      if(source.second != soundKey){
        alas(alSourcei(source.first, AL_BUFFER, buffer));
        source.second = soundKey;
      }
      alas(alSourcei(source.first, AL_LOOPING, loop ? AL_TRUE : AL_FALSE));
      alas(alSourcePlay(source.first));
      return;
    }
  }

  log::log(log::WARN, log::msg_sfx_no_free_sources);
}

void stopSound(ResourceKey_t soundKey)
{
  for(auto source : soundSources){
    if(source.second == soundKey){
      alas(alSourceStop(source.first));
    }
  }
}

} // namespace sfx
} // namespace pxr
