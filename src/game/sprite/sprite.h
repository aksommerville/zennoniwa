/* sprite.h
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

struct sprres {
  uint16_t rid;
  uint8_t tileid;
  const struct sprite_type *type;
  // (imageid,xform) are stored but I don't expect to use them.
};

int sprres_init();
const struct sprres *sprres_get(int rid);

struct sprite {
  const struct sprite_type *type;
  double x,y;
  uint8_t tileid,xform;
  int defunct;
  uint32_t arg;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* If you implement a custom render, you must either use (tr) or flush it first.
   * (x,y) corresponds to (sprite->x,y) in framebuffer space.
   */
  void (*render)(struct sprite *sprite,int x,int y,struct tilerenderer *tr);
};

void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(const struct sprite_type *type,double x,double y,uint32_t arg);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _
const struct sprite_type *sprite_type_by_id(int id);

/* Nonzero if position changed.
 * XXX I've switched to quantized hero motion, no need for this anymore.
 */
int physics_rectify(struct sprite *sprite);

/* Session calls between rendering grid+plants and sprites.
 * (x0,y0) are the origin of the playfield.
 */
void hero_prerender(struct sprite *sprite,int x0,int y0);

#endif
