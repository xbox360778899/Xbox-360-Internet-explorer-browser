// X360Browser 0.1.0. GPL-3.0. Native Xbox 360 client using orbita's XUI renderer.
#include <xtl.h>
#include <winsockx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xui.h"

enum { W=1280, H=720, PAGE_Y=100, PAGE_H=560, FRAME_BYTES=W*PAGE_H*4 };
static IDirect3DDevice9* device;
static IDirect3DTexture9* texture;
static IDirect3DVertexShader9* vs;
static IDirect3DPixelShader9* ps;
static IDirect3DVertexDeclaration9* declaration;
static uint32_t canvasPixels[W*H], pagePixels[W*PAGE_H];
static CRITICAL_SECTION guard;
static bool haveFrame=false, running=true;
static char statusText[192]="Connecting to the PC browser...";
static char host[64]="192.168.1.100", token[128]="";
static int port=36080;
static float pointerX=640, pointerY=280;
static xui_canvas_t canvas;
struct Event { char path[4096]; };
static Event events[16];
static int head=0, tail=0;
static XOVERLAPPED keyboard;
static WCHAR keyboardText[1025];
static int keyboardMode=0;
static DWORD previousButtons=0;

static void setStatus(const char* value) {
    EnterCriticalSection(&guard);
    strncpy_s(statusText,sizeof(statusText),value,_TRUNCATE);
    LeaveCriticalSection(&guard);
}
static void encode(const char* input,char* output,size_t capacity) {
    const char hex[]="0123456789ABCDEF";
    size_t n=0;
    for(const unsigned char* p=(const unsigned char*)input;*p && n+4<capacity;++p) {
        if((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'||*p=='.') output[n++]=*p;
        else {output[n++]='%';output[n++]=hex[*p>>4];output[n++]=hex[*p&15];}
    }
    output[n]=0;
}
static void enqueue(const char* type,const char* value="") {
    char escaped[3200];encode(value,escaped,sizeof(escaped));
    EnterCriticalSection(&guard);
    int next=(tail+1)%16;
    if(next!=head) {
        sprintf_s(events[tail].path,sizeof(events[tail].path),"/action?type=%s&value=%s&x=%d&y=%d",type,escaped,(int)pointerX,(int)pointerY);
        tail=next;
    } else strcpy_s(statusText,"Browser is busy. Please wait.");
    LeaveCriticalSection(&guard);
}
static bool sendAll(SOCKET socket,const char* data,int size) {
    int offset=0;
    while(offset<size) {int n=send(socket,data+offset,size-offset,0);if(n<=0)return false;offset+=n;}
    return true;
}
static bool receiveAll(SOCKET socket,char* data,int size) {
    int offset=0;
    while(offset<size) {int n=recv(socket,data+offset,size-offset,0);if(n<=0)return false;offset+=n;}
    return true;
}
static bool request(const char* path,char* output,int capacity,int* length) {
    *length=0;
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==INVALID_SOCKET) {setStatus("Network unavailable. Check DashLaunch and restart.");return false;}
    DWORD timeout=20000;
    setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    sockaddr_in addr;ZeroMemory(&addr,sizeof(addr));addr.sin_family=AF_INET;
    addr.sin_port=htons((u_short)port);addr.sin_addr.s_addr=inet_addr(host);
    // Bound connection time; an unreachable PC must not freeze the worker indefinitely.
    u_long nonblocking=1;ioctlsocket(s,FIONBIO,&nonblocking);
    int connected=connect(s,(sockaddr*)&addr,sizeof(addr));
    if(connected==SOCKET_ERROR) {
        fd_set writable,errors;FD_ZERO(&writable);FD_ZERO(&errors);FD_SET(s,&writable);FD_SET(s,&errors);
        timeval wait;wait.tv_sec=3;wait.tv_usec=0;
        int ready=select(0,NULL,&writable,&errors,&wait);
        if(ready<=0 || FD_ISSET(s,&errors) || !FD_ISSET(s,&writable)) {
            closesocket(s);setStatus("PC not reachable. Start Edge/Chrome bridge and check browser.ini / firewall.");return false;
        }
    }
    nonblocking=0;ioctlsocket(s,FIONBIO,&nonblocking);
    char req[4608];sprintf_s(req,sizeof(req),"GET %s HTTP/1.0\r\nHost: %s\r\nX-X360Browser-Token: %s\r\nConnection: close\r\n\r\n",path,host,token);
    if(!sendAll(s,req,(int)strlen(req))) {closesocket(s);setStatus("Could not send browser request.");return false;}
    char headers[4096];int used=0;
    while(used<(int)sizeof(headers)-1) {
        if(recv(s,headers+used,1,0)!=1)break;
        ++used;
        if(used>=4 && memcmp(headers+used-4,"\r\n\r\n",4)==0)break;
    }
    headers[used]=0;
    int code=0;sscanf_s(headers,"HTTP/%*s %d",&code);
    const char* sizeHeader=strstr(headers,"Content-Length:");
    long size=sizeHeader?strtol(sizeHeader+15,NULL,10):-1;
    if(code!=200 || size<0 || size>capacity || used<4 || memcmp(headers+used-4,"\r\n\r\n",4)!=0) {
        char error[192];sprintf_s(error,sizeof(error),"Bridge HTTP %d. %s",code,code==401?"Copy matching browser.ini to the Xbox.":"Page could not load. Try another address.");
        setStatus(error);closesocket(s);return false;
    }
    bool ok=receiveAll(s,output,(int)size);closesocket(s);
    if(!ok) {setStatus("Incomplete page frame. Check the wired network connection.");return false;}
    *length=(int)size;return true;
}
static DWORD WINAPI networkWorker(void*) {
    char* incoming=(char*)malloc(FRAME_BYTES);
    if(!incoming) {setStatus("Not enough memory for browser frames.");return 1;}
    while(running) {
        char path[4096]="";
        EnterCriticalSection(&guard);
        if(head!=tail) {strcpy_s(path,events[head].path);head=(head+1)%16;}
        LeaveCriticalSection(&guard);
        int length=0;
        bool actionOk=true;
        if(path[0])actionOk=request(path,incoming,FRAME_BYTES,&length);
        if(request("/frame",incoming,FRAME_BYTES,&length) && length==FRAME_BYTES) {
            EnterCriticalSection(&guard);
            memcpy(pagePixels,incoming,FRAME_BYTES);haveFrame=true;
            LeaveCriticalSection(&guard);
            if(actionOk)setStatus("Connected to PC browser  |  Google search  |  Preview 0.1.0");
        }
        Sleep(350);
    }
    free(incoming);return 0;
}
static void loadConfig() {
    FILE* file=NULL;
    if(fopen_s(&file,"game:\\browser.ini","rb")!=0 || !file)return;
    char line[256];
    while(fgets(line,sizeof(line),file)) {
        line[strcspn(line,"\r\n")]=0;
        if(strncmp(line,"host=",5)==0)strncpy_s(host,sizeof(host),line+5,_TRUNCATE);
        if(strncmp(line,"token=",6)==0)strncpy_s(token,sizeof(token),line+6,_TRUNCATE);
        if(strncmp(line,"port=",5)==0) {int v=atoi(line+5);if(v>0 && v<65536)port=v;}
    }
    fclose(file);
}
static bool compileShader(const char* source,const char* profile,ID3DXBuffer** result) {
    ID3DXBuffer* errors=NULL;
    HRESULT hr=D3DXCompileShader(source,(UINT)strlen(source),NULL,NULL,"main",profile,0,result,&errors,NULL);
    if(errors) {OutputDebugStringA((char*)errors->GetBufferPointer());errors->Release();}
    return SUCCEEDED(hr);
}
static bool initializeGraphics() {
    IDirect3D9* d3d=Direct3DCreate9(D3D_SDK_VERSION);if(!d3d)return false;
    D3DPRESENT_PARAMETERS pp;ZeroMemory(&pp,sizeof(pp));
    pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferCount=1;
    pp.BackBufferFormat=(D3DFORMAT)MAKESRGBFMT(D3DFMT_A8R8G8B8);
    pp.FrontBufferFormat=(D3DFORMAT)MAKESRGBFMT(D3DFMT_LE_X8R8G8B8);
    pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
    HRESULT hr=d3d->CreateDevice(0,D3DDEVTYPE_HAL,NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device);d3d->Release();if(FAILED(hr))return false;
    const char* vertex="struct V{float2 p:POSITION;float2 uv:TEXCOORD0;};struct O{float4 p:POSITION;float2 uv:TEXCOORD0;};O main(V v){O o;o.p=float4(v.p,0,1);o.uv=v.uv;return o;}";
    const char* pixel="sampler2D image:register(s0);float4 main(float2 uv:TEXCOORD0):COLOR{return float4(tex2D(image,uv).rgb,1);}";
    ID3DXBuffer* code=NULL;
    if(!compileShader(vertex,"vs_2_0",&code))return false;
    hr=device->CreateVertexShader((DWORD*)code->GetBufferPointer(),&vs);code->Release();if(FAILED(hr))return false;
    if(!compileShader(pixel,"ps_2_0",&code))return false;
    hr=device->CreatePixelShader((DWORD*)code->GetBufferPointer(),&ps);code->Release();if(FAILED(hr))return false;
    const D3DVERTEXELEMENT9 elements[]={
        {0,0,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
        {0,8,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
    if(FAILED(device->CreateVertexDeclaration(elements,&declaration)))return false;
    if(FAILED(device->CreateTexture(W,H,1,0,D3DFMT_LIN_A8R8G8B8,D3DPOOL_DEFAULT,&texture,NULL)))return false;
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    xui_canvas_init(&canvas,canvasPixels,W,H,W);return true;
}
static void label(int x,int y,const char* text,uint32_t color=0xe4ebef,int font=XUI_F_BODY) {
    xui_text(&canvas,font,x,y,color,text,0,W-x-20);
}
static void draw() {
    xui_fill(&canvas,0,0,W,H,0x12191e);
    xui_round_rect(&canvas,18,10,246,30,7,0x2c363c,255);label(32,15,"X360Browser  /  New tab",0xdce8e8,XUI_F_LABEL);
    label(286,15,"+",0x91dabc,XUI_F_LABEL);label(1030,15,"Xbox 360  |  Preview",0x8daca7,XUI_F_LABEL);
    label(22,59,"<    >    @",0xb5c6c9);
    xui_round_rect(&canvas,155,48,1100,39,7,0x27343b,255);
    label(174,57,"Press Y to search Google or enter a website address",0xc1d2d5);
    EnterCriticalSection(&guard);
    bool connected=haveFrame;
    if(connected)memcpy(canvasPixels+W*PAGE_Y,pagePixels,FRAME_BYTES);
    char status[192];strcpy_s(status,statusText);
    LeaveCriticalSection(&guard);
    if(!connected) {
        xui_vgrad(&canvas,0,PAGE_Y,W,PAGE_H,0x286d69,120,0);
        label(70,230,"Your web. On your 360.",0xe1f6ec,XUI_F_TITLE);
        label(74,298,"Start the Edge or Chrome bridge on your PC.");
        label(74,340,"Copy browser.ini beside default.xex, then reopen the app.");
        char info[160];sprintf_s(info,sizeof(info),"PC: %s:%d",host,port);label(74,392,info,0x8fd8be);
        if(!token[0])label(74,438,"browser.ini is missing. Generate it with scripts/configure.py.",0xf3ba78);
    } else {
        int x=(int)pointerX,y=(int)pointerY+PAGE_Y;
        xui_circle(&canvas,x,y,9,0x00130c,240);xui_circle(&canvas,x,y,5,0x8fffc6,255);
    }
    label(22,668,"A Click   B Back   X Type   Y Address   START Enter   LB/RB History   LT/RT Scroll   BACK Home",0xd3e2df,XUI_F_LABEL);
    label(22,694,status,0x8db5a8,XUI_F_LABEL);
    D3DLOCKED_RECT lock;
    if(SUCCEEDED(texture->LockRect(0,&lock,NULL,0))) {
        for(int y=0;y<H;++y)memcpy((char*)lock.pBits+y*lock.Pitch,canvasPixels+y*W,W*4);
        texture->UnlockRect(0);
    }
    struct Vertex{float x,y,u,v;};
    const Vertex vertices[]={{-1,1,0,0},{1,1,1,0},{-1,-1,0,1},{1,-1,1,1}};
    device->SetVertexShader(vs);device->SetPixelShader(ps);device->SetVertexDeclaration(declaration);device->SetTexture(0,texture);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));
    device->Present(NULL,NULL,NULL,NULL);
}
static void openKeyboard(int mode) {
    if(keyboardMode)return;
    ZeroMemory(&keyboard,sizeof(keyboard));ZeroMemory(keyboardText,sizeof(keyboardText));
    DWORD result=XShowKeyboardUI(0,VKBD_DEFAULT,L"",mode==1?L"Search or address":L"Type on page",mode==1?L"Enter a Google search or an HTTPS website address.":L"Click a page text field first. START submits Enter.",keyboardText,1025,&keyboard);
    if(result==ERROR_IO_PENDING || result==ERROR_SUCCESS)keyboardMode=mode;
}
static void input() {
    if(keyboardMode && XHasOverlappedIoCompleted(&keyboard)) {
        DWORD count=0;
        DWORD result=XGetOverlappedResult(&keyboard,&count,FALSE);
        if(result==ERROR_SUCCESS && keyboardText[0]) {
            char text[3073];WideCharToMultiByte(CP_UTF8,0,keyboardText,-1,text,sizeof(text),NULL,NULL);
            enqueue(keyboardMode==1?"navigate":"text",text);
        }
        keyboardMode=0;
    }
    XINPUT_STATE state;if(XInputGetState(0,&state)!=ERROR_SUCCESS)return;
    WORD pressed=state.Gamepad.wButtons & ~previousButtons;previousButtons=state.Gamepad.wButtons;
    if(keyboardMode)return;
    float dx=state.Gamepad.sThumbLX/32768.0f,dy=state.Gamepad.sThumbLY/32768.0f;
    if(fabsf(dx)>0.18f)pointerX+=dx*10;
    if(fabsf(dy)>0.18f)pointerY-=dy*10;
    if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT)pointerX-=5;
    if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT)pointerX+=5;
    if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_UP)pointerY-=5;
    if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN)pointerY+=5;
    if(pointerX<0)pointerX=0;if(pointerX>W-1)pointerX=W-1;
    if(pointerY<0)pointerY=0;if(pointerY>PAGE_H-1)pointerY=PAGE_H-1;
    if(pressed&XINPUT_GAMEPAD_A)enqueue("click");
    if(pressed&XINPUT_GAMEPAD_B)enqueue("back");
    if(pressed&XINPUT_GAMEPAD_X)openKeyboard(2);
    if(pressed&XINPUT_GAMEPAD_Y)openKeyboard(1);
    if(pressed&XINPUT_GAMEPAD_START)enqueue("enter");
    if(pressed&XINPUT_GAMEPAD_BACK)enqueue("home");
    if(pressed&XINPUT_GAMEPAD_LEFT_SHOULDER)enqueue("back");
    if(pressed&XINPUT_GAMEPAD_RIGHT_SHOULDER)enqueue("forward");
    if(pressed&XINPUT_GAMEPAD_RIGHT_THUMB)enqueue("reload");
    static DWORD lastScroll=0;
    if(GetTickCount()-lastScroll>300) {
        if(state.Gamepad.bRightTrigger>50) {enqueue("scroll","420");lastScroll=GetTickCount();}
        else if(state.Gamepad.bLeftTrigger>50) {enqueue("scroll","-420");lastScroll=GetTickCount();}
    }
}
void __cdecl main() {
    InitializeCriticalSection(&guard);loadConfig();
    if(!initializeGraphics()) {OutputDebugStringA("X360Browser graphics initialization failed.");return;}
    XNetStartupParams params;ZeroMemory(&params,sizeof(params));params.cfgSizeOfStruct=sizeof(params);params.cfgFlags=XNET_STARTUP_BYPASS_SECURITY;
    if(XNetStartup(&params)!=0)setStatus("Network startup failed.");
    WSADATA data;if(WSAStartup(MAKEWORD(2,2),&data)!=0)setStatus("Socket startup failed.");
    HANDLE worker=CreateThread(NULL,0,networkWorker,NULL,0,NULL);
    if(!worker)setStatus("Could not start the browser connection.");
    for(;;) {input();draw();Sleep(1);}
}
