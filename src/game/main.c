#include "zennoniwa.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    //fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  
  if (egg_texture_load_image(g.texid_tilesheet=egg_texture_new(),RID_image_scratch)<0) return -1;

  //TODO hello modal
  if (!(g.session=session_new())) return -1;

  return 0;
}

void egg_client_update(double elapsed) {

  int input=egg_input_get_one(0);
  int pvinput=g.pvinput;
  if (input!=g.pvinput) {
    g.pvinput=input;
  }
  
  //TODO modals
  if (g.session) session_update(g.session,elapsed,input,pvinput);
}

void egg_client_render() {
  if (g.session) session_render(g.session);
}
