#ifndef GAMESAVE_H
#define GAMESAVE_H

#include "logic.h"
#include "sprite.h"

extern uint16_t chipmem_allocoffset;
extern uint16_t chipmem_lockoffset;
extern uint16_t chipmem2_allocoffset;
extern uint32_t atticmem_allocoffset;

extern bool input_ok;
extern bool player_control;
extern char game_id[8];
extern uint8_t horizon_line;

extern uint8_t block_active, block_x1, block_y1, block_x2, block_y2;

extern uint8_t animated_sprite_count;
extern uint16_t free_point;

#pragma clang section bss="extradata"
extern __far agisprite_t sprites[256];
extern __far uint8_t animated_sprites[256];
extern __far uint16_t views[256];
extern __far logic_info_t logic_infos[256];
extern __far uint8_t object_locations[256];
#pragma clang section bss=""

void gamesave_save_to_attic(void);
void gamesave_load_from_attic(void);

#endif