#include "game/zennoniwa.h"

#define OPTION_LIMIT 16

/* Option ID zero for noninteractive labels, which will stack in the lower right corner.
 * >0 for interactive options, centered.
 * <0 for noninteractive labels with some special position.
 */
#define OPTION_ID_HISCORE -1
#define OPTION_ID_PLAY 1
#define OPTION_ID_QUIT 2

struct modal_hello {
  struct modal hdr;
  int titleid,titlew,titleh;
  double clock;
  struct option { // Things to choose, and also the static text labels.
    const char *src; // Must be constant and long-lived.
    int srcc;
    int x,y; // Center of first glyph; what you give to render_text().
    uint32_t rgba;
    int enabled; // zero or OPTION_ID_*
  } optionv[OPTION_LIMIT];
  int optionc;
  int optionp;
  struct egg_render_tile bgtilev[20*11];
  double fishclock;
  int fishframe;
  char hstext[18];
};

#define MODAL ((struct modal_hello*)modal)

/* Cleanup.
 */
 
static void _hello_del(struct modal *modal) {
  egg_texture_del(MODAL->titleid);
}

/* Populate (bgtilev). It gets set once and never changes.
 */
 
static void hello_generate_background(struct modal *modal) {
  struct egg_render_tile *vtx=MODAL->bgtilev;
  int i,x,y;
  #define TILE(col,row,tid,xf) { \
    vtx=MODAL->bgtilev+((row)*20)+(col); \
    vtx->tileid=tid; \
    vtx->xform=xf; \
  }
  
  // First place them all LRTB and fill with empty grass.
  for (y=0;y<11;y++) {
    for (x=0;x<20;x++,vtx++) {
      vtx->x=x*NS_sys_tilesize+(NS_sys_tilesize>>1);
      vtx->y=y*NS_sys_tilesize+(NS_sys_tilesize>>1);
      vtx->tileid=0x33;
      vtx->xform=0;
    }
  }
  
  // Bamboo on the edges.
  for (y=0;y<11;y++) {
    TILE(0,y,0x40+(y<<4),0)
    TILE(19,y,0x40+(y<<4),EGG_XFORM_XREV)
  }
  
  // 3-row high pond, centered, with 1 row clearance from the bottom.
  int ponda=4;
  int pondz=15;
  TILE(ponda,7,0xc1,0)
  TILE(pondz,7,0xc6,0)
  TILE(ponda,8,0xd1,0)
  TILE(pondz,8,0xd6,0)
  TILE(ponda,9,0xe1,0)
  TILE(pondz,9,0xe6,0)
  for (x=ponda+1;x<=pondz-1;x++) {
    TILE(x,7,0xc2+(x&3),0)
    TILE(x,8,0xd2+(x&3),0)
    TILE(x,9,0xe2+(x&3),0)
  }
  
  // Rock and lily pad in the pond. Unlike the session background, we're three rows high so they fit nicely tilewise.
  TILE(5,8,0x43,0)
  TILE(6,8,0x44,0)
  TILE(12,8,0x45,0)
  TILE(13,8,0x46,0)
  
  // Two decorative statues whose job is to occupy space.
  TILE(3,4,0x62,0)
  TILE(16,4,0x62,0)
  
  #undef TILE
}

/* Options.
 */
 
static int hello_add_option(struct modal *modal,const char *src,int srcc,uint32_t rgba,int enabled) {
  if (MODAL->optionc>=OPTION_LIMIT) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct option *option=MODAL->optionv+MODAL->optionc++;
  option->src=src;
  option->srcc=srcc;
  option->rgba=rgba&0xffffff00;
  option->enabled=enabled;
  return 0;
};

static void hello_layout_options(struct modal *modal) {
  if (MODAL->optionc<1) return;
  int enabledy=(FBH*3)/4;
  int disabledy=FBH-15;
  struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    if (option->enabled==OPTION_ID_HISCORE) {
      option->x=30;
      option->y=FBH-15;
    } else if (option->enabled) {
      option->x=300;
      option->y=enabledy;
      enabledy+=16;
    } else {
      option->x=FBW-option->srcc*8-16;
      option->y=disabledy;
      disabledy-=16;
    }
  }
}

/* Init.
 */
 
static int _hello_init(struct modal *modal) {
  egg_texture_load_image(MODAL->titleid=egg_texture_new(),RID_image_title);
  egg_texture_get_size(&MODAL->titlew,&MODAL->titleh,MODAL->titleid);
  hello_generate_background(modal);
  hello_add_option(modal,"Play",4,0xffffffff,OPTION_ID_PLAY);
  hello_add_option(modal,"Quit",4,0xffffffff,OPTION_ID_QUIT);
  hello_add_option(modal,"and Alex Hansen",-1,0x000800ff,0);
  hello_add_option(modal,"By AK Sommerville",-1,0x000800ff,0);
  hello_add_option(modal,"GDEX Game Jam",-1,0x000800ff,0);
  hello_add_option(modal,"August 2025",-1,0x000800ff,0);
  
  if (g.hiscore.score>0) {
    memcpy(MODAL->hstext,"High score: 000000",18);
    int v=g.hiscore.score;
    if (v>999999) v=999999;
    int i=1; for (;i<=6;i++,v/=10) MODAL->hstext[18-i]+=v%10;
    hello_add_option(modal,MODAL->hstext,18,0x000800ff,OPTION_ID_HISCORE);
  }
  
  hello_layout_options(modal);
  play_song(RID_song_only_soil_deep,1);
  while ((MODAL->optionp<MODAL->optionc)&&!MODAL->optionv[MODAL->optionp].enabled) MODAL->optionp++;
  return 0;
}

/* Move cursor.
 */
 
static void hello_move(struct modal *modal,int d) {
  if (MODAL->optionc<1) return;
  int panic=MODAL->optionc+1;
  for (;;) {
    if (--panic<=0) {
      MODAL->optionp=-1;
      return;
    }
    MODAL->optionp+=d;
    if (MODAL->optionp<0) MODAL->optionp=MODAL->optionc-1;
    else if (MODAL->optionp>=MODAL->optionc) MODAL->optionp=0;
    if (MODAL->optionv[MODAL->optionp].enabled>0) break;
  }
  egg_play_sound(RID_sound_uimotion,1.0,0.0);
}

/* Activate.
 */
 
static void hello_activate(struct modal *modal) {
  if ((MODAL->optionp<0)||(MODAL->optionp>=MODAL->optionc)) return;
  struct option *option=MODAL->optionv+MODAL->optionp;
  switch (option->enabled) {
    case OPTION_ID_PLAY: {
        modal->defunct=1;
        session_del(g.session);
        if (!(g.session=session_new())) egg_terminate(1);
      } break;
    case OPTION_ID_QUIT: {
        egg_terminate(0);
      } break;
  }
}

/* Input.
 */
 
static void _hello_input(struct modal *modal,int input,int pvinput) {
  if (MODAL->clock<4.0) {
    if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
      MODAL->clock=4.0;
      egg_play_sound(RID_sound_uimotion,1.0,0.0);
    }
  } else {
    if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) hello_move(modal,-1);
    if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) hello_move(modal,1);
    if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) hello_activate(modal);
  }
}

/* Update.
 */
 
static void _hello_update(struct modal *modal,double elapsed) {
  MODAL->clock+=elapsed;
  
  if ((MODAL->fishclock-=elapsed)<=0.0) {
    MODAL->fishclock+=0.150;
    if (++(MODAL->fishframe)>=10) MODAL->fishframe=0;
  }
}

/* Render.
 */
 
static void _hello_render(struct modal *modal) {
  
  // Background. Static tiles filling the framebuffer exactly.
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=1,
    .srctexid=g.texid_tilesheet,
    .alpha=0xff,
  };
  egg_render(&un,MODAL->bgtilev,sizeof(MODAL->bgtilev));
  
  // Title a little up of center, fades in over 0..3 s.
  int title_alpha=0xff;
  if (MODAL->clock<3.0) {
    title_alpha=(int)((MODAL->clock*255.0)/3.0);
    if (title_alpha<0) title_alpha=0; else if (title_alpha>0xff) title_alpha=0xff;
  }
  if (title_alpha) blit_texture((FBW>>1)-(MODAL->titlew>>1),(FBH/3)-(MODAL->titleh>>1),MODAL->titleid,MODAL->titlew,MODAL->titleh,title_alpha);
  
  // Option labels, pre-placed. Fade in over 2..4 s.
  if (MODAL->clock>=2.0) {
    int alpha=(int)(((MODAL->clock-2.0)*255.0)/2.0);
    if (alpha<1) alpha=1; else if (alpha>0xff) alpha=0xff;
    struct option *option=MODAL->optionv;
    int i=MODAL->optionc;
    for (;i-->0;option++) {
      render_text(option->x,option->y,option->src,option->srcc,option->rgba|alpha);
    }
  }
  
  if ((MODAL->clock>=4.0)&&(MODAL->optionp>=0)&&(MODAL->optionp<MODAL->optionc)) {
    struct option *option=MODAL->optionv+MODAL->optionp;
    if (option->enabled) {
      struct tilerenderer tr={0};
      tilerenderer_add(&tr,option->x-24,option->y,0x53+MODAL->fishframe,0);
      tilerenderer_flush(&tr);
    }
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_hello={
  .name="hello",
  .objlen=sizeof(struct modal_hello),
  .del=_hello_del,
  .init=_hello_init,
  .input=_hello_input,
  .update=_hello_update,
  .render=_hello_render,
};
