// Small native HTML reader: bounded text layout, entities, links and HTTPS URLs.
#ifndef X360_NATIVE_DOCUMENT_H
#define X360_NATIVE_DOCUMENT_H
#include "xui.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "styled_page.h"
enum {WEB_LINES=1800,WEB_LINKS=512,WEB_URL=2048,WEB_TEXT=200,WEB_HTML_LIMIT=1024*1024};
enum {WEB_TEXT_ROW=0,WEB_IMAGE_ROW=1,WEB_VIDEO_ROW=2,WEB_IMAGES=8};
struct WebLine {char text[WEB_TEXT];int link,font,y,height,kind,media;};
struct WebImage {char url[WEB_URL],alt[160];uint32_t* pixels;int width,height,state;};
struct WebDocument {char url[WEB_URL],title[128];WebLine lines[WEB_LINES];char links[WEB_LINKS][WEB_URL];int count,linkCount,height;bool truncated;WebImage images[WEB_IMAGES];int imageCount;void* styled;};
static void webReleaseImages(WebDocument* doc){for(int i=0;i<doc->imageCount;++i){free(doc->images[i].pixels);doc->images[i].pixels=NULL;}doc->imageCount=0;if(doc->styled){browserStyledDestroy(doc->styled);doc->styled=NULL;}}
static void webLayout(WebDocument* doc){doc->height=0;for(int i=0;i<doc->count;++i){WebLine* row=&doc->lines[i];if(row->kind==WEB_IMAGE_ROW){WebImage* image=&doc->images[row->media];row->height=image->pixels?image->height+36:132;}row->y=doc->height;doc->height+=row->height;}}
static bool webPrefix(const char* s,const char* prefix) {
    while(*prefix)if(tolower((unsigned char)*s++)!=tolower((unsigned char)*prefix++))return false;return true;
}
static bool webEqual(const char* a,const char* b) {return strlen(a)==strlen(b) && webPrefix(a,b);}
static const char* webFind(const char* text,const char* needle) {for(const char* p=text;*p;++p)if(webPrefix(p,needle))return p;return NULL;}
static bool webCopy(char* out,size_t capacity,const char* in) {if(strlen(in)>=capacity)return false;strcpy(out,in);return true;}
static bool webResolveURL(const char* base,const char* reference,char* output,size_t capacity) {
    char ref[WEB_URL];while(isspace((unsigned char)*reference))++reference;
    if(!webCopy(ref,sizeof(ref),reference))return false;
    size_t length=strlen(ref);while(length && isspace((unsigned char)ref[length-1]))ref[--length]=0;
    char* fragment=strchr(ref,'#');if(fragment)*fragment=0;
    char temp[WEB_URL*2];temp[0]=0;
    if(webPrefix(ref,"https://")) {if(!webCopy(temp,sizeof(temp),ref))return false;}
    else if(webPrefix(ref,"http://")) {if(strlen(ref)+2>=sizeof(temp))return false;sprintf(temp,"https://%s",ref+7);}
    else if(ref[0]=='/' && ref[1]=='/') {if(strlen(ref)+7>=sizeof(temp))return false;sprintf(temp,"https:%s",ref);}
    else {
        const char* scheme=strchr(ref,':');const char* delimiter=strpbrk(ref,"/?");
        if(scheme && (!delimiter || scheme<delimiter))return false;
        if(!webPrefix(base,"https://"))return false;
        char origin[WEB_URL];if(!webCopy(origin,sizeof(origin),base))return false;
        char* path=strchr(origin+8,'/');if(path)*path=0;
        if(!ref[0]) {if(!webCopy(temp,sizeof(temp),base))return false;}
        else if(ref[0]=='/') {if(strlen(origin)+strlen(ref)>=sizeof(temp))return false;sprintf(temp,"%s%s",origin,ref);}
        else {
            char directory[WEB_URL];if(!webCopy(directory,sizeof(directory),base))return false;
            char* query=strchr(directory,'?');if(query)*query=0;
            if(ref[0]=='?') {if(strlen(directory)+strlen(ref)>=sizeof(temp))return false;sprintf(temp,"%s%s",directory,ref);}
            else {
                char* slash=strrchr(directory+8,'/');if(slash)slash[1]=0;
                else {if(strlen(directory)+1>=sizeof(directory))return false;strcat(directory,"/");}
                if(strlen(directory)+strlen(ref)>=sizeof(temp))return false;sprintf(temp,"%s%s",directory,ref);
            }
        }
    }
    if(!webPrefix(temp,"https://"))return false;
    // Resolve dot segments before fetching CSS-relative images and imports.
    const char* pathStart=strchr(temp+8,'/');
    if(pathStart) {
        const char* pathEnd=strchr(pathStart,'?');if(!pathEnd)pathEnd=temp+strlen(temp);
        char normalized[WEB_URL*2];size_t root=(size_t)(pathStart-temp)+1;
        memcpy(normalized,temp,root);size_t written=root;const char* segment=pathStart+1;
        while(segment<pathEnd) {
            const char* end=segment;while(end<pathEnd && *end!='/')++end;size_t n=end-segment;
            if(n==2 && segment[0]=='.' && segment[1]=='.'){if(written>root){while(written>root && normalized[written-1]!='/')--written;if(written>root)--written;}}
            else if(n && !(n==1 && segment[0]=='.')){if(written>root)normalized[written++]='/';if(written+n>=sizeof(normalized))return false;memcpy(normalized+written,segment,n);written+=n;}
            segment=end<pathEnd?end+1:end;
        }
        if(pathEnd>pathStart+1 && pathEnd[-1]=='/' && written>root)normalized[written++]='/';
        size_t suffix=strlen(pathEnd);if(written+suffix>=sizeof(normalized))return false;memcpy(normalized+written,pathEnd,suffix+1);strcpy(temp,normalized);
    }
    if(strlen(temp)>=capacity)return false;
    const char* h=temp+8;const char* end=h;while(*end && *end!='/' && *end!='?')++end;
    if(end==h)return false;
    for(const char* p=h;p<end;++p)if(!isalnum((unsigned char)*p) && *p!='.' && *p!='-')return false;
    for(const unsigned char* p=(const unsigned char*)temp;*p;++p)if(*p<=32 || *p>=127)return false;
    strcpy(output,temp);return true;
}
static bool webSearchOrURL(const char* input,char* output,size_t capacity,bool google=false) {
    if(webEqual(input,"browser:home")||webEqual(input,"browser:help"))return webCopy(output,capacity,input);
    if(webPrefix(input,"https://") || webPrefix(input,"http://"))return webResolveURL("https://www.bing.com/",input,output,capacity);
    if(strchr(input,'.') && !strchr(input,' ') && !strchr(input,':')) {
        char temp[WEB_URL];if(strlen(input)+9>=sizeof(temp))return false;sprintf(temp,"https://%s",input);return webResolveURL(temp,temp,output,capacity);
    }
    const char* origin="https://www.google.com/search?q="; // Google is the only default search provider.
    if(strlen(origin)+1>=capacity)return false;strcpy(output,origin);size_t n=strlen(output);const char* hex="0123456789ABCDEF";
    for(const unsigned char* p=(const unsigned char*)input;*p;++p) {
        if(n+4>=capacity)return false;
        if(isalnum(*p) || *p=='-' || *p=='_')output[n++]=*p;
        else {output[n++]='%';output[n++]=hex[*p>>4];output[n++]=hex[*p&15];}
    }
    output[n]=0;return true;
}
static int webUTF8(unsigned code,char* out) {
    if(!code || code>0x10ffff || (code>=0xd800 && code<=0xdfff))code='?';
    if(code<128){out[0]=(char)code;return 1;}
    if(code<2048){out[0]=(char)(0xc0|(code>>6));out[1]=(char)(0x80|(code&63));return 2;}
    if(code<65536){out[0]=(char)(0xe0|(code>>12));out[1]=(char)(0x80|((code>>6)&63));out[2]=(char)(0x80|(code&63));return 3;}
    out[0]=(char)(0xf0|(code>>18));out[1]=(char)(0x80|((code>>12)&63));out[2]=(char)(0x80|((code>>6)&63));out[3]=(char)(0x80|(code&63));return 4;
}
static int webEntity(const char* text,char* output,int* consumed) {
    *consumed=1;if(*text!='&'){output[0]=*text;return 1;}
    const char* end=strchr(text,';');if(!end || end-text>16){output[0]='&';return 1;}
    char name[20];int len=(int)(end-text-1);memcpy(name,text+1,len);name[len]=0;unsigned code=0;
    if(!strcmp(name,"amp"))code='&';else if(!strcmp(name,"lt"))code='<';else if(!strcmp(name,"gt"))code='>';
    else if(!strcmp(name,"quot"))code='"';else if(!strcmp(name,"apos"))code='\'';else if(!strcmp(name,"nbsp"))code=' ';
    else if(!strcmp(name,"ndash") || !strcmp(name,"mdash"))code='-';else if(!strcmp(name,"hellip"))code=0x2026;
    else if(name[0]=='#') {
        char* parseEnd=NULL;code=(unsigned)strtoul(name+(name[1]=='x'||name[1]=='X'?2:1),&parseEnd,name[1]=='x'||name[1]=='X'?16:10);
        if(!parseEnd || *parseEnd)code=0;
    }
    if(!code){output[0]='&';return 1;}
    *consumed=(int)(end-text+1);return webUTF8(code,output);
}
static void webDecode(const char* input,char* output,size_t capacity) {
    size_t n=0;for(const char* p=input;*p && n+5<capacity;) {char bytes[4];int consumed=0,count=webEntity(p,bytes,&consumed);memcpy(output+n,bytes,count);n+=count;p+=consumed;}output[n]=0;
}
static bool webAttribute(const char* tag,const char* key,char* output,size_t capacity) {
    const char* p=tag;while(*p && !isspace((unsigned char)*p))++p;
    while(*p) {
        while(isspace((unsigned char)*p)||*p=='/')++p;if(!*p)break;
        char name[32];int n=0;while(*p && !isspace((unsigned char)*p)&&*p!='=') {if(n<31)name[n++]=*p;++p;}name[n]=0;
        while(isspace((unsigned char)*p))++p;if(*p!='=')continue;++p;while(isspace((unsigned char)*p))++p;
        char quote=(*p=='\''||*p=='"')?*p++:0;const char* start=p;
        while(*p && (quote?*p!=quote:!isspace((unsigned char)*p)))++p;
        size_t length=p-start;
        if(webEqual(name,key) && length<capacity) {char temp[WEB_URL];if(length>=sizeof(temp))return false;memcpy(temp,start,length);temp[length]=0;webDecode(temp,output,capacity);return true;}
        if(quote && *p)++p;
    }
    output[0]=0;return false;
}
static void webEmit(WebDocument* doc,char* text,int link,int font) {
    char* p=text;while(isspace((unsigned char)*p))++p;size_t length=strlen(p);while(length && isspace((unsigned char)p[length-1]))p[--length]=0;
    while(*p && doc->count<WEB_LINES) {
        char line[WEB_TEXT];int n=0,lastSpace=-1;
        while(p[n] && n<WEB_TEXT-5) {
            int bytes=((unsigned char)p[n]<128)?1:((unsigned char)p[n]<224?2:((unsigned char)p[n]<240?3:4));
            if(n+bytes>=WEB_TEXT-1)break;
            for(int i=0;i<bytes && p[n];++i){line[n]=p[n];++n;}line[n]=0;
            if(line[n-1]==' ')lastSpace=n-1;
            if(xui_text_width(font,line,0)>1050){n=lastSpace>0?lastSpace:n-bytes;break;}
        }
        if(!n){n=1;}line[n]=0;
        WebLine* row=&doc->lines[doc->count++];strcpy(row->text,line);row->link=link;row->font=font;row->y=doc->height;
        row->height=font==XUI_F_H2?42:28;row->kind=WEB_TEXT_ROW;row->media=-1;doc->height+=row->height;
        p+=n;while(*p==' ')++p;
    }
    if(*p)doc->truncated=true;text[0]=0;
}
static void webParseHTML(WebDocument* doc,const char* html,const char* url) {
    memset(doc,0,sizeof(*doc));webCopy(doc->url,sizeof(doc->url),url);strcpy(doc->title,"Web page");
    char chunk[4096]="",base[WEB_URL];webCopy(base,sizeof(base),url);int used=0,link=-1,font=XUI_F_BODY;bool hidden=false,inTitle=false;
    char hiddenTag[16]="";bool inVideo=false;
    const char* p=html;
    while(*p && doc->count<WEB_LINES) {
        if(webPrefix(p,"<!--")){const char* end=strstr(p+4,"-->");p=end?end+3:p+strlen(p);continue;}
        if(*p=='<') {
            const char* end=p+1;char quote=0;
            while(*end) {if(quote){if(*end==quote)quote=0;}else if(*end=='\''||*end=='"')quote=*end;else if(*end=='>')break;++end;}
            if(!*end)break;
            char tag[4096];int length=(int)(end-p-1);if(length>=4096){p=end+1;continue;}memcpy(tag,p+1,length);tag[length]=0;
            char name[32];const char* t=tag;bool close=*t=='/';if(close)++t;int n=0;while(*t && !isspace((unsigned char)*t) && *t!='/' && n<31)name[n++]=(char)tolower((unsigned char)*t++);name[n]=0;
            if(hidden) {if(close && webEqual(name,hiddenTag))hidden=false;p=end+1;continue;}
            if(webEqual(name,"script")||webEqual(name,"style")) {
                if(!close) {
                    char closing[40];sprintf(closing,"</%s",name);
                    const char* stop=webFind(end+1,closing);
                    if(!stop){p=end+strlen(end);break;}
                    const char* stopEnd=strchr(stop,'>');p=stopEnd?stopEnd+1:stop+strlen(stop);
                }else p=end+1;
                continue;
            }
            if(webEqual(name,"title")) {
                if(close){chunk[used]=0;webDecode(chunk,doc->title,sizeof(doc->title));used=0;chunk[0]=0;}
                inTitle=!close;p=end+1;continue;
            }
            if(inTitle){p=end+1;continue;}
            bool imageTag=webEqual(name,"img")&&!close;
            bool videoTag=webEqual(name,"video");
            if(videoTag)inVideo=!close;
            if(imageTag || (videoTag&&!close) || (webEqual(name,"source") && inVideo && !close)) {
                chunk[used]=0;webEmit(doc,chunk,link,font);used=0;
                char src[WEB_URL]="",resolved[WEB_URL];
                bool has=webAttribute(tag,"src",src,sizeof(src));
                if(!has || !src[0] || webPrefix(src,"data:"))has=webAttribute(tag,"data-src",src,sizeof(src));
                if(has && webResolveURL(base,src,resolved,sizeof(resolved)) && doc->count<WEB_LINES) {
                    if(imageTag && doc->imageCount<WEB_IMAGES) {
                        int index=doc->imageCount++;WebImage* image=&doc->images[index];strcpy(image->url,resolved);
                        webAttribute(tag,"alt",image->alt,sizeof(image->alt));
                        WebLine* row=&doc->lines[doc->count++];memset(row,0,sizeof(*row));row->kind=WEB_IMAGE_ROW;row->media=index;row->link=link;row->font=XUI_F_LABEL;row->height=132;row->y=doc->height;doc->height+=row->height;
                    } else if(!imageTag && doc->linkCount<WEB_LINKS) {
                        int index=doc->linkCount++;strcpy(doc->links[index],resolved);
                        WebLine* row=&doc->lines[doc->count++];memset(row,0,sizeof(*row));strcpy(row->text,"Play video");row->kind=WEB_VIDEO_ROW;row->media=-1;row->link=index;row->font=XUI_F_BODY;row->height=92;row->y=doc->height;doc->height+=row->height;
                    }
                }
                p=end+1;continue;
            }
            bool anchor=webEqual(name,"a");bool block=anchor||webEqual(name,"p")||webEqual(name,"div")||webEqual(name,"li")||webEqual(name,"br")||webEqual(name,"section")||webEqual(name,"article")||webEqual(name,"tr")||webEqual(name,"hr")||webEqual(name,"h1")||webEqual(name,"h2")||webEqual(name,"h3");
            if(block){chunk[used]=0;webEmit(doc,chunk,link,font);used=0;}
            if(anchor) {
                link=-1;if(!close && doc->linkCount<WEB_LINKS) {
                    char href[WEB_URL],resolved[WEB_URL];if(webAttribute(tag,"href",href,sizeof(href)) && href[0] && webResolveURL(base,href,resolved,sizeof(resolved))) {link=doc->linkCount++;strcpy(doc->links[link],resolved);}
                }
            }
            if(webEqual(name,"h1")||webEqual(name,"h2")||webEqual(name,"h3"))font=close?XUI_F_BODY:XUI_F_H2;
            if(webEqual(name,"base")&&!close){char href[WEB_URL],resolved[WEB_URL];if(webAttribute(tag,"href",href,sizeof(href))&&webResolveURL(base,href,resolved,sizeof(resolved)))strcpy(base,resolved);}
            p=end+1;continue;
        }
        if(hidden){++p;continue;}
        char bytes[4];int consumed=0,count=webEntity(p,bytes,&consumed);p+=consumed;
        if(count==1 && isspace((unsigned char)bytes[0])){if(used && chunk[used-1]!=' ')chunk[used++]=' ';}
        else if(used+count<(int)sizeof(chunk)-1){memcpy(chunk+used,bytes,count);used+=count;}
        if(used>=(int)sizeof(chunk)-8){chunk[used]=0;if(!inTitle)webEmit(doc,chunk,link,font);used=0;}
    }
    if(!inTitle){chunk[used]=0;webEmit(doc,chunk,link,font);}
    if(*p)doc->truncated=true;
    if(!doc->count){strcpy(chunk,"This page has no readable HTML text. It may require JavaScript.");webEmit(doc,chunk,-1,XUI_F_BODY);}
}
#endif
