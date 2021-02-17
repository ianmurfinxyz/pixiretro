#ifndef _PIXIRETRO_IO_RC_H_
#define _PIXIRETRO_IO_RC_H_

#include <unordered_map>
#include <initializer_list>
#include <variant>
#include <string>

namespace pxr
{
namespace io
{

//
// The directory on the filesystem where rc files are expected to be found w.r.t the app
// root directory.
//
static constexpr const char* RESOURCE_PATH_RC {"assets/rc/"};

//
// Represents a generic resource/config file which contains name=value property pairs. Property
// values can be of either int, float or bool type.
//
// To create a new rc file type derive from this base class and write a default constructor 
// which calls the protected initializer list constuctor. This default constructor should add
// the list of properties expected in the new rc file type.
//
// An rc file read from the file system need not include name=value pairs for all properties 
// supported in an rc file type as default values must be included for all properties when 
// deriving a new rc file type.
//
// Once a new file type is created and instantiated, call load to then load an rc file of this
// type from the file system. Use the accessors to set/get the property values read.
//
// Call write to output any changes made to property values back to the file system. Calling
// write can also be used to generate an rc file of this type in the file system if one does not
// exist. The generated file will contain name=value entries for all properties with value equal
// to the default value for the properties (if changes to the properties have not been made).
//
class RC
{
public:
  //
  // The expected rc file extension.
  //
  static constexpr const char* FILE_EXTENSION {".rc"};

  static constexpr char comment {'#'};     // prefix for comment lines in an rc file.
  static constexpr char seperator {'='};   // seperator used in name=value pairs.

  using Key_t = int;
  using Value_t = std::variant<int, float, bool>;

  //
  // When creating a new property it is a precondition that the 'default', 'min' and 'max' 
  // values be of the same type; this precondition is asserted true.
  //
  struct Property
  {
    Property() = default;
    Property(Key_t key, std::string name, Value_t default_, Value_t min, Value_t max); 

    Key_t _key;             // key used to access the property.
    std::string _name;      // name of the property in the file.
    Value_t _value;         // value of the property in the file.
    Value_t _default;       // default value for the property.
    Value_t _min;           // min value for the property.
    Value_t _max;           // max value for the property.
  };

public:

  //
  // Load and rc file from the filesystem. Expects to find the file in the RESOURCE_PATH_RC 
  // directory.
  //
  int load(const std::string& filename);

  //
  // Writes name=value pairs to a file for all properties. If genComments=true a comment
  // is generated for each property which informs of the default, min and max values.
  //
  bool write(const std::string& filepath, bool genComments = true);

  int getIntValue(Key_t key) const;
  float getFloatValue(Key_t key) const;
  bool getBoolValue(Key_t key) const;

  void setIntValue(Key_t key, int value);
  void setFloatValue(Key_t key, float value);
  void setBoolValue(Key_t key, bool value);

  //
  // Resets all properties to their default values.
  //
  void applyDefaults();

protected:

  //
  // Call this base constructor in the default constructor of the derived class to
  // add/enumerate the properties in the new rc file type being derived.
  //
  RC(std::initializer_list<Property> properties);

private:
  bool parseInt(const std::string& value, int& result);
  bool parseFloat(const std::string& value, float& result);
  bool parseBool(const std::string& value, bool& result);
  void printValue(const Value_t& value, std::ostream& os);
  
private:
  std::unordered_map<Key_t, Property> _properties;
};

} // namespace io
} // namespace pxr

#endif
