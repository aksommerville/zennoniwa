# Zen no Niwa

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

Create and corrupt flowers to match a pattern.

For GDEX Game Jam 2025, theme "CREATION AND CORRUPTION".

## TODO

 - [x] Logical game model.
 - [x] Hello modal.
 - [x] Quantize hero position. Keep it toggleable until we decide.
 - [ ] Post-level denouement modal.
 - [ ] Gameover modal.
 - [x] Scorekeeping.
 - [x] Music.
 - [ ] Sound effects.
 - [ ] Graphics.
 - [ ] Maps.
 - [ ] `res_get`: Build a TOC initially rather than decoding the ROM on every access.
 
 - [x] Web: Moving pushes the hero OOB near the top left corner. Started happening after adding the hello modal. Doesn't happen natively.
 - - just needed a clean build. Make a note to repair eggdev build
