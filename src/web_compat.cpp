#include "web_compat.h"
#include "native_document.h"
#include "../vendor/duktape/duktape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* catalogSite="https://x360hub.vercel.app/ports.html";
static const char* catalogOrigin="https://x360hub.vercel.app/";
bool browserCompatCatalogURL(const char* url){
    if(!url || _strnicmp(url,catalogSite,strlen(catalogSite)))return false;
    char ch=url[strlen(catalogSite)];
    return ch==0 || ch=='?' || ch=='#';
}
static duk_ret_t safelyParse(duk_context* ctx,void*){
    duk_json_decode(ctx,0);
    return 1;
}
static std::string escaped(const std::string& value){
    std::string result;
    for(size_t i=0;i<value.size();++i){
        switch(value[i]){
        case '&':result+="&amp;";break;
        case '<':result+="&lt;";break;
        case '>':result+="&gt;";break;
        case '"':result+="&quot;";break;
        case '\'':result+="&#39;";break;
        default:result+=value[i];
        }
    }
    return result;
}
static std::string property(duk_context* ctx,const char* key){
    duk_get_prop_string(ctx,-1,key);
    std::string s;
    if(duk_is_string(ctx,-1)){
        duk_size_t length=0;const char* text=duk_get_lstring(ctx,-1,&length);
        if(text && length<1024)s.assign(text,length);
    }
    duk_pop(ctx);
    return s;
}
static bool httpsURL(const char* base,const std::string& value,std::string& result){
    if(value.empty()||value.size()>=WEB_URL)return false;
    char resolved[WEB_URL];
    if(!webResolveURL(base,value.c_str(),resolved,sizeof(resolved)))return false;
    result.assign(resolved);
    return true;
}
static int pageOf(const char* url){
    const char* found=strstr(url,"x360page=");
    if(!found)return 1;
    int page=atoi(found+9);
    return page>=1&&page<=100?page:1;
}
bool browserCompatRenderCatalog(const char* url,const char* json,int bytes,std::string& html,int* totalItems){
    if(totalItems)*totalItems=0;
    html.clear();
    if(!browserCompatCatalogURL(url)||!json||bytes<10||bytes>512*1024)return false;
    duk_context* ctx=duk_create_heap_default();
    if(!ctx)return false;
    duk_push_lstring(ctx,json,bytes);
    bool parsed=duk_safe_call(ctx,safelyParse,NULL,1,1)==DUK_EXEC_SUCCESS && duk_is_object(ctx,-1);
    if(!parsed){duk_destroy_heap(ctx);return false;}
    duk_get_prop_string(ctx,-1,"projects");
    if(!duk_is_array(ctx,-1)){duk_destroy_heap(ctx);return false;}
    int total=(int)duk_get_length(ctx,-1);
    if(total<1||total>300){duk_destroy_heap(ctx);return false;}
    if(totalItems)*totalItems=total;
    const int perPage=8;
    int pages=(total+perPage-1)/perPage;
    int page=pageOf(url);
    if(page>pages)page=pages;
    char tmp[256];
    html="<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>X360HUB Catalog - Native Compatibility View</title>";
    html+="<style>body{margin:0;background:#07110b;color:#e8f4ec;font-family:Arial;font-size:18px;}"
          ".wrap{padding:22px 34px;background:#07110b;}h1{font-size:32px;color:#b9efc9;margin:6px 0 12px;}"
          "p{color:#b5c8bb;}a{color:#71e5a8;text-decoration:none;}h2{font-size:21px;color:#e8fff1;}"
          ".grid{display:grid;grid-template-columns:repeat(2,1fr);}"
          ".card{background:#173b2b;padding:12px;margin:8px;border:1px solid #33754c;}"
          ".card img{width:100%;height:155px;object-fit:cover;}"
          ".nav a{font-size:20px;padding:12px;background:#204a32;margin:10px;}"
          ".pill{color:#a7d8b8;font-size:15px;}</style></head><body><div class='wrap'>";
    html+="<h1>X360HUB / Ports &amp; Homebrew</h1>"
          "<p>Live Xbox 360 catalog, displayed directly from X360HUB data. "
          "Compatibility view: this browser cannot execute the website's modern scripts.</p>";
    sprintf_s(tmp,sizeof(tmp),"%d projects | Page %d of %d",total,page,pages);
    html+="<p class='pill'>";html+=tmp;html+="</p><div class='grid'>";
    int included=0;
    for(int i=(page-1)*perPage;i<total && included<perPage;++i){
        duk_get_prop_index(ctx,-1,i);
        if(!duk_is_object(ctx,-1)){duk_pop(ctx);continue;}
        std::string title=property(ctx,"title"),description=property(ctx,"description");
        std::string image=property(ctx,"imageHorizontal"),repository=property(ctx,"repositoryUrl");
        std::string download=property(ctx,"downloadUrl"),category=property(ctx,"category");
        duk_pop(ctx);
        if(title.empty())continue;
        std::string imgURL,sourceURL,downloadURL;
        if(!image.empty() && httpsURL(catalogOrigin,image,imgURL)){
            // Only show images hosted by the real catalog origin.
            if(imgURL.compare(0,strlen(catalogOrigin),catalogOrigin))imgURL.clear();
        }
        httpsURL(catalogOrigin,repository,sourceURL);
        httpsURL(catalogOrigin,download,downloadURL);
        html+="<div class='card'>";
        if(!imgURL.empty())html+="<img src='"+escaped(imgURL)+"' width='440' height='155' alt='"+escaped(title)+"'>";
        html+="<h2>"+escaped(title)+"</h2>";
        if(!category.empty())html+="<p class='pill'>"+escaped(category)+"</p>";
        if(!description.empty())html+="<p>"+escaped(description)+"</p>";
        if(!sourceURL.empty())html+="<p><a href='"+escaped(sourceURL)+"'>Source / Repository</a></p>";
        if(!downloadURL.empty())html+="<p><a href='"+escaped(downloadURL)+"'>Original download link</a></p>";
        html+="</div>";++included;
    }
    html+="</div><p class='nav'>";
    if(page>1){sprintf_s(tmp,sizeof(tmp),"%s?x360page=%d",catalogSite,page-1);html+="<a href='"+std::string(tmp)+"'>Previous page</a>";}
    if(page<pages){sprintf_s(tmp,sizeof(tmp),"%s?x360page=%d",catalogSite,page+1);html+="<a href='"+std::string(tmp)+"'>Next page</a>";}
    html+="</p><p><a href='https://x360hub.vercel.app/'>X360HUB home</a></p></div></body></html>";
    duk_destroy_heap(ctx);
    return included>0 && html.size()<WEB_HTML_LIMIT;
}
