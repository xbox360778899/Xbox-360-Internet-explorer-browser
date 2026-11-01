#ifndef X360_NATIVE_UI_H
#define X360_NATIVE_UI_H
#include "browser_ui.h"
#include "native_document.h"
struct NativeView {bool home,loading,toolbar;int selected,cursorX,cursorY,scroll;const char* url;const char* status;const WebDocument* doc;};
static int nativeHitLink(const NativeView* view) {
    if(!view->doc || view->home || view->cursorX<84 || view->cursorX>=1196 || view->cursorY<128 || view->cursorY>=600)return -1;
    int y=view->cursorY-148+view->scroll;
    for(int i=0;i<view->doc->count;++i) {
        const WebLine* row=&view->doc->lines[i];
        if(row->link>=0 && y>=row->y && y<row->y+row->height && view->cursorX<92+xui_text_width(row->font,row->text,0))return row->link;
    }
    return -1;
}
static int nativeHomeHit(int x,int y) {
    if(y>=294 && y<352 && x>=280 && x<1000)return 0;
    if(y>=400 && y<454 && x>=280 && x<1000){int index=(x-280)/184;return index<4 && (x-280)%184<168?index+1:-1;}
    return -1;
}
static void nativePointer(xui_canvas_t* canvas,int x,int y,bool hand) {
    for(int row=0;row<21;++row) {
        int width=row<15?row*3/4+1:5;
        xui_fill(canvas,x-1,y+row-1,width+3,3,0x02100d);
    }
    xui_fill(canvas,x+4,y+13,5,13,0x02100d);
    for(int row=0;row<21;++row){int width=row<15?row*3/4+1:5;xui_fill(canvas,x,y+row,width,1,hand?0x80efc0:0xf2fff9);}
    xui_fill(canvas,x+5,y+13,3,12,hand?0x80efc0:0xf2fff9);
}
static void nativeRender(xui_canvas_t* c,const NativeView* view) {
    xui_fill(c,0,0,1280,720,0x0c141b);xui_vgrad(c,0,0,1280,720,0x184044,75,0);
    browserText(c,XUI_F_ROW,64,20,"X360Browser",0xf0f7f6,500);
    browserText(c,XUI_F_LABEL,952,27,"NATIVE 0.2.0",0x9de0c6,264);
    for(int i=0;i<4;++i){int x=64+i*46;bool focus=view->toolbar && view->selected==i;xui_round_rect(c,x,62,36,36,9,focus?0x84dfbf:0x1e2b31,255);browserNavIcon(c,i,x,62,focus?0x0b2520:0xb5cbc9);}
    xui_round_rect(c,262,62,954,36,9,view->toolbar && view->selected==4?0x385e56:0x213139,255);
    xui_circle(c,282,80,4,view->loading?0xe1c279:0x8fe7bf,255);
    browserText(c,XUI_F_LABEL,298,69,view->home?"Search Bing or enter an HTTPS address":view->url,0xc7d9d7,850);
    browserCentered(c,XUI_F_LABEL,1175,62,30,36,"Y",0x9aeac7);
    xui_round_rect(c,60,108,1160,512,12,0x304648,255);xui_fill(c,64,112,1152,504,0x11252c);
    if(view->home) {
        xui_vgrad(c,64,112,1152,504,0x397d75,130,0);
        browserText(c,XUI_F_LABEL,102,144,"YOUR WEB. ON YOUR 360.",0x9cddc8,1040);
        browserCentered(c,XUI_F_TITLE,160,211,960,58,"A new window to the web.",0xeffaf4);
        bool search=nativeHomeHit(view->cursorX,view->cursorY)==0;
        xui_round_rect(c,280,294,720,58,10,search?0x385e56:0x263f46,255);
        browserText(c,XUI_F_BODY,304,311,"Search Bing or type a website address",0xc4d6d5,600);
        xui_round_rect(c,922,294,78,58,10,0x86dfba,255);browserCentered(c,XUI_F_BODY,922,294,78,58,"Y",0x092e21);
        const char* names[]={"Bing","Google","Wikipedia","X360HUB"};
        for(int i=0;i<4;++i){int x=280+i*184;bool hover=nativeHomeHit(view->cursorX,view->cursorY)==i+1;xui_round_rect(c,x,400,168,54,10,hover?0x385e56:0x1d353b,255);browserCentered(c,XUI_F_BODY,x,400,168,54,names[i],0xe1eeea);}
        browserCentered(c,XUI_F_LABEL,150,511,980,30,"Direct HTTPS on your console. No PC or browser.ini required.",0x99c7ba);
        browserCentered(c,XUI_F_LABEL,150,550,980,30,"Experimental HTML reader: text and links; JavaScript, CSS and media are not supported.",0x9ab5b1);
    } else if(view->doc) {
        xui_rect_t pageClip={80,128,1116,472};xui_set_clip(c,pageClip);
        int hover=nativeHitLink(view);
        for(int i=0;i<view->doc->count;++i) {
            const WebLine* row=&view->doc->lines[i];int top=148+row->y-view->scroll;
            if(top+row->height<128 || top>=600)continue;
            uint32_t color=row->link>=0?0x92d9ff:0xd9e4e7;
            if(hover>=0 && row->link==hover){xui_round_rect(c,86,top-2,xui_text_width(row->font,row->text,0)+12,row->height,4,0x254a4d,255);color=0xaff2d0;}
            browserText(c,row->font,92,top,row->text,color,1060);
        }
        xui_clip_all(c);
        if(view->doc->height>440) {
            xui_round_rect(c,1202,130,4,468,2,0x253f46,255);
            int thumb=468*440/view->doc->height;if(thumb<24)thumb=24;
            int maximum=view->doc->height-440;if(maximum<1)maximum=1;
            int offset=(468-thumb)*view->scroll/maximum;xui_round_rect(c,1202,130+offset,4,thumb,2,0x8abdab,255);
        }
    }
    if(view->loading){xui_round_rect(c,907,118,292,34,8,0x243e3f,255);browserCentered(c,XUI_F_LABEL,907,118,292,34,"Loading directly from the web...",0xa5e7c8);}
    browserHint(c,64,136,"A","Open",0x88ddb0);browserHint(c,210,136,"B","Back",0xea9d9e);
    browserHint(c,356,136,"X","Google",0x91bbf0);browserHint(c,502,156,"Y","Address",0xe3ce89);
    browserHint(c,668,186,"LT/RT","Scroll",0x9bb4ba);browserHint(c,864,166,"L3","Toolbar",0x9bb4ba);browserHint(c,1040,176,"BACK","Home",0x9bb4ba);
    browserText(c,XUI_F_LABEL,64,682,view->status,0x90b4a9,1152);
    if(!view->toolbar)nativePointer(c,view->cursorX,view->cursorY,view->home?nativeHomeHit(view->cursorX,view->cursorY)>=0:nativeHitLink(view)>=0);
}
#endif
