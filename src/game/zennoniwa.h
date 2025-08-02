#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "session.h"

#define FBW 640
#define FBH 352

extern struct g {
  void *rom;
  int romc;
  int pvinput;
  int texid_tilesheet;
  struct session *session;
} g;

/* Render helpers.
 */
void fill_rect(int x,int y,int w,int h,uint32_t rgba);
void render_grid(int x,int y,const uint8_t *src,int colc,int rowc);

#endif
