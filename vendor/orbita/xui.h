/* Derived from orbita legacy 0.21.44, GPL-3.0. Comments removed; original symbols retained. */

#ifndef XCLOUD360_XUI_H
#define XCLOUD360_XUI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XUI_RGB(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define XUI_R(p) (((p) >> 16) & 0xff)
#define XUI_G(p) (((p) >> 8) & 0xff)
#define XUI_B(p) ((p) & 0xff)


#define XUI_C_BG      XUI_RGB(27, 28, 30)      
#define XUI_C_PANEL   XUI_RGB(45, 48, 53)      
#define XUI_C_BUTTON  XUI_RGB(59, 62, 68)
#define XUI_C_PH      XUI_RGB(52, 55, 61)      
#define XUI_C_TEXT    XUI_RGB(255, 255, 255)
#define XUI_C_DIM     XUI_RGB(182, 185, 191)
#define XUI_C_FAINT   XUI_RGB(120, 124, 132)
#define XUI_C_GREEN   XUI_RGB(16, 124, 16)     

#define XUI_C_ACCENT  XUI_RGB(56, 182, 255)    
#define XUI_C_FLARE   XUI_RGB(255, 107, 61)    
#define XUI_C_HAZE    XUI_RGB(154, 160, 166)   
#define XUI_C_RED     XUI_RGB(232, 72, 72)
#define XUI_C_BLUE    XUI_RGB(52, 120, 220)
#define XUI_C_YELLOW  XUI_RGB(240, 190, 30)

#define XUI_MAX_W 2048           

typedef struct { int x, y, w, h; } xui_rect_t;

typedef struct {
    uint32_t  *px;
    int        w, h, stride;   
    xui_rect_t clip;           
} xui_canvas_t;


enum { XUI_FMT_XRGB = 0, XUI_FMT_UIRGB = 1 };
typedef struct {
    const uint32_t *px;
    int             w, h, stride;
    int             fmt;
} xui_image_t;


typedef struct {
    const uint32_t      *px;
    const unsigned char *a;
    int                  w, h;
} xui_sprite_t;


typedef struct {
    const unsigned char *a;
    int                  w, h;
} xui_mask_t;


typedef struct {
    int  off;
    unsigned char w, h;
    signed char bx, by;        
    unsigned short adv64;      
} xui_glyph_t;
typedef struct { unsigned short pair; short k64; } xui_kern_t;
typedef struct {
    const unsigned char *bits;
    const xui_glyph_t   *glyphs;
    const short         *map;          
    const xui_kern_t    *kern;         
    int                  nkern;
    int                  ascent, descent, px;
} xui_face_t;

enum { XUI_F_BODY = 0, XUI_F_LABEL, XUI_F_ROW, XUI_F_TITLE, XUI_F_CODE, XUI_F_H2, XUI_F_COUNT };


void xui_canvas_init(xui_canvas_t *c, uint32_t *px, int w, int h, int stride);
void xui_set_clip(xui_canvas_t *c, xui_rect_t r);   
void xui_clip_all(xui_canvas_t *c);
xui_rect_t xui_rect_union(xui_rect_t a, xui_rect_t b);   
xui_rect_t xui_rect_inter(xui_rect_t a, xui_rect_t b);
static inline int xui_rect_empty(xui_rect_t r) { return r.w <= 0 || r.h <= 0; }

static inline uint32_t xui_mix(uint32_t d, uint32_t s, unsigned a ) {
    a += a >> 7;                        
    unsigned na = 256 - a;
    uint32_t rb = (((s & 0xff00ff) * a + (d & 0xff00ff) * na) >> 8) & 0xff00ff;
    uint32_t g  = (((s & 0x00ff00) * a + (d & 0x00ff00) * na) >> 8) & 0x00ff00;
    return rb | g;
}

void xui_fill(xui_canvas_t *c, int x, int y, int w, int h, uint32_t col);

void xui_fill_a(xui_canvas_t *c, int x, int y, int w, int h, uint32_t col, unsigned alpha);

void xui_round_rect(xui_canvas_t *c, int x, int y, int w, int h, int r, uint32_t col, unsigned alpha);

void xui_round_frame(xui_canvas_t *c, int x, int y, int w, int h, int r, int t, uint32_t col);
void xui_circle(xui_canvas_t *c, int cx, int cy, int rad, uint32_t col, unsigned alpha);

void xui_vgrad(xui_canvas_t *c, int x, int y, int w, int h, uint32_t col, unsigned a0, unsigned a1);
void xui_hgrad(xui_canvas_t *c, int x, int y, int w, int h, uint32_t col, unsigned a0, unsigned a1);

void xui_blit(xui_canvas_t *c, const xui_image_t *img, int x, int y, int r);

void xui_sprite(xui_canvas_t *c, const xui_sprite_t *s, int x, int y);

void xui_mask(xui_canvas_t *c, const xui_mask_t *m, int x, int y, uint32_t col, unsigned alpha);

void xui_scale_cover(uint32_t *dst, int dw, int dh, const xui_image_t *src);

void xui_scale_cover_part(uint32_t *dst, int dw, int dh, const xui_image_t *src, int rows);

void xui_copy_from(xui_canvas_t *c, const uint32_t *src, int src_stride, xui_rect_t r);


const xui_face_t *xui_face(int f);

int  xui_text_width(int f, const char *utf8, int letter);

int  xui_text(xui_canvas_t *c, int f, int x, int y, uint32_t col, const char *utf8, int letter, int max_w);

void xui_text_center(xui_canvas_t *c, int f, int x, int w, int y, uint32_t col, const char *utf8, int letter);

int  xui_text_wrap(xui_canvas_t *c, int f, int x, int y, int w, int line_h, int max_lines,
                   int align, uint32_t col, const char *utf8);
int  xui_line_h(int f);

unsigned xui_utf8_next(const char **s);

int  xui_utf8_to_latin1(char *dst, int cap, const char *utf8);


void xui_tile_copy(uint32_t *fb, int fb_stride, int fb_w, int fb_h,
                   const xui_canvas_t *c, xui_rect_t r);

uint32_t xui_tile_offset(int x, int y, int fb_stride);


enum {
    XUI_IT_GAMEPASS = 1 << 0,   
    XUI_IT_SESSION  = 1 << 1,   
};
typedef struct {
    const char *id;             
    const char *name;           
    unsigned    flags;          
    int         game;           
} xui_item_t;
typedef struct {
    const char       *title;    
    const xui_item_t *items;
    int               n;
} xui_row_t;



enum { XUI_HB_SETTINGS = 0, XUI_HB_UPDATE, XUI_HB_REBOOT, XUI_HB_SHUTDOWN, XUI_HB_COUNT };

typedef struct {
    const xui_row_t *rows;
    int              nrows;
    unsigned         rows_gen;   
    
    int              row, col;
    
    const xui_image_t *(*cover)(void *u, const char *id);
    const xui_image_t *(*hero)(void *u, const char *id);
    void            *u;
    unsigned         img_gen;    
    
    const char      *session_id;
    const char      *session_name;
    
    const char      *hint;
    const char      *notice;     
    const char      *version;    
    
    int              update_avail;
    const char      *update_label;
    
    int              settings_avail;
    
    int              net_kind;
} xui_home_data_t;


enum {
    XUI_ST_LOADING = 0,   
    XUI_ST_LOGIN,         
    XUI_ST_STARTING,      
    XUI_ST_QUEUE,         
    XUI_ST_ERROR,
    XUI_ST_BUSY,          
};
typedef struct {
    int         kind;
    const char *title;       
    const char *text;        
    const char *url;         
    const char *code;        
    const char *detail;      
    const char *hint;        
    const char *game_id;     
    int         progress;    
} xui_status_t;


typedef struct {
    const char *title;       
    const char *text;
    const char *yes;         
    const char *no;          
    int         focus;       
} xui_confirm_t;


#define XUI_KEY_COLS 8
#define XUI_KEY_ROWS 4
typedef struct {
    const char *title;        
    const char *text;         
    const char *alphabet;     
    const char *typed;        
    int         len;          
    int         row, col;     
    const char *console_id;   
    const char *error;        
    const char *status;       
    int         busy;
    const char *hint;
} xui_key_t;


enum {
    XUI_CR_ARROW   = 1 << 0,   
    XUI_CR_DIM     = 1 << 1,   
    XUI_CR_WARN    = 1 << 2,   
    XUI_CR_RADIO   = 1 << 3,   
    XUI_CR_ON      = 1 << 4,   
    XUI_CR_CHOICES = 1 << 5,   
    XUI_CR_CENTER  = 1 << 6,   
};
typedef struct {
    const char *head;       
    const char *label;
    const char *detail;     
    const char *value;      
    const char *tag;        
    const char *after;      
    unsigned    flags;      
    const char *const *choices;
    int         nchoices;
    int         choice;     
} xui_cfg_row_t;


#define XUI_CAL_TRAIL 8
typedef struct {
    int         stick;               
    int         dz_pct, out_pct;     
    int         x, y;                
    int         tx[XUI_CAL_TRAIL], ty[XUI_CAL_TRAIL], ntrail;   
    int         pos_pct, game_pct;   
    int         focus;               
    const char *step;                
    const char *next;                
    const char *status;              
    int         measuring;
    const char *pad;                 
} xui_calib_t;

enum { XUI_CFG_ROWS = 0, XUI_CFG_NOTES, XUI_CFG_CALIB };
#define XUI_CFG_NSEC 6   
typedef struct {
    const char *sections[XUI_CFG_NSEC];
    int         item;          
    int         right;         
    int         page;          
    const char *crumb;         
    const char *title, *subtitle;
    struct { const char *label, *big, *small; int big_title; } info[2];   
    int         ninfo;
    int         card;          
    int         card_ok, card_bars;
    const char *card_title, *card_text, *card_text2;
    const xui_cfg_row_t *rows;
    int         nrows;
    int         focus;         
    const char *text;          
    const char *note;          
    const char *notes;         
    const char *notes_empty;   
    int         scroll;        
    const char *footer;        
    const xui_calib_t   *calib;
    const xui_confirm_t *confirm;
    const char *notice;        
    const char *hint;
} xui_cfg_t;


typedef struct {
    const char *ssid;          
    int         bars;          
    int         lock;          
    const char *tag;           
    int         dim;           
} xui_wifi_net_t;

enum { XUI_WS_NEUTRAL = 0, XUI_WS_OK, XUI_WS_BUSY, XUI_WS_ERROR };

typedef struct {
    const char *status_title;  
    const char *status_text;   
    int         status_kind;   
    const char *saved;         
    const xui_wifi_net_t *nets;
    int         nnets;
    
    int         focus;
    int         can_forget;    
    int         scanning;      
    const char *empty;         
    const char *hint;
} xui_wifi_t;


#define XUI_TK_COLS 10
#define XUI_TK_ROWS 4
typedef struct {
    const char *title;         
    const char *text;          
    const char *rows[XUI_TK_ROWS];   
    const char *shift_label;   
    const char *sym_label;     
    const char *typed;         
    int         len, minlen, maxlen;
    int         show;          
    int         row, col;      
    const char *error;
    const char *status;        
    int         busy;
    const char *hint;
} xui_textkey_t;


typedef struct xui_view xui_view_t;


xui_view_t *xui_view_new(int w, int h);
void        xui_view_free(xui_view_t *v);

void        xui_view_invalidate(xui_view_t *v);

void        xui_view_repaint(xui_view_t *v);


xui_rect_t  xui_home_frame(xui_view_t *v, xui_canvas_t *c, const xui_home_data_t *d,
                           const xui_confirm_t *confirm, unsigned now_ms, int *animating);

xui_rect_t  xui_status_frame(xui_view_t *v, xui_canvas_t *c, const xui_status_t *s,
                             const xui_home_data_t *d, unsigned now_ms, int *animating);


xui_rect_t  xui_key_frame(xui_view_t *v, xui_canvas_t *c, const xui_key_t *k,
                          const xui_home_data_t *d, unsigned now_ms, int *animating);

xui_rect_t  xui_wifi_frame(xui_view_t *v, xui_canvas_t *c, const xui_wifi_t *w,
                           const xui_home_data_t *d, unsigned now_ms, int *animating);
xui_rect_t  xui_textkey_frame(xui_view_t *v, xui_canvas_t *c, const xui_textkey_t *k,
                              const xui_home_data_t *d, unsigned now_ms, int *animating);

xui_rect_t  xui_cfg_frame(xui_view_t *v, xui_canvas_t *c, const xui_cfg_t *s,
                          const xui_home_data_t *d, unsigned now_ms, int *animating);
xui_rect_t  xui_cfg_calib_rect(void);


typedef struct {
    unsigned frames, full, partial;
    unsigned hero_builds, card_scales;
} xui_stats_t;
void xui_view_stats(const xui_view_t *v, xui_stats_t *out);


int  xui_home_visible(const xui_view_t *v, const xui_home_data_t *d, const xui_item_t **out, int max);

#ifdef __cplusplus
}
#endif

#endif
