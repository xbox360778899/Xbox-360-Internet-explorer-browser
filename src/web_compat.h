#pragma once
#include <string>
// Live, on-device compatibility view for the JS-only X360HUB catalog.
// Not a general JavaScript/DOM renderer and never presents synthetic results as the site.
bool browserCompatCatalogURL(const char* url);
bool browserCompatRenderCatalog(const char* url,const char* json,int bytes,std::string& html,int* totalItems);
