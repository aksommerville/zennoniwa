/* session.h
 * Models the game across different levels. Everything between trips to the main menu.
 */
 
#ifndef SESSION_H
#define SESSION_H

struct sprite;

#define SESSION_TERMCLOCK_LIMIT 2.000
#define SESSION_TERMCLOCK_BEGIN 1.000

struct score {
  double time; // s
  double life; // 0..1
  int pinkc;
};

/* Decode is guaranteed not to touch (score) if input malformed.
 */
int score_encode(char *dst,int dsta/*17*/,struct score *score);
int score_decode(struct score *score,const char *src,int srcc);

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
  double termclock; // Counts up when (qualified), to effect a tasteful interval before denouement.
  
  int input_blackout; // Ignore the south button until it's been released once.
  int input,pvinput;
  int rid;
  int load_failed; // Nonzero if the last map load failed; gameover.
  
  struct score lscore; // Score for this or most recent level.
  struct score tscore; // Score for the entire session. (life) is the sum per level; need to divide by levels played.
};

void session_del(struct session *session);
struct session *session_new();

void session_update(struct session *session,double elapsed,int input,int pvinput);
void session_render(struct session *session);

int session_load_map(struct session *session,int rid);
int session_load_next(struct session *session);

#endif
