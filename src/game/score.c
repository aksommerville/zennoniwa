#include "zennoniwa.h"

/* Calculate simple scalar score.
 */
 
void score_calculate(struct score *score) {
  /* 2025-08-09T14:38: My first playthrough with the final maps: 09:05.783;727;139
   * Was fairly efficient but by no means optimal.
   * Playing a bit harder 6:27.753 + 128. 333216. Still nowhere near optimal.
   * There are 290 sand tiles total (see main.c to recalc if maps change). I'm not sure whether it's technically possible for them all to be pink.
   * Score will saturate at 999999. It's OK for that to be possible, but if so it should be devilishly hard to do.
   * I think we can ignore the "life" variable. It's telling us essentially the same thing as "pinkc", but it has an unpredictable floor.
   * So the formula:
   *  - Completion bonus = 1000
   *  - Time bonus = max(0, (10:00 - elapsed)s * 1000). Range 0..600000 but the top end is definitely not reachable. 300k might be.
   *  - Pink bonus = 2000 * pinkc. Range 0..580000.
   * That makes saturation at least mathematically possible, but I think extremely unlikely (and maybe technically impossible).
   * The minimum score of 1000 is highly reachable, but probably wouldn't happen by accident.
   * Try achieving the minimum just to see it: 75000. Getting no pinks is actually a lot harder than you'd think, might be impossible.
   */
  score->score=1000; // Completion bonus.
  double bonussec=10.0*60.0-score->time;
  if (bonussec>0.0) score->score+=(int)(bonussec*1000.0);
  score->score+=score->pinkc*2000;
  if (score->score>999999) score->score=999999;
  else if (score->score<0) score->score=0; // oops?
}

/* Encode.
 */
 
int score_encode(char *dst,int dsta,struct score *score) {
  if (dsta>=17) {
    int ms=(int)(score->time*1000.0);
    if (ms<0) ms=0;
    int sec=ms/1000; ms%=1000;
    int min=sec/60; sec%=60;
    if (min>99) { min=99; sec=99; ms=999; }
    int life=(int)(score->life*1000.0);
    if (life<0) life=0; else if (life>999) life=999;
    int pinkc=score->pinkc;
    if (pinkc<0) pinkc=0; else if (pinkc>999) pinkc=999;
    dst[0]='0'+min/10;
    dst[1]='0'+min%10;
    dst[2]=':';
    dst[3]='0'+sec/10;
    dst[4]='0'+sec%10;
    dst[5]='.';
    dst[6]='0'+ms/100;
    dst[7]='0'+(ms/10)%10;
    dst[8]='0'+ms%10;
    dst[9]=';';
    dst[10]='0'+life/100;
    dst[11]='0'+(life/10)%10;
    dst[12]='0'+life%10;
    dst[13]=';';
    dst[14]='0'+pinkc/100;
    dst[15]='0'+(pinkc/10)%10;
    dst[16]='0'+pinkc%10;
  }
  return 17;
}

/* Decode.
 */

int score_decode(struct score *score,const char *src,int srcc) {
  if (!src||(srcc!=17)) return -1;
  #define DIGIT(p) if ((src[p]<'0')||(src[p]>'9')) return -1;
  DIGIT(0)
  DIGIT(1)
  if (src[2]!=':') return -1;
  DIGIT(3)
  DIGIT(4)
  if (src[5]!='.') return -1;
  DIGIT(6)
  DIGIT(7)
  DIGIT(8)
  if (src[9]!=';') return -1;
  DIGIT(10)
  DIGIT(11)
  DIGIT(12)
  if (src[13]!=';') return -1;
  DIGIT(14)
  DIGIT(15)
  DIGIT(16)
  #undef DIGIT
  int min=(src[0]-'0')*10+(src[1]-'0');
  int sec=(src[3]-'0')*10+(src[4]-'0');
  int ms=(src[6]-'0')*100+(src[7]-'0')*10+(src[8]-'0');
  int life=(src[10]-'0')*100+(src[11]-'0')*10+(src[12]-'0');
  int pinkc=(src[14]-'0')*100+(src[15]-'0')*10+(src[16]-'0');
  score->time=(double)(min*60+sec)+(double)ms/1000.0;
  score->life=(double)life/1000.0;
  score->pinkc=pinkc;
  score_calculate(score);
  return 0;
}

/* Default.
 */
 
static void score_default(struct score *score) {
  score->time=999999.0;
  score->life=0.0;
  score->pinkc=0;
  score->score=0;
}

/* High score.
 */
 
void hiscore_load() {
  char tmp[17];
  int tmpc=egg_store_get(tmp,sizeof(tmp),"hiscore",7);
  if ((tmpc!=17)||(score_decode(&g.hiscore,tmp,tmpc)<0)) {
    score_default(&g.hiscore);
  }
}

void hiscore_save() {
  char tmp[17];
  int tmpc=score_encode(tmp,sizeof(tmp),&g.hiscore);
  if (tmpc!=sizeof(tmp)) return;
  egg_store_set("hiscore",7,tmp,tmpc);
}
