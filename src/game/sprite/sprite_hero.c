#include "game/zennoniwa.h"

#define CORRUPTION_RATE 0.180 /* hz */
#define CREATION_RATE   0.800 /* hz */
#define TURN_TIME       0.100

#define WATERPATTERN_SINGLE 1 /* Just (qx,qy). */
#define WATERPATTERN_THREE  3 /* Three in a row, perpedicular to the direction of travel. */
#define WATER_LIMIT 9 /* The most cells addressable by a waterpattern. */

struct sprite_hero {
  struct sprite hdr;
  int indx,indy;
  int facedx,facedy; // always cardinal
  int stickydx; // facedx but retains its value, in case we only get horizontal faces.
  double pourclock;
  int qx,qy; // Quantized position, updates each cycle.
  int waterpattern;
  double turnclock;
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
  return 0;
}

/* Waterpattern definitions.
 */
 
struct delta2d { int dx,dy; };

static int waterpattern_get(struct delta2d *dst/*WATER_LIMIT*/,struct sprite *sprite,int pattern) {
  switch (pattern) {
    case WATERPATTERN_SINGLE: {
        dst[0]=(struct delta2d){0,0};
        return 1;
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

/* Water plants.
 */
 
static void hero_water_plants(struct sprite *sprite,double elapsed,int enable) {
  if (!enable) {
    SPRITE->pourclock=0.0;
    return;
  }
  SPRITE->pourclock+=elapsed;
  struct delta2d storage[WATER_LIMIT];
  int patternc=waterpattern_get(storage,sprite,SPRITE->waterpattern);
  const struct delta2d *delta=storage;
  for (;patternc-->0;delta++) {
    int x=SPRITE->qx+delta->dx;
    int y=SPRITE->qy+delta->dy;
    if ((x>=0)&&(y>=0)&&(x<g.session->mapw)&&(y<g.session->maph)) {
      struct cell *cell=g.session->cellv+y*g.session->mapw+x;
      if (cell->life<1.0) {
        if ((cell->life+=elapsed*CREATION_RATE)>=1.0) {
          cell->life=1.0;
        }
      }
    }
  }
}

/* Corrupt plants.
 */
 
static void hero_apply_corruption_1(struct sprite *sprite,int x,int y,double elapsed) {
  if ((x<0)||(y<0)||(x>=g.session->mapw)||(y>=g.session->maph)) return;
  struct cell *cell=g.session->cellv+y*g.session->mapw+x;
  if (cell->life<=0.0) return;
  if ((cell->life-=elapsed*CORRUPTION_RATE)<=0.0) {
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

/* Walking.
 */
 
static void hero_update_motion_continuous(struct sprite *sprite,double elapsed) {
  const double speed=6.0;
  
  /* Watch for changes to the input state.
   * When the input changes, face that direction.
   */
  int nindx=0,nindy=0;
  switch (g.pvinput&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: nindx=-1; break;
    case EGG_BTN_RIGHT: nindx=1; break;
  }
  switch (g.pvinput&(EGG_BTN_UP|EGG_BTN_DOWN)) {
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
  int nindx=0,nindy=0;
  switch (g.pvinput&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: nindx=-1; break;
    case EGG_BTN_RIGHT: nindx=1; break;
  }
  switch (g.pvinput&(EGG_BTN_UP|EGG_BTN_DOWN)) {
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
  int xmok=0,ymok=0; // True if we're at the target on the given axis, ie new motion is ok.
  double targetx=SPRITE->qx+0.5;
  double targety=SPRITE->qy+0.5;
  double dx=targetx-sprite->x;
  double dy=targety-sprite->y;
  const double close_enough=0.010; // <1/32
  if (dx>close_enough) {
    if ((sprite->x+=speed*elapsed)>=targetx) {
      //TODO if there are impassable cells, we need to check here (and the three other cases)
      if ((SPRITE->indx==1)&&(SPRITE->qx<g.session->mapw-1)) SPRITE->qx++;
      else { sprite->x=targetx; xmok=1; }
    }
  } else if (dx<-close_enough) {
    if ((sprite->x-=speed*elapsed)<=targetx) {
      if ((SPRITE->indx==-1)&&(SPRITE->qx>0)) SPRITE->qx--;
      else { sprite->x=targetx; xmok=1; }
    }
  } else { sprite->x=targetx; xmok=1; }
  if (dy>close_enough) {
    if ((sprite->y+=speed*elapsed)>=targety) {
      if ((SPRITE->indy==1)&&(SPRITE->qy<g.session->maph-1)) SPRITE->qy++;
      else { sprite->y=targety; ymok=1; }
    }
  } else if (dy<-close_enough) {
    if ((sprite->y-=speed*elapsed)<=targety) {
      if ((SPRITE->indy==-1)&&(SPRITE->qy>0)) SPRITE->qy--;
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
      if ((dstcol>=0)&&(dstrow>=0)&&(dstcol<g.session->mapw)&&(dstrow<g.session->maph)) {
        SPRITE->qx=dstcol;
        SPRITE->qy=dstrow;
      }
    }
  }
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  // Walking etc.
  if (g.quantize_hero) {
    hero_update_motion_quantized(sprite,elapsed);
  } else {
    hero_update_motion_continuous(sprite,elapsed);
  }
  
  // Water plants if holding SOUTH.
  hero_water_plants(sprite,elapsed,g.pvinput&EGG_BTN_SOUTH);
  
  // Corrupt nearby plants.
  hero_apply_corruption(sprite,elapsed);
}

/* Render tile focus, after map but before sprites.
 */
 
void hero_prerender(struct sprite *sprite,int x0,int y0) {
  
  /* Corruption zone.
   */
  int x=SPRITE->qx-1,y=SPRITE->qy-1,w=3,h=3;
  if (x<0) { w+=x; x=0; } else if (x+w>g.session->mapw) w=g.session->mapw-x;
  if (y<0) { h+=y; y=0; } else if (y+h>g.session->maph) h=g.session->maph-y;
  frame_rect(x0+x*NS_sys_tilesize,y0+y*NS_sys_tilesize,w*NS_sys_tilesize,h*NS_sys_tilesize,0xff000080);
  
  /* Creation zone.
   */
  struct delta2d storage[WATER_LIMIT];
  int patternc=waterpattern_get(storage,sprite,SPRITE->waterpattern);
  const struct delta2d *delta=storage;
  for (;patternc-->0;delta++) {
    int x=SPRITE->qx+delta->dx;
    int y=SPRITE->qy+delta->dy;
    if ((x>=0)&&(y>=0)&&(x<g.session->mapw)&&(y<g.session->maph)) {
      frame_rect(x0+x*NS_sys_tilesize,y0+y*NS_sys_tilesize,NS_sys_tilesize,NS_sys_tilesize,0x4050ff80);
    }
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y,struct tilerenderer *tr) {
  uint8_t tileid=0x12;
  uint8_t xform=0; // Natural orientation of watercan is left.
  if (SPRITE->stickydx>0) xform=EGG_XFORM_XREV;
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
