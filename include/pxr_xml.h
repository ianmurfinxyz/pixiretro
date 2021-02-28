#ifndef _PIXIRETRO_IO_XML_H_
#define _PIXIRETRO_IO_XML_H_

#include <string>
#include "lib/tinyxml2/tinyxml2.h"

namespace pxr
{
namespace io
{

using namespace tinyxml2;

static constexpr const char* XML_FILE_EXTENSION {".xml"};

//
// Helper which wraps parsing an xml file via the tinyxml2 library. The wrapper handles
// errors and returns true/false to indicate parsing status. Errors are logged to the 
// engine log.
//
bool parseXmlDocument(XMLDocument* doc, const std::string& xmlpath);

//
// Helper which wraps extracting a child element of an xml element. The wrapper handles
// errors and returns true/false to indicate extraction status. Errors are logged to the
// engine log.
//
// Arg 'parent' must be a valid (non-null) xml element from which to extract the child
// element with name 'childname'.
//
bool extractChildElement(XMLNode* parent, XMLElement** child, const char* childname);

//
// Helpers which wrap extracting attributes of an xml element. The wrapper handles
// errors and returns true/false to indicate extraction status. Errors are logged to the
// engine log.
//
// Arg 'element' must be a valid (non-null) xml element.
//
bool extractIntAttribute(XMLElement* element, const char* attribute, int* value);
bool extractFloatAttribute(XMLElement* element, const char* attribute, float* value);
bool extractStringAttribute(XMLElement* element, const char* attribute, const char** value);

} // namespace io
} // namespace pxr

#endif
