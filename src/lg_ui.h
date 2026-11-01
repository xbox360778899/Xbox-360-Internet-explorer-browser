// LG webOS-inspired browser chrome. Original layout and icon artwork; no LG assets.
#pragma once
#include "browser_ui.h"
#include "native_document.h"
#include "cursor_icon.h"
#ifndef X360BROWSER_PRERELEASE
#define X360BROWSER_PRERELEASE 1
#endif
struct NativeView {bool home,loading,toolbar;int selected,cursorX,cursorY,scroll;const char* url;const char* status;const WebDocument* doc;bool chrome,menu;int menuSelected;bool video;};
static bool lgChromeVisible(unsigned now,unsigned last,bool menu,bool keyboard,bool loading){return menu||keyboard||loading||(unsigned)(now-last)<4500;}
static int lgPageTop(const NativeView* view){return view->chrome?104:30;}
static int nativeHitLink(const NativeView* view) {
    if(!view->doc || view->home || view->cursorX<48 || view->cursorX>=1232 || view->cursorY<lgPageTop(view) || view->cursorY>=654)return -1;
    int y=view->cursorY-lgPageTop(view)+view->scroll;
    if(view->doc->styled)return browserStyledHover(view->doc->styled,view->cursorX-48,y)?0:-1;
    for(int i=0;i<view->doc->count;++i){const WebLine* row=&view->doc->lines[i];int width=row->kind==WEB_TEXT_ROW?xui_text_width(row->font,row->text,0):1100;if(row->link>=0 && y>=row->y && y<row->y+row->height && view->cursorX<72+width)return row->link;}
    return -1;
}
static int nativeHomeHit(int x,int y){if(x>=80 && x<1200 && y>=166 && y<224)return 0;if(x>=80 && x<1200 && y>=306 && y<434){int slot=(x-80)/284;return slot<4 && (x-80)%284<268?slot+1:-1;}return -1;}
static int lgMenuHit(int x,int y){if(x<916 || x>=1228 || y<128 || y>=416)return -1;return (y-128)/48;}
static void nativePointer(xui_canvas_t* c,int x,int y,bool hover){xui_sprite(c,hover?&cursor_hover:&cursor_normal,x-4,y-2);}
static void lgWatermark(xui_canvas_t* c) {
#if X360BROWSER_PRERELEASE
    const char* text="Developed by @heltonleiras";int width=xui_text_width(XUI_F_LABEL,text,0)+26;
    xui_round_rect(c,1248-width,680,width,26,6,0x1c3a2a,245);browserCentered(c,XUI_F_LABEL,1248-width,680,width,26,text,0xe2e7e9);
#endif
}
static void lgToolbar(xui_canvas_t* c,const NativeView* view) {
    xui_fill(c,0,0,1280,76,0x173b2b);
    for(int i=0;i<4;++i){int x=12+i*46;bool hover=view->toolbar&&view->selected==i;if(hover)xui_round_frame(c,x-2,18,40,40,18,3,0x63e19c);browserNavIcon(c,i,x,20,0xf1f5f5);}
    xui_round_rect(c,204,18,904,42,4,0x2b513d,255);
    if((view->toolbar && view->selected==4)||view->cursorY<76 && view->cursorX>=204 && view->cursorX<1108)xui_round_frame(c,199,13,914,52,7,3,0x63e19c);
    xui_mask(c,&shell_search,208,22,0xd9f5e3,255);browserText(c,XUI_F_LABEL,244,30,view->home?"Search or enter an HTTPS address":view->url,0xf8fafb,835);
    xui_round_rect(c,1121,25,28,26,3,0x375f48,255);browserCentered(c,XUI_F_LABEL,1121,25,28,26,"1",0xffffff);
    bool menuFocus=view->menu||(view->toolbar&&view->selected==5);if(menuFocus)xui_round_frame(c,1158,16,42,44,20,3,0x63e19c);
    for(int i=0;i<3;++i)xui_circle(c,1179,29+i*10,2,0xffffff,255);
    xui_mask(c,&shell_close,1212,20,0xf5f8f8,255);
}
static void lgMenu(xui_canvas_t* c,const NativeView* view) {
    xui_round_rect(c,910,86,324,342,8,0x070b0c,220);xui_round_rect(c,914,82,316,338,5,0x193c2c,255);xui_round_frame(c,914,82,316,338,5,3,0x63e19c);
    browserText(c,XUI_F_BODY,934,96,"Menu",0xffffff,270);
    const char* items[]={"Home","Back","Forward","Reload page","Images & video guide","Close menu"};
    for(int i=0;i<6;++i){int y=128+i*48;bool selected=view->menuSelected==i||lgMenuHit(view->cursorX,view->cursorY)==i;if(selected)xui_round_rect(c,923,y,298,44,3,0x2d6242,255);browserText(c,XUI_F_BODY,936,y+10,items[i],0xffffff,274);}
}
static void lgVideoOverlay(xui_canvas_t* c,const NativeView* view){xui_fill(c,0,0,1280,720,0);if(view->chrome){lgToolbar(c,view);xui_round_rect(c,34,620,502,40,5,0x263238,255);browserCentered(c,XUI_F_LABEL,34,620,502,40,"A  Play / Pause     B  Stop     Y  Address",0xf0f4f4);}if(view->menu)lgMenu(c,view);lgWatermark(c);if(view->chrome)nativePointer(c,view->cursorX,view->cursorY,false);}
static void nativeRender(xui_canvas_t* c,const NativeView* view) {
    if(view->video){lgVideoOverlay(c,view);return;}
    xui_fill(c,0,0,1280,720,0x08150f);
    if(view->home) {
        browserText(c,XUI_F_ROW,80,108,"X360Browser",0xeff4f4,700);
        xui_round_rect(c,80,166,1120,58,5,0xf1faf3,255);browserText(c,XUI_F_BODY,106,185,"Search the web",0x526164,990);
        xui_mask(c,&shell_search,1148,177,0x365c43,255);
        browserText(c,XUI_F_LABEL,80,270,"Recommended Sites",0xb7c2c4,1080);
        const char* names[]={"Bing","Google","Wikipedia","X360HUB"};const char* initials[]={"B","G","W","X"};const uint32_t colors[]={0x78dca4,0x4bbd87,0xb6e8cb,0x62be8b};
        for(int i=0;i<4;++i){int x=80+i*284;bool hover=nativeHomeHit(view->cursorX,view->cursorY)==i+1;xui_fill(c,x,306,268,128,0x1a3e2d);if(hover)xui_round_frame(c,x-3,303,274,134,3,3,0x63e19c);xui_circle(c,x+134,351,26,colors[i],255);browserCentered(c,XUI_F_ROW,x+102,324,64,54,initials[i],0x111a1d);browserCentered(c,XUI_F_LABEL,x,394,268,28,names[i],0xe6eeee);}
    } else if(view->doc) {
        int top=lgPageTop(view);xui_rect_t clip={48,top,1184,654-top};xui_set_clip(c,clip);int hover=nativeHitLink(view);
        if(view->doc->styled)browserStyledDraw(view->doc->styled,c,48,top,1184,654-top,view->scroll);
        for(int i=0;!view->doc->styled&&i<view->doc->count;++i){const WebLine* row=&view->doc->lines[i];int y=top+row->y-view->scroll;if(y+row->height<top || y>=654)continue;
            if(row->kind==WEB_IMAGE_ROW){const WebImage* image=&view->doc->images[row->media];if(image->pixels){xui_image_t bitmap={image->pixels,image->width,image->height,image->width,XUI_FMT_XRGB};xui_blit(c,&bitmap,64,y,0);browserText(c,XUI_F_LABEL,64,y+image->height+4,image->alt,0x9db0b5,1090);}else{xui_round_rect(c,64,y+4,1100,110,4,0x26343a,255);browserText(c,XUI_F_LABEL,82,y+24,image->state<0?"Image unavailable or unsupported":"Loading image...",0xa9bec4,1050);browserText(c,XUI_F_LABEL,82,y+54,image->alt,0xd4dfe2,1050);}continue;}
            if(row->kind==WEB_VIDEO_ROW){xui_round_rect(c,64,y+5,1100,74,5,hover==row->link?0x2d6242:0x29383e,255);for(int dx=0;dx<18;++dx)xui_fill(c,88+dx,y+24+dx/2,1,28-dx,0x75e8ac);browserText(c,XUI_F_BODY,132,y+22,"Play video",0xf1f5f5,900);browserText(c,XUI_F_LABEL,132,y+49,"Direct WMV/XMV files - availability depends on the video codec",0x9bafb4,900);continue;}
            uint32_t color=row->link>=0?0xa3d8f4:0xdce4e7;if(row->link>=0&&row->link==hover){xui_round_rect(c,58,y-1,xui_text_width(row->font,row->text,0)+14,row->height,3,0x2d6242,255);color=0xffffff;}browserText(c,row->font,64,y,row->text,color,1100);
        }
        xui_clip_all(c);
    }
    if(view->loading){xui_round_rect(c,900,92,330,34,4,0x3e4b50,255);browserCentered(c,XUI_F_LABEL,900,92,330,34,"Loading from the console...",0xecf4f3);}
    if(view->chrome)lgToolbar(c,view);
    if(view->menu)lgMenu(c,view);lgWatermark(c);
    if(view->chrome)nativePointer(c,view->cursorX,view->cursorY,view->home?nativeHomeHit(view->cursorX,view->cursorY)>=0:nativeHitLink(view)>=0);
}
