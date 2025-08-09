/* shared_symbols.h
 * This file is consumed by eggdev and editor, in addition to compiling in with the game.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define NS_sys_tilesize 32
// Our playfield does not fill the framebuffer.
#define NS_sys_mapw 16
#define NS_sys_maph 8

#define CMD_map_image     0x20 /* u16:imageid */
#define CMD_map_hero      0x21 /* u16:position */
#define CMD_map_plant     0x40 /* u16:position u8:level u8:reserved */
#define CMD_map_sprite    0x42 /* u16:position u16:spriteid */

#define CMD_sprite_image   0x20 /* u16:imageid */
#define CMD_sprite_tile    0x21 /* u8:tileid, u8:xform */
#define CMD_sprite_sprtype 0x22 /* u16:sprtype */

#define NS_tilesheet_physics 1
#define NS_tilesheet_family 0
#define NS_tilesheet_neighbors 0
#define NS_tilesheet_weight 0

#define NS_sprtype_dummy 0
#define NS_sprtype_hero 1
#define NS_sprtype_toast 2
#define FOR_EACH_SPRTYPE \
  _(dummy) \
  _(hero) \
  _(toast)

#endif
