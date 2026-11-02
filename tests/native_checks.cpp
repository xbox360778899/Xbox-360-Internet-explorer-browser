#include "host_net_platform.h"
#include <assert.h>
#include "native_document.h"
#include "lg_ui.h"
#include "image_decoder.h"
#include "native_net.h"
static void errorMessage(char* output,size_t capacity,DWORD error){sprintf_s(output,capacity,"Native request failed (%lu)",error);}
#include "native_fetch.h"
#include "web_js.h"
#include "web_compat.h"
#include "google_home.h"
#include "search_results.h"
static WebDocument document;
static uint32_t screen[1280*720];
static void save(const char* path) {
    FILE* out=fopen(path,"wb");assert(out);fprintf(out,"P6\n1280 720\n255\n");
    for(int i=0;i<1280*720;++i){fputc((screen[i]>>16)&255,out);fputc((screen[i]>>8)&255,out);fputc(screen[i]&255,out);}fclose(out);
}
#include "styled_checks.h"
static void checks() {
    char url[WEB_URL];
    assert(webSearchOrURL("Xbox 360",url,sizeof(url)) && !strcmp(url,"https://www.google.com/search?q=Xbox%20360"));
    assert(webSearchOrURL("xbox 360",url,sizeof(url),true) && webPrefix(url,"https://www.google.com/search?q="));
    assert(webSearchOrURL("x360hub.com",url,sizeof(url),true) && !strcmp(url,"https://www.google.com/search?q=x360hub%2Ecom"));
    assert(browserGoogleHomeURL("https://www.google.com/"));
    assert(!browserGoogleHomeURL("https://www.google.com/search?q=test"));
    std::string invalidGoogle;assert(!browserGooglePrepareHTML("https://www.google.com/","<html>not Google</html>",invalidGoogle));
    assert(webResolveURL("https://example.com/a/b","/test?q=1",url,sizeof(url)) && !strcmp(url,"https://example.com/test?q=1"));
    assert(webResolveURL("https://example.com/a/b","next",url,sizeof(url)) && !strcmp(url,"https://example.com/a/next"));
    assert(webResolveURL("https://example.com/styles/main.css","../images/a.png",url,sizeof(url)) && !strcmp(url,"https://example.com/images/a.png"));
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
    assert(lgTabsHit(820,150)==0 && lgTabsHit(820,211)==1);assert(lgMenuHit(940,136)==0 && lgMenuHit(1000,405)==5 && lgMenuHit(500,200)==-1);
    view.home=true;view.menu=true;view.menuSelected=4;nativeRender(&canvas,&view);save("obj/lg-menu.ppm");
    view.menu=false;view.chrome=false;nativeRender(&canvas,&view);save("obj/lg-hidden.ppm");
    assert(screen[690*1280+1190]!=0x080d0f); // Preview watermark remains visible with hidden chrome.
    view.video=true;nativeRender(&canvas,&view);assert(screen[0]==0 && screen[690*1280+1190]!=0);view.video=false;
    printf("PASS: toolbar inactivity, menu hit testing, icon rendering and watermark visibility.\n");
    printf("PASS: native URL/search handling, entity decoding, hidden scripts/styles, clickable links and shared UI rendering.\n");
}
static bool jsFixtureFetch(void*,const char* url,unsigned char** bytes,int* count,char* finalURL,int capacity){
    if(!strstr(url,"/app.js"))return false;
    const char* src="document.getElementById('target').innerHTML='<b>external JS</b>';";
    *count=(int)strlen(src);
    *bytes=(unsigned char*)malloc(*count+1);
    if(!*bytes)return false;
    memcpy(*bytes,src,*count+1);
    strncpy_s(finalURL,capacity,url,_TRUNCATE);
    return true;
}
static void compatCatalogChecks(){
    assert(browserCompatCatalogURL("https://x360hub.vercel.app/ports.html"));
    assert(browserCompatCatalogURL("https://x360hub.vercel.app/ports.html?x360page=2"));
    assert(!browserCompatCatalogURL("https://untrusted.example/ports.html"));
    std::string json="{\"projects\":[";
    for(int i=0;i<10;++i){
        char item[320];sprintf_s(item,sizeof(item),
          "%s{\"title\":\"Sample %d\",\"description\":\"Live & <demo>\",\"category\":\"Game ports\","
          "\"imageHorizontal\":\"assets/images/test%d.jpg\",\"repositoryUrl\":\"https://github.com/test/repo\"}",
          i?",":"",i,i);
        json+=item;
    }
    json+="]}";
    std::string html;int count=0;
    assert(browserCompatRenderCatalog("https://x360hub.vercel.app/ports.html",json.c_str(),(int)json.size(),html,&count));
    assert(count==10 && html.find("Sample 0")!=std::string::npos &&
           html.find("Sample 8")==std::string::npos &&
           html.find("Live &amp; &lt;demo&gt;")!=std::string::npos &&
           html.find("x360page=2")!=std::string::npos);
    void* styled=browserStyledCreate(html.c_str(),"https://x360hub.vercel.app/ports.html",NULL,NULL);
    assert(styled && browserStyledHeight(styled)>150);browserStyledDestroy(styled);
    assert(browserCompatRenderCatalog("https://x360hub.vercel.app/ports.html?x360page=2",json.c_str(),(int)json.size(),html,&count));
    assert(html.find("Sample 8")!=std::string::npos && html.find("Sample 0")==std::string::npos);
    assert(!browserCompatRenderCatalog("https://x360hub.vercel.app/ports.html","{}",2,html,&count));
    printf("PASS: actual-site JSON compatibility renderer, escaped text, image links and pagination.\n");
}
static void searchResultsChecks(){
    const char* googlePage="<html><head><title>Google Search</title></head><body>"
      "<noscript><style>div,p{display:none}</style><a href='/httpservice/retry/enablejs?sei=test'>Enable JS</a></noscript></body></html>";
    assert(browserIsGoogleSearchURL("https://www.google.com/search?q=Xbox%20360"));
    assert(!browserIsGoogleSearchURL("https://www.google.com/"));
    assert(browserGoogleRequiresJS(googlePage));
    assert(!browserGoogleRequiresJS("<html><title>Google Search</title><p>results</p></html>"));
    std::string html;
    assert(browserGoogleSearchNotice("https://www.google.com/search?q=Xbox%20360",html));
    assert(html.find("https://www.bing.com/search?q=Xbox%20360&amp;format=rss")!=std::string::npos);
    assert(html.find("NOT Google")==std::string::npos || html.find("Bing")!=std::string::npos);
    assert(html.find("Xbox 360")!=std::string::npos);
    assert(browserIsBingRSSURL("https://www.bing.com/search?q=Xbox%20360&format=rss"));
    assert(!browserIsBingRSSURL("https://www.bing.com/search?q=Xbox%20360"));
    webParseHTML(&document,html.c_str(),"https://www.google.com/search?q=Xbox%20360");
    assert(document.linkCount>=2);
    bool optIn=false;
    for(int i=0;i<document.linkCount;++i)
        if(strstr(document.links[i],"bing.com/search?q=Xbox%20360&format=rss"))optIn=true;
    assert(optIn);
    const char* feed="<?xml version='1.0'?><rss version='2.0'><channel>"
       "<item><title>X &amp; Y</title><link>https://example.com/a?x=1&amp;y=2</link>"
       "<description>Real &lt;b&gt;content&lt;/b&gt;.</description></item>"
       "<item><title>Another</title><link>https://example.com/b</link>"
       "<description>Second result</description></item></channel></rss>";
    int items=0;
    assert(browserRenderBingRSS("https://www.bing.com/search?q=one&format=rss",feed,(int)strlen(feed),html,&items));
    assert(items==2 && html.find("X &amp; Y")!=std::string::npos &&
           html.find("Real content")!=std::string::npos &&
           html.find("https://example.com/a?x=1&amp;y=2")!=std::string::npos);
    assert(html.find("NOT Google")!=std::string::npos);
    assert(!browserRenderBingRSS("https://www.bing.com/search?q=test&format=rss",
           "<html>not rss</html>",20,html,&items));
    printf("PASS: real Google JS-required interstitial detected, opt-in Bing fallback, RSS escaped and clickable.\n");
}
static void jsChecks(){
    std::string output;int ran=0,failed=0;
    const char* html="<html><body><div id='target'>OLD</div><script>"
       "var a=20+22;document.getElementById('target').textContent=String(a);"
       "document.write('<p id=\"generated\">JavaScript worked</p>');"
       "</script></body></html>";
    assert(browserJSPreprocess(html,"https://example.com/",output,&ran,&failed));
    assert(ran==1&&failed==0);
    assert(output.find(">42</div>")!=std::string::npos);
    assert(output.find("JavaScript worked")!=std::string::npos);
    assert(browserJSPreprocess("<html><body><div id='target'>OLD</div><script src='/app.js'></script></body></html>","https://example.com/",output,&ran,&failed,jsFixtureFetch,NULL));
    assert(ran==1&&failed==0&&output.find("<b>external JS</b>")!=std::string::npos);
    const char* dynamicHTML="<html><body><div id='stage'>Start</div><script>"
      "document.addEventListener('DOMContentLoaded',function(){"
      "var card=document.createElement('article');card.setAttribute('class','card');"
      "var title=document.createElement('h2');title.textContent='DOM initialized';"
      "card.appendChild(title);document.getElementById('stage').appendChild(card);"
      "var footer=document.createElement('footer');footer.textContent='Loaded event';"
      "document.body.appendChild(footer);"
      "});"
      "window.addEventListener('load',function(){var n=document.createElement('p');"
      "n.textContent='Window load callback';document.body.appendChild(n);});"
      "</script></body></html>";
    assert(browserJSPreprocess(dynamicHTML,"https://example.com/",output,&ran,&failed));
    printf("DOM TEST: ran=%d failed=%d markup=%.420s\n",ran,failed,output.c_str());
    assert(failed==0 && output.find("<article class=\"card\"><h2>DOM initialized</h2></article>")!=std::string::npos);
    assert(output.find("<footer>Loaded event</footer>")!=std::string::npos);
    assert(output.find("<p>Window load callback</p>")!=std::string::npos);
    void* dynamicPage=browserStyledCreate(output.c_str(),"https://example.com/",NULL,NULL);
    assert(dynamicPage && browserStyledHeight(dynamicPage)>0);
    xui_canvas_t domCanvas;xui_canvas_init(&domCanvas,screen,1280,720,1280);
    browserStyledDraw(dynamicPage,&domCanvas,0,0,1280,720,0);
    save("obj/dom-ready-native-render.ppm");
    browserStyledDestroy(dynamicPage);
    DWORD started=GetTickCount();
    assert(browserJSPreprocess("<html><body><script>while(true){}</script></body></html>","https://example.com/",output,&ran,&failed));
    assert(failed>=1 && GetTickCount()-started<2500);
    printf("PASS: actual Duktape JavaScript, basic DOM changes and bounded infinite-loop timeout.\n");
}
static void compressedFixtures(){
    const char* files[]={"tests/fixtures/encoded-gzip.bin","tests/fixtures/encoded-deflate.bin"};
    for(int j=0;j<2;++j){
        FILE* file=fopen(files[j],"rb");assert(file);
        unsigned char encoded[128]={0},decoded[128]={0};
        size_t count=fread(encoded,1,sizeof(encoded),file);fclose(file);assert(count>10);
        z_stream z;memset(&z,0,sizeof(z));
        assert(inflateInit2(&z,j==0?15+16:15)==Z_OK);
        z.next_in=(Bytef*)encoded;z.avail_in=(uInt)count;
        z.next_out=(Bytef*)decoded;z.avail_out=sizeof(decoded);
        int result=inflate(&z,Z_FINISH);inflateEnd(&z);
        assert(result==Z_STREAM_END);
        assert(z.total_out==44 && !memcmp(decoded,"<html><body>Compressed website</body></html>",44));
    }
    printf("PASS: zlib native gzip and deflate decoding.\n");
}
static void cacheChecks(){
    WebDownload a={0},b={0};a.bytes=(unsigned char*)malloc(5);assert(a.bytes);memcpy(a.bytes,"TEST",5);
    a.size=4;a.kind=WEB_DOWNLOAD_HTML;strcpy_s(a.url,"https://example.com/asset.css");strcpy_s(a.type,"text/css");
    webCacheStore(a.url,&a);
    assert(webCacheLookup("https://example.com/asset.css",&b,false));assert(b.cacheHit&&b.size==4&&!memcmp(b.bytes,"TEST",4));
    free(b.bytes);webCacheForget(a.url);free(a.bytes);
    assert(browserCacheBytes==0);printf("PASS: bounded in-memory web cache with invalidation.\n");
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
    WebDownload page={0};char error[192]="";DWORD started=GetTickCount();
    bool ok=webDownload(url,&page,error,sizeof(error));
    if(ok){
        webParseHTML(&document,(const char*)page.bytes,page.url);
        char fixturePath[128];sprintf_s(fixturePath,sizeof(fixturePath),"obj/net-%s.html",name);
        FILE* file=fopen(fixturePath,"wb");if(file){fwrite(page.bytes,1,page.size,file);fclose(file);}
    }
    printf("NETWORK %s: HTTP=%lu bytes=%lu read=%s title=%s links=%d ms=%lu %s\n",
      name,webNetState.httpCode,page.size,ok?"OK":"FAILED",ok?document.title:"",ok?document.linkCount:0,GetTickCount()-started,error);
    free(page.bytes);
}
static void networkRealCSSCheck(const char* url,const char* name){
    webNetState.cancel=0;WebDownload dl={0};char error[192]="";DWORD start=GetTickCount();
    bool ok=webDownload(url,&dl,error,sizeof(error));
    void* page=ok?browserStyledCreate((const char*)dl.bytes,dl.url,webStyledResourceFetch,NULL):NULL;
    int css=browserStyledCSSCount(page),failed=browserStyledCSSFailed(page);
    if(!ok||!page||css<1||failed){fprintf(stderr,"FAIL: live website CSS did not load.\n");exit(5);}
    printf("NETWORK CSS %s: HTML=%s CSS=%d failed=%d ms=%lu %s\n",
       name,ok?"OK":"FAILED",css,failed,GetTickCount()-start,error);
    if(page){xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);browserStyledDraw(page,&canvas,0,0,1280,720,0);printf("RENDER %s: pixel(4,4)=%06X pixel(640,400)=%06X\n",name,screen[4*1280+4]&0xffffff,screen[400*1280+640]&0xffffff);save("obj/x360hub-real.ppm");browserStyledDestroy(page);}
    free(dl.bytes);
}
static void networkGoogleSearchFallbackCheck(){
    webNetState.cancel=0;
    const char* googleURL="https://www.google.com/search?q=Xbox%20360";
    WebDownload google={0};char message[192]="";
    bool gotGoogle=webDownload(googleURL,&google,message,sizeof(message));
    bool blocked=gotGoogle && browserGoogleRequiresJS((const char*)google.bytes);
    std::string notice;bool shown=blocked && browserGoogleSearchNotice(googleURL,notice);
    void* page=shown?browserStyledCreate(notice.c_str(),googleURL,NULL,NULL):NULL;
    if(page){
        char clicked[WEB_URL]="";
        bool clickedOptIn=browserStyledClick(page,135,275,clicked,sizeof(clicked));
        printf("GOOGLE notice clickable: hit=%d url=%.150s\n",clickedOptIn,clicked);
        if(!clickedOptIn || strcmp(clicked,"https://www.bing.com/search?q=Xbox%20360&format=rss"))exit(10);
        webParseHTML(&document,notice.c_str(),googleURL);
        document.styled=page;
        xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);
        NativeView v={false,false,false,4,230,245,0,googleURL,
             "Google JS required; Bing requires explicit opt-in",&document,false,false,0,false};
        nativeRender(&canvas,&v);save("obj/google-js-required-notice.ppm");
        document.styled=NULL;
    }
    printf("NETWORK GOOGLE query: download=%s requiresJS=%d nativeNotice=%d links=%d %s\n",
       gotGoogle?"OK":"FAIL",blocked,shown,shown?document.linkCount:0,message);
    if(!gotGoogle||!blocked||!shown||!page||document.linkCount<2)exit(8);
    browserStyledDestroy(page);free(google.bytes);
    const char* alternate="https://www.bing.com/search?q=Xbox%20360&format=rss";
    WebDownload feed={0};char issue[192]="";
    bool downloaded=webDownload(alternate,&feed,issue,sizeof(issue));
    std::string listing;int count=0;
    bool rendered=downloaded && browserRenderBingRSS(alternate,(const char*)feed.bytes,(int)feed.size,listing,&count);
    void* styled=rendered?browserStyledCreate(listing.c_str(),alternate,NULL,NULL):NULL;
    if(styled){
        webParseHTML(&document,listing.c_str(),alternate);
        document.styled=styled;
        xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);
        NativeView v={false,false,false,4,240,340,0,alternate,
                        "Bing RSS real results, not Google",&document,false,false,0,false};
        nativeRender(&canvas,&v);save("obj/bing-live-search.ppm");
        document.styled=NULL;
    }
    printf("NETWORK BING RSS: download=%s results=%d links=%d rendered=%d %s\n",
        downloaded?"OK":"FAIL",count,rendered?document.linkCount:0,styled!=NULL,issue);
    if(!downloaded||!rendered||count<5||!styled||document.linkCount<count)exit(9);
    browserStyledDestroy(styled);free(feed.bytes);
}
static void networkGoogleHomepageCheck(){
    webNetState.cancel=0;WebDownload page={0};char err[192]="";
    DWORD start=GetTickCount();
    bool ok=webDownload("https://www.google.com/",&page,err,sizeof(err));
    std::string realGoogle;
    bool transformed=ok && browserGooglePrepareHTML(page.url,(const char*)page.bytes,realGoogle);
    void* styled=transformed?browserStyledCreate(realGoogle.c_str(),page.url,webStyledResourceFetch,NULL):NULL;
    int imageRequests=0,loaded=0;char firstImage[WEB_URL]="";
    if(styled){
        char asset[WEB_URL]="";
        if(browserStyledNextImage(styled,asset,sizeof(asset))){
            ++imageRequests;strcpy_s(firstImage,asset);
            WebDownload picture={0};char msg[192]="";
            if(webDownload(asset,&picture,msg,sizeof(msg),true)){
                browserStyledCompleteImage(styled,asset,picture.bytes,(int)picture.size);
                loaded=browserStyledImageCount(styled);
            }else printf("Google image download failed: %s\n",msg);
            free(picture.bytes);
        }
        xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);
        webParseHTML(&document,realGoogle.c_str(),page.url);
        document.styled=styled;
        NativeView v={false,false,false,4,640,325,0,page.url,"Google homepage loaded live",&document,false,false,0,false};
        assert(lgGoogleSearchHit(&v,640,305));
        assert(!lgGoogleSearchHit(&v,640,180));
        nativeRender(&canvas,&v);save("obj/google-homepage-live.ppm");
        v.chrome=true;v.cursorY=385;
        assert(lgGoogleSearchHit(&v,640,385));
        assert(!lgGoogleSearchHit(&v,640,300));
        nativeRender(&canvas,&v);save("obj/google-homepage-with-toolbar.ppm");
        document.styled=NULL;
    }
    printf("NETWORK GOOGLE HOME: HTML=%s bytes=%lu styled=%d CSS=%d/%d height=%d images=%d/%d first=%s ms=%lu %s\n",
     ok?"OK":"FAIL",page.size,styled!=NULL,browserStyledCSSCount(styled),browserStyledCSSFailed(styled),
     browserStyledHeight(styled),loaded,imageRequests,firstImage,GetTickCount()-start,err);
    if(!ok || !transformed || !styled || loaded!=1){fprintf(stderr,"FAIL: Google live homepage/logo rendering failed.\n");exit(7);}
    if(styled)browserStyledDestroy(styled);
    free(page.bytes);
}
static void networkLiveCatalogCheck(){
    webNetState.cancel=0;WebDownload feed={0};char message[192]="";
    DWORD start=GetTickCount();
    bool ok=webDownload("https://x360hub.vercel.app/data/catalog.json",&feed,message,sizeof(message),false,true,5500);
    std::string html;int total=0;
    bool rendered=ok && browserCompatRenderCatalog("https://x360hub.vercel.app/ports.html",
        (const char*)feed.bytes,(int)feed.size,html,&total);
    void* page=rendered?browserStyledCreate(html.c_str(),"https://x360hub.vercel.app/ports.html",webStyledResourceFetch,NULL):NULL;
    int loaded=0;
    printf("CATALOG markup bytes=%lu imgTag=%ld first=%s\n",(DWORD)html.size(),(long)html.find("<img"),html.substr(0,180).c_str());
    if(page){
        char url[WEB_URL]="";if(browserStyledNextImage(page,url,sizeof(url))){
            WebDownload image={0};char error[192]="";
            if(webDownload(url,&image,error,sizeof(error),true)){
                browserStyledCompleteImage(page,url,image.bytes,(int)image.size);
                loaded=browserStyledImageCount(page);
            }
            free(image.bytes);
        }
        xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);
        browserStyledDraw(page,&canvas,0,0,1280,720,0);save("obj/x360hub-live-catalog.ppm");
        browserStyledDestroy(page);
    }
    printf("NETWORK X360HUB live catalog: HTTP=%lu JSON=%s rendered=%s count=%d images=%d ms=%lu %s\n",
           webNetState.httpCode,ok?"OK":"FAILED",rendered?"OK":"FAILED",total,loaded,GetTickCount()-start,message);
    if(!rendered || !page || total<8 || loaded<1)exit(6);
    free(feed.bytes);
}
static void networkJSCheck(){
    WebDownload html={0};char message[192]="";
    DWORD start=GetTickCount();webNetState.cancel=0;
    if(webDownload("https://x360hub.vercel.app/",&html,message,sizeof(message))){
        std::string output;int ran=0,failed=0;
        browserJSPreprocess((const char*)html.bytes,html.url,output,&ran,&failed,webJavaScriptResourceFetch,NULL);
        printf("NETWORK X360HUB JavaScript: executed=%d errors=%d ms=%lu bytes=%lu\n",
            ran,failed,GetTickCount()-start,(DWORD)output.size());
    }else printf("NETWORK X360HUB JavaScript: HTML unavailable: %s\n",message);
    free(html.bytes);
}
static void networkImageCheck() {
    WebDownload image={0};char message[192]="";webNetState.cancel=0;
    const char* url="https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/colors.png";
    bool ok=webDownload(url,&image,message,sizeof(message),true);int width=0,height=0;uint32_t* rgb=ok?browserDecodeImage(image.bytes,image.size,&width,&height):NULL;
    printf("NETWORK image: fetch=%s decoded=%s size=%dx%d bytes=%lu %s\n",ok?"OK":"FAILED",rgb?"OK":"FAILED",width,height,image.size,message);
    free(rgb);free(image.bytes);
}
static void networkStyledCheck(){
    WebDownload html={0};char message[192]="";webNetState.cancel=0;
    DWORD started=GetTickCount();
    bool ok=webDownload("https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/layout.html",&html,message,sizeof(message));
    void* page=ok?browserStyledCreate((const char*)html.bytes,html.url,webStyledResourceFetch,NULL):NULL;
    if(page){char assetURL[WEB_URL]="";if(browserStyledNextImage(page,assetURL,sizeof(assetURL))){WebDownload asset={0};char error[192]="";if(webDownload(assetURL,&asset,error,sizeof(error),true))browserStyledCompleteImage(page,assetURL,asset.bytes,(int)asset.size);free(asset.bytes);}}
    printf("NETWORK styled page: fetch=%s engine=%s css=%d failed=%d images=%d elapsed=%lu ms %s\n",ok?"OK":"FAILED",page?"OK":"FAILED",browserStyledCSSCount(page),browserStyledCSSFailed(page),browserStyledImageCount(page),GetTickCount()-started,message);
    if(page){xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);browserStyledDraw(page,&canvas,0,76,1280,644,0);save("obj/css-network.ppm");browserStyledDestroy(page);}free(html.bytes);
}
int main(int argc,char**) {
    setvbuf(stdout,NULL,_IONBF,0);
    checks();
    compatCatalogChecks();
    searchResultsChecks();
    jsChecks();
    compressedFixtures();
    mediaChecks();
    cacheChecks();
    styledChecks();
    if(argc>1){WSADATA data;assert(WSAStartup(MAKEWORD(2,2),&data)==0);networkCheck("https://example.com/","example");networkCheck("https://www.bing.com/search?q=Xbox%20360","bing");networkCheck("https://www.google.com/search?q=Xbox%20360","google");networkImageCheck();networkStyledCheck();networkRealCSSCheck("https://x360hub.vercel.app/","X360HUB");networkJSCheck();networkGoogleHomepageCheck();networkGoogleSearchFallbackCheck();networkLiveCatalogCheck();WSACleanup();}
    return 0;
}
