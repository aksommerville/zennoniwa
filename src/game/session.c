#include "zennoniwa.h"

/* Delete.
 */
 
void session_del(struct session *session) {
  if (!session) return;
  if (session->spritev) {
    while (session->spritec-->0) sprite_del(session->spritev[session->spritec]);
    free(session->spritev);
  }
  free(session);
}

/* New.
 */
 
struct session *session_new() {
  struct session *session=calloc(1,sizeof(struct session));
  if (!session) return 0;
  
  if (!(session->spritev=malloc(sizeof(void*)*10))) return 0;
  session->spritea=10;
  if (!(session->spritev[session->spritec]=sprite_new(&sprite_type_hero,2.5,1.5,0))) return 0;
  session->spritec++;
  
  return session;
}

/* Update.
 */
 
void session_update(struct session *session,double elapsed,int input,int pvinput) {
  //TODO
  int i;
  
  for (i=session->spritec;i-->0;) {
    struct sprite *sprite=session->spritev[i];
    if (sprite->defunct) continue;
    if (sprite->type->update) sprite->type->update(sprite,elapsed);
  }
  
  for (i=session->spritec;i-->0;) {
    struct sprite *sprite=session->spritev[i];
    if (!sprite->defunct) continue;
    session->spritec--;
    memmove(session->spritev+i,session->spritev+i+1,sizeof(void*)*(session->spritec-i));
    sprite_del(sprite);
  }
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
  int fieldx=(FBW>>1)-((NS_sys_tilesize*NS_sys_mapw)>>1);
  int fieldy=(FBH>>1)-((NS_sys_tilesize*NS_sys_maph)>>1);
  render_grid(fieldx,fieldy,mygrid,NS_sys_mapw,NS_sys_maph);
  struct tilerenderer tr={0};
  int i=0;
  for (;i<session->spritec;i++) {
    struct sprite *sprite=session->spritev[i];
    int dstx=(int)(sprite->x*NS_sys_tilesize)+fieldx;
    int dsty=(int)(sprite->y*NS_sys_tilesize)+fieldy;
    tilerenderer_add(&tr,dstx,dsty,sprite->tileid,sprite->xform);
  }
  tilerenderer_flush(&tr);
}
