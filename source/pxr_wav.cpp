#include <fstream>
#include "pxr_wav.h"
#include "pxr_log.h"

namespace pxr
{
namespace io
{

Wav::Wav() :
  _waveData{nullptr},
  _waveSizeBytes{0},
  _sampleRate{0},
  _bitsPerSample{0},
  _numChannels{0}
{}

Wav::~Wav()
{
  if(_waveData != nullptr){
    delete[] _waveData;
    _waveData = nullptr;
  }
}

bool Wav::load(std::string filepath)
{
  unload();

  log::log(log::INFO, log::msg_wav_loading, filepath);

  std::ifstream file {filepath, std::ios::binary};  
  if(!file){
    log::log(log::ERROR, log::msg_wav_fail_open, filepath);
    return false;
  }

  auto readFail = [](){
    log::log(log::ERROR, log::msg_wav_read_fail);
    return false;
  };

  RiffHeader riff {};
  if(!file.read(reinterpret_cast<char*>(&riff._riffMagic), sizeof(riff._riffMagic))) return readFail();

  if(riff._riffMagic != RIFFMAGIC){
    log::log(log::ERROR, log::msg_wav_not_riff);
    return false;
  }

  if(!file.read(reinterpret_cast<char*>(&riff._chunkSize), sizeof(riff._chunkSize))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&riff._waveMagic), sizeof(riff._waveMagic))) return readFail();

  if(riff._waveMagic != WAVEMAGIC){
    log::log(log::ERROR, log::msg_wav_not_wave);
    return false;
  }

  FormatSubChunk fmt {};
  if(!file.read(reinterpret_cast<char*>(&fmt._formatMagic), sizeof(fmt._formatMagic))) return readFail();

  if(fmt._formatMagic != FORMATMAGIC){
    log::log(log::ERROR, log::msg_wav_fmt_chunk_missing);
    return false;
  }

  if(!file.read(reinterpret_cast<char*>(&fmt._subChunkSize), sizeof(fmt._subChunkSize))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&fmt._audioFormat), sizeof(fmt._audioFormat))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&fmt._numChannels), sizeof(fmt._numChannels))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&fmt._sampleRate), sizeof(fmt._sampleRate))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&fmt._byteRate), sizeof(fmt._byteRate))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&fmt._blockAlign), sizeof(fmt._blockAlign))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&fmt._bitsPerSample), sizeof(fmt._bitsPerSample))) return readFail();

  if(fmt._subChunkSize != 16){
    log::log(log::ERROR, log::msg_wav_not_pcm);
    return false;
  }

  if(fmt._audioFormat != 1){
    log::log(log::ERROR, log::msg_wav_bad_compressed);
    return false;
  }

  if(fmt._numChannels != 1 && fmt._numChannels != 2){
    log::log(log::ERROR, log::msg_wav_odd_channels, std::to_string(fmt._numChannels));
    return false;
  }

  if(fmt._bitsPerSample != 8 && fmt._bitsPerSample != 16){
    log::log(log::ERROR, log::msg_wav_odd_sample_bits, std::to_string(fmt._bitsPerSample));
    return false;
  }

  _numChannels = fmt._numChannels;
  _sampleRate = fmt._sampleRate;
  _bitsPerSample = fmt._bitsPerSample;

  DataSubChunk data {};
  if(!file.read(reinterpret_cast<char*>(&data._dataMagic), sizeof(data._dataMagic))) return readFail();
  if(!file.read(reinterpret_cast<char*>(&data._subChunkSize), sizeof(data._subChunkSize))) return readFail();

  if(data._dataMagic != DATAMAGIC){
    log::log(log::ERROR, log::msg_wav_data_chunk_missing);
    return false;
  }

  _waveSizeBytes = data._subChunkSize;

  if(!(0 < _waveSizeBytes || _waveSizeBytes <= SOUND_DATA_SIZE_MAX_BYTES)){
    log::log(log::ERROR, log::msg_wav_odd_data_size, std::to_string(_waveSizeBytes));
    return false;
  }

  _waveData = new char[_waveSizeBytes];
  
  if(_numChannels == 1){
    if(!file.read(_waveData, _waveSizeBytes)){ 
      delete[] _waveData;
      return readFail();
    }
  }

  //
  // For stereo PCM data we need it in a channel interleaved format; .wav files store channel
  // data in a non-interleaved format with right channel samples first and then left channels
  // second.
  //
  else {
    int bytesPerSample = _bitsPerSample / 8;
    int samplesPerChannel = _waveSizeBytes / (_numChannels * bytesPerSample);

    //
    // load right channel.
    //
    for(int rsample = 1; rsample < samplesPerChannel; rsample += 2){
      if(!file.read(_waveData + (rsample * bytesPerSample), bytesPerSample)){
        delete[] _waveData;
        return readFail();
      }
    }

    //
    // load left channel.
    //
    for(int lsample = 0; lsample < samplesPerChannel; lsample += 2){
      if(!file.read(_waveData + (lsample * bytesPerSample), bytesPerSample)){
        delete[] _waveData;
        return readFail();
      }
    }
  }

  log::log(log::INFO, log::msg_wav_load_success, filepath);

  return true;
}

void Wav::unload()
{
  if(_waveData != nullptr)
    delete[] _waveData;

  _waveData = nullptr;
  _waveSizeBytes = 0;
  _sampleRate = 0;
  _bitsPerSample = 0;
  _numChannels = 0;
}

} // namespace io
} // namespace pxr
