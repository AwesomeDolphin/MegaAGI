/***************************************************************************
    MEGA65-AGI -- Sierra AGI interpreter for the MEGA65
    Copyright (C) 2025  Keith Henrickson

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
***************************************************************************/

#ifndef GAMESAVE_H
#define GAMESAVE_H

#include "logic.h"
#include "sprite.h"

typedef struct add_to_pic_command {
    uint8_t view_number;
    uint8_t loop_index;
    uint8_t cel_index;
    uint8_t x_pos;
    uint8_t y_pos;
    uint8_t priority;
    uint8_t margin;
} add_to_pic_command_t;

extern uint16_t chipmem_allocoffset;
extern uint16_t chipmem_lockoffset;
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
extern __far uint8_t views_in_pic;
extern __far add_to_pic_command_t add_to_pic_commands[16];
#pragma clang section bss=""

uint32_t gamesave_save_to_attic(void);
uint8_t gamesave_save_to_disk(char *filename);
uint8_t gamesave_load_from_attic(void);
uint8_t gamesave_load_from_disk(char *filename);
#endif