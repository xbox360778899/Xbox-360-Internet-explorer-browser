#include "host_net_platform.h"
#include <assert.h>
#include "native_document.h"
#include "native_ui.h"
#include "native_net.h"
static WebDocument document;
static uint32_t screen[1280*720];
static void save(const char* path) {
    FILE* out=fopen(path,"wb");assert(out);fprintf(out,"P6\n1280 720\n255\n");
    for(int i=0;i<1280*720;++i){fputc((screen[i]>>16)&255,out);fputc((screen[i]>>8)&255,out);fputc(screen[i]&255,out);}fclose(out);
}
static void checks() {
    char url[WEB_URL];
    assert(webSearchOrURL("Xbox 360",url,sizeof(url)) && !strcmp(url,"https://www.bing.com/search?q=Xbox%20360"));
    assert(webSearchOrURL("xbox 360",url,sizeof(url),true) && webPrefix(url,"https://www.google.com/search?q="));
    assert(webResolveURL("https://example.com/a/b","/test?q=1",url,sizeof(url)) && !strcmp(url,"https://example.com/test?q=1"));
    assert(webResolveURL("https://example.com/a/b","next",url,sizeof(url)) && !strcmp(url,"https://example.com/a/next"));
    assert(webResolveURL("https://example.com/a/b","//other.example/a",url,sizeof(url)) && !strcmp(url,"https://other.example/a"));
    assert(!webResolveURL("https://example.com/","javascript:alert(1)",url,sizeof(url)));
    assert(!webResolveURL("https://example.com/","https://user:pass@example.com/",url,sizeof(url)));
    assert(!webResolveURL("https://example.com/","https://example.com/a\r\nInjected",url,sizeof(url)));
    const char* fixture="<html><head><title>Native &amp; independent</title><style>CSS must be hidden</style></head><body><h1>Browse directly on Xbox 360</h1><p>This page is parsed and drawn by the console. No computer is used to render it.</p><script>if(a<b){var s=\"<div>\";} SECRET_SCRIPT</script><a href='/one?a=1&amp;b=2'>Open the next page</a><p>Accents: caf&#233; &#xE9; &lt;Xbox&gt;</p><p>Use the mouse pointer to open links. Scroll with the triggers or right stick.</p></body></html>";
    webParseHTML(&document,fixture,"https://example.com/");
    assert(!strcmp(document.title,"Native & independent"));assert(document.linkCount==1);
    assert(!strcmp(document.links[0],"https://example.com/one?a=1&b=2"));
    bool visible=false;
    for(int i=0;i<document.count;++i){assert(!strstr(document.lines[i].text,"SECRET_SCRIPT"));assert(!strstr(document.lines[i].text,"CSS must"));if(strstr(document.lines[i].text,"caf"))visible=true;}assert(visible);
    xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);
    NativeView view={true,false,false,4,640,325,0,"browser:home","Native browser ready. No PC, bridge or browser.ini is used.",&document};
    nativeRender(&canvas,&view);save("obj/native-home.ppm");
    view.home=false;view.url=document.url;view.cursorX=150;view.cursorY=245;view.status="Native layout demonstration. Website compatibility depends on the HTML reader.";
    nativeRender(&canvas,&view);save("obj/native-reader.ppm");
    for(int i=0;i<document.count;++i)if(document.lines[i].link>=0){view.cursorX=100;view.cursorY=150+document.lines[i].y;assert(nativeHitLink(&view)==0);}
    assert(nativeHomeHit(350,320)==0 && nativeHomeHit(320,420)==1);
    printf("PASS: native URL/search handling, entity decoding, hidden scripts/styles, clickable links and shared UI rendering.\n");
}
static void networkCheck(const char* url,const char* name) {
    webNetState.cancel=0;webNetState.socketError=webNetState.tlsError=webNetState.httpCode=0;
    DWORD status=0;HubTLSRequest* request=hubNetOpen(url,&status);
    if(!request){printf("NETWORK %s: failed, HTTP=%lu phase=%lu socket=%lu TLS=%lu error=%lu\n",name,status,webNetState.httpPhase,webNetState.socketError,webNetState.tlsError,GetLastError());return;}
    char* html=(char*)malloc(WEB_HTML_LIMIT+1);assert(html);DWORD received=0;bool ok=true;
    while(received<WEB_HTML_LIMIT){DWORD count=0;DWORD want=WEB_HTML_LIMIT-received;if(want>16384)want=16384;if(!hubNetRead(request,html+received,want,&count)){ok=false;break;}if(!count)break;received+=count;}
    html[received]=0;webParseHTML(&document,html,request->finalURL);
    char fixturePath[128];sprintf(fixturePath,"obj/net-%s.html",name);FILE* fixture=fopen(fixturePath,"wb");if(fixture){fwrite(html,1,received,fixture);fclose(fixture);}
    printf("NETWORK %s: HTTP=%lu bytes=%lu read=%s title=%s links=%d\n",name,status,received,ok?"OK":"FAILED",document.title,document.linkCount);
    free(html);hubNetClose(request);
}
int main(int argc,char**) {
    setvbuf(stdout,NULL,_IONBF,0);
    checks();
    if(argc>1){WSADATA data;assert(WSAStartup(MAKEWORD(2,2),&data)==0);networkCheck("https://example.com/","example");networkCheck("https://www.bing.com/search?q=Xbox%20360","bing");networkCheck("https://www.google.com/search?q=Xbox%20360","google");WSACleanup();}
    return 0;
}
