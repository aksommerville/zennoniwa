/* session.h
 * Models the game across different levels. Everything between trips to the main menu.
 */
 
#ifndef SESSION_H
#define SESSION_H

struct sprite;

struct session {

  struct sprite **spritev;
  int spritec,spritea;
  struct sprite *hero; // WEAK, must also be in (spritev).
  
  struct cell {
    uint8_t tileid;
    double life;
  } *cellv;
  int mapw,maph;
  
  int qualified; // Updated each cycle. Nonzero if every cell that wants a plant has one, and none that doesn't.
  double life; // All plant life, divided by plant-here cell count. Can exceed 1, if disqualified by a false plant.
};

void session_del(struct session *session);
struct session *session_new();

void session_update(struct session *session,double elapsed,int input,int pvinput);
void session_render(struct session *session);

int session_load_map(struct session *session,int rid);

#endif
