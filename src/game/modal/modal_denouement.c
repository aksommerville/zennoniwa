/* modal_denouement.c
 * Appears after completing each level.
 * The session remains live but suspended beneath us.
 */
 
#include "game/zennoniwa.h"
#include <stdarg.h>

#define LABEL_TEXT_LIMIT 20
#define LABEL_LIMIT 3

struct modal_denouement {
  struct modal hdr;
  struct egg_render_tile tnvtxv[18*10]; // Thumbnail. 16*8 with a border.
  struct label {
    char text[LABEL_TEXT_LIMIT];
    int textc;
    int x,y;
    uint32_t rgba;
  } labelv[LABEL_LIMIT];
  int labelc;
};

#define MODAL ((struct modal_denouement*)modal)

/* Cleanup.
 */
 
static void _denouement_del(struct modal *modal) {
}

/* Populate tnvtxv.
 */
 
static void denouement_populate_thumbnail(struct modal *modal) {
  if (!g.session||(g.session->mapw!=16)||(g.session->maph!=8)) return;
  const int tilesize=16;
  #define TILE(_x,_y,tid) MODAL->tnvtxv[(_y)*18+(_x)].tileid=tid;
  
  // First fill in the positions (LRTB), and default everything to blank paper.
  int x0=(FBW>>1)-(tilesize*9),y0=FBH-tilesize*10-30;
  struct egg_render_tile *vtx=MODAL->tnvtxv;
  int y,x,i;
  for (y=0;y<10;y++) {
    for (x=0;x<18;x++,vtx++) {
      vtx->x=x0+x*tilesize+(tilesize>>1);
      vtx->y=y0+y*tilesize+(tilesize>>1);
      vtx->tileid=0x61;
      vtx->xform=0;
    }
  }
  
  // Border. The edges have two options which must alternate. No transforms.
  TILE(0,0,0x50)
  TILE(17,0,0x53)
  TILE(0,9,0x80)
  TILE(17,9,0x83)
  for (x=1;x<=16;x++) {
    TILE(x,0,0x51+(x&1))
    TILE(x,9,0x81+(x&1))
  }
  for (y=1;y<=8;y++) {
    TILE(0,y,0x60+((y&1)<<4))
    TILE(17,y,0x63+((y&1)<<4))
  }
  
  // Now populate the inner section based on the map.
  struct egg_render_tile *dstrow=MODAL->tnvtxv+19;
  const struct cell *src=g.session->cellv;
  for (y=0;y<8;y++,dstrow+=18) {
    for (x=0,vtx=dstrow;x<16;x++,vtx++,src++) {
      if (src->life<=0.0) continue; // Empty.
      int frame=src->life*5.0; // Same way session_render does it...
      if (frame>=4) vtx->tileid=0x72; // pink
      else if (frame>=2) vtx->tileid=0x71; // ok
      else vtx->tileid=0x62; // tolerable
    }
  }
  
  #undef TILE
}

/* Add a label.
 */
 
static void denouement_add_label(struct modal *modal,const char *fmt,...) {
  if (MODAL->labelc>=LABEL_LIMIT) {
    fprintf(stderr,"%s:%d: too many labels\n",__FILE__,__LINE__);
    return;
  }
  struct label *label=MODAL->labelv+MODAL->labelc++;
  label->textc=0;
  label->rgba=0xffffffff;
  va_list vargs;
  va_start(vargs,fmt);
  while (*fmt) {
    if (*fmt=='%') {
      fmt++;
      switch (*fmt++) {
        case 'd': {
            int v=va_arg(vargs,int);
            if (v<0) v=0;
            int digitc=1,limit=10;
            while (limit<=v) { digitc++; if (limit>INT_MAX/10) break; limit*=10; }
            if (label->textc<=sizeof(label->text)-digitc) {
              int i=digitc;
              for (;i-->0;v/=10) label->text[label->textc+i]='0'+v%10;
              label->textc+=digitc;
            }
          } break;
        case 't': {
            double v=va_arg(vargs,double);
            if (label->textc<=sizeof(label->text)-9) { // fixed size, always 9
              int ms=(int)(v*1000.0);
              if (ms<0) ms=0;
              int sec=ms/1000; ms%=1000;
              int min=sec/60; sec%=60;
              if (min>99) { min=99; sec=99; ms=999; }
              label->text[label->textc++]='0'+min/10;
              label->text[label->textc++]='0'+min%10;
              label->text[label->textc++]=':';
              label->text[label->textc++]='0'+sec/10;
              label->text[label->textc++]='0'+sec%10;
              label->text[label->textc++]='.';
              label->text[label->textc++]='0'+ms/100;
              label->text[label->textc++]='0'+(ms/10)%10;
              label->text[label->textc++]='0'+ms%10;
            }
          } break;
      }
    } else {
      if (label->textc<sizeof(label->text)) label->text[label->textc++]=*fmt;
      fmt++;
    }
  }
  label->y=40+MODAL->labelc*24;
  label->x=(FBW>>1)-(label->textc*4)+8;
}

/* Count the sand tiles, ie the upper limit for pinkc.
 */
 
static int count_sands() {
  int i=NS_sys_mapw*NS_sys_maph,sandc=0;
  const struct cell *cell=g.session->cellv;
  for (;i-->0;cell++) if (tileid_is_sand(cell->tileid)) sandc++;
  return sandc;
}

/* Init.
 */
 
static int _denouement_init(struct modal *modal) {
  denouement_populate_thumbnail(modal);
  denouement_add_label(modal,"Level %d/12 complete",g.session->rid);
  denouement_add_label(modal,"Time: %t",g.session->lscore.time);
  denouement_add_label(modal,"Pink: %d/%d",g.session->lscore.pinkc,count_sands());
  return 0;
}

/* Input.
 */
 
static void _denouement_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    modal->defunct=1;
    if (g.session) session_load_next(g.session);
  }
}

/* Update.
 */
 
static void _denouement_update(struct modal *modal,double elapsed) {
}

/* Render.
 */
 
static void _denouement_render(struct modal *modal) {
  fill_rect(0,0,FBW,FBH,0x286b46ff);
  
  // Thumbnail.
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=1,
    .srctexid=g.texid_halftiles,
    .alpha=0xff,
  };
  egg_render(&un,MODAL->tnvtxv,sizeof(MODAL->tnvtxv));
  
  // Labels.
  struct label *label=MODAL->labelv;
  int i=MODAL->labelc;
  for (;i-->0;label++) {
    render_text(label->x,label->y,label->text,label->textc,label->rgba);
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_denouement={
  .name="denouement",
  .objlen=sizeof(struct modal_denouement),
  .del=_denouement_del,
  .init=_denouement_init,
  .input=_denouement_input,
  .update=_denouement_update,
  .render=_denouement_render,
};
