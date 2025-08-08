#include "game/zennoniwa.h"

/* Global resource registry.
 */
 
#define SPRRES_LIMIT 16 /* Arbitrary; at least as many 'sprite' resources as we have. */

static struct sprres sprresv[SPRRES_LIMIT];
static int sprresc=0;

static int sprres_apply_command(struct sprres *sprres,uint8_t opcode,const uint8_t *arg,int argc) {
  switch (opcode) {
    case CMD_sprite_tile: sprres->tileid=arg[0]; break;
    case CMD_sprite_sprtype: if (!(sprres->type=sprite_type_by_id((arg[0]<<8)|arg[1]))) {
        fprintf(stderr,"sprite:%d: type %d unknown\n",sprres->rid,(arg[0]<<8)|arg[1]);
        return -1;
      } break;
  }
  return 0;
}

static int sprres_add(int rid,const uint8_t *src,int srcc) {
  if (sprresc>=SPRRES_LIMIT) {
    fprintf(stderr,"too many sprites\n");
    return -1;
  }
  if ((srcc<4)||memcmp(src,"\0ESP",4)) {
    fprintf(stderr,"sprite:%d not a command list\n",rid);
    return -1;
  }
  struct sprres *sprres=sprresv+sprresc++;
  memset(sprres,0,sizeof(struct sprres));
  sprres->rid=rid;
  sprres->type=&sprite_type_dummy;
  int srcp=4;
  while (srcp<srcc) {
    uint8_t lead=src[srcp++];
    const uint8_t *arg=src+srcp;
    int argc=0;
    switch (lead&0xe0) {
      case 0x00: break;
      case 0x20: argc=2; break;
      case 0x40: argc=4; break;
      case 0x60: argc=8; break;
      case 0x80: argc=12; break;
      case 0xa0: argc=16; break;
      case 0xc0: argc=20; break;
      case 0xe0: if (srcp>=srcc) return -1; argc=src[srcp++]; break;
    }
    if (srcp>srcc-argc) return -1;
    if (sprres_apply_command(sprres,lead,arg,argc)<0) return -1;
    srcp+=argc;
  }
  return 0;
}

int sprres_init() {
  int rtid=1,rrid=1,srcp=4;
  while (srcp<g.romc) {
    uint8_t lead=g.rom[srcp++];
    if (!lead) break;
    switch (lead&0xc0) {
      case 0x00: rtid+=lead; rrid=1; break;
      case 0x40: {
          if (srcp>g.romc-1) return -1;
          int d=(lead&0x3f)<<8;
          d|=g.rom[srcp++];
          d++;
          rrid+=d;
        } break;
      case 0x80: {
          if (srcp>g.romc-2) return -1;
          int l=(lead&0x3f)<<16;
          l|=g.rom[srcp++]<<8;
          l|=g.rom[srcp++];
          l++;
          if (srcp>g.romc-l) return 0;
          if (rtid==EGG_TID_sprite) {
            if (sprres_add(rrid,g.rom+srcp,l)<0) return -1;
          }
          srcp+=l;
          rrid++;
        } break;
      default: return -1;
    }
  }
  return 0;
}

const struct sprres *sprres_get(int rid) {
  const struct sprres *sprres=sprresv;
  int i=sprresc;
  for (;i-->0;sprres++) {
    if (sprres->rid<rid) continue;
    if (sprres->rid>rid) return 0;
    return sprres;
  }
  return 0;
}

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* New.
 */
 
struct sprite *sprite_new(const struct sprite_type *type,double x,double y,uint32_t arg) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  sprite->type=type;
  sprite->x=x;
  sprite->y=y;
  sprite->arg=arg;
  if (type->init&&(type->init(sprite)<0)) {
    sprite_del(sprite);
    return 0;
  }
  return sprite;
}

/* Type by id.
 */
 
const struct sprite_type *sprite_type_by_id(int id) {
  switch (id) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}
