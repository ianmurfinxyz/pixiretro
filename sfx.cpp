#include <string>
#include <sstream>
#include <array>
#include <map>
#include "sfx.h"
#include "log.h"

namespace pxr
{
namespace sfx
{

static ALCdevice* sfxDevice {nullptr};
static ALCcontext* sfxContext {nullptr};

static constexpr int SOUND_SOURCE_COUNT {16};
static std::array<ALuint, SOUND_SOURCE_COUNT> soundSources;

ResourceKey_t nextSoundKey {0};
static std::map<ResourceKey_t, ALuint> soundBuffers;
ALuint errorSoundBuffer;

//
// Generate a sinusoidal beep.
//
static void genErrorSound()
{
  static constexpr int sampleFreqHz {44100};
  static constexpr int sampleCount {freq / 2};
  static constexpr float samplePeriodSec {1.f / sampleFreqHz};

  //
  // Nyquist-Shannon sampling theorem states sampling frequency should be atleast twice 
  // that of largest wave frequency.
  //
  static constexpr int waveToSampleFreqRatio {8};
  static constexpr float waveFreqRadPSec {(sampleFreq * waveToSampleFreqRatio) * (2.f * M_PI)};

  char* pcm = new char[sampleCount];
  for(int s = 0; s < sampleCount; ++s){
    pcm[s] = std::sinf(waveFreqRadPSec * (s * samplePeriodSec));  // sin(wt)
  }

  ALuint buffer {0};
  alGenBuffer(1, &buffer);
  assert(alGetError() == AL_NO_ERROR);

  alBufferData(buffer, AL_FORMAT_MONO8, reinterpret_cast<void*>(pcm), sampleCount, sampleFreqHz);
  assert(alGetError() == AL_NO_ERROR);
}

bool initialize(int deviceid)
{
  log::log(log::INFO, log::msg_sfx_initializing);
  
  // create device.

  std::vector<ALCChar*> deviceNames;
  const ALCchar* names = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
  do{
    deviceNames.push_back(names);
    names += deviceNames.back().size() + 1;
  }
  while(*(names + 1) != '\0');

  log::log(log::INFO, log::msg_sfx_listing_devices);
  for(int i = 0; i < devices.size(); ++i){
    std::stringstream ss {};
    ss << "[" << i << "] : " << deviceNames[i];
    log::log(log::INFO, log::msg_sfx_device, ss.c_str());
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
      deviceName = deviceNames[deviceid];
    }
  }

  log::log(log::ERROR, log::msg_sfx_creating_device, deviceName ? deviceName : std::string{});

  sfxDevice = alcOpenDevice(deviceName);
  if(!sfxDevice){
    log::log(log::ERROR, log::msg_sfx_fail_create_device);
    return false;
  }

  // setup context.

  sfxContext = alcCreateContext(sfxDevice, nullptr);
  if(!sfxContext){
    log::log(log::ERROR, log::msg_sfx_fail_create_context);
    alcCloseDevice(sfxDevice);
    return false;
  }

  alcMakeContextCurrent(sfxContext);

  // setup listener.

  alListenerf(AL_GAIN, 1.f);

  float vecs[6] {0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  alListener3f(AL_POSITION, vecs);
  alListener3f(AL_VELOCITY, vecs);
  alListener3f(AL_ORIENTATION, vecs);

  // setup sources.
  
  alGenSources(SOUND_SOURCE_COUNT, sources.data());
  if(error != AL_NO_ERROR){
    log::log(log::ERROR, log::msg_sfx_fail_gen_sources);
    log::log(log::INFO, log::msg_sfx_openal_error, alGetString());
    shutdown();
    return false;
  }

  for(auto source : sources){
    alSourcef(source, AL_PITCH, 1.f);
    assert(alGetError() == AL_NO_ERROR);
    alSourcef(source, AL_GAIN, 1.f);
    assert(alGetError() == AL_NO_ERROR);
    alSource3f(source, AL_POSITION, 0.f, 0.f, 0.f);
    assert(alGetError() == AL_NO_ERROR);
    alSource3f(source, AL_VELOCITY, 0.f, 0.f, 0.f);
    assert(alGetError() == AL_NO_ERROR);
  }

  genErrorSound();
}

void shutdown()
{
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

  ALuint buffer {0};
  alGenBuffer(1, &buffer);
  assert(alGetError() == AL_NO_ERROR);

  int sampleBits = wav.getBitsPerSample();
  int nChannels = wav.getNumChannels();
  ALenum format = (nChannels == 1) ? 
                  (sampleBits == 8) ? AL_FORMAT_MONO8   : AL_FORMAT_MONO16 :
                  (sampleBits == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;

  alBufferData(buffer, format, wav.getSampleData(), wav.getSampleDataSize(), wav.getSampleRate());
  if((auto error = alGetError()) != AL_NO_ERROR){
    log::log(log::ERROR, log::msg_sfx_fail_gen_buffer);
    log::log(log::INFO, log::msg_sfx_openal_error, alGetString());
    return useErrorSound();
  }

  ResourceKey_t newKey = nextSoundKey++;
  soundBuffers.emplace(std::make_pair(newKey, buffer));
  return newKey;
}

void unloadSound(ResourceKey_t soundKey)
{
  auto search = soundBuffers.find(soundKey);
  if(search != soundBuffers.end())
    soundBuffers.erase(search);
}

void playSound(ResourceKey_t soundkey)
{
  auto search = soundBuffers.find(soundKey);
  if(search == soundBuffers.end()){
    log::log(log::WARN, log::msg_sfx_missing_sound, soundkey);
    return;
  }
  ALuint buffer = search->second;

  for(auto source : sources){
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    assert(alGetError() == AL_NO_ERROR);
    if(state != AL_PLAYING){
      alSourcei(source, AL_BUFFER, buffer);
      assert(alGetError() == AL_NO_ERROR);
      alSourcePlay(source);
      assert(alGetError() == AL_NO_ERROR);
      return;
    }
  }

  log::log(log::WARN, log::msg_sfx_no_free_sources);
}

} // namespace sfx
} // namespace pxr
