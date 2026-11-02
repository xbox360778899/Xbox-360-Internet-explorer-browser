// Google /search sometimes returns no HTML results and an enable-JavaScript page.
// Preserve that fact. Never present third-party results as Google results.
#include "search_results.h"
#include "native_document.h"
#include <string>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool searchBegins(const char* url,const char* prefix){
    return url && !_strnicmp(url,prefix,strlen(prefix));
}
static bool searchParam(const char* url,const char* name,std::string& output){
    output.clear();if(!url)return false;
    const char* q=strchr(url,'?');if(!q)return false;
    const size_t length=strlen(name);
    for(++q;*q && *q!='#';){
        const char* end=strchr(q,'&');
        if(!end)end=q+strlen(q);
        if((size_t)(end-q)>length && !_strnicmp(q,name,length) && q[length]=='='){
            output.assign(q+length+1,end);
            return true;
        }
        if(!*end)break;
        q=end+1;
    }
    return false;
}
bool browserIsGoogleSearchURL(const char* url){
    return searchBegins(url,"https://www.google.com/search?") ||
           searchBegins(url,"https://google.com/search?");
}
bool browserIsBingRSSURL(const char* url){
    if(!searchBegins(url,"https://www.bing.com/search?"))return false;
    std::string format;
    return searchParam(url,"format",format) && format=="rss";
}
bool browserGoogleRequiresJS(const char* html){
    if(!html)return false;
    return strstr(html,"/httpservice/retry/enablejs")!=NULL &&
           (strstr(html,"<title>Google Search</title>")!=NULL ||
            strstr(html,"<title>Google</title>")!=NULL);
}
static std::string escaped(const std::string& s){
    std::string out;
    for(size_t i=0;i<s.size();++i)switch(s[i]){
        case '&':out+="&amp;";break;
        case '<':out+="&lt;";break;
        case '>':out+="&gt;";break;
        case '"':out+="&quot;";break;
        case 39:out+="&#39;";break;
        default:if((unsigned char)s[i]>=32 || s[i]=='\n')out+=s[i];break;
    }
    return out;
}
static std::string css(){
    return "<style>html,body{background:#07140e;color:#e6f5e9;font-family:Arial;font-size:20px;margin:0;min-height:800px;}"
           ".wrap{padding:32px 48px;min-height:740px;}h1{font-size:34px;color:#abf3c9;}h2{font-size:24px;color:#e5fff0;}"
           "p{font-size:19px;color:#bdd5c5;line-height:1.5;}"
           ".entry{background:#173c2a;border:1px solid #2c7050;margin:14px 0;padding:18px;}"
           ".entry p{margin:8px 0;color:#c5d8cc;}a{color:#85e7b0;text-decoration:underline;}"
           ".action{display:block;background:#286342;padding:16px;margin:20px 0;}"
           ".note{color:#ffe7a6;}small{color:#adccba;}</style>";
}
static bool safeQuery(const std::string& q){
    if(q.empty() || q.size()>1200)return false;
    for(size_t i=0;i<q.size();++i){
        unsigned char c=(unsigned char)q[i];
        if(!isalnum(c) && c!='%' && c!='+' && c!='_' && c!='-' && c!='.' && c!='~')return false;
        if(c=='%' && (i+2>=q.size() || !isxdigit((unsigned char)q[i+1]) ||
            !isxdigit((unsigned char)q[i+2])))return false;
    }
    return true;
}
static std::string decodeQuery(const std::string& q){
    std::string result;
    for(size_t i=0;i<q.size()&&result.size()<300;++i){
        unsigned char c=(unsigned char)q[i];
        if(c=='+')result+=' ';
        else if(c=='%'&&i+2<q.size()){
            char number[3]={q[i+1],q[i+2],0};
            unsigned char decoded=(unsigned char)strtoul(number,NULL,16);
            if(decoded>=32 && decoded!=127)result+=(char)decoded;
            i+=2;
        }else result+=(char)c;
    }
    return result;
}
bool browserGoogleSearchNotice(const char* googleURL,std::string& html){
    html.clear();if(!browserIsGoogleSearchURL(googleURL))return false;
    std::string query;
    searchParam(googleURL,"q",query);
    html="<html><head><meta charset='UTF-8'><title>Google requires JavaScript - X360Browser</title>"+css()+
         "</head><body><div class='wrap'><h1>Google Search - JavaScript required</h1>";
    html+="<p>The actual Google response did not contain search results. It requested a modern JavaScript-enabled browser. X360Browser cannot run this Google page yet.</p>";
    if(safeQuery(query)){
        html+="<p>Search requested: <strong>"+escaped(decodeQuery(query))+"</strong></p>";
        std::string alternate="https://www.bing.com/search?q="+query+"&format=rss&mkt=en-US";
        html+="<a class='action' href='"+escaped(alternate)+"'>Search with Bing instead (real results, different provider)</a>";
        html+="<p class='note'>Bing is NOT Google. Selecting this link sends your search terms to Bing. Nothing is sent to Bing unless you select it.</p>";
    }
    html+="<a href='"+escaped(googleURL)+"'>Retry Google</a>";
    html+="<p><small>No search results have been invented. Press B to go back or Y to enter another search.</small></p></div></body></html>";
    return html.size()<WEB_HTML_LIMIT;
}
static std::string part(const std::string& src,const char* name){
    std::string a="<";a+=name;a+=">";
    std::string b="</";b+=name;b+=">";
    size_t start=src.find(a);if(start==std::string::npos)return "";
    start+=a.size();size_t end=src.find(b,start);
    if(end==std::string::npos || end-start>4090)return "";
    return src.substr(start,end-start);
}
static std::string decodeXML(const std::string& src){
    char out[4096]={0};
    if(src.size()>=sizeof(out))return "";
    webDecode(src.c_str(),out,sizeof(out));
    return std::string(out);
}
static std::string plainText(const std::string& src){
    std::string out;
    bool inTag=false;
    for(size_t i=0;i<src.size() && out.size()<800;++i){
        char c=src[i];
        if(c=='<'){inTag=true;continue;}
        if(c=='>' && inTag){inTag=false;continue;}
        if(!inTag && ((unsigned char)c>=32 || c=='\n'))out+=c;
    }
    return out;
}
bool browserRenderBingRSS(const char* url,const char* rss,int size,std::string& html,int* items){
    html.clear();if(items)*items=0;
    if(!browserIsBingRSSURL(url) || !rss || size<80 || size>512*1024)return false;
    std::string feed(rss,(size_t)size);
    if(feed.find("<rss")==std::string::npos || feed.find("<channel>")==std::string::npos)return false;
    html="<html><head><meta charset='UTF-8'><title>Bing Results - X360Browser</title>"+css()+
          "</head><body><div class='wrap'><h1>Live search results - Bing</h1>"
          "<p>Results loaded from Bing RSS directly on your Xbox. These are NOT Google results.</p>";
    std::string q;
    if(searchParam(url,"q",q) && safeQuery(q))
        html+="<p>Search: <strong>"+escaped(decodeQuery(q))+"</strong></p>";
    size_t start=0;int found=0;
    while(found<12 && start<feed.size()){
        start=feed.find("<item>",start);
        if(start==std::string::npos)break;
        size_t finish=feed.find("</item>",start+6);
        if(finish==std::string::npos || finish-start>16384)break;
        std::string item=feed.substr(start+6,finish-start-6);
        start=finish+7;
        std::string title=plainText(decodeXML(part(item,"title")));
        std::string link=decodeXML(part(item,"link"));
        std::string desc=plainText(decodeXML(part(item,"description")));
        char verified[WEB_URL]={0};
        if(title.empty() || !webResolveURL("https://www.bing.com/",link.c_str(),verified,sizeof(verified)))continue;
        html+="<div class='entry'><h2><a href='"+escaped(verified)+"'>"+escaped(title)+"</a></h2>";
        if(!desc.empty())html+="<p>"+escaped(desc)+"</p>";
        html+="<small>"+escaped(verified)+"</small></div>";
        ++found;
    }
    if(items)*items=found;
    html+="<p><a href='https://www.google.com/?hl=en'>Google home</a></p></div></body></html>";
    return found>0 && html.size()<WEB_HTML_LIMIT;
}
