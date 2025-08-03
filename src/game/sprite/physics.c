#include "game/zennoniwa.h"

/* Rectify position of one sprite.
 */
 
int physics_rectify(struct sprite *sprite) {
  int result=0;
  const double hbl=-0.500;
  const double hbr= 0.500;
  const double hbt=-0.500;
  const double hbb= 0.500;
  
  /* Screen edges.
   */
  if (sprite->x+hbl<0.0) { sprite->x=-hbl; result=1; }
  else if (sprite->x+hbr>g.session->mapw) { sprite->x=g.session->mapw-hbr; result=1; }
  if (sprite->y+hbt<0.0) { sprite->y=-hbt; result=1; }
  else if (sprite->y+hbb>g.session->maph) { sprite->y=g.session->maph-hbb; result=1; }
  
  /* Solid map cells.
   */
  //TODO
  
  return result;
}
