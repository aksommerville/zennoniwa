/* modal_gameover.c
 * Appears after completing each level.
 * The session remains live but suspended beneath us.
 */
 
#include "game/zennoniwa.h"
#include <stdarg.h>

#define LABEL_TEXT_LIMIT 30
#define LABEL_LIMIT 8

struct modal_gameover {
  struct modal hdr;
  int new_high_score;
  struct label {
    char text[LABEL_TEXT_LIMIT];
    int textc;
    uint32_t rgba;
    int x,y;
  } labelv[LABEL_LIMIT];
  int labelc;
};

#define MODAL ((struct modal_gameover*)modal)

/* Cleanup.
 */
 
static void _gameover_del(struct modal *modal) {
}

/* Add a label.
 */
 
static void gameover_add_label(struct modal *modal,uint32_t rgba,const char *fmt,...) {
  if (MODAL->labelc>=LABEL_LIMIT) {
    fprintf(stderr,"%s:%d: too many labels\n",__FILE__,__LINE__);
    return;
  }
  struct label *label=MODAL->labelv+MODAL->labelc++;
  label->textc=0;
  label->rgba=rgba;
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
  label->x=(FBW>>1)-(label->textc*4)+8;
}

/* Arrange labels vertically. We take care of the horizontal as we make them.
 */
 
static void gameover_arrange_labels(struct modal *modal) {
  if (MODAL->labelc<1) return;
  int totalh=MODAL->labelc*16;
  int y=(FBH>>1)-(totalh>>1)+8;
  struct label *label=MODAL->labelv;
  int i=MODAL->labelc;
  for (;i-->0;label++,y+=16) {
    label->y=y;
  }
}

/* Init.
 */
 
static int _gameover_init(struct modal *modal) {
  if (!g.session) return -1;
  
  play_song(RID_song_only_soil_deep,1);
  
  MODAL->new_high_score=0;
  int levelc=g.session->rid-1;
  if (levelc<1) levelc=1;
  g.session->tscore.life/=(double)levelc;
  score_calculate(&g.session->tscore);
  fprintf(stderr,"Final score: %.06d\n",g.session->tscore.score);
  if (g.session->tscore.score>g.hiscore.score) {
    memcpy(&g.hiscore,&g.session->tscore,sizeof(struct score));
    hiscore_save();
    MODAL->new_high_score=1;
  }
  
  gameover_add_label(modal,0xff0000ff,"Game over");
  gameover_add_label(modal,0xffffffff,"");
  gameover_add_label(modal,0xffffffff,"Time: %t",g.session->tscore.time);
  gameover_add_label(modal,0xffffffff,"Pink: %d",g.session->tscore.pinkc);
  gameover_add_label(modal,0xffffffff,"Score: %d",g.session->tscore.score);
  gameover_add_label(modal,0xffffffff,"");
  if (MODAL->new_high_score) {
    gameover_add_label(modal,0xffff00ff,"New high score!");
  } else {
    gameover_add_label(modal,0xc0c0c0ff,"High score: %d",g.hiscore.score);
  }
  gameover_arrange_labels(modal);
  
  return 0;
}

/* Input.
 */
 
static void _gameover_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    modal->defunct=1;
  }
}

/* Update.
 */
 
static void _gameover_update(struct modal *modal,double elapsed) {
}

/* Render.
 */
 
static void _gameover_render(struct modal *modal) {
  fill_rect(0,0,FBW,FBH,0x000000ff);
  struct label *label=MODAL->labelv;
  int i=MODAL->labelc;
  for (;i-->0;label++) {
    render_text(label->x,label->y,label->text,label->textc,label->rgba);
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_gameover={
  .name="gameover",
  .objlen=sizeof(struct modal_gameover),
  .del=_gameover_del,
  .init=_gameover_init,
  .input=_gameover_input,
  .update=_gameover_update,
  .render=_gameover_render,
};
