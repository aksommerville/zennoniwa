#include "game/zennoniwa.h"

#define CORRUPTION_RATE 0.125 /* hz */
#define CREATION_RATE   1.000 /* hz */

struct sprite_hero {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->tileid=0x10;
  sprite->xform=0;
  return 0;
}

/* Water plants.
 */
 
static void hero_water_plants(struct sprite *sprite,double elapsed) {
  //TODO There should be a visible "focus cell", led a bit in the hero's direction of last motion. For now, it's just his position.
  int x=(int)sprite->x,y=(int)sprite->y;
  if ((x<0)||(y<0)||(x>=g.session->mapw)||(y>=g.session->maph)) return;
  struct cell *cell=g.session->cellv+y*g.session->mapw+x;
  if (cell->life>=1.0) return;
  if ((cell->life+=elapsed*CREATION_RATE)>=1.0) {
    cell->life=1.0;
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
  int x=(int)sprite->x,y=(int)sprite->y;
  int dy=-1; for (;dy<=1;dy++) {
    int dx=-1; for (;dx<=1;dx++) {
      hero_apply_corruption_1(sprite,x+dx,y+dy,elapsed);
    }
  }
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  // Motion. XXX Will need further refinement.
  const double speed=6.0;
  switch (g.pvinput&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: sprite->x-=elapsed*speed; break;
    case EGG_BTN_RIGHT: sprite->x+=elapsed*speed; break;
  }
  switch (g.pvinput&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: sprite->y-=elapsed*speed; break;
    case EGG_BTN_DOWN: sprite->y+=elapsed*speed; break;
  }
  
  // Water plants if holding SOUTH.
  if (g.pvinput&EGG_BTN_SOUTH) {
    hero_water_plants(sprite,elapsed);
  }
  
  // Corrupt nearby plants.
  hero_apply_corruption(sprite,elapsed);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
};
