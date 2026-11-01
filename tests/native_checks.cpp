#include "host_net_platform.h"
#include <assert.h>
#include "native_document.h"
#include "lg_ui.h"
#include "image_decoder.h"
#include "native_net.h"
static void errorMessage(char* output,size_t capacity,DWORD error){sprintf_s(output,capacity,"Native request failed (%lu)",error);}
#include "native_fetch.h"
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
    NativeView view={true,false,false,4,640,325,0,"browser:home","Native browser ready. No PC, bridge or browser.ini is used.",&document,true,false,0,false};
    nativeRender(&canvas,&view);save("obj/native-home.ppm");
    view.home=false;view.url=document.url;view.cursorX=150;view.cursorY=245;view.status="Native layout demonstration. Website compatibility depends on the HTML reader.";
    nativeRender(&canvas,&view);save("obj/native-reader.ppm");
    for(int i=0;i<document.count;++i)if(document.lines[i].link>=0){view.cursorX=100;view.cursorY=lgPageTop(&view)+2+document.lines[i].y;assert(nativeHitLink(&view)==0);}
    assert(nativeHomeHit(350,190)==0 && nativeHomeHit(180,350)==1);
    assert(lgChromeVisible(1000,0,false,false,false));assert(!lgChromeVisible(4500,0,false,false,false));assert(lgChromeVisible(9000,0,true,false,false));
    assert(lgMenuHit(940,136)==0 && lgMenuHit(1000,405)==5 && lgMenuHit(500,200)==-1);
    view.home=true;view.menu=true;view.menuSelected=4;nativeRender(&canvas,&view);save("obj/lg-menu.ppm");
    view.menu=false;view.chrome=false;nativeRender(&canvas,&view);save("obj/lg-hidden.ppm");
    assert(screen[690*1280+1190]!=0x080d0f); // Preview watermark remains visible with hidden chrome.
    view.video=true;nativeRender(&canvas,&view);assert(screen[0]==0 && screen[690*1280+1190]!=0);view.video=false;
    printf("PASS: toolbar inactivity, menu hit testing, icon rendering and watermark visibility.\n");
    printf("PASS: native URL/search handling, entity decoding, hidden scripts/styles, clickable links and shared UI rendering.\n");
}
static void mediaChecks() {
    const char* extensions[]={"png","jpg","gif","bmp"};
    for(int i=0;i<4;++i){char path[128];sprintf(path,"tests/fixtures/colors.%s",extensions[i]);FILE* file=fopen(path,"rb");assert(file);fseek(file,0,SEEK_END);long count=ftell(file);fseek(file,0,SEEK_SET);unsigned char* bytes=(unsigned char*)malloc(count);assert(bytes);assert(fread(bytes,1,count,file)==(size_t)count);fclose(file);int width=0,height=0;uint32_t* rgb=browserDecodeImage(bytes,count,&width,&height);free(bytes);assert(rgb && width==320 && height==180);if(i!=1)assert((rgb[50*width+50]&0xff0000)>0xf00000);free(rgb);}
    int width=0,height=0;assert(!browserDecodeImage((const unsigned char*)"not an image",12,&width,&height));
    webReleaseImages(&document);webParseHTML(&document,"<h1>Media page</h1><img src='/image.png' alt='Photo'><video><source src='movie.wmv' type='video/x-ms-wmv'></video>","https://example.com/dir/page");
    assert(document.imageCount==1 && !strcmp(document.images[0].url,"https://example.com/image.png"));assert(document.linkCount==1 && !strcmp(document.links[0],"https://example.com/dir/movie.wmv"));
    document.images[0].width=320;document.images[0].height=180;document.images[0].pixels=(uint32_t*)malloc(320*180*4);assert(document.images[0].pixels);
    for(int y=0;y<180;++y)for(int x=0;x<320;++x)document.images[0].pixels[y*320+x]=((x/20+y/20)%2)?0xf12b69:0x344f60;
    document.images[0].state=1;webLayout(&document);assert(document.lines[1].kind==WEB_IMAGE_ROW && document.lines[1].height==216);assert(document.lines[2].kind==WEB_VIDEO_ROW);
    xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);NativeView view={false,false,false,0,260,400,0,document.url,"Native image + video layout fixture",&document,true,false,0,false};nativeRender(&canvas,&view);save("obj/lg-media.ppm");webReleaseImages(&document);
    printf("PASS: JPEG/PNG/GIF/BMP decoding, malformed image rejection, media URL parsing and page layout.\n");
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
static void networkImageCheck() {
    WebDownload image={0};char message[192]="";webNetState.cancel=0;
    const char* url="https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/colors.png";
    bool ok=webDownload(url,&image,message,sizeof(message),true);int width=0,height=0;uint32_t* rgb=ok?browserDecodeImage(image.bytes,image.size,&width,&height):NULL;
    printf("NETWORK image: fetch=%s decoded=%s size=%dx%d bytes=%lu %s\n",ok?"OK":"FAILED",rgb?"OK":"FAILED",width,height,image.size,message);
    free(rgb);free(image.bytes);
}
int main(int argc,char**) {
    setvbuf(stdout,NULL,_IONBF,0);
    checks();
    mediaChecks();
    if(argc>1){WSADATA data;assert(WSAStartup(MAKEWORD(2,2),&data)==0);networkCheck("https://example.com/","example");networkCheck("https://www.bing.com/search?q=Xbox%20360","bing");networkCheck("https://www.google.com/search?q=Xbox%20360","google");networkImageCheck();WSACleanup();}
    return 0;
}
