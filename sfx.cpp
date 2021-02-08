#include <string>
#include <sstream>
#include "sfx.h"
#include "log.h"

namespace pxr
{

static ALCdevice* sfxDevice {nullptr};

bool initialize(int deviceid = -1)
{
  log::log(log::INFO, log::msg_sfx_initializing);
  
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

  
}

} // namespace pxr
