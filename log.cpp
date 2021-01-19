#include <iostream>
#include <fstream>
#include <array>

#include "log.h"

namespace pxr
{
namespace log
{

static constexpr std::array<const char*, 4> lvlstr {"fatal", "error", "warning", "info"};
static constexpr const char* filename {"log"};
static constexpr const char* delim {" : "};

static std::ofstream _os;

void initialize()
{
  _os.open(filename, std::ios_base::trunc);
  if(!_os){
    log(ERROR, logstr::fail_open_log);
    log(INFO, logstr::info_stderr_log);
  }
}

void shutdown()
{
  if(_os)
    _os.close();
}

void log(Level level, const char* error, const std::string& addendum)
{
  std::ostream& o {_os ? _os : std::cerr}; 
  o << lvlstr[level] << delim << error;
  if(!addendum.empty())
    o << delim << addendum;
  o << std::endl;
}

} // namespace log
} // namespace pxr
