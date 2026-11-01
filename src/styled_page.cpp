// Native litehtml container: CSS layout, Orbita text, image decoding and HTTPS callbacks.
#include "styled_page.h"
#include "native_document.h"
#include "image_decoder.h"
#include "master_css.h"
#include "../vendor/litehtml/include/litehtml.h"
#include <set>
#include <vector>
#include <string>
#include <algorithm>
struct BrowserStyleFont{int face,size,decoration;};
struct BrowserStyleImage{std::string url;uint32_t* pixels;int width,height,state;BrowserStyleImage():pixels(NULL),width(0),height(0),state(0){}};
class BrowserStyleContainer:public litehtml::document_container {
public:
    BrowserStyleFetch fetch;void* context;std::string base,clicked;
    litehtml::context engineContext;litehtml::document::ptr document;
    std::vector<BrowserStyleImage> images;std::set<std::string> styles;
    std::vector<xui_rect_t> clips;std::vector<uint32_t> cache;
    unsigned cssBytes;int cssCount;bool ready,hand,dirty;int lastScroll,lastHeight;
    xui_canvas_t* drawing;
    BrowserStyleContainer(const char* url,BrowserStyleFetch callback,void* user):fetch(callback),context(user),base(url),cssBytes(0),cssCount(0),ready(false),hand(false),dirty(true),lastScroll(-1),lastHeight(0),drawing(NULL){}
    ~BrowserStyleContainer(){document=0;for(size_t i=0;i<images.size();++i)free(images[i].pixels);}
    uint32_t color(litehtml::web_color value){return ((uint32_t)value.red<<16)|((uint32_t)value.green<<8)|value.blue;}
    BrowserStyleFont* font(litehtml::uint_ptr handle){return (BrowserStyleFont*)handle;}
    virtual litehtml::uint_ptr create_font(const char*,int size,int weight,litehtml::font_style,unsigned int decoration,litehtml::font_metrics* fm){
        BrowserStyleFont* f=new BrowserStyleFont;f->face=weight>=600?XUI_F_ROW:XUI_F_BODY;f->size=size<8?8:size>80?80:size;f->decoration=decoration;
        const xui_face_t* face=xui_face(f->face);fm->ascent=face->ascent*f->size/face->px;fm->descent=face->descent*f->size/face->px;fm->height=fm->ascent+fm->descent;fm->x_height=f->size/2;return (litehtml::uint_ptr)f;
    }
    virtual void delete_font(litehtml::uint_ptr handle){delete font(handle);}
    virtual int text_width(const char* text,litehtml::uint_ptr handle){BrowserStyleFont* f=font(handle);return xui_text_width(f->face,text,0)*f->size/xui_face(f->face)->px;}
    virtual void draw_text(litehtml::uint_ptr,const char* text,litehtml::uint_ptr handle,litehtml::web_color ink,const litehtml::position& pos){
        if(!drawing || pos.width<1 || pos.height<1 || pos.y+pos.height<drawing->clip.y || pos.y>=drawing->clip.y+drawing->clip.h)return;
        BrowserStyleFont* f=font(handle);const xui_face_t* face=xui_face(f->face);
        int sourceWidth=xui_text_width(f->face,text,0)+4;if(sourceWidth<1 || sourceWidth>8192)return;
        int sourceHeight=face->ascent+face->descent+4;
        int width=sourceWidth*f->size/face->px,height=sourceHeight*f->size/face->px;if(width<1 || height<1 || width>8192)return;
        std::vector<uint32_t> source(sourceWidth*sourceHeight),scaled(width*height);
        xui_canvas_t mask;xui_canvas_init(&mask,&source[0],sourceWidth,sourceHeight,sourceWidth);xui_text(&mask,f->face,0,face->ascent,0xffffff,text,0,sourceWidth);
        xui_image_t image={&source[0],sourceWidth,sourceHeight,sourceWidth,XUI_FMT_XRGB};xui_scale_cover(&scaled[0],width,height,&image);
        int baseY=pos.y;uint32_t rgb=color(ink);
        for(int y=0;y<height;++y){int yy=baseY+y;if(yy<drawing->clip.y || yy>=drawing->clip.y+drawing->clip.h)continue;for(int x=0;x<width;++x){int xx=pos.x+x;if(xx<drawing->clip.x || xx>=drawing->clip.x+drawing->clip.w)continue;unsigned alpha=(scaled[y*width+x]&255)*ink.alpha/255;if(alpha){uint32_t* dest=drawing->px+yy*drawing->stride+xx;*dest=xui_mix(*dest,rgb,alpha);}}}
        if(f->decoration&litehtml::font_decoration_underline)xui_fill_a(drawing,pos.x,pos.y+f->size,pos.width,1,rgb,ink.alpha);
    }
    virtual void fill_rect(litehtml::uint_ptr,const litehtml::position& pos,const litehtml::web_color ink,const litehtml::css_border_radius& radius){if(!drawing||pos.width<1||pos.height<1)return;int r=(int)radius.top_left_x.val();if(r<0)r=0;if(r>100)r=100;xui_round_rect(drawing,pos.x,pos.y,pos.width,pos.height,r,color(ink),ink.alpha);}
    virtual int pt_to_px(int pt){return pt*96/72;}
    virtual int get_default_font_size(){return 18;}
    virtual const char* get_default_font_name(){return "Inter";}
    virtual void draw_list_marker(litehtml::uint_ptr,litehtml::list_style_type,int x,int y,int height,const litehtml::web_color& ink){if(drawing)xui_circle(drawing,x+height/2,y+height/2,3,color(ink),ink.alpha);}
    std::string resolve(const char* src,const char* baseurl){char url[WEB_URL];if(!webResolveURL(baseurl&&*baseurl?baseurl:base.c_str(),src,url,sizeof(url)))return "";return url;}
    BrowserStyleImage* findImage(const char* src,const char* baseurl){std::string url=resolve(src,baseurl);for(size_t i=0;i<images.size();++i)if(images[i].url==url)return &images[i];return NULL;}
    virtual void load_image(const char* src,const char* baseurl){
        if(ready || !fetch || images.size()>=12 || findImage(src,baseurl))return;
        std::string url=resolve(src,baseurl);if(url.empty())return;BrowserStyleImage image;image.url=url;images.push_back(image);
        // Do not block first paint on images. The worker fetches them after the CSS layout is visible.
    }
    virtual void get_image_size(const char* src,const char* baseurl,litehtml::size& size){load_image(src,baseurl);BrowserStyleImage* image=findImage(src,baseurl);if(image&&image->pixels){size.width=image->width;size.height=image->height;}else{size.width=0;size.height=0;}}
    void paintImage(BrowserStyleImage* image,const litehtml::position& pos){if(!drawing||!image||!image->pixels||pos.width<1||pos.height<1||pos.width>1280||pos.height>1440||pos.width*pos.height>1280*720*2||pos.y+pos.height<drawing->clip.y||pos.y>=drawing->clip.y+drawing->clip.h)return;std::vector<uint32_t> scaled(pos.width*pos.height);xui_image_t bitmap={image->pixels,image->width,image->height,image->width,XUI_FMT_XRGB};xui_scale_cover(&scaled[0],pos.width,pos.height,&bitmap);xui_image_t out={&scaled[0],pos.width,pos.height,pos.width,XUI_FMT_XRGB};xui_blit(drawing,&out,pos.x,pos.y,0);}
    virtual void draw_image(litehtml::uint_ptr,const char* src,const char* baseurl,const litehtml::position& pos){paintImage(findImage(src,baseurl),pos);}
    virtual void draw_background(litehtml::uint_ptr,const char* src,const char* baseurl,const litehtml::position& pos,const litehtml::css_position& bgpos,litehtml::background_repeat repeat,litehtml::background_attachment){
        BrowserStyleImage* image=findImage(src,baseurl);if(!drawing||!image||!image->pixels)return;
        xui_rect_t old=drawing->clip,clip={pos.x,pos.y,pos.width,pos.height};xui_set_clip(drawing,xui_rect_inter(old,clip));
        litehtml::css_position position=bgpos;
        int w=(int)position.width.calc_percent(pos.width),h=(int)position.height.calc_percent(pos.height);if(w<1)w=image->width;if(h<1)h=image->height;
        int ox=pos.x+position.x.calc_percent(pos.width-w),oy=pos.y+position.y.calc_percent(pos.height-h);
        bool rx=repeat==litehtml::background_repeat_repeat||repeat==litehtml::background_repeat_repeat_x,ry=repeat==litehtml::background_repeat_repeat||repeat==litehtml::background_repeat_repeat_y;
        int firstY=oy;if(ry)while(firstY>pos.y)firstY-=h;int count=0;
        for(int y=firstY;y<pos.y+pos.height && count<512;y+=h){int firstX=ox;if(rx)while(firstX>pos.x)firstX-=w;for(int x=firstX;x<pos.x+pos.width && count++<512;x+=w){litehtml::position p=pos;p.x=x;p.y=y;p.width=w;p.height=h;paintImage(image,p);if(!rx)break;}if(!ry)break;}
        xui_set_clip(drawing,old);
    }
    virtual void draw_borders(litehtml::uint_ptr,const litehtml::css_borders& b,const litehtml::position& p){if(!drawing)return;int left=(int)b.left.width.val(),top=(int)b.top.width.val(),right=(int)b.right.width.val(),bottom=(int)b.bottom.width.val();if(left>0&&b.left.style!=litehtml::border_style_none)xui_fill_a(drawing,p.x,p.y,left,p.height,color(b.left.color),b.left.color.alpha);if(top>0&&b.top.style!=litehtml::border_style_none)xui_fill_a(drawing,p.x,p.y,p.width,top,color(b.top.color),b.top.color.alpha);if(right>0&&b.right.style!=litehtml::border_style_none)xui_fill_a(drawing,p.x+p.width-right,p.y,right,p.height,color(b.right.color),b.right.color.alpha);if(bottom>0&&b.bottom.style!=litehtml::border_style_none)xui_fill_a(drawing,p.x,p.y+p.height-bottom,p.width,bottom,color(b.bottom.color),b.bottom.color.alpha);}
    virtual void set_caption(const char*){}
    virtual void set_base_url(const char* url){std::string resolved=resolve(url,NULL);if(!resolved.empty())base=resolved;}
    virtual void link(litehtml::document* doc,litehtml::element::ptr el){const char* rel=el->get_attr("rel","");if(webFind(rel,"stylesheet")){litehtml::tstring text,baseurl=base;litehtml::string_vector media;import_css(text,el->get_attr("href",""),baseurl,media);if(!text.empty())doc->add_stylesheet(text.c_str(),baseurl.c_str());}}
    virtual void on_anchor_click(const char* url,litehtml::element::ptr){clicked=resolve(url,NULL);}
    virtual void set_cursor(const char* cursor){hand=cursor&&webEqual(cursor,"pointer");}
    virtual char toupper(const char c){return (char)::toupper((unsigned char)c);}
    virtual char tolower(const char c){return (char)::tolower((unsigned char)c);}
    virtual void import_css(litehtml::tstring& text,const litehtml::tstring& url,litehtml::tstring& baseurl,const litehtml::string_vector&){
        text.clear();std::string resolved=resolve(url.c_str(),baseurl.c_str());if(ready||!fetch||resolved.empty()||styles.size()>=8||styles.count(resolved)||cssBytes>=512*1024)return;
        styles.insert(resolved);unsigned char* bytes=NULL;int size=0;char finalURL[WEB_URL];if(fetch(context,resolved.c_str(),&bytes,&size,finalURL,sizeof(finalURL))&&size<=256*1024 && cssBytes+(unsigned)size<=512*1024){text.assign((const char*)bytes,size);cssBytes+=size;++cssCount;baseurl=finalURL;}free(bytes);
    }
    virtual void set_clip(const litehtml::position& pos,bool,bool){if(!drawing)return;clips.push_back(drawing->clip);xui_rect_t next={pos.x,pos.y,pos.width,pos.height};xui_set_clip(drawing,xui_rect_inter(drawing->clip,next));}
    virtual void del_clip(){if(drawing&&!clips.empty()){xui_set_clip(drawing,clips.back());clips.pop_back();}}
};
void* browserStyledCreate(const char* html,const char* url,BrowserStyleFetch fetch,void* user){
    if(!html||strlen(html)>512*1024)return NULL;
    BrowserStyleContainer* page=new BrowserStyleContainer(url,fetch,user);page->engineContext.load_master_stylesheet(browserMasterCSS);
    page->engineContext.load_master_stylesheet("script,noscript,template{display:none;}body{font-family:Inter;font-size:18px;color:#111;background:#fff;}a{color:#005a33;cursor:pointer;}");
    page->document=litehtml::document::createFromString(html,page,&page->engineContext);if(!page->document){delete page;return NULL;}page->document->render(1280);page->ready=true;return page;
}
void browserStyledDestroy(void* handle){delete (BrowserStyleContainer*)handle;}
int browserStyledHeight(void* handle){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;return p&&p->document?p->document->height():0;}
int browserStyledCSSCount(void* handle){return handle?((BrowserStyleContainer*)handle)->cssCount:0;}
bool browserStyledNextImage(void* handle,char* url,int capacity){
    BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p)return false;
    for(size_t i=0;i<p->images.size();++i)if(p->images[i].state==0){
        if(p->images[i].url.size()>=(size_t)capacity)return false;
        strcpy_s(url,capacity,p->images[i].url.c_str());p->images[i].state=2;return true;
    }
    return false;
}
void browserStyledCompleteImage(void* handle,const char* url,const unsigned char* data,int bytes){
    BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p)return;
    for(size_t i=0;i<p->images.size();++i)if(p->images[i].url==url && p->images[i].state==2){
        BrowserStyleImage& image=p->images[i];
        if(data&&bytes>0)image.pixels=browserDecodeImage(data,bytes,&image.width,&image.height);
        image.state=image.pixels?1:-1;
        if(p->document)p->document->render(1280);
        p->dirty=true;return;
    }
}
int browserStyledImageCount(void* handle){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;int n=0;if(p)for(size_t i=0;i<p->images.size();++i)if(p->images[i].pixels)++n;return n;}
void browserStyledDraw(void* handle,xui_canvas_t* target,int x,int y,int width,int height,int scroll){
    BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p||width!=1280||height<1||height>720)return;
    if(p->dirty||p->lastScroll!=scroll||p->lastHeight!=height){p->cache.assign(width*height,0xffffff);xui_canvas_t canvas;xui_canvas_init(&canvas,&p->cache[0],width,height,width);p->drawing=&canvas;p->clips.clear();litehtml::position clip;clip.x=0;clip.y=0;clip.width=width;clip.height=height;p->document->draw((litehtml::uint_ptr)0,0,-scroll,&clip);p->drawing=NULL;p->dirty=false;p->lastScroll=scroll;p->lastHeight=height;}
    xui_image_t image={&p->cache[0],width,height,width,XUI_FMT_XRGB};xui_blit(target,&image,x,y,0);
}
bool browserStyledHover(void* handle,int x,int y){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p)return false;litehtml::position::vector boxes;if(p->document->on_mouse_over(x,y,boxes))p->dirty=true;return p->hand;}
bool browserStyledClick(void* handle,int x,int y,char* url,int capacity){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p)return false;litehtml::position::vector boxes;p->clicked.clear();p->document->on_mouse_over(x,y,boxes);p->document->on_lbutton_down(x,y,boxes);p->document->on_lbutton_up(x,y,boxes);p->dirty=true;return !p->clicked.empty()&&webCopy(url,capacity,p->clicked.c_str());}
