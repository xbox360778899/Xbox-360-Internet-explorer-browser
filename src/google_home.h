#pragma once
#include <string>
// Google landing page uses genuine live Google HTML/CSS and its published logo.
// Only legacy layout compatibility CSS is added; search input uses Xbox VKBD.
bool browserGoogleHomeURL(const char* url);
bool browserGooglePrepareHTML(const char* url,const char* markup,std::string& output);
