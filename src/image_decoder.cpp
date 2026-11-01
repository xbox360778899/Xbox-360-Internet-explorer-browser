#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_SIMD
#define STBI_MAX_DIMENSIONS 4096
#include "../vendor/stb/stb_image.h"
#include "image_decoder.h"
#include <stdlib.h>
uint32_t* browserDecodeImage(const unsigned char* data,int bytes,int* width,int* height) {
    int w=0,h=0,channels=0;
    if(!stbi_info_from_memory(data,bytes,&w,&h,&channels) || w<1 || h<1 || w>4096 || h>4096 || (unsigned)w*(unsigned)h>4194304)return NULL;
    unsigned char* rgba=stbi_load_from_memory(data,bytes,&w,&h,&channels,4);if(!rgba)return NULL;
    int dw=w,dh=h;if(dw>1100){dh=dh*1100/dw;dw=1100;}if(dh>560){dw=dw*560/dh;dh=560;}if(dw<1)dw=1;if(dh<1)dh=1;
    uint32_t* rgb=(uint32_t*)malloc(dw*dh*4);
    if(rgb)for(int y=0;y<dh;++y)for(int x=0;x<dw;++x){const unsigned char* p=rgba+((y*h/dh)*w+x*w/dw)*4;unsigned a=p[3];unsigned r=(p[0]*a+20*(255-a))/255,g=(p[1]*a+27*(255-a))/255,b=(p[2]*a+29*(255-a))/255;rgb[y*dw+x]=(r<<16)|(g<<8)|b;}
    stbi_image_free(rgba);*width=dw;*height=dh;return rgb;
}
