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
    .alpha=0xff,
  };
  egg_render(&un,tr->vtxv,sizeof(struct egg_render_tile)*tr->vtxc);
  tr->vtxc=0;
}
