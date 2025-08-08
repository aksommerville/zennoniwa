/* sprite_rock.c
 */
 
#include "game/zennoniwa.h"
 
struct sprite_rock {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_rock*)sprite)

const struct sprite_type sprite_type_rock={
  .name="rock",
  .objlen=sizeof(struct sprite_rock),
};
