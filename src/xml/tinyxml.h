#ifndef NPP_PLUGIN_TINYXML_H
#define NPP_PLUGIN_TINYXML_H

#include "tinyxml2.h"

typedef tinyxml2::XMLHandle    tXmlHnd;
typedef tinyxml2::XMLDocument  tXmlDoc;
typedef tinyxml2::XMLDocument* tXmlDocP;
typedef tinyxml2::XMLElement*  tXmlEleP;
typedef tinyxml2::XMLError     tXmlError;
const INT kXmlSuccess = tinyxml2::XML_SUCCESS;

#endif // NPP_PLUGIN_TINYXML_H
