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
  egg_play_song(RID_song_only_soil_deep,0,1);
  if (g.session) {
    int levelc=g.session->rid-1;
    if (levelc<1) levelc=1;
    g.session->tscore.life/=(double)levelc;
    int changed=0;
    if (g.session->tscore.time<g.hiscore.time) {
      g.hiscore.time=g.session->tscore.time;
      changed=1;
    }
    if (g.session->tscore.life>g.hiscore.life) {
      g.hiscore.life=g.session->tscore.life;
      changed=1;
    }
    if (g.session->tscore.pinkc>g.hiscore.pinkc) {
      g.hiscore.pinkc=g.session->tscore.pinkc;
      changed=1;
    }
    if (changed) {
      hiscore_save();
    }
  }
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
  //TODO highly temporary
  fill_rect(0,0,FBW,FBH,0x800000ff);
  render_text(40,100,"Game over",-1,0xffffffff);
  if (g.session) {
    render_text(50,120,"Time:",-1,0xffff00ff);
    render_time(140,120,g.session->tscore.time,0xffff00ff);
    render_time(320,120,g.hiscore.time,0xffff00ff);
    render_text(50,136,"Life:",-1,0xffff00ff);
    render_uint(140,136,(int)(g.session->tscore.life*1000.0),999,0xffff00ff);
    render_uint(320,136,(int)(g.hiscore.life*1000.0),999,0xffff00ff);
    render_text(50,152,"Pink:",-1,0xffff00ff);
    render_uint(140,152,g.session->tscore.pinkc,999,0xffff00ff);
    render_uint(320,152,g.hiscore.pinkc,999,0xffff00ff);
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
