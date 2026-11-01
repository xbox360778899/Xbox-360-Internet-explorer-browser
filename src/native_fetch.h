#pragma once
enum {WEB_DOWNLOAD_HTML=0,WEB_DOWNLOAD_IMAGE=1,WEB_DOWNLOAD_VIDEO=2};
struct WebDownload {unsigned char* bytes;DWORD size;int kind;char url[WEB_URL],type[128];bool truncated;};
static bool webVideoExtension(const char* url){const char* end=strchr(url,'?');size_t n=end?(size_t)(end-url):strlen(url);return n>=4 && (webPrefix(url+n-4,".wmv")||webPrefix(url+n-4,".xmv"));}
static bool webDownload(const char* url,WebDownload* result,char* message,size_t messageSize,bool imageOnly=false) {
    memset(result,0,sizeof(*result));DWORD status=0;HubTLSRequest* request=hubNetOpen(url,&status);
    if(!request){errorMessage(message,messageSize,GetLastError());return false;}
    strncpy_s(result->url,sizeof(result->url),request->finalURL,_TRUNCATE);strcpy_s(result->type,request->contentType);
    bool image=webPrefix(request->contentType,"image/");
    bool video=webVideoExtension(url)||webPrefix(request->contentType,"video/x-ms-wmv")||webPrefix(request->contentType,"video/x-ms-asf");
    if(webPrefix(request->contentType,"video/") && !video){strcpy_s(message,messageSize,"This video format needs another decoder. This preview supports direct WMV/XMV, not MP4/HLS/DRM.");hubNetClose(request);return false;}
    if(imageOnly && !image){strcpy_s(message,messageSize,"The image server did not return an image.");hubNetClose(request);return false;}
    if(!image && !video && request->contentType[0] && !webPrefix(request->contentType,"text/")&&!webPrefix(request->contentType,"application/xhtml+xml")){strcpy_s(message,messageSize,"Unsupported content type. Open HTML, a supported image, or a direct WMV/XMV file.");hubNetClose(request);return false;}
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
    if(!ok){errorMessage(message,messageSize,hubNetError(request));free(result->bytes);result->bytes=NULL;}
    else result->bytes[result->size]=0;
    hubNetClose(request);return ok;
}
