#pragma once
#include <string>
// Bounded ES5 interpreter with a small DOM adapter; no full browser DOM.
typedef bool (*BrowserJSFetch)(void*,const char*,unsigned char**,int*,char*,int);
bool browserJSPreprocess(const char* html,const char* baseURL,std::string& output,int* ran,int* failed,
                         BrowserJSFetch fetch=NULL,void* user=NULL);
