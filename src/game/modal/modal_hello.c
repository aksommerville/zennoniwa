#include "game/zennoniwa.h"

#define OPTION_LIMIT 16

#define OPTION_ID_PLAY 1
#define OPTION_ID_QUIT 2
#define OPTION_ID_QUANTIZE_HERO 3 /* XXX TEMP */

struct modal_hello {
  struct modal hdr;
  int titleid,titlew,titleh;
  double clock;
  struct option { // Things to choose, and also the static text labels.
    const char *src;
    int srcc;
    int x,y; // Center of first glyph; what you give to render_text().
    uint32_t rgba;
    int enabled; // zero or OPTION_ID_*
  } optionv[OPTION_LIMIT];
  int optionc;
  int optionp;
};

#define MODAL ((struct modal_hello*)modal)

/* Cleanup.
 */
 
static void _hello_del(struct modal *modal) {
  egg_texture_del(MODAL->titleid);
}

/* Options.
 */
 
static int hello_add_option(struct modal *modal,const char *src,int srcc,uint32_t rgba,int enabled) {
  if (MODAL->optionc>=OPTION_LIMIT) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct option *option=MODAL->optionv+MODAL->optionc++;
  option->src=src;
  option->srcc=srcc;
  option->rgba=rgba;
  option->enabled=enabled;
  return 0;
};

static void hello_layout_options(struct modal *modal) {
  if (MODAL->optionc<1) return;
  int enabledy=(FBH*3)/4;
  int disabledy=FBH-24;
  struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    if (option->enabled) {
      option->x=200;
      option->y=enabledy;
      enabledy+=24;
    } else {
      option->x=FBW-option->srcc*16;
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
  hello_add_option(modal,"Play",4,0xffffffff,OPTION_ID_PLAY);
  hello_add_option(modal,"Quit",4,0xffffffff,OPTION_ID_QUIT);
  hello_add_option(modal,g.quantize_hero?"Quantized":"Continuous",-1,0xffffffff,OPTION_ID_QUANTIZE_HERO);
  hello_add_option(modal,"and Alex Hansen",-1,0x402808ff,0);
  hello_add_option(modal,"By AK Sommerville",-1,0x402808ff,0);
  hello_add_option(modal,"GDEX Game Jam",-1,0x402808ff,0);
  hello_add_option(modal,"August 2025",-1,0x402808ff,0);
  hello_layout_options(modal);
  egg_play_song(RID_song_corruption_stands_alone,0,1);
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
    if (MODAL->optionv[MODAL->optionp].enabled) break;
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
    case OPTION_ID_QUANTIZE_HERO: {
        g.quantize_hero=g.quantize_hero?0:1;
        if (g.quantize_hero) {
          option->src="Quantized";
        } else {
          option->src="Continuous";
        }
        option->srcc=0;
        while (option->src[option->srcc]) option->srcc++;
      } break;
  }
}

/* Input.
 */
 
static void _hello_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) hello_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) hello_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) hello_activate(modal);
}

/* Update.
 */
 
static void _hello_update(struct modal *modal,double elapsed) {
  MODAL->clock+=elapsed;
}

/* Render.
 */
 
static void _hello_render(struct modal *modal) {
  fill_rect(0,0,FBW,FBH,0x805010ff);
  
  int alpha=0xff;
  if (MODAL->clock<3.0) {
    alpha=(int)((MODAL->clock*255.0)/3.0);
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
  }
  if (alpha) blit_texture((FBW>>1)-(MODAL->titlew>>1),10,MODAL->titleid,MODAL->titlew,MODAL->titleh,alpha);
  
  struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    render_text(option->x,option->y,option->src,option->srcc,option->rgba);
  }
  
  if ((MODAL->optionp>=0)&&(MODAL->optionp<MODAL->optionc)) {
    option=MODAL->optionv+MODAL->optionp;
    if (option->enabled) {
      struct tilerenderer tr={0};
      tilerenderer_add(&tr,option->x-32,option->y,0x12,EGG_XFORM_XREV);
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
