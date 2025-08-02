/* session.h
 * Models the game across different levels. Everything between trips to the main menu.
 */
 
#ifndef SESSION_H
#define SESSION_H

struct sprite;

struct session {
  struct sprite **spritev;
  int spritec,spritea;
};

void session_del(struct session *session);
struct session *session_new();

void session_update(struct session *session,double elapsed,int input,int pvinput);
void session_render(struct session *session);

#endif
