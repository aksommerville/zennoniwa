#include "zennoniwa.h"

/* Delete.
 */
 
void session_del(struct session *session) {
  if (!session) return;
  if (session->spritev) {
    while (session->spritec-->0) sprite_del(session->spritev[session->spritec]);
    free(session->spritev);
  }
  if (session->cellv) free(session->cellv);
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
  session->hero=session->spritev[session->spritec++];
  
  if (session_load_map(session,RID_map_trial)<0) {
    session_del(session);
    return 0;
  }
  
  return session;
}

/* Update.
 */
 
void session_update(struct session *session,double elapsed,int input,int pvinput) {
  int i;
  
  // Update sprites.
  for (i=session->spritec;i-->0;) {
    struct sprite *sprite=session->spritev[i];
    if (sprite->defunct) continue;
    if (sprite->type->update) sprite->type->update(sprite,elapsed);
  }
  
  /* Gather score constituents.
   */
  double goodlife=0.0,badlife=0.0;
  int trueplantc=0,wantplantc=0,falseplantc=0,noplantc=0;
  const struct cell *cell=session->cellv;
  for (i=session->mapw*session->maph;i-->0;cell++) {
    if (cell->tileid==1) {//TODO There will be more than just 1 "plant on me" tile.
      wantplantc++;
      if (cell->life>0.0) {
        trueplantc++;
        goodlife+=cell->life;
      }
    } else {
      noplantc++;
      if (cell->life>0.0) {
        falseplantc++;
        badlife+=cell->life;
      }
    }
  }
  if ((trueplantc==wantplantc)&&!falseplantc) {
    session->qualified=1;
  } else {
    session->qualified=0;
  }
  if (wantplantc>0) {
    session->life=(goodlife+badlife)/wantplantc;
  } else {
    session->life=0.0;
  }
  
  // Kill defunct sprites.
  for (i=session->spritec;i-->0;) {
    struct sprite *sprite=session->spritev[i];
    if (!sprite->defunct) continue;
    if (sprite==session->hero) session->hero=0;
    session->spritec--;
    memmove(session->spritev+i,session->spritev+i+1,sizeof(void*)*(session->spritec-i));
    sprite_del(sprite);
  }
}

/* Render.
 */
 
void session_render(struct session *session) {
  fill_rect(0,0,FBW,FBH,0xff8000ff);
  struct tilerenderer tr={0};

  int fieldx=(FBW>>1)-((NS_sys_tilesize*session->mapw)>>1);
  int fieldy=(FBH>>1)-((NS_sys_tilesize*session->maph)>>1);
  if (session->cellv) {
    int dstx0=fieldx+(NS_sys_tilesize>>1);
    int dsty=fieldy+(NS_sys_tilesize>>1);
    const struct cell *cell=session->cellv;
    int yi=session->maph;
    int havelife=0;
    for (;yi-->0;dsty+=NS_sys_tilesize) {
      int dstx=dstx0;
      int xi=session->mapw;
      for (;xi-->0;dstx+=NS_sys_tilesize,cell++) {
        tilerenderer_add(&tr,dstx,dsty,cell->tileid,0);
        if (cell->life>0.0) havelife=1;
      }
    }
    if (havelife) {
      tilerenderer_flush(&tr);
      struct fancyrenderer fr={0};
      for (yi=session->maph,dsty=fieldy+(NS_sys_tilesize>>1),cell=session->cellv;yi-->0;dsty+=NS_sys_tilesize) {
        int dstx=dstx0;
        int xi=session->mapw;
        for (;xi-->0;dstx+=NS_sys_tilesize,cell++) {
          if (cell->life<=0.0) continue;
          int frame=cell->life*5.0;
          if (frame<0) frame=0; else if (frame>4) frame=4;
          fancyrenderer_add(&fr,dstx,dsty,0x02+frame,0,0,NS_sys_tilesize,0,0xff);
        }
      }
      fancyrenderer_flush(&fr);
    }
  }
  
  int i=0;
  for (;i<session->spritec;i++) {
    struct sprite *sprite=session->spritev[i];
    int dstx=(int)(sprite->x*NS_sys_tilesize)+fieldx;
    int dsty=(int)(sprite->y*NS_sys_tilesize)+fieldy;
    if (sprite->type->render) {
      sprite->type->render(sprite,dstx,dsty,&tr);
    } else {
      tilerenderer_add(&tr,dstx,dsty,sprite->tileid,sprite->xform);
    }
  }
  tilerenderer_flush(&tr);
  
  /* Quickie indicators for qualified and score.
   */
  fill_rect(FBW-20,(FBH>>1)-5,10,10,session->qualified?0x008000ff:0xff0000ff);
  int barw=session->life*FBW;
  fill_rect(0,FBH-10,barw,5,0x404060ff);
}

/* Apply command from map.
 */
 
static int session_apply_map_command(struct session *session,uint8_t opcode,const uint8_t *arg,int argc) {
  //fprintf(stderr,"%s 0x%02x argc=%d\n",__func__,opcode,argc);
  switch (opcode) {
    case CMD_map_plant: {
        int x=arg[0],y=arg[1],life=arg[2];
        if ((x<session->mapw)&&(y<session->maph)) {
          struct cell *cell=session->cellv+y*session->mapw+x;
          cell->life=life/255.0;
        }
      } break;
  }
  return 0;
}

/* Load map.
 */
 
int session_load_map(struct session *session,int rid) {
  const uint8_t *serial=0;
  int serialc=res_get(&serial,EGG_TID_map,rid);
  if ((serialc<6)||memcmp(serial,"\0EMP",4)) return -1;
  int colc=serial[4];
  int rowc=serial[5];
  int cellc=colc*rowc;
  int srcp=6;
  if (!colc||!rowc||(srcp>serialc-cellc)) return -1;
  if (session->cellv) free(session->cellv);
  if (!(session->cellv=malloc(sizeof(struct cell)*cellc))) {
    session->mapw=session->maph=0;
    return -1;
  }
  const uint8_t *cellsrc=serial+srcp;
  struct cell *cell=session->cellv;
  int i=cellc;
  for (;i-->0;cellsrc++,cell++) {
    cell->tileid=*cellsrc;
    cell->life=0.0;
  }
  session->mapw=colc;
  session->maph=rowc;
  srcp+=cellc;
  while (srcp<serialc) {
    uint8_t lead=serial[srcp++];
    const uint8_t *arg=serial+srcp;
    int argc=0;
    switch (lead&0xe0) {
      case 0x00: break;
      case 0x20: argc=2; break;
      case 0x40: argc=4; break;
      case 0x60: argc=8; break;
      case 0x80: argc=12; break;
      case 0xa0: argc=16; break;
      case 0xc0: argc=20; break;
      case 0xe0: if (srcp>=serialc) return -1; argc=serial[srcp++]; break;
    }
    if (srcp>serialc-argc) return -1;
    if (session_apply_map_command(session,lead,arg,argc)<0) return -1;
    srcp+=argc;
  }
  return 0;
}
