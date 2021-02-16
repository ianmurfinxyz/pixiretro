#ifndef _PIXIRETRO_WAVSOUND_H_
#define _PIXIRETRO_WAVSOUND_H_

#include <string>
#include <cinttypes>

namespace pxr
{
namespace io
{

//
// Represent a wave (.wav) sound file.
//
// This class only supports wave sounds with:
//      
//      sample depths == 8 or 16
//      num channels  == 1 or 2
//
// i.e. mono8, mono16, stereo8 or stereo16.
//
class Wav
{
public:
  static constexpr const char* FILE_EXTENSION {".wav"};

public:
  Wav();
  ~Wav();

  bool load(std::string filepath);

  const void* getSampleData() const {return reinterpret_cast<void*>(_waveData);}
  int getSampleDataSize() const {return _waveSizeBytes;}
  int getSampleRate() const {return _sampleRate;}
  int getNumChannels() const {return _numChannels;}
  int getBitsPerSample() const {return _bitsPerSample;}

private:

  //
  // These magics are in little endian format.
  //
  static constexpr int32_t RIFFMAGIC   {0x46464952};
  static constexpr int32_t WAVEMAGIC   {0x45564157};
  static constexpr int32_t FORMATMAGIC {0x20746d66};
  static constexpr int32_t DATAMAGIC   {0x61746164};

  //
  // Used to guard against excessive file sizes.
  //
  static constexpr int ONE_MEBIBYTE {1024 * 1024};
  static constexpr int SOUND_DATA_SIZE_MAX_BYTES {10 * ONE_MEBIBYTE};

  struct RiffHeader
  {
    int32_t _riffMagic;
    int32_t _chunkSize;
    int32_t _waveMagic;
  };

  struct FormatSubChunk
  {
    int32_t _formatMagic;
    int32_t _subChunkSize;
    int16_t _audioFormat;
    int16_t _numChannels;
    int32_t _sampleRate;
    int32_t _byteRate;
    int16_t _blockAlign;
    int16_t _bitsPerSample;
  };

  struct DataSubChunk
  {
    int32_t _dataMagic;
    int32_t _subChunkSize;
  };

private:
  void unload();

private:

  //
  // Raw wave sound data. For stereo data the samples are interleaved with the left channel
  // coming first (lower index) than the right. This formatting is such as to be compatible
  // with openal.
  //
  char* _waveData;

  int _waveSizeBytes;
  int _sampleRate;
  int _bitsPerSample;
  int _numChannels;
};

} // namespace io
} // namespace pxr

#endif
