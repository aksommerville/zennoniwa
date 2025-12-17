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

/* Generate the static background tiles.
 */
 
static int session_generate_bgtilev(struct session *session) {
  session->bgtilec=0;
  #define VTXPX(_x,_y,_tileid,_xform) { \
    if (session->bgtilec>=SESSION_BGTILES_SIZE) { \
      fprintf(stderr,"too many background tiles\n"); \
      return -1; \
    } \
    struct egg_render_tile *vtx=session->bgtilev+session->bgtilec++; \
    vtx->x=_x; \
    vtx->y=_y; \
    vtx->tileid=_tileid; \
    vtx->xform=_xform; \
  }
  #define VTX(col,row,_tileid,_xform) { \
    VTXPX((col)*NS_sys_tilesize+(NS_sys_tilesize>>1),(row)*NS_sys_tilesize+(NS_sys_tilesize>>1),_tileid,_xform) \
  }
  int x,y;
  
  // Bamboo on the edges.
  for (y=0;y<11;y++) {
    VTX(0,y,0x40+(y<<4),0)
    VTX(19,y,0x40+(y<<4),EGG_XFORM_XREV)
  }
  
  // Second and second-to-last columns: Grass and sand edges.
  VTX(1,0,0x33,0)
  VTX(18,0,0x33,0)
  VTX(1,1,0x37,EGG_XFORM_XREV|EGG_XFORM_YREV)
  VTX(18,1,0x37,EGG_XFORM_YREV)
  for (y=2;y<=9;y++) {
    VTX(1,y,0x34+(y%3),EGG_XFORM_SWAP|EGG_XFORM_YREV)
    VTX(18,y,0x34+(y%3),EGG_XFORM_SWAP)
  }
  VTX(1,10,0x37,EGG_XFORM_XREV)
  VTX(18,10,0x37,0)
  
  // Remainder of bottom row is sand edge -- the stepping stones are not part of this set.
  for (x=2;x<=17;x++) VTX(x,10,0x34+(x%3),0)
  
  // Pond. No rock, fish, or lily pad.
  VTX(2,0,0x63,0)
  VTX(2,1,0x73,0)
  VTX(17,0,0x63,EGG_XFORM_XREV)
  VTX(17,1,0x73,EGG_XFORM_XREV)
  for (x=3;x<=16;x++) {
    VTX(x,0,0x47,0)
    VTX(x,1,0x74+(x%4),0)
  }
  
  // Rocks and lily pad. These are static and opaque, but they're a bit offset from the grid.
  VTXPX(185,30,0x43,0)
  VTXPX(217,30,0x44,0)
  VTXPX(360,30,0x45,0)
  VTXPX(392,30,0x46,0)
  
  #undef VTX
  #undef VTXPX
  return 0;
}

/* New.
 */
 
struct session *session_new() {
  struct session *session=calloc(1,sizeof(struct session));
  if (!session) return 0;
  
  session_generate_bgtilev(session);
  session->fishx=130;
  
  if (session_load_map(session,1)<0) {
    session_del(session);
    return 0;
  }
  
  session->input_blackout=1;
  
  return session;
}

/* Win, enter denouement.
 */
 
static void session_win(struct session *session) {
  // (lscore.time,pinkc) get updated on the fly, they're already ready.
  session->lscore.life=session->life;
  session->tscore.time+=session->lscore.time;
  session->tscore.life+=session->lscore.life;
  session->tscore.pinkc+=session->lscore.pinkc;
  if (!g.modal) g.modal=modal_new(&modal_type_denouement);
}

/* Update.
 */
 
void session_update(struct session *session,double elapsed,int input,int pvinput) {
  int i;
  
  // East to reset. (for p1, that's "R" by default)
  if ((input&EGG_BTN_EAST)&&!(pvinput&EGG_BTN_EAST)) {
    session_load_map(session,session->rid);
  }
  
  session->cameoclock+=elapsed;
  if (session->cameoclock>20.0) session->cameoclock-=20.0;
  
  session->lscore.time+=elapsed;
  
  if ((session->fishclock-=elapsed)<=0.0) {
    session->fishclock+=0.150;
    if (++(session->fishframe)>=10) session->fishframe=0;
  }
  
  // Update input blackout.
  if (session->input_blackout) {
    if (input&EGG_BTN_SOUTH) {
      input&=~EGG_BTN_SOUTH;
      pvinput&=~EGG_BTN_SOUTH;
    } else {
      session->input_blackout=0;
    }
  }
  session->input=input;
  session->pvinput=pvinput;
  
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
    if (tileid_is_sand(cell->tileid)) {
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
    int netplantc=trueplantc-falseplantc;
    if (netplantc<0) netplantc=0;
    session->completion=(netplantc*8)/wantplantc;
    if (netplantc&&!session->completion) session->completion=1; // Zero means exactly zero.
    else if ((netplantc==wantplantc)&&!falseplantc) session->completion=8; // Eight means exactly right.
    else if (session->completion==8) session->completion=7; // Ahem.
  } else {
    session->life=0.0;
    session->completion=0;
  }
  if (session->qualified) {
    if ((session->termclock+=elapsed)>=SESSION_TERMCLOCK_LIMIT) {
      session_win(session);
    }
  } else {
    session->termclock=0.0;
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
  struct tilerenderer tr={0};
  
  /* Jammio cameo.
   * On a 20-second loop, replace the tile for the right stone, in the background tiles.
   * 0x44 = natural
   * 0xe7..0xea = arising
   * 0xeb,0xec = waving
   */
  if (session->bgtilec>20*11-16*8+1) {
    struct egg_render_tile *vtx=session->bgtilev+20*11-16*8+1;
    int frame=(int)(session->cameoclock*7.0); // to about 100
    switch (frame) {
      case 60: vtx->tileid=0xe7; break;
      case 61: vtx->tileid=0xe8; break;
      case 62: vtx->tileid=0xe9; break;
      case 63: vtx->tileid=0xea; break;
      case 64: vtx->tileid=0xeb; break; // begin waving...
      case 65: vtx->tileid=0xec; break;
      case 66: vtx->tileid=0xeb; break;
      case 67: vtx->tileid=0xec; break;
      case 68: vtx->tileid=0xeb; break;
      case 69: vtx->tileid=0xec; break;
      case 70: vtx->tileid=0xeb; break;
      case 71: vtx->tileid=0xea; break; // retreating...
      case 72: vtx->tileid=0xe9; break;
      case 73: vtx->tileid=0xe8; break;
      case 74: vtx->tileid=0xe7; break;
      default: vtx->tileid=0x44; break;
    }
  }

  /* Background.
   */
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=1,
    .srctexid=g.texid_tilesheet,
    .alpha=0xff,
  };
  egg_render(&un,session->bgtilev,session->bgtilec*sizeof(struct egg_render_tile));
  
  /* Stepping stones indicating completion.
   */
  int i;
  for (i=0;i<8;i++) {
    uint8_t tileid=0x38;
    if (i<session->completion) tileid=0x39;
    tilerenderer_add(&tr,NS_sys_tilesize*3+i*NS_sys_tilesize*2,FBH-(NS_sys_tilesize>>1),tileid,0);
  }
  
  /* Swimming fish.
   */
  {
    uint8_t fishtile=0x53+session->fishframe;
    int y=30;
    if (!g.modal) {
      if ((session->fishframe>=0)&&(session->fishframe<=2)) {
        session->fishx++;
      } else if ((session->fishframe>=5)&&(session->fishframe<=7)) {
        session->fishx--;
      }
    }
    tilerenderer_add(&tr,session->fishx,y,fishtile,0);
  }

  /* Map and plants.
   */
  int fieldx=NS_sys_tilesize*2;
  int fieldy=NS_sys_tilesize*2;
  session->lscore.pinkc=0;
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
        uint8_t xform=0;
        if (!cell->tileid) xform=(yi+xi)&7;
        tilerenderer_add(&tr,dstx,dsty,cell->tileid,xform);
        if (cell->life>0.0) havelife=1;
      }
    }
    // Focus cell indicators.
    if (session->hero) {
      tilerenderer_flush(&tr);
      hero_prerender(session->hero,fieldx,fieldy);
    }
    // Plants
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
          if (frame==4) session->lscore.pinkc++; // Set (pinkc) based on tiles rendered, so the score can't help staying in sync with the visuals.
          fancyrenderer_add(&fr,dstx,dsty,0x02+frame,0,0,NS_sys_tilesize,0,0xff);
        }
      }
      fancyrenderer_flush(&fr);
    }
  }
  
  /* Sprites.
   */
  for (i=0;i<session->spritec;i++) {
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
  
  /* Fade out if terminating.
   */
  if (session->termclock>SESSION_TERMCLOCK_BEGIN) {
    int alpha=(int)(((session->termclock-SESSION_TERMCLOCK_BEGIN)*255.0)/(SESSION_TERMCLOCK_LIMIT-SESSION_TERMCLOCK_BEGIN));
    if (alpha>0) {
      if (alpha>0xff) alpha=0xff;
      fill_rect(0,0,FBW,FBH,0x286b4600|alpha);
    }
  }
}

/* Kill all sprites.
 * Do not call during an update!
 */
 
static void session_drop_sprites(struct session *session) {
  session->hero=0;
  while (session->spritec>0) {
    struct sprite *sprite=session->spritev[--(session->spritec)];
    sprite_del(sprite);
  }
}

/* Spawn sprite.
 */
 
struct sprite *session_spawn_sprite(struct session *session,const struct sprite_type *type,double x,double y,uint32_t arg) {
  if (session->spritec>=session->spritea) {
    int na=session->spritea+32;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(session->spritev,sizeof(void*)*na);
    if (!nv) return 0;
    session->spritev=nv;
    session->spritea=na;
  }
  struct sprite *sprite=sprite_new(type,x,y,arg);
  if (!sprite) return 0;
  session->spritev[session->spritec++]=sprite;
  if (type==&sprite_type_hero) session->hero=sprite;
  return sprite;
}

/* Apply command from map.
 */
 
static int session_apply_map_command(struct session *session,uint8_t opcode,const uint8_t *arg,int argc) {
  //fprintf(stderr,"%s 0x%02x argc=%d\n",__func__,opcode,argc);
  switch (opcode) {
    case CMD_map_hero: {
        int x=arg[0],y=arg[1];
        if ((x<session->mapw)&&(y<session->maph)) {
          struct sprite *sprite=session_spawn_sprite(session,&sprite_type_hero,x+0.5,y+0.5,0);
        }
      } break;
    case CMD_map_waterpattern: {
        session->waterpattern=(arg[0]<<8)|arg[1];
      } break;
    case CMD_map_plant: {
        int x=arg[0],y=arg[1],life=arg[2];
        if ((x<session->mapw)&&(y<session->maph)) {
          struct cell *cell=session->cellv+y*session->mapw+x;
          cell->life=life/255.0;
        }
      } break;
    case CMD_map_sprite: {
        int x=arg[0],y=arg[1],rid=(arg[2]<<8)|arg[3];
        const struct sprres *sprres=sprres_get(rid);
        uint32_t sarg=0; // Usually I include a 32-bit argument in the map command. Here we didn't.
        struct sprite *sprite=session_spawn_sprite(session,sprres->type,x+0.5,y+0.5,sarg);
        if (!sprite) return -1;
        sprite->tileid=sprres->tileid;
      } break;
  }
  return 0;
}

/* Load map.
 */
 
int session_load_map(struct session *session,int rid) {
  session->rid=rid;
  session->input_blackout=1;
  session->lscore.time=0.0;
  session->lscore.life=0.0;
  session->lscore.pinkc=0;
  session->waterpattern=NS_waterpattern_single;
  session->cameoclock=0.0;
  play_song(RID_song_willow_reed,1);

  const uint8_t *serial=0;
  int serialc=res_get(&serial,EGG_TID_map,rid);
  if ((serialc<6)||memcmp(serial,"\0EMP",4)) {
    session->load_failed=1;
    return -1;
  }
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
  
  session_drop_sprites(session);
  
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
  if (!session->hero) {
    fprintf(stderr,"map:%d did not spawn a hero\n",rid);
    return -1;
  }
  return 0;
}

/* Load next map.
 */
 
int session_load_next(struct session *session) {
  return session_load_map(session,session->rid+1);
}
