/* modal_denouement.c
 * Appears after completing each level.
 * The session remains live but suspended beneath us.
 */
 
#include "game/zennoniwa.h"

struct modal_denouement {
  struct modal hdr;
};

#define MODAL ((struct modal_denouement*)modal)

/* Cleanup.
 */
 
static void _denouement_del(struct modal *modal) {
}

/* Init.
 */
 
static int _denouement_init(struct modal *modal) {
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
  fill_rect(0,0,FBW,FBH,0x1020c0ff);
  render_text(40,100,"Level complete",-1,0xffffffff);
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
