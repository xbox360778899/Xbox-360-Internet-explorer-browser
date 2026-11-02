#pragma once
#include <string>
// Explicitly display Google's genuine JS-required interstitial instead of a blank screen.
// Bing RSS is a separate, opt-in provider triggered only after the user clicks its link.
bool browserIsGoogleSearchURL(const char* url);
bool browserIsBingRSSURL(const char* url);
bool browserGoogleRequiresJS(const char* html);
bool browserGoogleSearchNotice(const char* googleURL,std::string& html);
bool browserRenderBingRSS(const char* url,const char* rss,int size,std::string& html,int* items);
