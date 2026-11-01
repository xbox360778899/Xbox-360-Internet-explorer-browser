// X360Browser 0.2.0: direct-console experimental HTML reader. GPL-3.0.
#include <xtl.h>
#include <winsockx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "native_ui.h"
#include "native_net.h"
enum {W=1280,H=720};
static IDirect3DDevice9* device;
static IDirect3DTexture9* texture;
static IDirect3DVertexShader9* vs;
static IDirect3DPixelShader9* ps;
static IDirect3DVertexDeclaration9* declaration;
static uint32_t canvasPixels[W*H];
static xui_canvas_t canvas;
static CRITICAL_SECTION guard;
static WebDocument* shownDoc;
static char currentURL[WEB_URL]="browser:home",pendingURL[WEB_URL]="";
static char statusText[192]="Native browser ready. Move the mouse with the left stick; press Y to search.";
static char history[32][WEB_URL];
static int historyIndex=0,historyCount=1,pageScroll=0,toolbarSelection=4;
static bool home=true,loading=false,toolbarFocus=false,pending=false;
static unsigned generation=0;
static float mouseX=640,mouseY=325;
static XOVERLAPPED keyboard;
static WCHAR keyboardText[513];
static bool keyboardActive=false,keyboardGoogle=false;
static DWORD previousButtons=0;

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
static void navigate(const char* input,bool addHistory=true,bool google=false) {
    char url[WEB_URL];
    if(!webSearchOrURL(input,url,sizeof(url),google)) {
        EnterCriticalSection(&guard);strcpy_s(statusText,"Address is too long or unsupported. Use an HTTPS website or a search query.");LeaveCriticalSection(&guard);return;
    }
    EnterCriticalSection(&guard);
    if(addHistory && strcmp(history[historyIndex],url)) {
        if(historyIndex==31){memmove(history,history+1,sizeof(history[0])*31);historyIndex=30;}
        ++historyIndex;strcpy_s(history[historyIndex],url);historyCount=historyIndex+1;
    }
    ++generation;InterlockedExchange(&webNetState.cancel,1);pageScroll=0;
    strcpy_s(currentURL,url);home=webEqual(url,"browser:home");pending=!home;loading=!home;
    if(pending){strcpy_s(pendingURL,url);strcpy_s(statusText,"Resolving the website directly on your Xbox...");}
    else strcpy_s(statusText,"Native browser ready. No PC, bridge or browser.ini is used.");
    LeaveCriticalSection(&guard);
}
static void goHistory(int delta) {
    int index=historyIndex+delta;if(index<0 || index>=historyCount)return;
    historyIndex=index;navigate(history[index],false);
}
static void errorMessage(char* output,size_t capacity,DWORD error) {
    if(error==ERROR_INVALID_TIME)sprintf_s(output,capacity,"Console date/time is invalid. Correct it before opening HTTPS websites.");
    else if(webNetState.tlsError)sprintf_s(output,capacity,"HTTPS failed: TLS %lu, network 0x%08lX. Check date/time and website compatibility.",webNetState.tlsError,webNetState.socketError);
    else if(webNetState.httpCode>=400)sprintf_s(output,capacity,"Website returned HTTP %lu. It may require JavaScript, cookies or verification.",webNetState.httpCode);
    else sprintf_s(output,capacity,"Website connection failed at step %lu (0x%08lX). Check console Internet and the address.",webNetState.httpPhase,error);
}
static DWORD WINAPI nativeWorker(void*) {
    WebDocument* spare=(WebDocument*)malloc(sizeof(WebDocument));
    char* html=(char*)malloc(WEB_HTML_LIMIT+1);
    if(!spare || !html){EnterCriticalSection(&guard);strcpy_s(statusText,"Not enough memory for the web reader.");loading=false;LeaveCriticalSection(&guard);free(spare);free(html);return 1;}
    for(;;) {
        char url[WEB_URL]="";unsigned requestGeneration=0;
        EnterCriticalSection(&guard);
        if(pending){strcpy_s(url,pendingURL);pending=false;requestGeneration=generation;}
        LeaveCriticalSection(&guard);
        if(!url[0]){Sleep(30);continue;}
        InterlockedExchange(&webNetState.cancel,0);webNetState.httpCode=webNetState.tlsError=webNetState.socketError=0;
        DWORD status=0;HubTLSRequest* request=hubNetOpen(url,&status);
        bool success=request!=NULL;size_t used=0;char finalURL[WEB_URL];strcpy_s(finalURL,url);char message[192]="";
        if(request) {
            if(strlen(request->finalURL)<sizeof(finalURL))strcpy_s(finalURL,request->finalURL);
            if(request->contentType[0] && !webPrefix(request->contentType,"text/") && !webPrefix(request->contentType,"application/xhtml+xml")) {
                success=false;strcpy_s(message,"This file type is not supported. The native preview reads HTML and text pages.");
            }
            while(success && used<WEB_HTML_LIMIT && !webNetState.cancel) {
                DWORD count=0;DWORD want=(DWORD)(WEB_HTML_LIMIT-used);if(want>16384)want=16384;
                if(!hubNetRead(request,html+used,want,&count)){success=false;break;}
                if(!count)break;used+=count;
            }
            if(webNetState.cancel)success=false;
            if(!success && !message[0])errorMessage(message,sizeof(message),hubNetError(request));
            hubNetClose(request);
        } else errorMessage(message,sizeof(message),GetLastError());
        html[used]=0;
        if(success) {
            webParseHTML(spare,html,finalURL);if(used==WEB_HTML_LIMIT)spare->truncated=true;
            sprintf_s(message,sizeof(message),"HTTPS loaded directly  /  %d links  /  %s",spare->linkCount,spare->truncated?"Page shortened to fit memory":"Text reader; JavaScript is not supported");
        } else {
            char errorHTML[1024];sprintf_s(errorHTML,sizeof(errorHTML),"<h1>Page could not load</h1><p>%s</p><p>Press Y to enter another address, or BACK to return home.</p>",message);
            webParseHTML(spare,errorHTML,url);
        }
        EnterCriticalSection(&guard);
        if(requestGeneration==generation) {
            WebDocument* previous=shownDoc;shownDoc=spare;spare=previous;
            strcpy_s(currentURL,success?finalURL:url);strcpy_s(statusText,message);loading=false;home=false;pageScroll=0;
        }
        LeaveCriticalSection(&guard);
    }
}
static void openKeyboard(bool google=false) {
    if(keyboardActive)return;
    ZeroMemory(&keyboard,sizeof(keyboard));ZeroMemory(keyboardText,sizeof(keyboardText));keyboardGoogle=google;
    DWORD result=XShowKeyboardUI(0,VKBD_DEFAULT,L"",google?L"Google search":L"Search or address",L"Enter a search query or an HTTPS website address. Runs directly on the console.",keyboardText,513,&keyboard);
    keyboardActive=result==ERROR_IO_PENDING || result==ERROR_SUCCESS;
}
static void toolbarAction(int selected) {
    if(selected==0)goHistory(-1);else if(selected==1)goHistory(1);else if(selected==2)navigate(currentURL,false);
    else if(selected==3)navigate("browser:home");else openKeyboard();
}
static void click() {
    if(mouseY>=62 && mouseY<98) {
        if(mouseX>=262 && mouseX<1216){openKeyboard();return;}
        for(int i=0;i<4;++i)if(mouseX>=64+i*46 && mouseX<100+i*46){toolbarAction(i);return;}
    }
    if(home) {
        int hit=nativeHomeHit((int)mouseX,(int)mouseY);
        const char* urls[]={"https://www.bing.com/","https://www.google.com/","https://en.wikipedia.org/wiki/Main_Page","https://x360hub.vercel.app/"};
        if(hit==0)openKeyboard();else if(hit>=1 && hit<=4)navigate(urls[hit-1]);
    } else {
        EnterCriticalSection(&guard);
        NativeView view={home,loading,toolbarFocus,toolbarSelection,(int)mouseX,(int)mouseY,pageScroll,currentURL,statusText,shownDoc};
        int link=nativeHitLink(&view);char url[WEB_URL]="";if(link>=0 && link<shownDoc->linkCount)strcpy_s(url,shownDoc->links[link]);
        LeaveCriticalSection(&guard);if(url[0])navigate(url);
    }
}
static void input() {
    if(keyboardActive && XHasOverlappedIoCompleted(&keyboard)) {
        DWORD count=0;DWORD result=XGetOverlappedResult(&keyboard,&count,FALSE);keyboardActive=false;
        if(result==ERROR_SUCCESS && keyboardText[0]){char text[1600];WideCharToMultiByte(CP_UTF8,0,keyboardText,-1,text,sizeof(text),NULL,NULL);navigate(text,true,keyboardGoogle);}
    }
    XINPUT_STATE state;if(XInputGetState(0,&state)!=ERROR_SUCCESS)return;
    WORD pressed=state.Gamepad.wButtons & ~previousButtons;previousButtons=state.Gamepad.wButtons;if(keyboardActive)return;
    if(pressed&XINPUT_GAMEPAD_LEFT_THUMB)toolbarFocus=!toolbarFocus;
    if(toolbarFocus) {
        if(pressed&XINPUT_GAMEPAD_DPAD_LEFT)toolbarSelection=(toolbarSelection+4)%5;
        if(pressed&XINPUT_GAMEPAD_DPAD_RIGHT)toolbarSelection=(toolbarSelection+1)%5;
        if(pressed&XINPUT_GAMEPAD_A)toolbarAction(toolbarSelection);
        if(pressed&XINPUT_GAMEPAD_B)toolbarFocus=false;
    } else {
        float dx=state.Gamepad.sThumbLX/32768.0f,dy=state.Gamepad.sThumbLY/32768.0f;
        if(fabsf(dx)>0.18f)mouseX+=dx*fabsf(dx)*14;
        if(fabsf(dy)>0.18f)mouseY-=dy*fabsf(dy)*14;
        if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT)mouseX-=5;
        if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT)mouseX+=5;
        if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_UP)mouseY-=5;
        if(state.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN)mouseY+=5;
        if(mouseX<10)mouseX=10;if(mouseX>1260)mouseX=1260;if(mouseY<10)mouseY=10;if(mouseY>696)mouseY=696;
        if(pressed&XINPUT_GAMEPAD_A)click();
        if(pressed&XINPUT_GAMEPAD_B)goHistory(-1);
    }
    if(pressed&XINPUT_GAMEPAD_Y)openKeyboard();
    if(pressed&XINPUT_GAMEPAD_X)openKeyboard(true);
    if(pressed&XINPUT_GAMEPAD_BACK)navigate("browser:home");
    if(pressed&XINPUT_GAMEPAD_LEFT_SHOULDER)goHistory(-1);
    if(pressed&XINPUT_GAMEPAD_RIGHT_SHOULDER)goHistory(1);
    if(pressed&XINPUT_GAMEPAD_RIGHT_THUMB || pressed&XINPUT_GAMEPAD_START)navigate(currentURL,false);
    EnterCriticalSection(&guard);
    if(!home && shownDoc) {
        int change=(state.Gamepad.bRightTrigger>40?12:0)-(state.Gamepad.bLeftTrigger>40?12:0);
        if(abs(state.Gamepad.sThumbRY)>8000)change-=state.Gamepad.sThumbRY/2000;
        pageScroll+=change;int maximum=shownDoc->height-440;if(maximum<0)maximum=0;if(pageScroll<0)pageScroll=0;if(pageScroll>maximum)pageScroll=maximum;
    }
    LeaveCriticalSection(&guard);
}
static void draw() {
    EnterCriticalSection(&guard);
    NativeView view={home,loading,toolbarFocus,toolbarSelection,(int)mouseX,(int)mouseY,pageScroll,currentURL,statusText,shownDoc};
    nativeRender(&canvas,&view);LeaveCriticalSection(&guard);
    D3DLOCKED_RECT lock;
    if(SUCCEEDED(texture->LockRect(0,&lock,NULL,0))){for(int y=0;y<H;++y)memcpy((char*)lock.pBits+y*lock.Pitch,canvasPixels+y*W,W*4);texture->UnlockRect(0);}
    struct Vertex{float x,y,u,v;};const Vertex vertices[]={{-1,1,0,0},{1,1,1,0},{-1,-1,0,1},{1,-1,1,1}};
    device->SetVertexShader(vs);device->SetPixelShader(ps);device->SetVertexDeclaration(declaration);device->SetTexture(0,texture);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));device->Present(NULL,NULL,NULL,NULL);
}
void __cdecl main() {
    InitializeCriticalSection(&guard);shownDoc=(WebDocument*)calloc(1,sizeof(WebDocument));if(!shownDoc)return;
    strcpy_s(history[0],"browser:home");
    if(!initializeGraphics()){OutputDebugStringA("Native browser graphics initialization failed.");return;}
    XNetStartupParams params={0};params.cfgSizeOfStruct=sizeof(params);params.cfgFlags=XNET_STARTUP_BYPASS_SECURITY;
    params.cfgSockDefaultRecvBufsizeInK=64;params.cfgSockDefaultSendBufsizeInK=32;
    int networkResult=XNetStartup(&params);WSADATA data;int socketResult=networkResult?networkResult:WSAStartup(MAKEWORD(2,2),&data);
    if(networkResult || socketResult)sprintf_s(statusText,sizeof(statusText),"Xbox network startup failed (%d / %d).",networkResult,socketResult);
    else {HANDLE worker=CreateThread(NULL,512*1024,nativeWorker,NULL,0,NULL);if(!worker)strcpy_s(statusText,"Could not start the direct Internet connection.");}
    for(;;){input();draw();Sleep(1);}
}
