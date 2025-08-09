#include "zennoniwa.h"

/* Get resource.
 */
 
int res_get(void *dstpp,int tid,int rid) {
  int rtid=1,rrid=1,srcp=4;
  while (srcp<g.romc) {
    uint8_t lead=g.rom[srcp++];
    if (!lead) return 0;
    switch (lead&0xc0) {
      case 0x00: rtid+=lead; rrid=1; break;
      case 0x40: {
          if (srcp>g.romc-1) return 0;
          int d=(lead&0x3f)<<8;
          d|=g.rom[srcp++];
          d++;
          rrid+=d;
        } break;
      case 0x80: {
          if (srcp>g.romc-2) return 0;
          int l=(lead&0x3f)<<16;
          l|=g.rom[srcp++]<<8;
          l|=g.rom[srcp++];
          l++;
          if (srcp>g.romc-l) return 0;
          if ((rtid==tid)&&(rrid==rid)) {
            *(void**)dstpp=g.rom+srcp;
            return l;
          }
          srcp+=l;
          rrid++;
        } break;
      default: return 0;
    }
  }
  return 0;
}
