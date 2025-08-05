/* modal_gameover.c
 * Appears after completing each level.
 * The session remains live but suspended beneath us.
 */
 
#include "game/zennoniwa.h"

struct modal_gameover {
  struct modal hdr;
};

#define MODAL ((struct modal_gameover*)modal)

/* Cleanup.
 */
 
static void _gameover_del(struct modal *modal) {
}

/* Init.
 */
 
static int _gameover_init(struct modal *modal) {
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
  fill_rect(0,0,FBW,FBH,0x800000ff);
  render_text(40,100,"Game over",-1,0xffffffff);
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
