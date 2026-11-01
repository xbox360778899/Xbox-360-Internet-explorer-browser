// Orbita XUI typography uses baseline coordinates; all layout helpers use measured metrics.
#ifndef X360BROWSER_UI_H
#define X360BROWSER_UI_H
#include "xui.h"
#include <stdio.h>

enum { BROWSER_PAGE_X=64,BROWSER_PAGE_Y=112,BROWSER_PAGE_W=1152,BROWSER_PAGE_H=504 };
struct BrowserView {
    bool connected,configured,toolbar;
    int selected,cursorX,cursorY;
    const char* host;int port;
    const char* status;const char* configPath;const char* configError;
    unsigned frameVersion;
    const uint32_t* pixels;
};
static void browserText(xui_canvas_t* c,int font,int x,int top,const char* text,uint32_t color,int maxWidth=1100) {
    xui_text(c,font,x,top+xui_face(font)->ascent,color,text,0,maxWidth);
}
static void browserCentered(xui_canvas_t* c,int font,int x,int y,int w,int h,const char* text,uint32_t color) {
    const xui_face_t* face=xui_face(font);
    int baseline=y+(h-face->ascent-face->descent)/2+face->ascent;
    xui_text_center(c,font,x,w,baseline,color,text,0);
}
static void browserLine(xui_canvas_t* c,int x1,int y1,int x2,int y2,uint32_t color) {
    int dx=x2-x1,dy=y2-y1,steps=abs(dx)>abs(dy)?abs(dx):abs(dy);
    if(!steps)return;
    for(int i=0;i<=steps;++i)xui_fill(c,x1+dx*i/steps,y1+dy*i/steps,2,2,color);
}
static void browserNavIcon(xui_canvas_t* c,int id,int x,int y,uint32_t color) {
    int cx=x+18,cy=y+18;
    if(id==0 || id==1) {
        int direction=id==0?-1:1;
        browserLine(c,cx-direction*8,cy,cx+direction*8,cy,color);
        browserLine(c,cx+direction*8,cy,cx,cy-8,color);browserLine(c,cx+direction*8,cy,cx,cy+8,color);
    } else if(id==2) {
        xui_round_frame(c,x+9,y+9,18,18,8,2,color);
        xui_fill(c,x+19,y+7,11,6,0x1e2b31);browserLine(c,x+24,y+6,x+28,y+13,color);browserLine(c,x+28,y+13,x+19,y+13,color);
    } else {
        browserLine(c,x+7,y+17,cx,y+7,color);browserLine(c,cx,y+7,x+29,y+17,color);
        xui_round_frame(c,x+11,y+16,15,13,2,2,color);xui_fill(c,cx-2,y+22,5,8,color);
    }
}
static void browserHint(xui_canvas_t* c,int x,int width,const char* key,const char* action,uint32_t color) {
    int keyWidth=xui_text_width(XUI_F_LABEL,key,0)+16;if(keyWidth<34)keyWidth=34;
    xui_round_rect(c,x,632,width,38,9,0x1c2a30,255);
    xui_round_rect(c,x+7,638,keyWidth,26,7,color,255);
    browserCentered(c,XUI_F_LABEL,x+7,638,keyWidth,26,key,0x0c171b);
    browserCentered(c,XUI_F_LABEL,x+keyWidth+17,632,width-keyWidth-24,38,action,0xd6e4e6);
}
static void browserRender(xui_canvas_t* c,const BrowserView* view) {
    static uint32_t scaled[BROWSER_PAGE_W*BROWSER_PAGE_H];
    static unsigned cachedVersion=0;
    xui_fill(c,0,0,1280,720,0x0c141b);
    xui_vgrad(c,0,0,1280,720,0x184044,75,0);
    browserText(c,XUI_F_ROW,64,20,"X360Browser",0xf0f7f6,500);
    browserText(c,XUI_F_LABEL,966,27,"PREVIEW 0.1.1",0x91aaa9,250);
    for(int i=0;i<4;++i) {
        int x=64+i*46;bool focused=view->toolbar && view->selected==i;
        xui_round_rect(c,x,62,36,36,9,focused?0x84dfbf:0x1e2b31,255);
        browserNavIcon(c,i,x,62,focused?0x0b2520:0xb5cbc9);
    }
    bool addressFocus=view->toolbar && view->selected==4;
    xui_round_rect(c,262,62,954,36,9,addressFocus?0x385e56:0x213139,255);
    xui_circle(c,282,80,4,view->connected?0x8fe7bf:0xc6a66c,255);
    browserText(c,XUI_F_LABEL,298,69,"Search Google or enter an address",0xc7d9d7,740);
    browserCentered(c,XUI_F_LABEL,1175,62,30,36,"Y",0x9aeac7);
    xui_round_rect(c,60,108,1160,512,12,0x304648,255);
    xui_fill(c,64,112,1152,504,0x142a30);
    if(view->connected && view->pixels) {
        if(cachedVersion!=view->frameVersion) {
            xui_image_t source={view->pixels,1280,560,1280,XUI_FMT_XRGB};
            xui_scale_cover(scaled,1152,504,&source);cachedVersion=view->frameVersion;
        }
        xui_image_t image={scaled,1152,504,1152,XUI_FMT_XRGB};xui_blit(c,&image,64,112,0);
        if(!view->toolbar) {
            int x=64+view->cursorX*1152/1280,y=112+view->cursorY*504/560;
            browserLine(c,x,y,x+3,y+18,0x041311);browserLine(c,x,y,x+15,y+12,0x041311);
            browserLine(c,x+2,y+2,x+4,y+15,0xe6fff2);browserLine(c,x+2,y+2,x+12,y+11,0xe6fff2);
        }
    } else {
        xui_vgrad(c,64,112,1152,504,0x2f6c64,90,0);
        xui_round_rect(c,100,161,110,27,7,view->configured?0x2d4e42:0x51432c,255);
        browserCentered(c,XUI_F_LABEL,100,161,110,27,view->configured?"INI LOADED":"SETUP",view->configured?0xa6efc8:0xf5d393);
        browserText(c,XUI_F_TITLE,100,214,view->configured?"Connecting to your browser":"Let's connect your Xbox",0xe9f5f2,1060);
        browserText(c,XUI_F_BODY,102,292,view->configured?"Your settings were read. Waiting for the PC renderer.":"The settings file could not be loaded or contains invalid values.",0xc2d4d3,1050);
        char endpoint[160];sprintf(endpoint,"PC  %s:%d",view->host,view->port);
        browserText(c,XUI_F_BODY,102,346,endpoint,0x9de3c2,1050);
        browserText(c,XUI_F_LABEL,102,397,view->configured?view->configPath:view->configError,view->configured?0x89aaa4:0xf4c88e,1050);
        browserText(c,XUI_F_LABEL,102,455,view->configured?"Keep start-edge.cmd or start-chrome.cmd running on your PC.":"Press START to reload settings after fixing browser.ini.",0xa6bfbd,1050);
        browserText(c,XUI_F_LABEL,102,494,view->status,0xe2e9e3,1050);
    }
    browserHint(c,64,136,"A","Click",0x88ddb0);
    browserHint(c,210,136,"B","Back",0xea9d9e);
    browserHint(c,356,136,"X","Type",0x91bbf0);
    browserHint(c,502,156,"Y","Address",0xe3ce89);
    browserHint(c,668,186,"LT/RT","Scroll",0x9bb4ba);
    browserHint(c,864,166,"L3","Toolbar",0x9bb4ba);
    browserHint(c,1040,176,"START","Enter",0x9bb4ba);
    browserText(c,XUI_F_LABEL,64,682,view->status,0x90b4a9,1152);
}
#endif
