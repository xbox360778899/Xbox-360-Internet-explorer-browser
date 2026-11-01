// Portable INI parser shared by Xbox and host regression tests.
#ifndef X360BROWSER_CONFIG_H
#define X360BROWSER_CONFIG_H
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

struct BrowserConfig { char host[64],token[128];int port; };
static char* browserTrim(char* s) {
    while(*s && isspace((unsigned char)*s))++s;
    char* end=s+strlen(s);while(end>s && isspace((unsigned char)end[-1]))--end;*end=0;return s;
}
static bool browserIPv4(const char* s) {
    int parts=0;
    while(*s) {
        int digits=0,value=0;
        while(*s>='0' && *s<='9') {value=value*10+*s++-'0';++digits;if(digits>3)return false;}
        if(!digits || value>255)return false;
        ++parts;if(!*s)break;if(*s++!='.' || !*s)return false;
    }
    return parts==4;
}
static bool browserParseConfig(const unsigned char* raw,int size,BrowserConfig* out,char* error) {
    memset(out,0,sizeof(*out));out->port=36080;
    char text[4097];int count=0;
    if(size<0 || size>4096) {strcpy(error,"Settings file is too large.");return false;}
    if(size>=2 && raw[0]==0xff && raw[1]==0xfe) {
        if(size%2) {strcpy(error,"Settings file has incomplete UTF-16 data.");return false;}
        for(int i=2;i+1<size;i+=2) {
            if(raw[i+1] || !raw[i]) {strcpy(error,"Settings must contain plain ASCII values.");return false;}
            text[count++]=(char)raw[i];
        }
    } else {
        int start=size>=3 && raw[0]==0xef && raw[1]==0xbb && raw[2]==0xbf?3:0;
        for(int i=start;i<size;++i) {if(!raw[i]) {strcpy(error,"Settings contain an unexpected zero byte.");return false;}text[count++]=(char)raw[i];}
    }
    text[count]=0;
    char* line=text;
    while(*line) {
        char* next=strpbrk(line,"\r\n");if(next)*next++=0;
        char* key=browserTrim(line);
        if(*key && *key!='#' && *key!=';' && *key!='[') {
            char* value=strchr(key,'=');
            if(value) {
                *value++=0;key=browserTrim(key);value=browserTrim(value);
                for(char* p=key;*p;++p)*p=(char)tolower((unsigned char)*p);
                size_t len=strlen(value);
                if(!strcmp(key,"host")) {
                    if(len>=sizeof(out->host)) {strcpy(error,"PC address is too long.");return false;}
                    strcpy(out->host,value);
                } else if(!strcmp(key,"token")) {
                    if(len>=sizeof(out->token)) {strcpy(error,"Bridge token is too long.");return false;}
                    strcpy(out->token,value);
                } else if(!strcmp(key,"port")) {
                    char* end=NULL;long port=strtol(value,&end,10);
                    if(!*value || *end || port<1 || port>65535) {strcpy(error,"Port must be a number from 1 to 65535.");return false;}
                    out->port=(int)port;
                }
            }
        }
        if(!next)break;line=next;
    }
    if(!browserIPv4(out->host)) {strcpy(error,"Missing or invalid PC IPv4 address (host).");return false;}
    size_t tokenLength=strlen(out->token);
    if(tokenLength<32) {strcpy(error,"Missing or incomplete bridge token.");return false;}
    for(size_t i=0;i<tokenLength;++i) {
        unsigned char c=(unsigned char)out->token[i];
        if(c<33 || c>126) {strcpy(error,"Bridge token contains invalid characters.");return false;}
    }
    error[0]=0;return true;
}
#endif
