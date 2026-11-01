#ifndef X360_HOST_PLATFORM_H
#define X360_HOST_PLATFORM_H
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
struct XNDNS {int iStatus;UINT cina;IN_ADDR aina[8];};
static int XNetDnsLookup(const char* name,HANDLE,XNDNS** output) {
    *output=(XNDNS*)calloc(1,sizeof(XNDNS));if(!*output)return ERROR_NOT_ENOUGH_MEMORY;
    hostent* entry=gethostbyname(name);
    if(!entry){(*output)->iStatus=WSAGetLastError();return 0;}
    for(int i=0;entry->h_addr_list[i] && i<8;++i){memcpy(&(*output)->aina[i],entry->h_addr_list[i],4);++(*output)->cina;}return 0;
}
static void XNetDnsRelease(XNDNS* dns){free(dns);}
extern "C" void XeCryptRandom(BYTE* buffer,DWORD size) {
    HCRYPTPROV provider=0;
    if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))abort();
    if(!CryptGenRandom(provider,size,buffer))abort();CryptReleaseContext(provider,0);
}
#endif
