#include <iostream>
#include <fstream>
#include "pxr_log.h"

namespace pxr
{
namespace log
{

static std::ofstream _os;

void initialize()
{
  _os.open(LOG_FILENAME, std::ios_base::trunc);
  if(!_os){
    log(ERROR, log::msg_log_fail_open);
    log(INFO, log::msg_log_to_stderr);
  }
}

void shutdown()
{
  if(_os)
    _os.close();
}

void log(Level level, const char* error, const std::string& addendum)
{
  std::ostream& os {_os ? _os : std::cerr}; 
  os << prefix[level] << LOG_DELIM << error;
  if(!addendum.empty())
    os << LOG_DELIM << addendum;
  os << std::endl;
}

} // namespace log
} // namespace pxr
