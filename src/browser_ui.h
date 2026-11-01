// Orbita XUI typography uses baseline coordinates; all layout helpers use measured metrics.
#ifndef X360BROWSER_UI_H
#define X360BROWSER_UI_H
#include "xui.h"
#include <stdio.h>

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
#endif
