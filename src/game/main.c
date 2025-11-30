#include "zennoniwa.h"

struct g g={0};

void egg_client_quit(int status) {
}

void egg_client_notify(int k,int v) {
}

/* XXX TEMP: Read all the maps and count the sand tiles.
 */
static int XXX_count_sand_1(const uint8_t *src,int srcc) {
  if ((srcc<6)||memcmp(src,"\0EMP",4)) return 0;
  int w=src[4];
  int h=src[5];
  int srcp=6;
  if (srcp>srcc-w*h) return 0;
  int i=w*h,c=0;
  for (;i-->0;srcp++) {
    if (tileid_is_sand(src[srcp])) c++;
  }
  return c;
}
static void XXX_count_sand() {
  int tid=1,srcp=4,c=0,mapc=0;
  while (srcp<g.romc) {
    uint8_t lead=g.rom[srcp++];
    if (!lead) break;
    switch (lead&0xc0) {
      case 0x00: tid+=lead; break;
      case 0x40: srcp+=1; break; // RID, don't bother tracking
      case 0x80: {
          if (srcp>g.romc-2) return;
          int l=(lead&0x3f)<<16;
          l|=g.rom[srcp++]<<8;
          l|=g.rom[srcp++];
          l++;
          if (srcp>g.romc-l) return;
          if (tid==EGG_TID_map) {
            mapc++;
            c+=XXX_count_sand_1(g.rom+srcp,l);
          }
          srcp+=l;
        } break;
      default: return;
    }
  }
  fprintf(stderr,"%s:%d: %d sand tiles in %d maps\n",__FILE__,__LINE__,c,mapc);
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
  if (sprres_init()<0) return -1;
  
  //XXX_count_sand();
  
  hiscore_load();
  
  if (egg_texture_load_image(g.texid_tilesheet=egg_texture_new(),RID_image_tilesheet)<0) return -1;
  if (egg_texture_load_image(g.texid_font=egg_texture_new(),RID_image_fonttiles)<0) return -1;
  if (egg_texture_load_image(g.texid_halftiles=egg_texture_new(),RID_image_halftiles)<0) return -1;
  
  if (!(g.modal=modal_new(&modal_type_hello))) return -1;

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

/* Tile ID.
 */
 
int tileid_is_sand(uint8_t tileid) {
  switch (tileid) {
    case 0x01:
    case 0xa7: case 0xa8: case 0xa9: case 0xaa:
    case 0xb7: case 0xb8: case 0xb9: case 0xba:
    case 0xc7: case 0xc8: case 0xc9: case 0xca:
    case 0xd7: case 0xd8: case 0xd9: case 0xda:
      return 1;
  }
  return 0;
}

/* Play song.
 */
 
void play_song(int rid,int repeat) {
  if (rid==g.playing_song_id) return;
  g.playing_song_id=rid;
  egg_play_song(1,rid,repeat,1.0f,0.0f);
}
