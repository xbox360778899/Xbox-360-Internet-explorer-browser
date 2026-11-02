#include "google_home.h"
#include <string.h>
#include <ctype.h>
#include <string>
static bool beginsCI(const char* url,const char* prefix){
    while(*prefix){
        if(!*url || tolower((unsigned char)*url)!=tolower((unsigned char)*prefix))return false;
        ++url;++prefix;
    }
    return true;
}
bool browserGoogleHomeURL(const char* url){
    if(!url)return false;
    const char* prefixes[]={"https://www.google.com/","https://google.com/"};
    for(int i=0;i<2;++i){
        if(!beginsCI(url,prefixes[i]))continue;
        const char* rest=url+strlen(prefixes[i]);
        return !*rest || *rest=='?' || *rest=='#';
    }
    return false;
}
bool browserGooglePrepareHTML(const char* url,const char* markup,std::string& output){
    output.clear();
    if(!browserGoogleHomeURL(url)||!markup)return false;
    size_t length=strlen(markup);
    if(length<100 || length>1024*1024)return false;
    output.assign(markup,length);
    if(output.find("<title>Google</title>")==std::string::npos ||
       output.find("name=\"q\"")==std::string::npos ||
       output.find("id=\"hplogo\"")==std::string::npos)return false;
    size_t pos=output.find("</head>");
    if(pos==std::string::npos)return false;
    // Google's actual legacy home markup contains <center> and a native form input.
    // litehtml v0.3 does not implement either: center the original logo and
    // draw a native searchable control over the real form in the Xbox shell.
    const char* style="<style type=\"text/css\">"
      "center{display:block;text-align:center!important;width:100%;}"
      "center>div{text-align:center!important;}"
      "#hplogo{display:block;margin-left:auto!important;margin-right:auto!important;}"
      "form[name=f]{text-align:center!important;padding-bottom:90px!important;}"
      "form[name=f] input{display:none!important;}"
      "</style>";
    output.insert(pos,style);
    return true;
}
