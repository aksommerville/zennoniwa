#include "game/zennoniwa.h"

#define CORRUPTION_RATE -0.400 /* hz */
#define CREATION_RATE    0.800 /* hz */
#define TURN_TIME        0.100

#define WATERPATTERN_SINGLE 1 /* Just (qx,qy). */
#define WATERPATTERN_CROSS 2
#define WATERPATTERN_THREE  3 /* Three in a row, perpendicular to the direction of travel. */
#define WATER_LIMIT 9 /* The most cells addressable by a waterpattern. */

static void hero_rebuild_watertiles(struct sprite *sprite);
static int hero_move_ok(const struct sprite *sprite,int x,int y);

struct sprite_hero {
  struct sprite hdr;
  int indx,indy;
  int facedx,facedy; // always cardinal
  int stickydx; // facedx but retains its value, in case we only get horizontal faces.
  double pourclock;
  int qx,qy; // Quantized position, updates each cycle.
  int waterpattern;
  double turnclock;
  double highlightclock;
  int highlightframe;
  struct egg_render_tile watertilev[6*6]; // Four tiles per map tile.
  int watertilec;
  int waterx0,watery0; // Playfield origin for (watertilev). Need to defer until after the first update.
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Quantized position.
 */
 
static void hero_update_qpos(struct sprite *sprite) {
  int ox=SPRITE->qx,oy=SPRITE->qy;
  SPRITE->qx=(int)sprite->x;
  SPRITE->qy=(int)sprite->y;
  if (SPRITE->qx<0) SPRITE->qx=0; else if (g.session&&(SPRITE->qx>=g.session->mapw)) SPRITE->qx=g.session->mapw-1;
  if (SPRITE->qy<0) SPRITE->qy=0; else if (g.session&&(SPRITE->qy>=g.session->maph)) SPRITE->qy=g.session->maph-1;
  if ((SPRITE->qx!=ox)||(SPRITE->qy!=oy)) {
    // Need to trigger anything on quantized position change?
  }
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->tileid=0x10;
  sprite->xform=0;
  SPRITE->facedy=1;
  SPRITE->stickydx=-1;
  SPRITE->waterpattern=WATERPATTERN_THREE;
  hero_update_qpos(sprite);
  SPRITE->waterx0=-1; // Signal to rebuild watertiles at the first prerender.
  //hero_rebuild_watertiles(sprite);
  return 0;
}

/* Waterpattern definitions.
 */
 
struct delta2d {
  int dx,dy;
  double rate; // >0 to create, <0 to corrupt, or 0 to make the caller wonder why you bothered returning it
};

static int waterpattern_get_always(struct delta2d *dst/*WATER_LIMIT*/,struct sprite *sprite,int pattern) {
  switch (pattern) {
    case WATERPATTERN_SINGLE: {
        dst[0]=(struct delta2d){0,0};
        return 1;
      }
    case WATERPATTERN_CROSS: {
        dst[0]=(struct delta2d){0,-1};
        dst[1]=(struct delta2d){-1,0};
        dst[2]=(struct delta2d){0,0};
        dst[3]=(struct delta2d){1,0};
        dst[4]=(struct delta2d){0,1};
        return 5;
      }
    case WATERPATTERN_THREE: {
        if (SPRITE->facedy) {
          dst[0]=(struct delta2d){-1,0};
          dst[1]=(struct delta2d){0,0};
          dst[2]=(struct delta2d){1,0};
        } else {
          dst[0]=(struct delta2d){0,-1};
          dst[1]=(struct delta2d){0,0};
          dst[2]=(struct delta2d){0,1};
        }
        return 3;
      }
  }
  return 0;
}

static int waterpattern_get_ondemand(struct delta2d *dst/*WATER_LIMIT*/,struct sprite *sprite,int pattern) {
  struct delta2d *delta=dst;
  int i=0; for (;i<9;i++) {
    dst[i].dx=(i%3)-1;
    dst[i].dy=(i/3)-1;
    dst[i].rate=CORRUPTION_RATE;
  }
  switch (pattern) {
    case WATERPATTERN_SINGLE: {
        dst[0].rate=CREATION_RATE;
      } break;
    case WATERPATTERN_CROSS: {
        dst[1].rate=CREATION_RATE;
        dst[3].rate=CREATION_RATE;
        dst[4].rate=CREATION_RATE;
        dst[5].rate=CREATION_RATE;
        dst[7].rate=CREATION_RATE;
      } break;
    case WATERPATTERN_THREE: {
        if (SPRITE->facedy) {
          dst[3].rate=CREATION_RATE;
          dst[4].rate=CREATION_RATE;
          dst[5].rate=CREATION_RATE;
        } else {
          dst[1].rate=CREATION_RATE;
          dst[4].rate=CREATION_RATE;
          dst[7].rate=CREATION_RATE;
        }
      } break;
  }
  // Refine a bit further: If any cell is slated for Creation but is not passable (ie a rock), call it Corruption.
  for (delta=dst,i=9;i-->0;delta++) {
    if (delta->rate<=0.0) continue;
    if (!hero_move_ok(sprite,SPRITE->qx+delta->dx,SPRITE->qy+delta->dy)) {
      delta->rate=CORRUPTION_RATE;
    }
  }
  return 9;
}

/* Rebuild vertex list for the waterpattern highlight.
 * Animation is not relevant, that gets rewritten at each render.
 */
 
static void hero_rebuild_watertiles(struct sprite *sprite) {
  SPRITE->watertilec=0;

  /* Get the generic pattern.
   */
  struct delta2d storage[WATER_LIMIT];
  int patternc=waterpattern_get_ondemand(storage,sprite,SPRITE->waterpattern);
  
  /* Turn that delta list into a flat 6x6 grid of moods.
   * (0,1,2)=(none,positive,negative), setting 4 at a time.
   */
  uint8_t moodv[6*6]={0};
  const struct delta2d *delta=storage;
  for (;patternc-->0;delta++) {
    uint8_t mood=(delta->rate>0.0)?1:(delta->rate<0.0)?2:0;
    if (!mood) continue;
    uint8_t *dst=moodv+(delta->dy+1)*12+(delta->dx+1)*2;
    dst[0]=dst[1]=dst[6]=dst[7]=mood;
  }
  
  /* Neighbor-join those moods into a preliminary 6x6 tileid grid.
   * 0x10=edge(N), 0x11=corner(NE), 0x21=concave(NE), 0x20=negative.
   * Don't animate.
   * Use the high 2 bits to indicate rotation counter-clockwise.
   */
  uint8_t tileidv[6*6]={0};
  uint8_t *dstp=tileidv;
  const uint8_t *srcp=moodv;
  int y=0; for (;y<6;y++) {
    int x=0; for (;x<6;x++,dstp++,srcp++) {
      if (!*srcp) continue; // mood zero is always tile zero, ie vacant
      if (*srcp==2) { // mood two is always tile 0x20, ie negative.
        *dstp=0x20;
        continue;
      }
      uint8_t neighbors=0;
      if ((x>0)&&(y>0)&&(srcp[-7]==srcp[0])) neighbors|=0x80;
      if ((y>0)&&(srcp[-6]==srcp[0])) neighbors|=0x40;
      if ((y>0)&&(x<5)&&(srcp[-5]==srcp[0])) neighbors|=0x20;
      if ((x>0)&&(srcp[-1]==srcp[0])) neighbors|=0x10;
      if ((x<5)&&(srcp[1]==srcp[0])) neighbors|=0x08;
      if ((x>0)&&(y<5)&&(srcp[5]==srcp[0])) neighbors|=0x04;
      if ((y<5)&&(srcp[6]==srcp[0])) neighbors|=0x02;
      if ((x<5)&&(y<5)&&(srcp[7]==srcp[0])) neighbors|=0x01;
      if (neighbors==0xff) *dstp=0; // Full inner. Effectively vacant.
      else if (neighbors==0xdf) *dstp=0x21; // Concave corners...
      else if (neighbors==0x7f) *dstp=0x21|0x40;
      else if (neighbors==0xfb) *dstp=0x21|0x80;
      else if (neighbors==0xfe) *dstp=0x21|0xc0;
      else if ((neighbors&0x1f)==0x1f) *dstp=0x10; // Edges...
      else if ((neighbors&0x6b)==0x6b) *dstp=0x10|0x40;
      else if ((neighbors&0xf8)==0xf8) *dstp=0x10|0x80;
      else if ((neighbors&0xd6)==0xd6) *dstp=0x10|0xc0;
      else if ((neighbors&0x16)==0x16) *dstp=0x11; // Corners...
      else if ((neighbors&0x0b)==0x0b) *dstp=0x11|0x40;
      else if ((neighbors&0x68)==0x68) *dstp=0x11|0x80;
      else if ((neighbors&0xd0)==0xd0) *dstp=0x11|0xc0;
      // No other positive arrangement is possible; leave it zero if something's broken.
    }
  }
  
  /* Turn (tileidv) into tile vertices.
   * Skip any outside the playfield.
   * Final tileids are from a 4x4 block that can be shifted 2, 4, or 6 spaces right for animation.
   */
  int dsty=SPRITE->watery0+(SPRITE->qy-1)*NS_sys_tilesize+(NS_sys_tilesize>>2);
  for (y=0,srcp=tileidv;y<6;y++,dsty+=NS_sys_tilesize>>1) {
    int row=SPRITE->qy-1+(y>>1);
    int dstx=SPRITE->waterx0+(SPRITE->qx-1)*NS_sys_tilesize+(NS_sys_tilesize>>2);
    int x=0; for (;x<6;x++,dstx+=NS_sys_tilesize>>1,srcp++) {
      if ((row<0)||(row>=NS_sys_maph)) continue;
      int col=SPRITE->qx-1+(x>>1);
      if ((col<0)||(col>=NS_sys_mapw)) continue;
      if (!*srcp) continue;
      struct egg_render_tile *vtx=SPRITE->watertilev+SPRITE->watertilec++;
      vtx->x=dstx;
      vtx->y=dsty;
      vtx->tileid=((*srcp)&0x3f);
      switch ((*srcp)&0xc0) {
        case 0x40: vtx->xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
        case 0x80: vtx->xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
        case 0xc0: vtx->xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
        default: vtx->xform=0;
      }
    }
  }
}

/* Water plants.
 * When (!g.corrupt_always), this effects both creation and corruption.
 */
 
static void hero_water_plants(struct sprite *sprite,double elapsed,int enable) {
  if (!enable) {
    SPRITE->pourclock=0.0;
    return;
  }
  SPRITE->pourclock+=elapsed;
  struct delta2d storage[WATER_LIMIT];
  int patternc=g.corrupt_always
    ?waterpattern_get_always(storage,sprite,SPRITE->waterpattern)
    :waterpattern_get_ondemand(storage,sprite,SPRITE->waterpattern);
  const struct delta2d *delta=storage;
  int highlightc=0;
  double highlightx=0.0,highlighty=0.0;
  for (;patternc-->0;delta++) {
    int x=SPRITE->qx+delta->dx;
    int y=SPRITE->qy+delta->dy;
    if ((x>=0)&&(y>=0)&&(x<g.session->mapw)&&(y<g.session->maph)) {
      struct cell *cell=g.session->cellv+y*g.session->mapw+x;
      if ((delta->rate>0.0)&&(cell->life<=0.0)) {
        highlightc++;
        highlightx+=x+0.5;
        highlighty+=y+0.5;
      }
      cell->life+=elapsed*delta->rate;
      if (cell->life>1.0) {
        cell->life=1.0;
      } else if (cell->life<0.0) {
        cell->life=0.0;
      }
    }
  }
  if (highlightc) {
    egg_play_sound(RID_sound_newlife,1.0,0.0);
    double dstx=highlightx/highlightc;
    double dsty=highlighty/highlightc;
    struct sprite *toast=session_spawn_sprite(g.session,&sprite_type_toast,dstx,dsty,0x220c0000);
  }
}

/* Corrupt plants.
 * This should only be called when (g.corrupt_always).
 */
 
static void hero_apply_corruption_1(struct sprite *sprite,int x,int y,double elapsed) {
  if ((x<0)||(y<0)||(x>=g.session->mapw)||(y>=g.session->maph)) return;
  struct cell *cell=g.session->cellv+y*g.session->mapw+x;
  if (cell->life<=0.0) return;
  if ((cell->life+=elapsed*CORRUPTION_RATE)<=0.0) {
    cell->life=0.0;
  }
}
 
static void hero_apply_corruption(struct sprite *sprite,double elapsed) {
  int dy=-1; for (;dy<=1;dy++) {
    int dx=-1; for (;dx<=1;dx++) {
      hero_apply_corruption_1(sprite,SPRITE->qx+dx,SPRITE->qy+dy,elapsed);
    }
  }
}

/* Is a given cell ok to move to?
 * Requested coords may be OOB.
 * Includes consideration of solid sprites.
 */
 
static int hero_move_ok(const struct sprite *sprite,int x,int y) {
  if ((x<0)||(y<0)||(x>=g.session->mapw)||(y>=g.session->maph)) return 0;
  switch (g.session->cellv[y*g.session->mapw+x].tileid) {
    case 0x30: case 0x31: case 0x32: return 0;
  }
  return 1;
}

/* Walking.
 */
 
static void hero_update_motion_continuous(struct sprite *sprite,double elapsed) {
  const double speed=6.0;
  
  /* Watch for changes to the input state.
   * When the input changes, face that direction.
   */
  int nindx=0,nindy=0;
  switch (g.session->input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: nindx=-1; break;
    case EGG_BTN_RIGHT: nindx=1; break;
  }
  switch (g.session->input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: nindy=-1; break;
    case EGG_BTN_DOWN: nindy=1; break;
  }
  if ((nindx!=SPRITE->indx)||(nindy!=SPRITE->indy)) {
    if (nindx&&(nindx!=SPRITE->indx)) {
      SPRITE->facedy=0;
      SPRITE->facedx=nindx;
    } else if (nindy&&(nindy!=SPRITE->indy)) {
      SPRITE->facedx=0;
      SPRITE->facedy=nindy;
    } else if (SPRITE->facedx&&!nindx&&nindy) {
      SPRITE->facedx=0;
      SPRITE->facedy=nindy;
    } else if (SPRITE->facedy&&!nindy&&nindx) {
      SPRITE->facedy=0;
      SPRITE->facedx=nindx;
    } else if (SPRITE->facedx&&nindx&&(SPRITE->facedx!=nindx)) {
      SPRITE->facedx=nindx;
    } else if (SPRITE->facedy&&nindy&&(SPRITE->facedy!=nindy)) {
      SPRITE->facedy=nindy;
    }
    SPRITE->indx=nindx;
    SPRITE->indy=nindy;
    if (SPRITE->facedx) SPRITE->stickydx=SPRITE->facedx;
  }
  
  // Input state zero, cool, we're done.
  if (!SPRITE->indx&&!SPRITE->indy) return;
  
  /* Move optimistically, then rectify position.
   * The hero is the only sprite that moves, for physics purposes.
   */
  sprite->x+=speed*SPRITE->indx*elapsed;
  sprite->y+=speed*SPRITE->indy*elapsed;
  physics_rectify(sprite);
  hero_update_qpos(sprite);
}

/* Motion (quantized-space model).
 */
 
static void hero_update_motion_quantized(struct sprite *sprite,double elapsed) {
  const double speed=6.0;
  
  /* Watch for changes to the input state.
   * When the input changes, face that direction.
   * A change to (ind) or (faced) does not change our actual motion, not yet.
   */
  int fxo=SPRITE->facedx,fyo=SPRITE->facedy;
  int nindx=0,nindy=0;
  switch (g.session->input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: nindx=-1; break;
    case EGG_BTN_RIGHT: nindx=1; break;
  }
  switch (g.session->input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: nindy=-1; break;
    case EGG_BTN_DOWN: nindy=1; break;
  }
  if ((nindx!=SPRITE->indx)||(nindy!=SPRITE->indy)) {
    if (nindx&&(nindx!=SPRITE->facedx)) {
      SPRITE->turnclock=TURN_TIME;
      SPRITE->facedy=0;
      SPRITE->facedx=nindx;
    } else if (nindy&&(nindy!=SPRITE->facedy)) {
      SPRITE->turnclock=TURN_TIME;
      SPRITE->facedx=0;
      SPRITE->facedy=nindy;
    } else if (SPRITE->facedx&&!nindx&&nindy) {
      SPRITE->turnclock=TURN_TIME;
      SPRITE->facedx=0;
      SPRITE->facedy=nindy;
    } else if (SPRITE->facedy&&!nindy&&nindx) {
      SPRITE->turnclock=TURN_TIME;
      SPRITE->facedy=0;
      SPRITE->facedx=nindx;
    } else if (SPRITE->facedx&&nindx&&(SPRITE->facedx!=nindx)) {
      SPRITE->turnclock=TURN_TIME;
      SPRITE->facedx=nindx;
    } else if (SPRITE->facedy&&nindy&&(SPRITE->facedy!=nindy)) {
      SPRITE->turnclock=TURN_TIME;
      SPRITE->facedy=nindy;
    }
    SPRITE->indx=nindx;
    SPRITE->indy=nindy;
    if (SPRITE->facedx) SPRITE->stickydx=SPRITE->facedx;
  }
  
  if ((SPRITE->turnclock-=elapsed)<=0.0) SPRITE->turnclock=0.0;
  
  /* Identify and approach the target point.
   * If we reach it and the associated key is still held, set a new target.
   * Otherwise, stop there and allow fresh motion.
   */
  int qxo=SPRITE->qx,qyo=SPRITE->qy;
  int xmok=0,ymok=0; // True if we're at the target on the given axis, ie new motion is ok.
  double targetx=SPRITE->qx+0.5;
  double targety=SPRITE->qy+0.5;
  double dx=targetx-sprite->x;
  double dy=targety-sprite->y;
  const double close_enough=0.010; // <1/32
  if (dx>close_enough) {
    if ((sprite->x+=speed*elapsed)>=targetx) {
      if ((SPRITE->indx==1)&&hero_move_ok(sprite,SPRITE->qx+1,SPRITE->qy)) SPRITE->qx++;
      else { sprite->x=targetx; xmok=1; }
    }
  } else if (dx<-close_enough) {
    if ((sprite->x-=speed*elapsed)<=targetx) {
      if ((SPRITE->indx==-1)&&hero_move_ok(sprite,SPRITE->qx-1,SPRITE->qy)) SPRITE->qx--;
      else { sprite->x=targetx; xmok=1; }
    }
  } else { sprite->x=targetx; xmok=1; }
  if (dy>close_enough) {
    if ((sprite->y+=speed*elapsed)>=targety) {
      if ((SPRITE->indy==1)&&hero_move_ok(sprite,SPRITE->qx,SPRITE->qy+1)) SPRITE->qy++;
      else { sprite->y=targety; ymok=1; }
    }
  } else if (dy<-close_enough) {
    if ((sprite->y-=speed*elapsed)<=targety) {
      if ((SPRITE->indy==-1)&&hero_move_ok(sprite,SPRITE->qx,SPRITE->qy-1)) SPRITE->qy--;
      else { sprite->y=targety; ymok=1; }
    }
  } else { sprite->y=targety; ymok=1; }
  
  /* If we're stable on both axes, permit new motion according to (indx,indy).
   * Use (faced) to break ties.
   * If the turnclock is still running, delay.
   */
  if (xmok&&ymok&&(SPRITE->turnclock<=0.0)) {
    int ndx=0,ndy=0;
    if (SPRITE->indx&&SPRITE->indy) {
      if (SPRITE->facedx==SPRITE->indx) ndx=SPRITE->indx;
      else if (SPRITE->facedy==SPRITE->indy) ndy=SPRITE->indy;
    } else if (SPRITE->indx) ndx=SPRITE->indx;
    else if (SPRITE->indy) ndy=SPRITE->indy;
    if (ndx||ndy) {
      int dstcol=SPRITE->qx+ndx;
      int dstrow=SPRITE->qy+ndy;
      if (hero_move_ok(sprite,dstcol,dstrow)) {
        SPRITE->qx=dstcol;
        SPRITE->qy=dstrow;
      }
    }
  }
  if ((qxo!=SPRITE->qx)||(qyo!=SPRITE->qy)) {
    egg_play_sound(RID_sound_step,1.0,0.0);
    hero_rebuild_watertiles(sprite);
  } else if ((fxo!=SPRITE->facedx)||(fyo!=SPRITE->facedy)) {
    egg_play_sound(RID_sound_turn,1.0,0.0);
    hero_rebuild_watertiles(sprite);
  }
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  // Animate the highlight.
  if ((SPRITE->highlightclock-=elapsed)<=0.0) {
    SPRITE->highlightclock+=0.125;
    if (++(SPRITE->highlightframe)>=8) SPRITE->highlightframe=0;
  }

  // Walking etc.
  if (g.quantize_hero) {
    hero_update_motion_quantized(sprite,elapsed);
  } else {
    hero_update_motion_continuous(sprite,elapsed);
  }
  
  // Creation and corruption.
  if (g.corrupt_always) {
    hero_water_plants(sprite,elapsed,g.session->input&EGG_BTN_SOUTH);
    hero_apply_corruption(sprite,elapsed);
  } else {
    hero_water_plants(sprite,elapsed,g.session->input&EGG_BTN_SOUTH);
  }
}

/* Render tile focus, after map but before sprites.
 */
 
void hero_prerender(struct sprite *sprite,int x0,int y0) {
  
  // Deferred prep.
  if (SPRITE->waterx0<0) {
    SPRITE->waterx0=x0;
    SPRITE->watery0=y0;
    hero_rebuild_watertiles(sprite);
  }
  
  if (SPRITE->watertilec<1) return;
  
  // Animate.
  struct egg_render_tile *vtx=SPRITE->watertilev;
  int i=SPRITE->watertilec;
  for (;i-->0;vtx++) {
    vtx->tileid=(vtx->tileid&0xf1)|(SPRITE->highlightframe<<1);
  }
  
  struct egg_render_uniform un={
    .mode=EGG_RENDER_TILE,
    .dsttexid=1,
    .srctexid=g.texid_halftiles,
    .alpha=0xff,
  };
  egg_render(&un,SPRITE->watertilev,sizeof(struct egg_render_tile)*SPRITE->watertilec);
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y,struct tilerenderer *tr) {
  uint8_t tileid=0x12;
  uint8_t xform=0; // Natural orientation of watercan is left.
  if (SPRITE->stickydx>0) xform=EGG_XFORM_XREV;
  
  // Shadow tile is centered, xform doesn't matter. Offset depending on main's xform.
  int shadowdx=6;
  if (xform) shadowdx=-shadowdx;
  tilerenderer_add(tr,x+shadowdx,y+4,0x17,0);
  
  if (SPRITE->pourclock>0.500) { // alternate 3,4
    int frame=(int)(SPRITE->pourclock*2.000)&1;
    tileid+=frame?3:4;
  } else if (SPRITE->pourclock>0.250) tileid+=2;
  else if (SPRITE->pourclock>0.0) tileid+=1;
  tilerenderer_add(tr,x,y,tileid,xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
