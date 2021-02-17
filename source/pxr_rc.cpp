#include <limits>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "pxr_rc.h"
#include "pxr_log.h"

namespace pxr
{
namespace io
{

constexpr static RC::Value_t unsetValue {std::numeric_limits<int>::min()};

RC::Property::Property(Key_t key, std::string name, Value_t default_, Value_t min, Value_t max) :
  _key{key}, 
  _name{name}, 
  _value{unsetValue},
  _default{default_}, 
  _min{min}, 
  _max{max}
{
  assert(_default.index() == _min.index() && _default.index() == _max.index());
}

RC::RC(std::initializer_list<Property> properties)
{
  for(const auto& property : properties)
    _properties.emplace(std::make_pair(property._key, property));
}

int RC::load(const std::string& filename)
{
  std::string path{};
  path += RESOURCE_PATH_RC;
  path += filename;
  path += RC::FILE_EXTENSION;
  std::ifstream file {path};
  if(!file){
    log::log(log::ERROR, log::msg_rcfile_fail_open, filename);
    log::log(log::INFO, log::msg_rcfile_using_default);
    for(auto& pair : _properties)
      pair.second._value = pair.second._default;
    return -1;
  }

  auto lineNoToString = [](int l){return std::string{" ["} + std::to_string(l) + "] ";};
  auto isSpace = [](char c){return std::isspace<char>(c, std::locale::classic());};

  int lineNo {0};
  int nErrors {0};

  for(std::string line; std::getline(file, line);){
    ++lineNo;

    line.erase(std::remove_if(line.begin(), line.end(), isSpace), line.end());

    if(line.front() == comment) 
      continue;
     
    int count {0};
    count = std::count(line.begin(), line.end(), seperator);
    if(count != 1){
      log::log(log::ERROR, log::msg_rcfile_malformed, filename); 
      log::log(log::INFO, log::msg_on_line, lineNoToString(lineNo) + line);
      log::log(log::INFO, log::msg_rcfile_excess_seperators, std::to_string(count));
      log::log(log::INFO, log::msg_ignoring_line);
      ++nErrors;
      continue;
    }

    std::size_t pos = line.find_first_of(seperator);
    std::string name {line.substr(0, pos)};
    std::string value {line.substr(pos + 1)};

    if(name.empty() || value.empty()){
      log::log(log::ERROR, log::msg_rcfile_malformed, filename); 
      log::log(log::INFO, log::msg_on_line, lineNoToString(lineNo) + line);
      log::log(log::INFO, log::msg_rcfile_malformed_property);
      log::log(log::INFO, log::msg_ignoring_line);
      ++nErrors;
      continue;
    }

    Property* p {nullptr};
    for(auto& pair : _properties){
      if(pair.second._name == name){
        p = &pair.second;
        break;
      }
    }

    if(p == nullptr){
      log::log(log::ERROR, log::msg_rcfile_malformed, filename); 
      log::log(log::INFO, log::msg_on_line, lineNoToString(lineNo) + line);
      log::log(log::INFO, log::msg_rcfile_unknown_property, name);
      log::log(log::INFO, log::msg_ignoring_line);
      continue;
    }

    switch(p->_default.index()){
      case 0:
        {
          int result {0};
          if(!parseInt(value, result)){
            log::log(log::ERROR, log::msg_rcfile_malformed, filename); 
            log::log(log::INFO, log::msg_on_line, lineNoToString(lineNo) + line);
            log::log(log::INFO, log::msg_rcfile_expected_int, value);
            log::log(log::INFO, log::msg_ignoring_line);
            ++nErrors;
            continue;
          }
          p->_value = std::clamp(result, std::get<int>(p->_min), std::get<int>(p->_max));
          if(std::get<int>(p->_value) != result){
            log::log(log::INFO, log::msg_rcfile_property_clamped, name);
          }
          log::log(log::INFO, log::msg_rcfile_property_read_success, filename + lineNoToString(lineNo) + line);
          break;
        }
      case 1:
        {
          float result {0.f};
          if(!parseFloat(value, result)){
            log::log(log::ERROR, log::msg_rcfile_malformed, filename); 
            log::log(log::INFO, log::msg_on_line, lineNoToString(lineNo) + line);
            log::log(log::INFO, log::msg_rcfile_expected_float, value);
            log::log(log::INFO, log::msg_ignoring_line);
            ++nErrors;
            continue;
          }
          p->_value = std::clamp(result, std::get<float>(p->_min), std::get<float>(p->_max));
          if(std::get<float>(p->_value) != result){
            log::log(log::INFO, log::msg_rcfile_property_clamped, name);
          }
          log::log(log::INFO, log::msg_rcfile_property_read_success, filename + lineNoToString(lineNo) + line);
          break;
        }
      case 2:
        {
          bool result {false};
          if(!parseBool(value, result)){
            log::log(log::ERROR, log::msg_rcfile_malformed, filename); 
            log::log(log::INFO, log::msg_on_line, lineNoToString(lineNo) + line);
            log::log(log::INFO, log::msg_rcfile_expected_bool, value);
            log::log(log::INFO, log::msg_ignoring_line);
            ++nErrors;
            continue;
          }
          p->_value = result;
          log::log(log::INFO, log::msg_rcfile_property_read_success, filename + lineNoToString(lineNo) + line);
          break;
        }
    }
  }

  for(auto& pair : _properties){
    if(pair.second._value == unsetValue){
      log::log(log::WARN, log::msg_rcfile_property_not_set, pair.second._name);
      log::log(log::INFO, log::msg_rcfile_using_property_default);
      pair.second._value = pair.second._default;
      ++nErrors;
    }
  }

  if(nErrors > 0)
    log::log(log::WARN, log::msg_rcfile_errors, std::to_string(nErrors));

  return nErrors;
}

bool RC::write(const std::string& filename, bool genComments)
{
  std::ofstream file {filename, std::ios_base::out | std::ios_base::trunc};
  if(!file){
    log::log(log::WARN, log::msg_rcfile_fail_create, filename);
    return false;
  }

  for(const auto& pair : _properties){
    const Property& p = pair.second;
    
    std::stringstream ss {};

    if(genComments){
      ss << comment;
      ss << " default=";
      printValue(p._default, ss);
      ss << " min=";
      printValue(p._min, ss);
      ss << " max=";
      printValue(p._max, ss);
      ss << '\n';
      file << ss.str();
    }

    std::stringstream().swap(ss);

    ss << p._name << "=";
    printValue(p._value, ss);
    ss << '\n';
    file << ss.str();
  }

  return true;
}

int RC::getIntValue(Key_t key) const
{
  assert(_properties.at(key)._default.index() == 0);
  return std::get<int>(_properties.at(key)._value);
}

float RC::getFloatValue(Key_t key) const
{
  assert(_properties.at(key)._default.index() == 1);
  return std::get<float>(_properties.at(key)._value);
}

bool RC::getBoolValue(Key_t key) const
{
  assert(_properties.at(key)._default.index() == 2);
  return std::get<bool>(_properties.at(key)._value);
}

void RC::setIntValue(Key_t key, int value)
{
  assert(_properties.at(key)._default.index() == 0);
  _properties[key]._value = value;
}

void RC::setFloatValue(int key, float value)
{
  assert(_properties.at(key)._default.index() == 1);
  _properties[key]._value = value;
}

void RC::setBoolValue(int key, bool value)
{
  assert(_properties.at(key)._default.index() == 2);
  _properties[key]._value = value;
}

void RC::applyDefaults()
{
  for(auto& pair : _properties){
    pair.second._value = pair.second._default;
  }
}

bool RC::parseInt(const std::string& value, int& result)
{
  auto isDigit = [](unsigned char c){return std::isdigit(c);};
  auto isSign = [](unsigned char c){return c == '+' || c == '-';};

  int nSigns = std::count_if(value.begin(), value.end(), isSign);
  if(nSigns > 1){
    return false;
  }
  else if(nSigns == 1 && isSign(value.front()) == false){
      return false;
  }

  int count = std::count_if(value.begin(), value.end(), isDigit);
  if(count != value.length() - nSigns){
    return false;
  }

  result = std::stoi(value);
  return true;
}

bool RC::parseFloat(const std::string& value, float& result)
{
  auto isDigit = [](unsigned char c){return std::isdigit(c);};
  auto isSign = [](unsigned char c){return c == '+' || c == '-';};

  int nSigns = std::count_if(value.begin(), value.end(), isSign);
  if(nSigns > 1){
    return false;
  }
  else if(nSigns == 1 && isSign(value.front()) == false){
      return false;
  }

  int nPoints = std::count(value.begin(), value.end(), '.');
  if(nPoints > 1)
    return false;

  int count = std::count_if(value.begin(), value.end(), isDigit);
  if(count != value.length() - nPoints - nSigns)
    return false;

  result = std::stof(value);
  return true;
}

bool RC::parseBool(const std::string& value, bool& result)
{
  if(value == "true"){
    result = true;
    return true;
  }
  else if(value == "false"){
    result = false;
    return true;
  }

  return false;
}

void RC::printValue(const Value_t& value, std::ostream& os)
{
  switch(value.index()){
    case 0:
      os << std::get<int>(value);
      break;
    case 1:
      os << std::get<float>(value);
      break;
    case 2:
      os << (std::get<bool>(value) ? "true" : "false");
      break;
  }
}

} // namespace io
} // namespace pxr
