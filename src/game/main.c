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
  
  if (egg_texture_load_image(g.texid_tilesheet=egg_texture_new(),RID_image_tilesheet)<0) return -1;
  if (egg_texture_load_image(g.texid_font=egg_texture_new(),RID_image_fonttiles)<0) return -1;
  
  g.quantize_hero=1;
  g.corrupt_always=0;

  if (!(g.modal=modal_new(&modal_type_hello))) return -1;
  //if (!(g.session=session_new())) return -1;

  return 0;
}

void egg_client_update(double elapsed) {

  int input=egg_input_get_one(0);
  int pvinput=g.pvinput;
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX1)&&!(pvinput&EGG_BTN_AUX1)&&!g.modal) {
      g.modal=modal_new(&modal_type_hello);
    } else {
      if (g.modal&&g.modal->type->input) g.modal->type->input(g.modal,input,pvinput);
    }
    g.pvinput=input;
  }
  
  if (g.modal) {
    g.modal->type->update(g.modal,elapsed);
    if (g.modal->defunct) {
      const struct modal_type *type=g.modal->type;
      modal_del(g.modal);
      g.modal=0;
      g.pvinput=pvinput=input; // We'll be updating the session. Ignore any input from this frame.
      if (type==&modal_type_gameover) { // If we just dropped the gameover modal, create a new hello.
        g.modal=modal_new(&modal_type_hello);
      }
    }
  }
  if (g.session&&!g.modal) { // <-- important: If we just removed the modal this cycle, DO update the session.
    if (g.session->load_failed) {
      g.modal=modal_new(&modal_type_gameover);
      session_del(g.session);
      g.session=0;
    } else {
      session_update(g.session,elapsed,input,pvinput);
    }
  }
}

void egg_client_render() {
  if (g.modal) {
    if (g.modal->type->render) g.modal->type->render(g.modal);
  } else if (g.session) {
    session_render(g.session);
  }
}
