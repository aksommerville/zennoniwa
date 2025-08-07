#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

struct tilerenderer;
struct fancyrenderer;

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "sprite/sprite.h"
#include "session.h"
#include "modal/modal.h"

#define FBW 640
#define FBH 352

extern struct g {
  uint8_t *rom;
  int romc;
  int pvinput;
  int texid_tilesheet;
  int texid_font;
  int texid_halftiles;
  struct session *session;
  struct modal *modal;
  int quantize_hero; // Dev only. TODO Once we choose which way we like, remove the other.
  int corrupt_always; // Dev only. ''
  struct score hiscore;
} g;

int res_get(void *dstpp,int tid,int rid);

/* Render helpers.
 */
void fill_rect(int x,int y,int w,int h,uint32_t rgba);
void frame_rect(int x,int y,int w,int h,uint32_t rgba);
void blit_texture(int dstx,int dsty,int texid,int w,int h,uint8_t alpha);
void render_grid(int x,int y,const uint8_t *src,int colc,int rowc);//XXX
void render_text(int x,int y,const char *src,int srcc,uint32_t rgba); // (x,y) is the center of the first glyph
void render_time(int x,int y,double s,uint32_t rgba);
void render_uint(int x,int y,int v,int limit,uint32_t rgba);
struct tilerenderer { struct egg_render_tile vtxv[64]; int vtxc; uint32_t tint; int texid; };
void tilerenderer_add(struct tilerenderer *tr,int x,int y,uint8_t tileid,uint8_t xform);
void tilerenderer_flush(struct tilerenderer *tr);
struct fancyrenderer { struct egg_render_fancy vtxv[64]; int vtxc; };
void fancyrenderer_add(struct fancyrenderer *fr,
  int x,int y,
  uint8_t tileid,
  uint8_t xform,
  uint8_t rotation,
  uint8_t size,
  uint32_t tint_rgba,
  uint32_t prim_rgba
);
void fancyrenderer_flush(struct fancyrenderer *fr);

/* These both operate directly on (g.hiscore).
 * Load should only happen once, at startup.
 * Gameover modal should populate (g.hiscore) and save if warranted, as it tabulates the score for display.
 */
void hiscore_load();
void hiscore_save();

#endif
