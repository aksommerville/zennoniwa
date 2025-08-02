#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "session.h"
#include "sprite/sprite.h"

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
struct tilerenderer { struct egg_render_tile vtxv[64]; int vtxc; };
void tilerenderer_add(struct tilerenderer *tr,int x,int y,uint8_t tileid,uint8_t xform);
void tilerenderer_flush(struct tilerenderer *tr);

#endif
