// Native litehtml container: CSS layout, Orbita text, image decoding and HTTPS callbacks.
#include "styled_page.h"
#include "native_document.h"
#include "image_decoder.h"
#include "master_css.h"
#include "../vendor/litehtml/include/litehtml.h"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
struct BrowserStyleFont{int face,size,decoration;};
struct BrowserStyleImage{
 std::string url;uint32_t* pixels;int width,height,state;
 std::vector<uint32_t> scaled;int scaledW,scaledH;
 BrowserStyleImage():pixels(NULL),width(0),height(0),state(0),scaledW(0),scaledH(0){}
};
class BrowserStyleContainer:public litehtml::document_container {
public:
    BrowserStyleFetch fetch;void* context;std::string base,clicked;
    litehtml::context engineContext;litehtml::document::ptr document;
    std::vector<BrowserStyleImage> images;std::set<std::string> styles;
    std::vector<xui_rect_t> clips;std::vector<uint32_t> cache;
    unsigned cssBytes;int cssCount,cssFailed,imageUpdates;bool ready,hand,dirty;int lastScroll,lastHeight,lastHoverX,lastHoverY;
    xui_canvas_t* drawing;
    BrowserStyleContainer(const char* url,BrowserStyleFetch callback,void* user):fetch(callback),context(user),base(url),cssBytes(0),cssCount(0),cssFailed(0),imageUpdates(0),ready(false),hand(false),dirty(true),lastScroll(-1),lastHeight(0),lastHoverX(-1),lastHoverY(-1),drawing(NULL){}
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
        if(!drawing || pos.width<1 || pos.height<1 || pos.y+pos.height<drawing->clip.y || pos.y>=drawing->clip.y+drawing->clip.h ||
            pos.x>=drawing->clip.x+drawing->clip.w || pos.x+pos.width<=drawing->clip.x)return;
        BrowserStyleFont* f=font(handle);const xui_face_t* face=xui_face(f->face);
        // Fast path: the common native font size skips two heap allocations and pixel scaling.
        if(f->size==face->px && ink.alpha==255){
            xui_text(drawing,f->face,pos.x,pos.y+face->ascent,color(ink),text,0,pos.width);
            if(f->decoration&litehtml::font_decoration_underline)xui_fill(drawing,pos.x,pos.y+f->size,pos.width,1,color(ink));
            return;
        }
        int sourceWidth=xui_text_width(f->face,text,0)+4;if(sourceWidth<1 || sourceWidth>4096)return;
        int sourceHeight=face->ascent+face->descent+4;
        int width=sourceWidth*f->size/face->px,height=sourceHeight*f->size/face->px;if(width<1 || height<1 || width>8192)return;
        if(width*height>1024*1024)return;std::vector<uint32_t> source(sourceWidth*sourceHeight),scaled(width*height);
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
    void paintImage(BrowserStyleImage* image,const litehtml::position& pos){
    if(!drawing||!image||!image->pixels||pos.width<1||pos.height<1||pos.width>1280||pos.height>1440||
       pos.width*pos.height>1280*720*2||pos.y+pos.height<drawing->clip.y||pos.y>=drawing->clip.y+drawing->clip.h ||
       pos.x+pos.width<=drawing->clip.x || pos.x>=drawing->clip.x+drawing->clip.w)return;
    // Cache the scaled bitmap, not just the JPEG/PNG pixels; scrolling then avoids repeated scaling.
    if(image->scaledW!=pos.width||image->scaledH!=pos.height||image->scaled.empty()){
        image->scaled.assign(pos.width*pos.height,0);
        xui_image_t bitmap={image->pixels,image->width,image->height,image->width,XUI_FMT_XRGB};
        xui_scale_cover(&image->scaled[0],pos.width,pos.height,&bitmap);
        image->scaledW=pos.width;image->scaledH=pos.height;
    }
    xui_image_t out={&image->scaled[0],pos.width,pos.height,pos.width,XUI_FMT_XRGB};
    xui_blit(drawing,&out,pos.x,pos.y,0);
}
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
    // litehtml v0.3 predates flex/grid. Stack these layouts instead of rejecting
    // entire boxes. This is a readability fallback, NOT complete CSS flex/grid support.
    static void legacyLayoutFallback(std::string& css) {
        // Litehtml table rows approximate simple flex/grid; not complete layout semantics.
        std::string supplemental;
        size_t start=0;int count=0;
        while(start<css.size() && count++<300){
            size_t open=css.find('{',start);
            if(open==std::string::npos)break;
            size_t close=css.find('}',open+1);
            if(close==std::string::npos)break;
            size_t selectorStart=css.rfind('}',open);
            selectorStart=selectorStart==std::string::npos?0:selectorStart+1;
            std::string selector=css.substr(selectorStart,open-selectorStart);
            size_t first=selector.find_first_not_of(" \t\r\n");
            if(first!=std::string::npos)selector.erase(0,first);
            size_t last=selector.find_last_not_of(" \t\r\n");
            if(last!=std::string::npos)selector.erase(last+1);
            std::string body=css.substr(open+1,close-open-1);
            const char* prefixes[]={"display:flex","display: flex","display:grid","display: grid",
                                  "display:inline-flex","display: inline-flex",
                                  "display:inline-grid","display: inline-grid"};
            bool layout=false,grid=false;
            for(int i=0;i<8;++i){
                size_t at=body.find(prefixes[i]);
                if(at!=std::string::npos){
                    grid=(i==2||i==3||i==6||i==7);
                    body.replace(at,strlen(prefixes[i]),"display:block");layout=true;break;
                }
            }
            if(layout){
                bool column=body.find("flex-direction:column")!=std::string::npos ||
                            body.find("flex-direction: column")!=std::string::npos;
                bool safe=!selector.empty()&&selector.size()<100&&selector[0]!='@'&&
                          selector.find('{')==std::string::npos&&selector.find(',')==std::string::npos;
                if(!column && safe && supplemental.size()<16384){
                    int columns=0;
                    if(grid){
                        size_t property=body.find("grid-template-columns:");
                        if(property!=std::string::npos){
                            size_t repeat=body.find("repeat(",property);
                            if(repeat!=std::string::npos)columns=atoi(body.c_str()+repeat+7);
                            if(columns<2||columns>4)columns=0;
                        }
                    }
                    char rule[192];
                    if(columns>1)sprintf_s(rule,sizeof(rule)," > *{display:block;float:left;width:%d%%;}\n",100/columns);
                    else strcpy_s(rule,sizeof(rule)," > *{display:block;float:left;}\n");
                    supplemental+=selector; supplemental+=rule;
                }
                css.replace(open+1,close-open-1,body);
                close=open+body.size()+1;
            }
            start=close+1;
        }
        css+=supplemental;
    }
    static void customProperties(std::string& css) {
        // Small legacy compatibility layer: resolve constant :root CSS variables.
        // Scoped variables, calc(), grid, and layout algorithms remain unsupported.
        std::map<std::string,std::string> values;
        size_t cursor=0;
        while(values.size()<128 && (cursor=css.find(":root",cursor))!=std::string::npos){
            size_t open=css.find('{',cursor+5);
            if(open==std::string::npos || open-cursor>90){cursor+=5;continue;}
            size_t close=css.find('}',open+1);
            if(close==std::string::npos)break;
            std::string body=css.substr(open+1,close-open-1);
            size_t off=0;
            while(off<body.size()){
                size_t stop=body.find(';',off);if(stop==std::string::npos)stop=body.size();
                std::string item=body.substr(off,stop-off);size_t sep=item.find(':');
                if(sep!=std::string::npos){
                    std::string key=item.substr(0,sep), value=item.substr(sep+1);
                    size_t b=key.find_first_not_of(" \t\r\n"),e=key.find_last_not_of(" \t\r\n");
                    if(b!=std::string::npos)key=key.substr(b,e-b+1);
                    b=value.find_first_not_of(" \t\r\n");e=value.find_last_not_of(" \t\r\n");
                    if(b!=std::string::npos)value=value.substr(b,e-b+1);
                    if(key.size()>2 && key.substr(0,2)=="--" && key.size()<96 && value.size()<256)
                        values[key]=value;
                }
                off=stop+1;
            }
            cursor=close+1;
        }
        cursor=0;
        for(int n=0;n<4096;++n) {
            size_t start=css.find("var(",cursor);
            if(start==std::string::npos)break;
            size_t close=start+4;int depth=1;
            while(close<css.size() && depth && close-start<512){
                if(css[close]=='(')++depth;
                else if(css[close]==')')--depth;
                if(depth)++close;
            }
            if(depth){cursor=start+4;continue;}
            std::string expression=css.substr(start+4,close-start-4);
            size_t comma=expression.find(',');
            std::string key=expression.substr(0,comma);
            size_t b=key.find_first_not_of(" \t\r\n"),e=key.find_last_not_of(" \t\r\n");
            if(b!=std::string::npos)key=key.substr(b,e-b+1);
            std::map<std::string,std::string>::const_iterator it=values.find(key);
            std::string replacement;
            if(it!=values.end())replacement=it->second;
            else if(comma!=std::string::npos){
                replacement=expression.substr(comma+1);
                b=replacement.find_first_not_of(" \t\r\n");
                if(b!=std::string::npos)replacement.erase(0,b);
            }else{cursor=close+1;continue;}
            css.replace(start,close-start+1,replacement);
            cursor=start+replacement.size();
        }
    }
    static void preprocessStyleElements(std::string& html){
        size_t pos=0;
        while((pos=html.find("<style",pos))!=std::string::npos){
            size_t open=html.find('>',pos+6);
            if(open==std::string::npos)break;
            size_t end=html.find("</style>",open+1);
            if(end==std::string::npos)break;
            std::string css=html.substr(open+1,end-open-1);
            compatibleMediaQueries(css);legacyLayoutFallback(css);customProperties(css);
            html.replace(open+1,end-open-1,css);
            pos=open+1+css.size()+8;
        }
    }
    static void compatibleMediaQueries(std::string& css){
        // Translate straightforward CSS @media width rules for the 1280px TV viewport.
        // Complex queries still require a newer layout engine.
        size_t start=0;int processed=0;
        while(processed++<128 && (start=css.find("@media",start))!=std::string::npos){
            size_t open=css.find('{',start+6);
            if(open==std::string::npos || open-start>256){start+=6;continue;}
            int depth=1;size_t end=open+1;
            while(end<css.size() && depth){if(css[end]=='{')++depth;else if(css[end]=='}')--depth;++end;}
            if(depth){start=open+1;continue;}
            std::string condition=css.substr(start+6,open-(start+6));
            bool visible=condition.find("print")==std::string::npos;
            const char* types[]={"max-width","min-width"};
            for(int j=0;j<2;++j){size_t k=condition.find(types[j]);if(k==std::string::npos)continue;
                k=condition.find(':',k);if(k==std::string::npos)continue;
                unsigned bound=(unsigned)strtoul(condition.c_str()+k+1,NULL,10);
                if(bound){if(j==0&&1280>bound)visible=false; if(j==1&&1280<bound)visible=false;}
            }
            std::string inner=visible?css.substr(open+1,end-open-2):"";
            css.replace(start,end-start,inner);
            start+=inner.size();
        }
    }
    virtual void import_css(litehtml::tstring& text,const litehtml::tstring& url,litehtml::tstring& baseurl,const litehtml::string_vector&){
        text.clear();std::string resolved=resolve(url.c_str(),baseurl.c_str());if(ready||!fetch||resolved.empty()||styles.size()>=10||styles.count(resolved)||cssBytes>=1536*1024)return;
        // Skip unavailable web fonts instead of blocking layout on another TLS handshake.
        if(resolved.find("fonts.googleapis.com/")!=std::string::npos)return;
        styles.insert(resolved);unsigned char* bytes=NULL;int size=0;char finalURL[WEB_URL];if(fetch(context,resolved.c_str(),&bytes,&size,finalURL,sizeof(finalURL))&&size<=512*1024 && cssBytes+(unsigned)size<=1536*1024){text.assign((const char*)bytes,size);compatibleMediaQueries(text);legacyLayoutFallback(text);customProperties(text);cssBytes+=size;++cssCount;baseurl=finalURL;}else ++cssFailed;free(bytes);
    }
    virtual void set_clip(const litehtml::position& pos,bool,bool){if(!drawing)return;clips.push_back(drawing->clip);xui_rect_t next={pos.x,pos.y,pos.width,pos.height};xui_set_clip(drawing,xui_rect_inter(drawing->clip,next));}
    virtual void del_clip(){if(drawing&&!clips.empty()){xui_set_clip(drawing,clips.back());clips.pop_back();}}
};
void* browserStyledCreate(const char* html,const char* url,BrowserStyleFetch fetch,void* user){
    if(!html||strlen(html)>1024*1024)return NULL;
    BrowserStyleContainer* page=new BrowserStyleContainer(url,fetch,user);page->engineContext.load_master_stylesheet(browserMasterCSS);
    page->engineContext.load_master_stylesheet("script,noscript,template{display:none;}body{font-family:Inter;font-size:18px;color:#111;background:#fff;}a{color:#005a33;cursor:pointer;}");
    std::string compatibleHTML(html);BrowserStyleContainer::preprocessStyleElements(compatibleHTML);
    page->document=litehtml::document::createFromString(compatibleHTML.c_str(),page,&page->engineContext);if(!page->document){delete page;return NULL;}page->document->render(1280);page->ready=true;return page;
}
void browserStyledDestroy(void* handle){delete (BrowserStyleContainer*)handle;}
int browserStyledHeight(void* handle){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;return p&&p->document?p->document->height():0;}
int browserStyledCSSCount(void* handle){return handle?((BrowserStyleContainer*)handle)->cssCount:0;}
int browserStyledCSSFailed(void* handle){return handle?((BrowserStyleContainer*)handle)->cssFailed:0;}
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
        image.scaled.clear();image.scaledW=image.scaledH=0;
        // Avoid full document re-layout for each decoded image. The first and
        // every third image reflow; otherwise known-dimension images still repaint.
        ++p->imageUpdates;
        bool allDone=true;for(size_t k=0;k<p->images.size();++k)if(p->images[k].state==0||p->images[k].state==2){allDone=false;break;}
        if(p->document&&(p->imageUpdates==1||p->imageUpdates%3==0||allDone))p->document->render(1280);
        p->dirty=true;return;
    }
}
int browserStyledImageCount(void* handle){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;int n=0;if(p)for(size_t i=0;i<p->images.size();++i)if(p->images[i].pixels)++n;return n;}
void browserStyledDraw(void* handle,xui_canvas_t* target,int x,int y,int width,int height,int scroll){
    BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p||width!=1280||height<1||height>720)return;
    if(p->dirty||p->lastScroll!=scroll||p->lastHeight!=height){p->cache.assign(width*height,0xffffff);xui_canvas_t canvas;xui_canvas_init(&canvas,&p->cache[0],width,height,width);p->drawing=&canvas;p->clips.clear();litehtml::position clip;clip.x=0;clip.y=0;clip.width=width;clip.height=height;p->document->draw((litehtml::uint_ptr)0,0,-scroll,&clip);p->drawing=NULL;p->dirty=false;p->lastScroll=scroll;p->lastHeight=height;}
    xui_image_t image={&p->cache[0],width,height,width,XUI_FMT_XRGB};xui_blit(target,&image,x,y,0);
}
bool browserStyledHover(void* handle,int x,int y){
    BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p)return false;
    if(x==p->lastHoverX&&y==p->lastHoverY)return p->hand;
    p->lastHoverX=x;p->lastHoverY=y;
    litehtml::position::vector boxes;if(p->document->on_mouse_over(x,y,boxes))p->dirty=true;
    return p->hand;
}
bool browserStyledClick(void* handle,int x,int y,char* url,int capacity){BrowserStyleContainer* p=(BrowserStyleContainer*)handle;if(!p)return false;litehtml::position::vector boxes;p->clicked.clear();p->lastHoverX=-1;p->lastHoverY=-1;p->document->on_mouse_over(x,y,boxes);p->document->on_lbutton_down(x,y,boxes);p->document->on_lbutton_up(x,y,boxes);p->dirty=true;return !p->clicked.empty()&&webCopy(url,capacity,p->clicked.c_str());}
