#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "browser_config.h"
#include "browser_ui.h"
static uint32_t pixels[1280*720],page[1280*560];
static void parserChecks() {
    const char* token="0123456789abcdef0123456789abcdef0123456789abcdef";
    char raw[512],error[192];BrowserConfig config;
    sprintf(raw,"host=192.168.0.168\nport=36080\ntoken=%s\n",token);
    assert(browserParseConfig((unsigned char*)raw,(int)strlen(raw),&config,error));
    assert(config.port==36080 && !strcmp(config.host,"192.168.0.168"));
    sprintf(raw,"\xef\xbb\xbf ; Comment\r\n[Browser]\r\n HOST = 192.168.0.168 \r\n PORT = 36080 \r\n TOKEN = %s \r\n",token);
    assert(browserParseConfig((unsigned char*)raw,(int)strlen(raw),&config,error));
    char plain[512];sprintf(plain,"host=192.168.0.168\r\nport=36080\r\ntoken=%s\r\n",token);
    unsigned char utf16[1024]={0xff,0xfe};int n=(int)strlen(plain);
    for(int i=0;i<n;++i){utf16[2+i*2]=plain[i];utf16[3+i*2]=0;}
    assert(browserParseConfig(utf16,2+n*2,&config,error));
    const char* invalid[]={"host=bad\ntoken=short", "host=192.168.0.168\nport=-1", "host=999.1.1.1", "host=1.2.3.4\nport=36080bad", ""};
    for(int i=0;i<5;++i){assert(!browserParseConfig((const unsigned char*)invalid[i],(int)strlen(invalid[i]),&config,error));assert(error[0]);}
    printf("Config: LF, CRLF, BOM, UTF-16LE, whitespace, and invalid fields passed.\n");
}
static void save(const char* file) {
    FILE* out=fopen(file,"wb");assert(out);fprintf(out,"P6\n1280 720\n255\n");
    for(int i=0;i<1280*720;++i){fputc((pixels[i]>>16)&255,out);fputc((pixels[i]>>8)&255,out);fputc(pixels[i]&255,out);}fclose(out);
}
int main() {
    parserChecks();
    xui_canvas_t canvas;xui_canvas_init(&canvas,pixels,1280,720,1280);
    BrowserView view={false,true,false,4,640,280,"192.168.0.168",36080,
        "Cannot connect to PC:36080. Check the PC bridge / firewall.","game:\\browser.ini","",1,NULL};
    browserRender(&canvas,&view);save("obj/preview-connecting.ppm");
    view.configured=false;view.configError="Missing or incomplete bridge token.";
    browserRender(&canvas,&view);save("obj/preview-config-error.ppm");
    FILE* frame=fopen("obj/home-frame.argb","rb");
    if(frame) {
        for(int i=0;i<1280*560;++i){int a=fgetc(frame),r=fgetc(frame),g=fgetc(frame),b=fgetc(frame);assert(a>=0 && b>=0);page[i]=((uint32_t)r<<16)|((uint32_t)g<<8)|b;}fclose(frame);
        view.connected=true;view.configured=true;view.pixels=page;
        view.status="Connected  /  Google search  /  LB and RB: history  /  BACK: home";
        browserRender(&canvas,&view);save("obj/preview-connected.ppm");
        view.toolbar=true;view.selected=3;browserRender(&canvas,&view);save("obj/preview-toolbar.ppm");
    }
    printf("UI rendered with the same Orbita XUI code used by the XEX.\n");
    return 0;
}
