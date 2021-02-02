#ifndef _PIXIRETRO_XMLUTIL_H_
#define _PIXIRETRO_XMLUTIL_H_

#include "tinyxml2.h" // TODO move to lib dir.

namespace pxr
{

using namespace tinyxml2;

bool parseXmlDocument(XMLDocument* doc, const std::string& xmlpath);
bool extractChildElement(XMLNode* parent, XMLElement** child, const char* childname);
bool extractIntAttribute(XMLElement* element, const char* attribute, int* value);

} // namespace pxr

#endif
