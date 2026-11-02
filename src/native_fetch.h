#pragma once
#include "../vendor/zlib/zlib.h"
enum {WEB_DOWNLOAD_HTML=0,WEB_DOWNLOAD_IMAGE=1,WEB_DOWNLOAD_VIDEO=2};
struct WebDownload {unsigned char* bytes;DWORD size;int kind;char url[WEB_URL],type[128];bool truncated,cacheHit;};
// A small in-memory cache: bounded, expiring, and restricted to public resources.
// No disk access, service, or external computer is required.
struct BrowserCacheEntry {char key[WEB_URL],url[WEB_URL],type[128];unsigned char* bytes;DWORD size,time;int kind;bool truncated;};
static BrowserCacheEntry browserCache[16];
static DWORD browserCacheBytes=0;
static void webCacheDrop(BrowserCacheEntry* e){
    if(e->bytes){browserCacheBytes-=e->size;free(e->bytes);}
    memset(e,0,sizeof(*e));
}
static void webCacheForget(const char* key){
    for(int i=0;i<16;++i)if(browserCache[i].bytes&&!strcmp(browserCache[i].key,key))webCacheDrop(&browserCache[i]);
}
static bool webCacheLookup(const char* key,WebDownload* output,bool imageOnly){
    if(strchr(key,'?'))return false;DWORD now=GetTickCount();
    for(int i=0;i<16;++i){BrowserCacheEntry* e=&browserCache[i];
        if(!e->bytes||strcmp(e->key,key))continue;
        if(now-e->time>180000){webCacheDrop(e);return false;}
        if(imageOnly&&e->kind!=WEB_DOWNLOAD_IMAGE)return false;
        output->bytes=(unsigned char*)malloc(e->size+1);if(!output->bytes)return false;
        memcpy(output->bytes,e->bytes,e->size);output->bytes[e->size]=0;
        output->size=e->size;output->kind=e->kind;output->truncated=e->truncated;output->cacheHit=true;
        strcpy_s(output->url,e->url);strcpy_s(output->type,e->type);
        return true;
    }
    return false;
}
static void webCacheStore(const char* key,const WebDownload* in){
    if(!in->bytes||!in->size||in->kind==WEB_DOWNLOAD_VIDEO||in->truncated||strchr(key,'?'))return;
    DWORD maxItem=in->kind==WEB_DOWNLOAD_IMAGE?2*1024*1024:512*1024;
    if(in->size>maxItem || in->size>6*1024*1024)return;
    webCacheForget(key);int slot=-1;DWORD oldest=0;DWORD now=GetTickCount();
    for(int i=0;i<16;++i){
        if(!browserCache[i].bytes){slot=i;break;}
        DWORD age=now-browserCache[i].time;if(slot<0||age>oldest){slot=i;oldest=age;}
    }
    while(browserCacheBytes+in->size>6*1024*1024){
        int victim=-1;oldest=0;for(int i=0;i<16;++i){if(!browserCache[i].bytes)continue;DWORD age=now-browserCache[i].time;if(victim<0||age>oldest){victim=i;oldest=age;}}
        if(victim<0)return;webCacheDrop(&browserCache[victim]);
    }
    BrowserCacheEntry* e=&browserCache[slot];webCacheDrop(e);
    e->bytes=(unsigned char*)malloc(in->size);if(!e->bytes)return;
    memcpy(e->bytes,in->bytes,in->size);e->size=in->size;e->time=now;e->kind=in->kind;e->truncated=in->truncated;
    strcpy_s(e->key,key);strcpy_s(e->url,in->url);strcpy_s(e->type,in->type);browserCacheBytes+=in->size;
}
static bool webVideoExtension(const char* url){const char* end=strchr(url,'?');size_t n=end?(size_t)(end-url):strlen(url);return n>=4 && (webPrefix(url+n-4,".wmv")||webPrefix(url+n-4,".xmv"));}
static bool webDownload(const char* url,WebDownload* result,char* message,size_t messageSize,bool imageOnly=false,bool resourceOnly=false,DWORD maxIdleMs=0) {
    memset(result,0,sizeof(*result));if(webCacheLookup(url,result,imageOnly))return true;
    const char* accept=resourceOnly?"text/css,*/*;q=0.7":imageOnly?"image/png,image/jpeg,image/gif,image/bmp,*/*;q=0.5":"text/html,application/xhtml+xml,text/plain;q=0.8";
    DWORD status=0;HubTLSRequest* request=hubNetOpen(url,&status,maxIdleMs?maxIdleMs:(resourceOnly?5500:10000),accept);
    if(!request){errorMessage(message,messageSize,GetLastError());return false;}
    strncpy_s(result->url,sizeof(result->url),request->finalURL,_TRUNCATE);strcpy_s(result->type,request->contentType);
    bool image=webPrefix(request->contentType,"image/");
    bool video=webVideoExtension(url)||webPrefix(request->contentType,"video/x-ms-wmv")||webPrefix(request->contentType,"video/x-ms-asf");
    if(webPrefix(request->contentType,"video/") && !video){strcpy_s(message,messageSize,"This video format needs another decoder. This preview supports direct WMV/XMV, not MP4/HLS/DRM.");hubNetClose(request);return false;}
    if(imageOnly && !image){strcpy_s(message,messageSize,"The image server did not return an image.");hubNetClose(request);return false;}
    if(!image && !video && request->contentType[0] && !webPrefix(request->contentType,"text/")&&!webPrefix(request->contentType,"application/xhtml+xml")&&!(resourceOnly&&(webPrefix(request->contentType,"application/css")||webPrefix(request->contentType,"application/x-css")||webPrefix(request->contentType,"application/json")||webPrefix(request->contentType,"application/javascript")||webPrefix(request->contentType,"application/x-javascript")||webPrefix(request->contentType,"application/octet-stream")))){strcpy_s(message,messageSize,"Unsupported content type. Open HTML, a supported image, or a direct WMV/XMV file.");hubNetClose(request);return false;}
    result->kind=video?WEB_DOWNLOAD_VIDEO:image?WEB_DOWNLOAD_IMAGE:WEB_DOWNLOAD_HTML;
    DWORD limit=video?64*1024*1024:image?2*1024*1024:WEB_HTML_LIMIT;
    if(request->lengthKnown && request->length>limit && result->kind!=WEB_DOWNLOAD_HTML){sprintf_s(message,messageSize,"Media exceeds this preview's %lu MB limit.",limit/(1024*1024));hubNetClose(request);return false;}
    DWORD capacity=request->lengthKnown && request->length<limit?request->length:limit;
    result->bytes=(unsigned char*)malloc(capacity+1);if(!result->bytes){strcpy_s(message,messageSize,"Not enough memory for this media file.");hubNetClose(request);return false;}
    bool ok=true;
    while(result->size<capacity && !webNetState.cancel){DWORD count=0,want=capacity-result->size;if(want>32768)want=32768;if(!hubNetRead(request,result->bytes+result->size,want,&count)){ok=false;break;}if(!count)break;result->size+=count;}
    if(webNetState.cancel)ok=false;
    if(result->size==capacity && (!request->lengthKnown || request->length>capacity)){
        if(result->kind==WEB_DOWNLOAD_HTML)result->truncated=true;
        else {DWORD extra=0;unsigned char byte;if(!hubNetRead(request,&byte,1,&extra)||extra)ok=false;}
    }
    if(ok && request->contentEncoding[0] && _stricmp(request->contentEncoding,"identity")){
        DWORD maxOutput=result->kind==WEB_DOWNLOAD_IMAGE?2*1024*1024:
                        result->kind==WEB_DOWNLOAD_VIDEO?64*1024*1024:WEB_HTML_LIMIT;
        unsigned char* inflated=(unsigned char*)malloc(maxOutput+1);
        if(!inflated){ok=false;strcpy_s(message,messageSize,"Not enough RAM to decompress website.");}
        else {
            z_stream z;memset(&z,0,sizeof(z));
            int format=!_stricmp(request->contentEncoding,"gzip")?15+16:15;
            int err=inflateInit2(&z,format);
            if(err==Z_OK){
                z.next_in=(Bytef*)result->bytes;z.avail_in=result->size;
                z.next_out=inflated;z.avail_out=maxOutput;
                err=inflate(&z,Z_FINISH);
                inflateEnd(&z);
            }
            if(err!=Z_STREAM_END || z.total_out>maxOutput){
                ok=false;strcpy_s(message,messageSize,"Compressed response invalid or above memory limit.");
            }else{
                free(result->bytes);result->bytes=inflated;
                result->size=(DWORD)z.total_out;result->bytes[result->size]=0;inflated=NULL;
            }
            free(inflated);
        }
    }
    if(!ok){
        if(!message[0])errorMessage(message,messageSize,hubNetError(request));
        free(result->bytes);result->bytes=NULL;
    }else result->bytes[result->size]=0;
    // Respect explicit privacy/revalidation directives; never cache multimedia or query URLs.
    bool permitted=!webFind(request->cacheControl,"no-store")&&!webFind(request->cacheControl,"private")&&!webFind(request->cacheControl,"no-cache")&&!webFind(request->cacheControl,"max-age=0");
    if(ok&&permitted)webCacheStore(url,result);
    hubNetClose(request);return ok;
}
static bool webJavaScriptResourceFetch(void*,const char* url,unsigned char** bytes,int* size,char* finalURL,int capacity){
    if(webNetState.cancel)return false;
    WebDownload dl={0};char message[192]="";
    if(!webDownload(url,&dl,message,sizeof(message),false,true,3000))return false;
    bool valid=dl.kind==WEB_DOWNLOAD_HTML && dl.size<=128*1024 &&
       (webPrefix(dl.type,"application/javascript") || webPrefix(dl.type,"text/javascript") ||
        webPrefix(dl.type,"application/x-javascript") || webPrefix(dl.type,"text/plain"));
    if(!valid){free(dl.bytes);return false;}
    *bytes=dl.bytes;*size=(int)dl.size;strncpy_s(finalURL,capacity,dl.url,_TRUNCATE);return true;
}
static bool webStyledResourceFetch(void*,const char* url,unsigned char** bytes,int* size,char* finalURL,int capacity) {
    if(webNetState.cancel)return false;WebDownload download={0};char message[192];
    if(!webDownload(url,&download,message,sizeof(message),false,true))return false;
    if(download.kind!=WEB_DOWNLOAD_HTML||download.size>512*1024||(!webPrefix(download.type,"text/css")&&!webPrefix(download.type,"text/plain")&&!webPrefix(download.type,"application/css")&&!webPrefix(download.type,"application/x-css")&&!webPrefix(download.type,"application/octet-stream"))){free(download.bytes);return false;}
    *bytes=download.bytes;*size=(int)download.size;strncpy_s(finalURL,capacity,download.url,_TRUNCATE);return true;
}
