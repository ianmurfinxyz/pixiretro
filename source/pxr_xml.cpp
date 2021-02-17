#include "pxr_xml.h"
#include "pxr_log.h"

namespace pxr
{
namespace io
{

bool parseXmlDocument(XMLDocument* doc, const std::string& xmlpath)
{
  log::log(log::INFO, log::msg_xml_parsing, xmlpath);
  doc->LoadFile(xmlpath.c_str());
  if(doc->Error()){
    log::log(log::ERROR, log::msg_xml_fail_parse, xmlpath); 
    log::log(log::INFO, log::msg_xml_tinyxml_error_name, doc->ErrorName());
    log::log(log::INFO, log::msg_xml_tinyxml_error_desc, doc->ErrorStr());
    return false;
  }
  return true;
}

bool extractChildElement(XMLNode* parent, XMLElement** child, const char* childname)
{
  *child = parent->FirstChildElement(childname);
  if(*child == 0){
    log::log(log::ERROR, log::msg_xml_fail_read_element, childname);
    return false;
  }
  return true;
}

bool extractIntAttribute(XMLElement* element, const char* attribute, int* value)
{
  XMLError xmlerror = element->QueryIntAttribute(attribute, value);
  if(xmlerror != XML_SUCCESS){
    log::log(log::ERROR, log::msg_xml_fail_read_attribute, attribute);
    return false;
  }
  return true;
}

bool extractFloatAttribute(XMLElement* element, const char* attribute, float* value)
{
  XMLError xmlerror = element->QueryFloatAttribute(attribute, value);
  if(xmlerror != XML_SUCCESS){
    log::log(log::ERROR, log::msg_xml_fail_read_attribute, attribute);
    return false;
  }
  return true;
}

bool extractStringAttribute(XMLElement* element, const char* attribute, const char** value)
{
  XMLError xmlerror = element->QueryStringAttribute(attribute, value);
  if(xmlerror != XML_SUCCESS){
    log::log(log::ERROR, log::msg_xml_fail_read_attribute, attribute);
    return false;
  }
  return true;
}

} // namespace io
} // namespace pxr
