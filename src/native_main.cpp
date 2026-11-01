// X360Browser 0.4.0: native HTML/CSS, media and green browser chrome. GPL-3.0.
#include <xtl.h>
#include <winsockx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lg_ui.h"
#include "native_net.h"
#include "image_decoder.h"
#include <xaudio2.h>
#include <xmedia2.h>
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
struct BrowserTab {WebDocument* doc;char url[WEB_URL];char history[32][WEB_URL];int historyIndex,historyCount,scroll;bool home,loading;};
static BrowserTab tabs[4];static int tabCount=1,activeTab=0,tabSelection=0;static bool tabPanelOpen=false;
static DWORD frameCPUms=0;static char perfText[128]="RAM(cache): 0KB | CPU: N/A | Frame: -- | Temp: N/A";
static unsigned generation=0;
static float mouseX=640,mouseY=325;
static XOVERLAPPED keyboard;
static WCHAR keyboardText[513];
static bool keyboardActive=false,keyboardGoogle=false;
static DWORD previousButtons=0;
static DWORD lastInteraction=0;
static bool chromeVisible=true,menuOpen=false;
static int menuSelection=0;
static IXAudio2* audioEngine=NULL;
static IXAudio2MasteringVoice* masterVoice=NULL;
static IXMedia2XmvPlayer* movie=NULL;
static unsigned char* movieBytes=NULL;
static unsigned char* readyMovieBytes=NULL;
static DWORD readyMovieSize=0;
static void stopMovie(){if(movie){movie->Stop(XMEDIA_STOP_IMMEDIATE);movie->Release();movie=NULL;}free(movieBytes);movieBytes=NULL;}
static NativeView makeView(){NativeView view={home,loading,toolbarFocus,toolbarSelection,(int)mouseX,(int)mouseY,pageScroll,currentURL,statusText,shownDoc,chromeVisible,menuOpen,menuSelection,movie!=NULL};view.perf=perfText;view.tabsOpen=tabPanelOpen;view.tabCount=tabCount;view.activeTab=activeTab;for(int i=0;i<tabCount;++i)view.tabUrls[i]=(i==activeTab)?currentURL:tabs[i].url;return view;}

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
    const char* pixel="sampler2D image:register(s0);float4 main(float2 uv:TEXCOORD0):COLOR{return tex2D(image,uv);}";
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
    stopMovie();menuOpen=false;lastInteraction=GetTickCount();
    char url[WEB_URL];
    if(!webSearchOrURL(input,url,sizeof(url),google)) {
        EnterCriticalSection(&guard);strcpy_s(statusText,"Address is too long or unsupported. Use an HTTPS website or a search query.");LeaveCriticalSection(&guard);return;
    }
    EnterCriticalSection(&guard);
    free(readyMovieBytes);readyMovieBytes=NULL;readyMovieSize=0;
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
static void rememberTab(){
    BrowserTab* t=&tabs[activeTab];t->doc=shownDoc;
    strcpy_s(t->url,currentURL);memcpy(t->history,history,sizeof(history));
    t->historyIndex=historyIndex;t->historyCount=historyCount;t->scroll=pageScroll;t->home=home;t->loading=loading;
}
static void activateTab(int index){
    if(index<0||index>=tabCount)return;
    EnterCriticalSection(&guard);
    rememberTab();++generation;InterlockedExchange(&webNetState.cancel,1);
    activeTab=index;BrowserTab* t=&tabs[index];shownDoc=t->doc;
    strcpy_s(currentURL,t->url);memcpy(history,t->history,sizeof(history));
    historyIndex=t->historyIndex;historyCount=t->historyCount;pageScroll=t->scroll;
    home=t->home;loading=t->loading&&!home;pending=loading;
    if(pending){strcpy_s(pendingURL,currentURL);strcpy_s(statusText,"Loading tab...");}
    else strcpy_s(statusText,"Tab ready.");
    tabPanelOpen=false;menuOpen=false;lastInteraction=GetTickCount();
    LeaveCriticalSection(&guard);
}
static void createTab(){
    if(tabCount>=4){tabPanelOpen=true;return;}
    BrowserTab* t=&tabs[tabCount];memset(t,0,sizeof(*t));
    t->doc=(WebDocument*)calloc(1,sizeof(WebDocument));if(!t->doc)return;
    strcpy_s(t->url,"https://www.google.com/");strcpy_s(t->history[0],"https://www.google.com/");
    t->historyCount=1;t->home=false;t->loading=true;int index=tabCount++;
    activateTab(index);
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
#include "native_fetch.h"
static DWORD WINAPI nativeWorker(void*) {
    WebDocument* spare=(WebDocument*)calloc(1,sizeof(WebDocument));
    if(!spare)return 1;
    for(;;) {
        char url[WEB_URL]="";unsigned requestGeneration=0;
        EnterCriticalSection(&guard);if(pending){strcpy_s(url,pendingURL);pending=false;requestGeneration=generation;}LeaveCriticalSection(&guard);
        if(!url[0]){Sleep(30);continue;}
        InterlockedExchange(&webNetState.cancel,0);webNetState.httpCode=webNetState.tlsError=webNetState.socketError=0;
        webReleaseImages(spare);memset(spare,0,sizeof(*spare));
        WebDownload download={0};char message[192]="";bool success=false;
        if(webEqual(url,"browser:help")) {
            const char* help="<h1>Images and video</h1><p>JPEG, PNG, BMP and the first frame of GIF images are decoded by the console.</p><p>Open a direct WMV/XMV video link. The file is buffered in RAM before playback. Maximum size: 64 MB.</p><p>A pauses/resumes video. B stops playback. The toolbar hides after 4.5 seconds without input.</p><p>MP4/H.264, HLS/DASH, DRM, YouTube and JavaScript video players are not supported by this build.</p><p>All networking and rendering runs on the Xbox. Physical console media testing remains pending.</p>";
            webParseHTML(spare,help,url);success=true;strcpy_s(message,"Native media guide");
        } else success=webDownload(url,&download,message,sizeof(message));
        if(success && download.bytes) {
            if(download.kind==WEB_DOWNLOAD_IMAGE) {
                strcpy_s(spare->url,download.url);strcpy_s(spare->title,"Image");spare->imageCount=1;
                WebImage* image=&spare->images[0];strcpy_s(image->url,download.url);
                image->pixels=browserDecodeImage(download.bytes,(int)download.size,&image->width,&image->height);image->state=image->pixels?1:-1;
                spare->count=1;spare->lines[0].kind=WEB_IMAGE_ROW;spare->lines[0].media=0;spare->lines[0].link=-1;webLayout(spare);
                strcpy_s(message,image->pixels?"Image loaded directly on the console":"Image decoder could not open this format or image dimensions");
            } else if(download.kind==WEB_DOWNLOAD_VIDEO) {
                EnterCriticalSection(&guard);
                if(requestGeneration==generation){free(readyMovieBytes);readyMovieBytes=download.bytes;readyMovieSize=download.size;download.bytes=NULL;loading=false;strcpy_s(statusText,"Video buffered. Starting native playback...");}
                LeaveCriticalSection(&guard);free(download.bytes);continue;
            } else {
                webParseHTML(spare,(const char*)download.bytes,download.url);spare->truncated=spare->truncated||download.truncated;
                EnterCriticalSection(&guard);if(requestGeneration==generation)strcpy_s(statusText,"Loading website styles and images...");LeaveCriticalSection(&guard);
                spare->styled=browserStyledCreate((const char*)download.bytes,download.url,webStyledResourceFetch,NULL);
                if(spare->styled){spare->height=browserStyledHeight(spare->styled);sprintf_s(message,sizeof(message),"Page ready. Stylesheets: %d",browserStyledCSSCount(spare->styled));}
                else strcpy_s(message,"Using the text reader for this page.");
            }
        }
        if(!success){char html[1024];sprintf_s(html,sizeof(html),"<h1>Page or media could not load</h1><p>%s</p><p>Press Y for another address or BACK for Home.</p>",message);webParseHTML(spare,html,url);}
        free(download.bytes);download.bytes=NULL;
        EnterCriticalSection(&guard);
        WebDocument* published=NULL;
        if(requestGeneration==generation){WebDocument* previous=shownDoc;shownDoc=spare;tabs[activeTab].doc=shownDoc;spare=previous;published=shownDoc;strcpy_s(currentURL,published->url);strcpy_s(statusText,message);loading=false;home=false;pageScroll=0;}
        LeaveCriticalSection(&guard);
        if(!published)continue;
        if(published->styled)for(int i=0;i<12&&requestGeneration==generation&&!webNetState.cancel;++i){
            char asset[WEB_URL]="";bool required=false;
            EnterCriticalSection(&guard);
            if(requestGeneration==generation && published==shownDoc)required=browserStyledNextImage(published->styled,asset,sizeof(asset));
            LeaveCriticalSection(&guard);if(!required)break;
            WebDownload picture={0};char problem[192]="";bool ok=webDownload(asset,&picture,problem,sizeof(problem),true);
            EnterCriticalSection(&guard);
            if(requestGeneration==generation&&published==shownDoc){
                browserStyledCompleteImage(published->styled,asset,ok?picture.bytes:NULL,ok?(int)picture.size:0);
                published->height=browserStyledHeight(published->styled);
                sprintf_s(statusText,sizeof(statusText),"Page ready  |  CSS %d  |  Images %d",browserStyledCSSCount(published->styled),browserStyledImageCount(published->styled));
            }
            LeaveCriticalSection(&guard);free(picture.bytes);
        }
        for(int i=0;!published->styled && i<published->imageCount && requestGeneration==generation && !webNetState.cancel;++i) {
            if(published->images[i].pixels)continue;
            WebDownload picture={0};char problem[192];uint32_t* rgb=NULL;int width=0,height=0;
            if(webDownload(published->images[i].url,&picture,problem,sizeof(problem),true))rgb=browserDecodeImage(picture.bytes,(int)picture.size,&width,&height);
            free(picture.bytes);
            EnterCriticalSection(&guard);
            if(requestGeneration==generation && published==shownDoc){WebImage* image=&published->images[i];image->pixels=rgb;image->width=width;image->height=height;image->state=rgb?1:-1;webLayout(published);}else free(rgb);
            LeaveCriticalSection(&guard);
        }
        EnterCriticalSection(&guard);
        if(requestGeneration==generation && success && download.kind==WEB_DOWNLOAD_HTML){int loaded=0;for(int i=0;i<shownDoc->imageCount;++i)if(shownDoc->images[i].pixels)++loaded;sprintf_s(statusText,sizeof(statusText),"Native page  |  %d links  |  Images %d/%d  |  L3: menu",shownDoc->linkCount,loaded,shownDoc->imageCount);}
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
    if(selected==0)goHistory(-1);else if(selected==1)goHistory(1);else if(selected==2){webCacheForget(currentURL);navigate(currentURL,false);}
    else if(selected==3||selected==6)navigate("https://www.google.com/");else if(selected==5){menuOpen=!menuOpen;menuSelection=0;}else openKeyboard();
}
static void menuAction(int selected){menuOpen=false;if(selected==0)navigate("https://www.google.com/");else if(selected==1)goHistory(-1);else if(selected==2)goHistory(1);else if(selected==3){webCacheForget(currentURL);navigate(currentURL,false);}else if(selected==4)navigate("browser:help");}
static void click() {
    if(tabPanelOpen){int hit=lgTabsHit((int)mouseX,(int)mouseY);if(hit==0)createTab();else if(hit>0&&hit<=tabCount)activateTab(hit-1);else tabPanelOpen=false;return;}
    if(menuOpen){int item=lgMenuHit((int)mouseX,(int)mouseY);if(item>=0)menuAction(item);else menuOpen=false;return;}
    if(chromeVisible && mouseY<76) {
        if(mouseX>=204 && mouseX<1108){openKeyboard();return;}
        for(int i=0;i<4;++i)if(mouseX>=12+i*46 && mouseX<48+i*46){toolbarAction(i);return;}
        if(mouseX>=1121 && mouseX<1155){tabPanelOpen=!tabPanelOpen;tabSelection=0;menuOpen=false;return;}
        if(mouseX>=1156 && mouseX<1204){toolbarAction(5);return;}
        if(mouseX>=1204){toolbarAction(6);return;}
    }
    if(home) {
        int hit=nativeHomeHit((int)mouseX,(int)mouseY);
        const char* urls[]={"https://www.google.com/","https://www.google.com/","https://en.wikipedia.org/wiki/Main_Page","https://x360hub.vercel.app/"};
        if(hit==0)openKeyboard();else if(hit>=1 && hit<=4)navigate(urls[hit-1]);else if(hit==5)navigate("browser:help");
    } else {
        EnterCriticalSection(&guard);
        NativeView view=makeView();
        int link=-1;char url[WEB_URL]="";
        if(shownDoc->styled)browserStyledClick(shownDoc->styled,(int)mouseX,(int)mouseY-lgPageTop(&view)+pageScroll,url,sizeof(url));
        else {link=nativeHitLink(&view);if(link>=0 && link<shownDoc->linkCount)strcpy_s(url,shownDoc->links[link]);}
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
    bool moving=abs(state.Gamepad.sThumbLX)>6000||abs(state.Gamepad.sThumbLY)>6000||abs(state.Gamepad.sThumbRY)>8000;
    if(moving || pressed || state.Gamepad.bLeftTrigger>40 || state.Gamepad.bRightTrigger>40)lastInteraction=GetTickCount();
    if(pressed&XINPUT_GAMEPAD_LEFT_THUMB){menuOpen=!menuOpen;tabPanelOpen=false;menuSelection=0;}
    if(tabPanelOpen) {
        if(pressed&XINPUT_GAMEPAD_B)tabPanelOpen=false;
        if(pressed&XINPUT_GAMEPAD_DPAD_DOWN)tabSelection=(tabSelection+1)%(tabCount+1);
        if(pressed&XINPUT_GAMEPAD_DPAD_UP)tabSelection=(tabSelection+tabCount)%(tabCount+1);
        if(pressed&(XINPUT_GAMEPAD_DPAD_DOWN|XINPUT_GAMEPAD_DPAD_UP)){
            mouseX=825;mouseY=(float)(tabSelection==0?153:207+(tabSelection-1)*69);
        }
        float dx=state.Gamepad.sThumbLX/32768.0f,dy=state.Gamepad.sThumbLY/32768.0f;
        if(fabsf(dx)>0.18f)mouseX+=dx*10;if(fabsf(dy)>0.18f)mouseY-=dy*10;
        if(pressed&XINPUT_GAMEPAD_A)click();
    } else if(menuOpen) {
        if(pressed&XINPUT_GAMEPAD_DPAD_UP)menuSelection=(menuSelection+5)%6;
        if(pressed&XINPUT_GAMEPAD_DPAD_DOWN)menuSelection=(menuSelection+1)%6;
        if(pressed&(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN)){mouseX=942;mouseY=(float)(150+menuSelection*48);}
        if(pressed&XINPUT_GAMEPAD_A){int hit=lgMenuHit((int)mouseX,(int)mouseY);menuAction(hit>=0?hit:menuSelection);}
        if(pressed&XINPUT_GAMEPAD_B)menuOpen=false;
        float dx=state.Gamepad.sThumbLX/32768.0f,dy=state.Gamepad.sThumbLY/32768.0f;if(fabsf(dx)>0.18f)mouseX+=dx*10;if(fabsf(dy)>0.18f)mouseY-=dy*10;
    } else if(toolbarFocus) {
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
        if(pressed&XINPUT_GAMEPAD_A){if(movie && !(chromeVisible && mouseY<76)){XMEDIA_PLAYBACK_STATUS playback;ZeroMemory(&playback,sizeof(playback));if(SUCCEEDED(movie->GetStatus(&playback))){if(playback.Status==XMEDIA_PLAYER_PAUSED)movie->Resume();else movie->Pause();}}else click();}
        if(pressed&XINPUT_GAMEPAD_B){if(movie){stopMovie();EnterCriticalSection(&guard);strcpy_s(statusText,"Video stopped. Press BACK for Home.");LeaveCriticalSection(&guard);}else goHistory(-1);}
    }
    if(mouseX<10)mouseX=10;if(mouseX>1260)mouseX=1260;if(mouseY<10)mouseY=10;if(mouseY>696)mouseY=696;
    if(pressed&XINPUT_GAMEPAD_Y)openKeyboard();
    if(pressed&XINPUT_GAMEPAD_X)openKeyboard(true);
    if(pressed&XINPUT_GAMEPAD_BACK)navigate("https://www.google.com/");
    if(pressed&XINPUT_GAMEPAD_LEFT_SHOULDER)goHistory(-1);
    if(pressed&XINPUT_GAMEPAD_RIGHT_SHOULDER)goHistory(1);
    if(pressed&XINPUT_GAMEPAD_RIGHT_THUMB || pressed&XINPUT_GAMEPAD_START){webCacheForget(currentURL);navigate(currentURL,false);}
    EnterCriticalSection(&guard);
    if(!home && shownDoc) {
        int change=(state.Gamepad.bRightTrigger>40?12:0)-(state.Gamepad.bLeftTrigger>40?12:0);
        if(abs(state.Gamepad.sThumbRY)>8000)change-=state.Gamepad.sThumbRY/2000;
        pageScroll+=change;int maximum=shownDoc->height-(720-(chromeVisible?76:0));if(maximum<0)maximum=0;if(pageScroll<0)pageScroll=0;if(pageScroll>maximum)pageScroll=maximum;
    }
    LeaveCriticalSection(&guard);
}
static void draw() {
    DWORD frameStart=GetTickCount();
    unsigned char* incomingMovie=NULL;DWORD incomingSize=0;
    EnterCriticalSection(&guard);if(readyMovieBytes){incomingMovie=readyMovieBytes;incomingSize=readyMovieSize;readyMovieBytes=NULL;readyMovieSize=0;}LeaveCriticalSection(&guard);
    if(incomingMovie) {
        stopMovie();HRESULT hr=S_OK;
        if(!audioEngine){hr=XAudio2Create(&audioEngine,0);if(SUCCEEDED(hr))hr=audioEngine->CreateMasteringVoice(&masterVoice);}
        if(SUCCEEDED(hr)){XMEDIA_XMV_CREATE_PARAMETERS parameters;ZeroMemory(&parameters,sizeof(parameters));parameters.createType=XMEDIA_CREATE_FROM_MEMORY;parameters.createFromMemory.pvBuffer=incomingMovie;parameters.createFromMemory.dwBufferSize=incomingSize;hr=XMedia2CreateXmvPlayer(device,audioEngine,&parameters,&movie);}
        if(SUCCEEDED(hr)){movieBytes=incomingMovie;RECT rect={0,0,1280,720};movie->SetRectangle(&rect);lastInteraction=GetTickCount();}
        else {free(incomingMovie);EnterCriticalSection(&guard);sprintf_s(statusText,sizeof(statusText),"Native video decoder could not open this file (0x%08lX). WMV/XMV codecs vary.",hr);LeaveCriticalSection(&guard);}
    }
    if(movie){device->Clear(0,NULL,D3DCLEAR_TARGET,0xff000000,1,0);HRESULT hr=movie->RenderNextFrame(0,NULL);if(FAILED(hr)||hr==XMEDIA_W_EOF){stopMovie();EnterCriticalSection(&guard);strcpy_s(statusText,"Video finished or stopped. Press BACK for Home.");LeaveCriticalSection(&guard);}}
    EnterCriticalSection(&guard);
    chromeVisible=lgChromeVisible(GetTickCount(),lastInteraction,menuOpen||tabPanelOpen,keyboardActive,loading);
    sprintf_s(perfText,sizeof(perfText),"RAM(cache): %luKB | CPU: N/A | Frame: %lums | Temp: N/A",browserCacheBytes/1024,frameCPUms);
    NativeView view=makeView();
    nativeRender(&canvas,&view);LeaveCriticalSection(&guard);
    // Do not overwrite the linear UI texture while the GPU samples its prior frame.
    // This XDK synchronization replaces the libxenon framebuffer/cache operations.
    device->BlockUntilIdle();
    D3DLOCKED_RECT lock;
    if(SUCCEEDED(texture->LockRect(0,&lock,NULL,0))){for(int y=0;y<H;++y){uint32_t* target=(uint32_t*)((char*)lock.pBits+y*lock.Pitch);for(int x=0;x<W;++x){uint32_t color=canvasPixels[y*W+x];target[x]=(!movie||color)?color|0xff000000:0;}}texture->UnlockRect(0);}
    struct Vertex{float x,y,u,v;};const Vertex vertices[]={{-1,1,0,0},{1,1,1,0},{-1,-1,0,1},{1,-1,1,1}};
    device->SetVertexShader(vs);device->SetPixelShader(ps);device->SetVertexDeclaration(declaration);device->SetTexture(0,texture);
    device->SetRenderState(D3DRS_VIEWPORTENABLE,TRUE);D3DVIEWPORT9 viewport={0,0,W,H,0,1};device->SetViewport(&viewport);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));device->Present(NULL,NULL,NULL,NULL);frameCPUms=GetTickCount()-frameStart;
}
void __cdecl main() {
    lastInteraction=GetTickCount();
    InitializeCriticalSection(&guard);shownDoc=(WebDocument*)calloc(1,sizeof(WebDocument));if(!shownDoc)return;
    strcpy_s(history[0],"browser:home");tabs[0].doc=shownDoc;strcpy_s(tabs[0].url,"browser:home");strcpy_s(tabs[0].history[0],"browser:home");tabs[0].historyCount=1;tabs[0].home=true;
    if(!initializeGraphics()){OutputDebugStringA("Native browser graphics initialization failed.");return;}
    XNetStartupParams params={0};params.cfgSizeOfStruct=sizeof(params);params.cfgFlags=XNET_STARTUP_BYPASS_SECURITY;
    params.cfgSockDefaultRecvBufsizeInK=64;params.cfgSockDefaultSendBufsizeInK=32;
    int networkResult=XNetStartup(&params);WSADATA data;int socketResult=networkResult?networkResult:WSAStartup(MAKEWORD(2,2),&data);
    if(networkResult || socketResult)sprintf_s(statusText,sizeof(statusText),"Xbox network startup failed (%d / %d).",networkResult,socketResult);
    else {HANDLE worker=CreateThread(NULL,512*1024,nativeWorker,NULL,0,NULL);if(!worker)strcpy_s(statusText,"Could not start the direct Internet connection.");else navigate("https://www.google.com/");}
    for(;;){input();draw();Sleep(1);}
}
