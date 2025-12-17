#include "game/zennoniwa.h"

#define OPTION_LIMIT 4
#define OPTIONID_RESUME 1
#define OPTIONID_RESET  2
#define OPTIONID_MENU   3
#define OPTIONID_QUIT   4

// image:fonttiles are 16-pixel tiles but with horizontal dead space.
#define GLYPHW 8
#define GLYPHH 16

struct modal_pause {
  struct modal hdr;
  int fldx,fldy,fldw,fldh; // The blacked out area where everything happens.
  struct option {
    const char *text; // Always static.
    int textc;
    int optionid;
    int x,y; // Center of the first glyph, exactly how render_text() likes it.
  } optionv[OPTION_LIMIT];
  int optionc;
  int optionp;
};

#define MODAL ((struct modal_pause*)modal)

/* Delete.
 */
 
static void _pause_del(struct modal *modal) {
}

/* Add option.
 * This leaves (x,y) unset. Caller must set those after adding all options.
 */
 
static void pause_add_option(struct modal *modal,int optionid,const char *text) {
  if (MODAL->optionc>=OPTION_LIMIT) return;
  struct option *option=MODAL->optionv+MODAL->optionc++;
  option->optionid=optionid;
  option->text=text?text:"";
  option->textc=0;
  while (option->text[option->textc]) option->textc++;
}

/* Init.
 */
 
static int _pause_init(struct modal *modal) {
  if (!g.session) return -1;

  pause_add_option(modal,OPTIONID_RESUME,"Resume");
  pause_add_option(modal,OPTIONID_RESET, "Reset");
  pause_add_option(modal,OPTIONID_MENU,  "Main Menu");
  pause_add_option(modal,OPTIONID_QUIT,  "Quit");
  
  // Our field is centered in the framebuffer, at a size determined by the options.
  MODAL->fldh=MODAL->optionc*GLYPHH;
  struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    if (option->textc>MODAL->fldw) MODAL->fldw=option->textc;
  }
  MODAL->fldw*=GLYPHW;
  MODAL->fldw+=16; // Arbitrary horizontal padding. Zero is technically adequate.
  MODAL->fldx=(FBW>>1)-(MODAL->fldw>>1);
  MODAL->fldy=(FBH>>1)-(MODAL->fldh>>1);
  
  // Walk the options again to establish their positions.
  int dsty=MODAL->fldy+(GLYPHH>>1);
  for (i=MODAL->optionc,option=MODAL->optionv;i-->0;option++,dsty+=GLYPHH) {
    int optw=option->textc*GLYPHW;
    option->x=MODAL->fldx+(MODAL->fldw>>1)-(optw>>1)+(GLYPHW>>1);
    option->y=dsty;
  }
  
  return 0;
}

/* Activate.
 */
 
static void pause_activate(struct modal *modal) {
  if ((MODAL->optionp<0)||(MODAL->optionp>=MODAL->optionc)) return;
  switch (MODAL->optionv[MODAL->optionp].optionid) {
    case OPTIONID_RESUME: modal->defunct=1; g.session->input_blackout|=EGG_BTN_SOUTH; break;
    case OPTIONID_RESET: {
        if (session_load_map(g.session,g.session->rid)>=0) {
          g.session->input_blackout|=EGG_BTN_SOUTH;
          modal->defunct=1;
        }
      } break;
    case OPTIONID_MENU: {
        session_del(g.session);
        g.session=0;
        modal->defunct=1;
      } break;
    case OPTIONID_QUIT: egg_terminate(0); break;
  }
}

/* Move cursor.
 */
 
static void pause_move(struct modal *modal,int d) {
  if (MODAL->optionc<1) return;
  MODAL->optionp+=d;
  if (MODAL->optionp<0) MODAL->optionp=MODAL->optionc-1;
  else if (MODAL->optionp>=MODAL->optionc) MODAL->optionp=0;
  egg_play_sound(RID_sound_uimotion,1.0,0.0);
}

/* Input.
 */
 
static void _pause_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_WEST)&&!(pvinput&EGG_BTN_WEST)) modal->defunct=1;
  if ((input&EGG_BTN_AUX1)&&!(pvinput&EGG_BTN_AUX1)) modal->defunct=1;
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) pause_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) pause_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) pause_activate(modal);
}

/* Update.
 */
 
static void _pause_update(struct modal *modal,double elapsed) {
}

/* Render.
 */
 
static void _pause_render(struct modal *modal) {

  session_render(g.session);
  fill_rect(0,0,FBW,FBH,0x00000080);
  const int shadow_offset=2;
  if (shadow_offset) {
    fill_rect(MODAL->fldx+shadow_offset,MODAL->fldy+shadow_offset,MODAL->fldw,MODAL->fldh,0x00000080);
  }
  fill_rect(MODAL->fldx,MODAL->fldy,MODAL->fldw,MODAL->fldh,0x000000ff);
  
  if ((MODAL->optionp>=0)&&(MODAL->optionp<MODAL->optionc)) {
    fill_rect(MODAL->fldx,MODAL->fldy+MODAL->optionp*GLYPHH,MODAL->fldw,GLYPHH,0x0040ffff);
  }
  
  const struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    render_text(option->x,option->y,option->text,option->textc,0xffffffff);
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_pause={
  .name="pause",
  .objlen=sizeof(struct modal_pause),
  .del=_pause_del,
  .init=_pause_init,
  .input=_pause_input,
  .update=_pause_update,
  .render=_pause_render,
};
