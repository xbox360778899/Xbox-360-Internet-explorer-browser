#pragma once
#include "xui.h"
typedef bool (*BrowserStyleFetch)(void* context,const char* url,unsigned char** bytes,int* size,char* finalURL,int capacity);
void* browserStyledCreate(const char* html,const char* url,BrowserStyleFetch fetch,void* context);
void browserStyledDestroy(void* page);
int browserStyledHeight(void* page);
void browserStyledDraw(void* page,xui_canvas_t* target,int x,int y,int width,int height,int scroll);
bool browserStyledHover(void* page,int x,int y);
bool browserStyledClick(void* page,int x,int y,char* url,int capacity);
int browserStyledCSSCount(void* page);
int browserStyledCSSFailed(void* page);
int browserStyledImageCount(void* page);
bool browserStyledNextImage(void* page,char* url,int capacity);
void browserStyledCompleteImage(void* page,const char* url,const unsigned char* data,int size);
