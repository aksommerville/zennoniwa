#include "zennoniwa.h"

/* Delete.
 */
 
void session_del(struct session *session) {
  if (!session) return;
  free(session);
}

/* New.
 */
 
struct session *session_new() {
  struct session *session=calloc(1,sizeof(struct session));
  if (!session) return 0;
  return session;
}

/* Update.
 */
 
void session_update(struct session *session,double elapsed,int input,int pvinput) {
  //TODO
}

/* Render.
 */
 
static uint8_t mygrid[NS_sys_mapw*NS_sys_maph]={
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};
 
void session_render(struct session *session) {
  fill_rect(0,0,FBW,FBH,0xff8000ff);
  render_grid(
    (FBW>>1)-((NS_sys_tilesize*NS_sys_mapw)>>1),
    (FBH>>1)-((NS_sys_tilesize*NS_sys_maph)>>1),
    mygrid,NS_sys_mapw,NS_sys_maph
  );
}
