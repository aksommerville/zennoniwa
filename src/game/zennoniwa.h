#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "shared_symbols.h"

#define FBW 640
#define FBH 352

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
} g;

#endif
