#pragma once
static int styledResourceCount=0;
static bool styledFixtureFetch(void*,const char* url,unsigned char** bytes,int* size,char* finalURL,int capacity){
    const char* path=webFind(url,"layout.css")?"tests/fixtures/layout.css":webFind(url,"colors.png")?"tests/fixtures/colors.png":NULL;
    if(!path)return false;FILE* file=fopen(path,"rb");if(!file)return false;fseek(file,0,SEEK_END);long count=ftell(file);fseek(file,0,SEEK_SET);*bytes=(unsigned char*)malloc(count+1);assert(*bytes);assert(fread(*bytes,1,count,file)==(size_t)count);(*bytes)[count]=0;fclose(file);*size=count;strncpy_s(finalURL,capacity,url,_TRUNCATE);++styledResourceCount;return true;
}
static void styledChecks(){
    FILE* file=fopen("tests/fixtures/layout.html","rb");assert(file);fseek(file,0,SEEK_END);long count=ftell(file);fseek(file,0,SEEK_SET);char* html=(char*)malloc(count+1);assert(fread(html,1,count,file)==(size_t)count);html[count]=0;fclose(file);
    void* page=browserStyledCreate(html,"https://example.com/layout.html",styledFixtureFetch,NULL);free(html);assert(page);assert(browserStyledCSSCount(page)==1);assert(browserStyledImageCount(page)==0);assert(styledResourceCount==1);
    char imageURL[WEB_URL]="";assert(browserStyledNextImage(page,imageURL,sizeof(imageURL)));
    unsigned char* bytes=NULL;int imageSize=0;char finalURL[WEB_URL];
    assert(styledFixtureFetch(NULL,imageURL,&bytes,&imageSize,finalURL,sizeof(finalURL)));
    browserStyledCompleteImage(page,imageURL,bytes,imageSize);free(bytes);
    assert(browserStyledImageCount(page)==1&&styledResourceCount==2);
    xui_canvas_t canvas;xui_canvas_init(&canvas,screen,1280,720,1280);xui_fill(&canvas,0,0,1280,720,0x08150f);browserStyledDraw(page,&canvas,0,76,1280,644,0);
    save("obj/css-page.ppm");assert(screen[(76+5)*1280+5]==0x195b38);
    char clicked[WEB_URL]="";bool foundLink=false;
    for(int y=280;y<450&&!foundLink;y+=3)for(int x=25;x<220&&!foundLink;x+=5)foundLink=browserStyledClick(page,x,y,clicked,sizeof(clicked));
    assert(foundLink && !strcmp(clicked,"https://example.com/next.html"));
    document.styled=page;strcpy_s(document.url,"https://example.com/layout.html");document.height=browserStyledHeight(page);
    NativeView view={false,false,false,4,700,370,0,document.url,"",&document,true,false,0,false};nativeRender(&canvas,&view);save("obj/css-browser.ppm");document.styled=NULL;
    // Explicit positioned elements prove styles affect the layout, beyond text extraction.
    browserStyledDestroy(page);
    page=browserStyledCreate("<html><head><style>body{margin:0;}#probe{position:absolute;left:10px;top:10px;width:80px;height:50px;background:#123456;} .hidden{display:none;}</style></head><body><div id='probe'></div><div class='hidden'>Hidden</div></body></html>","https://example.com/",NULL,NULL);assert(page);
    browserStyledDraw(page,&canvas,0,76,1280,644,0);assert(screen[(76+20)*1280+20]==0x123456);browserStyledDestroy(page);
    page=browserStyledCreate("<html><head><style>body{margin:0}#probe{height:40px;width:80px;background:#111111}@media screen and (max-width:1400px){#probe{background:#123abc}}@media screen and (max-width:600px){#probe{background:#ffffff}}.row{display:flex}</style></head><body><div id='probe'></div><div class='row'>Readable fallback</div></body></html>","https://example.com/",NULL,NULL);
    assert(page);browserStyledDraw(page,&canvas,0,76,1280,644,0);
    assert(screen[(76+10)*1280+10]==0x123abc);
    browserStyledDestroy(page);
    page=browserStyledCreate("<html><head><style>:root{--mygreen:#268b41;}body{margin:0}#probe{position:absolute;left:5px;top:5px;width:64px;height:48px;background:var(--mygreen);}</style></head><body><div id='probe'></div></body></html>","https://example.com/",NULL,NULL);
    assert(page);browserStyledDraw(page,&canvas,0,76,1280,644,0);
    assert(screen[(76+20)*1280+20]==0x268b41);
    browserStyledDestroy(page);
    printf("PASS: litehtml CSS, media-query widths, root CSS variables and flex/grid readability fallback.\n");
}
