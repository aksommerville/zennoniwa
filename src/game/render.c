#include "zennoniwa.h"

/* Fill rect.
 */
 
void fill_rect(int x,int y,int w,int h,uint32_t rgba) {
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TRIANGLE_STRIP,
    .dsttexid=1,
    .alpha=0xff,
  };
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
  struct egg_render_raw vtxv[]={
    {x,y,0,0,r,g,b,a},
    {x,y+h,0,0,r,g,b,a},
    {x+w,y,0,0,r,g,b,a},
    {x+w,y+h,0,0,r,g,b,a},
  };
  egg_render(&un,vtxv,sizeof(vtxv));
}

/* Frame rect.
 */
 
void frame_rect(int x,int y,int w,int h,uint32_t rgba) {
  if ((w<2)||(h<2)) return;
  fill_rect(x,y,1,h-1,rgba);
  fill_rect(x,y+h-1,w-1,1,rgba);
  fill_rect(x+w-1,y+1,1,h-1,rgba);
  fill_rect(x+1,y,w-1,1,rgba);
}

/* Simple decal.
 */
 
void blit_texture(int dstx,int dsty,int texid,int w,int h,uint8_t alpha) {
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TRIANGLE_STRIP,
    .dsttexid=1,
    .srctexid=texid,
    .alpha=alpha,
  };
  struct egg_render_raw vtxv[]={
    {dstx,dsty,0,0},
    {dstx,dsty+h,0,h},
    {dstx+w,dsty,w,0},
    {dstx+w,dsty+h,w,h},
  };
  egg_render(&un,vtxv,sizeof(vtxv));
}

/* Grid.
 */
 
void render_grid(int x,int y,const uint8_t *src,int colc,int rowc) {
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=1,
    .srctexid=g.texid_tilesheet,
    .alpha=0xff,
  };
  #define VTXA 128
  struct egg_render_tile vtxv[VTXA];
  int vtxc=0;
  x+=NS_sys_tilesize>>1;
  y+=NS_sys_tilesize>>1;
  int x0=x;
  struct egg_render_tile *vtx=vtxv;
  int row=0;
  for (;row<rowc;row++,y+=NS_sys_tilesize) {
    int x=x0,col=0;
    for (;col<colc;col++,src++,x+=NS_sys_tilesize) {
      if (vtxc>=VTXA) {
        egg_render(&un,vtxv,vtxc*sizeof(vtxv[0]));
        vtx=vtxv;
        vtxc=0;
      }
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=*src;
      vtx->xform=0;
      vtx++;
      vtxc++;
    }
  }
  if (vtxc) egg_render(&un,vtxv,vtxc*sizeof(vtxv[0]));
  #undef VTXA
}

/* Text with our tile font.
 */
 
void render_text(int x,int y,const char *src,int srcc,uint32_t rgba) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct tilerenderer tr={
    .tint=rgba|0xff,
    .alpha=rgba&0xff,
    .texid=g.texid_font,
  };
  for (;srcc-->0;src++,x+=8) {
    tilerenderer_add(&tr,x,y,*src,0);
  }
  tilerenderer_flush(&tr);
}

/* Text formatters.
 */
 
void render_time(int x,int y,double s,uint32_t rgba) {
  int ms=(int)(s*1000.0);
  if (ms<0) ms=0;
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  if (min>99) { min=99; sec=99; ms=999; }
  char tmp[9]={
    '0'+min/10,
    '0'+min%10,
    ':',
    '0'+sec/10,
    '0'+sec%10,
    '.',
    '0'+ms/100,
    '0'+(ms/10)%10,
    '0'+ms%10,
  };
  render_text(x,y,tmp,sizeof(tmp),rgba);
}

void render_uint(int x,int y,int v,int limit,uint32_t rgba) {
  if (v<0) v=0; else if (limit<0) ; else if (v>limit) v=limit;
  int tmpc=1,ceiling=10;
  while (v>=ceiling) { tmpc++; if (ceiling>INT_MAX/10) break; ceiling*=10; }
  char tmp[16];
  if (tmpc>sizeof(tmp)) return; // not possible
  int i=tmpc;
  for (;i-->0;v/=10) tmp[i]='0'+v%10;
  render_text(x,y,tmp,tmpc,rgba);
}

/* Structure tile renderer.
 */
 
void tilerenderer_add(struct tilerenderer *tr,int x,int y,uint8_t tileid,uint8_t xform) {
  if (tr->vtxc>=64) tilerenderer_flush(tr);
  struct egg_render_tile *vtx=tr->vtxv+tr->vtxc++;
  vtx->x=x;
  vtx->y=y;
  vtx->tileid=tileid;
  vtx->xform=xform;
}

void tilerenderer_flush(struct tilerenderer *tr) {
  if (tr->vtxc<1) return;
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=1,
    .srctexid=g.texid_tilesheet,
    .tint=tr->tint,
    .alpha=0xff,
  };
  if (tr->alpha) un.alpha=tr->alpha; // Never zero; default to 0xff in that case.
  if (tr->texid) un.srctexid=tr->texid;
  egg_render(&un,tr->vtxv,sizeof(struct egg_render_tile)*tr->vtxc);
  tr->vtxc=0;
}

/* Structured fancy-tile renderer.
 */
 
void fancyrenderer_add(struct fancyrenderer *fr,
  int x,int y,
  uint8_t tileid,
  uint8_t xform,
  uint8_t rotation,
  uint8_t size,
  uint32_t tint_rgba,
  uint32_t prim_rgba
) {
  if (fr->vtxc>=64) fancyrenderer_flush(fr);
  struct egg_render_fancy *vtx=fr->vtxv+fr->vtxc++;
  vtx->x=x;
  vtx->y=y;
  vtx->tileid=tileid;
  vtx->xform=xform;
  vtx->rotation=rotation;
  vtx->size=size;
  vtx->tr=tint_rgba>>24;
  vtx->tg=tint_rgba>>16;
  vtx->tb=tint_rgba>>8;
  vtx->ta=tint_rgba;
  vtx->pr=prim_rgba>>24;
  vtx->pg=prim_rgba>>16;
  vtx->pb=prim_rgba>>8;
  vtx->a=prim_rgba;
}

void fancyrenderer_flush(struct fancyrenderer *fr) {
  if (fr->vtxc<1) return;
  struct egg_render_uniform un={
    .mode=EGG_RENDER_FANCY,
    .dsttexid=1,
    .srctexid=g.texid_tilesheet,
    .alpha=0xff,
  };
  egg_render(&un,fr->vtxv,sizeof(struct egg_render_fancy)*fr->vtxc);
  fr->vtxc=0;
}
