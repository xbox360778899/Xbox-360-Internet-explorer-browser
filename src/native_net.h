// Native HTTPS transport adapted from the user-owned X360HUB TLS transport.
// MIT BearSSL and Mozilla CA bundle are retained with their upstream notices.
struct WebNetState { volatile LONG cancel; DWORD httpPhase,httpCode,socketError,tlsError; };
static WebNetState webNetState={0};
// X360HUB TLS 1.2 transport. BearSSL validates chains, dates and hostnames.
// This uses normal XDK sockets; no radio-driver or kernel-memory patches.
#pragma once
#ifndef X360BROWSER_HOST_TEST
#include <winsockx.h>
#endif
#include "../vendor/bearssl/inc/bearssl.h"
#include "tls_roots.h"

extern "C" void XeCryptRandom(BYTE*,DWORD);
static const DWORD HUB_NET_LIMIT=64*1024*1024;
struct HubTLSRequest {
 br_ssl_client_context ssl;
 br_x509_minimal_context x509;
 br_sslio_context io;
 unsigned char buffer[BR_SSL_BUFSIZE_BIDI];
 SOCKET socket;
 DWORD started,error,length,received,chunkLeft,maxIdleMs;
 bool lengthKnown,chunked,finished,chunkCRLF;
 char finalURL[8192],contentType[128],contentEncoding[32],cacheControl[160];
};
static bool hubNetSuffix(const char* host,const char* suffix){
 size_t a=strlen(host),b=strlen(suffix);
 return a>b&&!_stricmp(host+a-b,suffix);
}
static bool hubNetURL(const char* url,char* host,size_t hs,char* path,size_t ps){
 if(!url||strlen(url)>=8192||_strnicmp(url,"https://",8))return false;
 const char* a=url+8;const char* b=a;
 while(*b&&*b!='/'&&*b!='?'&&*b!='#')b++;
 size_t n=b-a;if(!n||n>=hs)return false;
 for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)a[i])&&a[i]!='.'&&a[i]!='-')return false;
 memcpy(host,a,n);host[n]=0;
 if(!*b||*b=='#')strcpy_s(path,ps,"/");
 else if(*b=='?'){if(strlen(b)+2>ps)return false;sprintf_s(path,ps,"/%s",b);}
 else{if(strlen(b)>=ps)return false;strcpy_s(path,ps,b);}
 char* fragment=strchr(path,'#');if(fragment)*fragment=0;
 for(char* c=path;*c;c++)if((unsigned char)*c<33||(unsigned char)*c>126)return false;
 return true;
}
// XDK/stealth wrappers may retain a facility in Winsock errors.
static DWORD hubSocketReason(DWORD err){return err & 0xFFFFu;}
static bool hubNetActive(HubTLSRequest* r){
 if(webNetState.cancel){r->error=ERROR_CANCELLED;return false;}
 if(GetTickCount()-r->started>=(r->maxIdleMs?r->maxIdleMs:18000)){r->error=ERROR_TIMEOUT;return false;}
 return true;
}
static bool hubNetWaitSocket(HubTLSRequest* r,bool writing){
 while(hubNetActive(r)){
  fd_set f;FD_ZERO(&f);FD_SET(r->socket,&f);timeval t={0,100000};
  int result=select(0,writing?NULL:&f,writing?&f:NULL,NULL,&t);
  if(result>0)return true;
  if(result<0){r->error=WSAGetLastError();return false;}
 }
 return false;
}
static int hubNetSocketRead(void* opaque,unsigned char* out,size_t count){
 HubTLSRequest* r=(HubTLSRequest*)opaque;
 while(hubNetWaitSocket(r,false)){
  int n=recv(r->socket,(char*)out,(int)count,0);
  if(n>0){r->started=GetTickCount();return n;}
  // BearSSL expects -1 on EOF; zero means retry and can spin until timeout.
  if(n==0){r->error=WSAECONNRESET;return -1;}
  DWORD e=(DWORD)WSAGetLastError();
  if(hubSocketReason(e)!=WSAEWOULDBLOCK){r->error=e;return -1;}
 }
 return -1;
}
static int hubNetSocketWrite(void* opaque,const unsigned char* in,size_t count){
 HubTLSRequest* r=(HubTLSRequest*)opaque;
 while(hubNetWaitSocket(r,true)){
  int n=send(r->socket,(const char*)in,(int)count,0);
  if(n>0){r->started=GetTickCount();return n;}
  if(n==0){r->error=WSAECONNRESET;return -1;}
  DWORD e=(DWORD)WSAGetLastError();
  if(hubSocketReason(e)!=WSAEWOULDBLOCK){r->error=e;return -1;}
 }
 return -1;
}
static DWORD hubNetError(HubTLSRequest* r){
 if(r->error)return r->error;
 int tls=br_ssl_engine_last_error(&r->ssl.eng);
 return tls?(0xE1000000u|(DWORD)tls):ERROR_INVALID_DATA;
}
static void hubNetClose(HubTLSRequest* r){
 if(!r)return;
 if(r->socket!=INVALID_SOCKET)closesocket(r->socket);
 SecureZeroMemory(r,sizeof(*r));free(r);
}
static bool hubNetResolve(HubTLSRequest* r,const char* host,IN_ADDR* address){
 XNDNS* dns=NULL;int error=XNetDnsLookup(host,NULL,&dns);
 if(error||!dns){r->error=error?error:WSAHOST_NOT_FOUND;return false;}
 while(dns->iStatus==WSAEINPROGRESS&&hubNetActive(r))Sleep(10);
 bool ok=!r->error&&dns->iStatus==0&&dns->cina>0;
 if(ok)*address=dns->aina[0];else if(!r->error)r->error=dns->iStatus;
 XNetDnsRelease(dns);return ok;
}
// XDK may wrap WSA errors with 0xE1020000. 0xE1022740 is
// WSAEADDRINUSE (10048), a socket collision, NOT a clock/TLS error.
static bool hubSocketRetryable(DWORD err){
 DWORD value=hubSocketReason(err);
 return value==WSAEADDRINUSE || value==WSAEADDRNOTAVAIL;
}
static bool hubNetConnect(HubTLSRequest* r,const char* host){
 r->started=GetTickCount();webNetState.httpPhase=3;
 sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons(443);
 if(!hubNetResolve(r,host,&addr.sin_addr))return false;
 bool connected=false;
 // Short, bounded retries make Xbox/stealth transient ephemeral-port collisions
 // recoverable without changing DashLaunch, kernel patches or system time.
 for(int attempt=0;attempt<3;attempt++){
  if(!hubNetActive(r))return false;
  webNetState.httpPhase=4;
  if(r->socket!=INVALID_SOCKET){closesocket(r->socket);r->socket=INVALID_SOCKET;}
  r->socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if(r->socket==INVALID_SOCKET){r->error=(DWORD)WSAGetLastError();}
  else{
   BOOL bypass=TRUE;
#ifndef X360BROWSER_HOST_TEST
   if(setsockopt(r->socket,SOL_SOCKET,0x5801,(const char*)&bypass,sizeof(bypass))==SOCKET_ERROR){
    r->error=(DWORD)WSAGetLastError();return false;
   }
#endif
   int recvBuffer=128*1024;
   setsockopt(r->socket,SOL_SOCKET,SO_RCVBUF,(const char*)&recvBuffer,sizeof(recvBuffer));
   u_long nonblock=1;
   if(ioctlsocket(r->socket,FIONBIO,&nonblock))r->error=(DWORD)WSAGetLastError();
   else if(connect(r->socket,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){
    DWORD code=(DWORD)WSAGetLastError();
    DWORD reason=hubSocketReason(code);
    if(reason==WSAEWOULDBLOCK||reason==WSAEINPROGRESS){
     if(!hubNetWaitSocket(r,true))return false;
     // Xbox 360 XDK winsockx.h does not expose SO_ERROR.
     // getpeername confirms the nonblocking connection before TLS starts.
     sockaddr_in peer={0};int peerSize=sizeof(peer);
     if(getpeername(r->socket,(sockaddr*)&peer,&peerSize))
      r->error=(DWORD)WSAGetLastError();
     else connected=true;
    }else r->error=code;
   }else connected=true;
  }
  if(connected){r->error=0;break;}
  if(!hubSocketRetryable(r->error)||attempt==2)return false;
  closesocket(r->socket);r->socket=INVALID_SOCKET;
  r->error=0;
  Sleep(250*(attempt+1));
  r->started=GetTickCount();
 }
 if(!connected)return false;
 webNetState.httpPhase=5;
 hubInitTrustAnchors();
 br_ssl_client_init_full(&r->ssl,&r->x509,hubTrustAnchors,hubTrustAnchorCount);
 br_ssl_engine_set_versions(&r->ssl.eng,BR_TLS12,BR_TLS12);
 br_ssl_engine_set_buffer(&r->ssl.eng,r->buffer,sizeof(r->buffer),1);
 unsigned char seed[48];XeCryptRandom(seed,sizeof(seed));
 br_ssl_engine_inject_entropy(&r->ssl.eng,seed,sizeof(seed));SecureZeroMemory(seed,sizeof(seed));
 // BearSSL validates certificates against absolute UTC, regardless of whether
 // a stealth server displays Brazil/US local time. Do not override console clock.
 SYSTEMTIME utc;GetSystemTime(&utc);
 if(utc.wYear<2023||utc.wYear>2099){r->error=ERROR_INVALID_TIME;return false;}
 FILETIME ft;GetSystemTimeAsFileTime(&ft);
 ULARGE_INTEGER now;now.LowPart=ft.dwLowDateTime;now.HighPart=ft.dwHighDateTime;
 if(now.QuadPart<116444736000000000ULL){r->error=ERROR_INVALID_TIME;return false;}
 ULONGLONG seconds=(now.QuadPart-116444736000000000ULL)/10000000ULL;
 br_x509_minimal_set_time(&r->x509,(uint32_t)(seconds/86400+719528),(uint32_t)(seconds%86400));
 if(!br_ssl_client_reset(&r->ssl,host,0)){r->error=ERROR_INVALID_DATA;return false;}
 br_sslio_init(&r->io,&r->ssl.eng,hubNetSocketRead,r,hubNetSocketWrite,r);
 return true;
}
static int hubNetRaw(HubTLSRequest* r,void* data,size_t count){
 if(!hubNetActive(r))return -1;
 int n=br_sslio_read(&r->io,data,count);
 if(n<0&&br_ssl_engine_last_error(&r->ssl.eng)==0&&!r->error)return 0;
 return n;
}
static bool hubNetLine(HubTLSRequest* r,char* line,size_t capacity){
 size_t n=0;
 while(n+1<capacity){
  char c;if(hubNetRaw(r,&c,1)!=1)return false;
  if(c=='\n'){
   if(!n||line[n-1]!='\r')return false;
   line[n-1]=0;return true;
  }
  if(!c)return false;line[n++]=c;
 }
 r->error=ERROR_INSUFFICIENT_BUFFER;return false;
}
static bool hubNetHeaders(HubTLSRequest* r,DWORD* status,char* location,size_t locSize){
 char line[12288];DWORD budget=0;
 if(!hubNetLine(r,line,sizeof(line)))return false;
 int code=0;if(sscanf_s(line,"HTTP/1.%*u %d",&code)!=1||code<100||code>599)return false;
 *status=(DWORD)code;
 bool haveEncoding=false;
 while(true){
  if(!hubNetLine(r,line,sizeof(line)))return false;
  budget+=(DWORD)strlen(line)+2;if(budget>32768){r->error=ERROR_INSUFFICIENT_BUFFER;return false;}
  if(!line[0])break;
  char* colon=strchr(line,':');if(!colon)return false;*colon++=0;
  while(*colon==' '||*colon=='\t')colon++;
  char* end=colon+strlen(colon);while(end>colon&&(end[-1]==' '||end[-1]=='\t'))*--end=0;
  if(!_stricmp(line,"Content-Length")){
   if(!*colon)return false;ULONGLONG n=0;
   for(const char* c=colon;*c;c++){if(*c<'0'||*c>'9')return false;n=n*10+(*c-'0');if(n>HUB_NET_LIMIT)return false;}
   if(r->lengthKnown&&r->length!=(DWORD)n)return false;
   r->lengthKnown=true;r->length=(DWORD)n;
  }else if(!_stricmp(line,"Transfer-Encoding")){
   if(haveEncoding||_stricmp(colon,"chunked"))return false;
   haveEncoding=true;r->chunked=true;
  }else if(!_stricmp(line,"Content-Encoding")){
   if(_stricmp(colon,"identity")&&_stricmp(colon,"gzip")&&_stricmp(colon,"deflate"))return false;
   strcpy_s(r->contentEncoding,sizeof(r->contentEncoding),colon);
  }else if(!_stricmp(line,"Content-Type")){
   strncpy_s(r->contentType,sizeof(r->contentType),colon,_TRUNCATE);
  }else if(!_stricmp(line,"Cache-Control") || !_stricmp(line,"Pragma") || !_stricmp(line,"Set-Cookie")){
   if(!_stricmp(line,"Set-Cookie"))strcpy_s(r->cacheControl,"private, no-store");
   else if(strlen(r->cacheControl)+strlen(colon)+2<sizeof(r->cacheControl)){strcat_s(r->cacheControl,",");strcat_s(r->cacheControl,colon);}
  }else if(!_stricmp(line,"Location")){
   if(location[0]||strlen(colon)>=locSize)return false;strcpy_s(location,locSize,colon);
  }
 }
 return !(r->chunked&&r->lengthKnown);
}
static HubTLSRequest* hubNetOpen(const char* url,DWORD* status,DWORD timeoutMs=10000,const char* accept="text/html,application/xhtml+xml,text/plain;q=0.8"){
 char current[8192],host[128],path[8192],location[8192];
 if(!url||strlen(url)>=sizeof(current)){SetLastError(ERROR_INVALID_PARAMETER);return NULL;}
 strcpy_s(current,sizeof(current),url);
 for(int redirects=0;redirects<=8;redirects++){
  if(!hubNetURL(current,host,sizeof(host),path,sizeof(path))){SetLastError(ERROR_ACCESS_DENIED);return NULL;}
  HubTLSRequest* r=(HubTLSRequest*)calloc(1,sizeof(HubTLSRequest));
  if(!r){SetLastError(ERROR_NOT_ENOUGH_MEMORY);return NULL;}
  r->socket=INVALID_SOCKET;r->maxIdleMs=timeoutMs;location[0]=0;
  bool ok=hubNetConnect(r,host);
  if(ok){
   char request[8704];sprintf_s(request,sizeof(request),"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: X360Browser/0.9 (Xbox 360; NativeBrowser)\r\nAccept: %s\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\nConnection: close\r\n\r\n",path,host,accept);
   r->started=GetTickCount();
   ok=br_sslio_write_all(&r->io,request,strlen(request))==0&&br_sslio_flush(&r->io)==0;
  }
  if(ok){webNetState.httpPhase=6;r->started=GetTickCount();ok=hubNetHeaders(r,status,location,sizeof(location));}
  if(!ok){
   // Keep each layer separately: an unexpected TLS engine value must not be
   // presented as DNS failure merely because it exceeds the normal TLS range.
   webNetState.socketError=r->error;
   webNetState.tlsError=(DWORD)br_ssl_engine_last_error(&r->ssl.eng);
   DWORD e=hubNetError(r);hubNetClose(r);SetLastError(e);return NULL;
  }
  webNetState.httpCode=*status;webNetState.httpPhase=7;
  if(*status==200){strcpy_s(r->finalURL,sizeof(r->finalURL),current);return r;}
  webNetState.httpPhase=9; // HTTP redirect to allowed origin
  bool redirect=*status==301||*status==302||*status==303||*status==307||*status==308;
  hubNetClose(r);
  if(!redirect||!location[0]){SetLastError(*status);return NULL;}
  if(!_strnicmp(location,"https://",8))strcpy_s(current,sizeof(current),location);
  else if(location[0]=='/'&&location[1]!='/')sprintf_s(current,sizeof(current),"https://%s%s",host,location);
  else{char resolved[8192];if(!webResolveURL(current,location,resolved,sizeof(resolved))){SetLastError(ERROR_ACCESS_DENIED);return NULL;}strcpy_s(current,sizeof(current),resolved);}
 }
 SetLastError(ERROR_TOO_MANY_CMDS);return NULL;
}
static bool hubNetRead(HubTLSRequest* r,void* out,DWORD want,DWORD* count){
 *count=0;if(r->finished||!want)return true;r->started=GetTickCount();
 if(r->chunked){
  while(!r->chunkLeft){
   char line[160];
   if(r->chunkCRLF){if(!hubNetLine(r,line,3)||line[0])goto failed;r->chunkCRLF=false;}
   if(!hubNetLine(r,line,sizeof(line)))goto failed;
   DWORD size=0;int digits=0;
   for(char* c=line;*c&&*c!=';';c++){
    int v=(*c>='0'&&*c<='9')?*c-'0':(*c>='a'&&*c<='f')?*c-'a'+10:(*c>='A'&&*c<='F')?*c-'A'+10:-1;
    if(v<0||++digits>8||size>(HUB_NET_LIMIT-(DWORD)v)/16)goto failed;size=size*16+v;
   }
   if(!digits)goto failed;
   if(!size){
    char trailer[4096];DWORD total=0;
    do{if(!hubNetLine(r,trailer,sizeof(trailer)))goto failed;total+=(DWORD)strlen(trailer)+2;if(total>16384)goto failed;}while(trailer[0]);
    r->finished=true;return true;
   }
   r->chunkLeft=size;r->chunkCRLF=true;
  }
  if(want>r->chunkLeft)want=r->chunkLeft;
 }else if(r->lengthKnown){
  if(r->received==r->length){r->finished=true;return true;}
  if(want>r->length-r->received)want=r->length-r->received;
 }
 {
  int n=hubNetRaw(r,out,want);
  if(n<0)goto failed;
  if(!n){if(r->chunked||(r->lengthKnown&&r->received!=r->length))goto failed;r->finished=true;return true;}
  if(r->received>HUB_NET_LIMIT-(DWORD)n)goto failed;
  *count=(DWORD)n;r->received+=*count;if(r->chunked)r->chunkLeft-=*count;return true;
 }
 failed:SetLastError(hubNetError(r));return false;
}
