/* sprite_toast.c
 * arg: (u8)tileid (u8)tilec (u16)unused
 */
 
#include "game/zennoniwa.h"

#define TOAST_FRAME_TIME 0.100
#define TOAST_SPEED_Y -1.0 /* m/s */

struct sprite_toast {
  struct sprite hdr;
  uint8_t tileid0;
  double animclock; // Animation is also the TTL, we stop at the end of the sequence.
  int animframe;
  int animframec;
};

#define SPRITE ((struct sprite_toast*)sprite)

static int _toast_init(struct sprite *sprite) {
  sprite->tileid=SPRITE->tileid0=sprite->arg>>24;
  SPRITE->animframec=(sprite->arg>>16)&0xff;
  SPRITE->animclock=TOAST_FRAME_TIME;
  return 0;
}

static void _toast_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=TOAST_FRAME_TIME;
    if (++(SPRITE->animframe)>=SPRITE->animframec) {
      sprite->defunct=1;
      SPRITE->animframe=SPRITE->animframec-1;
    }
    sprite->tileid=SPRITE->tileid0+SPRITE->animframe;
  }
  sprite->y+=TOAST_SPEED_Y*elapsed;
}

const struct sprite_type sprite_type_toast={
  .name="toast",
  .objlen=sizeof(struct sprite_toast),
  .init=_toast_init,
  .update=_toast_update,
};
