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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "dialog.h"
#include "engine.h"
#include "gamesave.h"
#include "gfx.h"
#include "irq.h"
#include "main.h"
#include "memmanage.h"
#include "pic.h"
#include "simplefile.h"

// Saveable data
char game_id[8];

uint16_t chipmem_allocoffset;
uint16_t chipmem_lockoffset;
uint16_t chipmem2_allocoffset;
uint32_t atticmem_allocoffset;

bool input_ok;
bool player_control; 
uint8_t horizon_line;
uint8_t block_active, block_x1, block_y1, block_x2, block_y2;

uint8_t animated_sprite_count;
uint16_t free_point;

bool saving;

#pragma clang section bss="extradata"
__far agisprite_t sprites[256];
__far uint8_t animated_sprites[256];
__far uint16_t views[256];
__far logic_info_t logic_infos[256];
__far uint8_t object_locations[256];
__far uint8_t views_in_pic;
__far add_to_pic_command_t add_to_pic_commands[16];
#pragma clang section bss=""

uint8_t __huge *gamesave_cache;

#pragma clang section bss="midmembss" data="midmemdata" rodata="midmemrodata" text="midmemtext"

uint32_t gamesave_save_to_attic(void) {
    gamesave_cache = attic_memory + atticmem_allocoffset;
    VICIV.bordercol = COLOR_RED;
    for (int i = 0; i < 8; i++) {
        gamesave_cache[i] = game_id[i];
    }

    gamesave_cache[8] = chipmem_allocoffset & 0xff;
    gamesave_cache[9] = (chipmem_allocoffset >> 8) & 0xff;
    gamesave_cache[10] = chipmem_lockoffset & 0xff;
    gamesave_cache[11] = (chipmem_lockoffset >> 8) & 0xff;
    gamesave_cache[12] = chipmem2_allocoffset & 0xff;
    gamesave_cache[13] = (chipmem2_allocoffset >> 8) & 0xff;
    gamesave_cache[14] = atticmem_allocoffset & 0xff;
    gamesave_cache[15] = (atticmem_allocoffset >> 8) & 0xff;
    gamesave_cache[16] = (atticmem_allocoffset >> 16) & 0xff;
    gamesave_cache[17] = (atticmem_allocoffset >> 24) & 0xff;
    gamesave_cache[18] = input_ok;
    gamesave_cache[19] = player_control;
    gamesave_cache[20] = horizon_line;
    gamesave_cache[21] = block_active;
    gamesave_cache[22] = block_x1;
    gamesave_cache[23] = block_y1;
    gamesave_cache[24] = block_x2;
    gamesave_cache[25] = block_y2;
    gamesave_cache[26] = animated_sprite_count;
    gamesave_cache[27] = free_point & 0xff;
    gamesave_cache[28] = (free_point >> 8) & 0xff;
    gamesave_cache[29] = views_in_pic;

    uint32_t offset = 30;
    memmanage_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)sprites, sizeof(sprites));
    offset += sizeof(sprites);
    memmanage_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)animated_sprites, sizeof(animated_sprites));
    offset += sizeof(animated_sprites);
    memmanage_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)views, sizeof(views));
    offset += sizeof(views);
    memmanage_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)logic_infos, sizeof(logic_infos));
    offset += sizeof(logic_infos);
    memmanage_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)object_locations, sizeof(object_locations));
    offset += sizeof(object_locations);
    memmanage_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)add_to_pic_commands, sizeof(add_to_pic_commands));
    offset += sizeof(add_to_pic_commands);

    memmanage_memcpy_far_huge(&gamesave_cache[offset], chipmem_base, 0x10000);
    offset += 0x10000;
    VICIV.bordercol = COLOR_BLACK;

    return offset;
}

void gamesave_save_to_disk(char *filename) {
    uint8_t buffer[256];

    uint32_t save_size = gamesave_save_to_attic();
    select_kernel_mem();
    POKE(0xD030, 0x64);
    VICIV.bordercol = COLOR_SOYLENTGREEN;

    simplecmdchan((uint8_t *)"I0:\r", 9);
    simpleerrchan(buffer, 9);
    uint8_t __huge *gamesave_file = gamesave_cache;
    strcat(filename, ".AGI,S,W");
    dialog_print_ascii(0,1, false, (uint8_t __far *)filename);
    simpleopen(filename, strlen(filename), 9);
    uint8_t __far *buffer_far = (uint8_t __far *)buffer;
    for (uint32_t i = 0; i < save_size; i += 250) {
        uint32_t chunk_size = (save_size - i > 250) ? 250 : (save_size - i);
        memmanage_memcpy_huge_far(buffer_far, &gamesave_file[i], chunk_size);
        simplewrite(buffer, chunk_size);
    }
    simpleclose();

    simpleerrchan(buffer, 9);
    dialog_print_ascii(0,2, false, (uint8_t __far *)buffer);

    VICIV.bordercol = COLOR_BLACK;
    POKE(0xD030, 0x44);
    select_execution_mem();
}


void gamesave_load_from_attic(void) {
    VICIV.bordercol = COLOR_RED;
    for (int i = 0; i < 8; i++) {
        if (gamesave_cache[i] != game_id[i]) {
            return;
        }
    }

    chipmem_allocoffset = gamesave_cache[8];
    chipmem_allocoffset |= (gamesave_cache[9] << 8);

    chipmem_lockoffset = gamesave_cache[10];
    chipmem_lockoffset |= (gamesave_cache[11] << 8);

    chipmem2_allocoffset = gamesave_cache[12];
    chipmem2_allocoffset |= (gamesave_cache[13] << 8);

    uint32_t temp;
    temp = gamesave_cache[14];
    atticmem_allocoffset = temp;
    temp = gamesave_cache[15];
    atticmem_allocoffset |= (temp << 8);
    temp = gamesave_cache[16];
    atticmem_allocoffset |= (temp << 16);
    temp = gamesave_cache[17];
    atticmem_allocoffset |= (temp << 24);

    input_ok = gamesave_cache[18];
    player_control = gamesave_cache[19];
    horizon_line = gamesave_cache[20];
    block_active = gamesave_cache[21];
    block_x1 = gamesave_cache[22];
    block_y1 = gamesave_cache[23];
    block_x2 = gamesave_cache[24];
    block_y2 = gamesave_cache[25];
    animated_sprite_count = gamesave_cache[26];
    free_point = gamesave_cache[27];
    free_point |= (gamesave_cache[28] << 8);
    views_in_pic = gamesave_cache[29];

    uint32_t offset = 30;
    memmanage_memcpy_huge_far((uint8_t __far *)sprites, &gamesave_cache[offset], sizeof(sprites));
    offset += sizeof(sprites);
    memmanage_memcpy_huge_far((uint8_t __far *)animated_sprites, &gamesave_cache[offset], sizeof(animated_sprites));
    offset += sizeof(animated_sprites);
    memmanage_memcpy_huge_far((uint8_t __far *)views, &gamesave_cache[offset], sizeof(views));
    offset += sizeof(views);
    memmanage_memcpy_huge_far((uint8_t __far *)logic_infos, &gamesave_cache[offset], sizeof(logic_infos));
    offset += sizeof(logic_infos);
    memmanage_memcpy_huge_far((uint8_t __far *)object_locations, &gamesave_cache[offset], sizeof(object_locations));
    offset += sizeof(object_locations);
    memmanage_memcpy_huge_far((uint8_t __far *)add_to_pic_commands, &gamesave_cache[offset], sizeof(add_to_pic_commands));
    offset += sizeof(add_to_pic_commands);

    memmanage_memcpy_huge_far(chipmem_base, &gamesave_cache[offset], 0x10000);
    logic_set_flag(12);

    gfx_hold_flip(true);
    VICIV.bordercol = COLOR_GREEN;
    pic_load(logic_vars[0]);
    draw_pic(true);
    pic_discard(logic_vars[0]);
    for (int i = 0; i < views_in_pic; i++) {
        if ((add_to_pic_commands[i].x_pos == 0xff) && (add_to_pic_commands[i].y_pos == 0xff)) {
            pic_offset = (add_to_pic_commands[i].loop_index << 8);
            pic_offset |= add_to_pic_commands[i].cel_index;
            draw_pic(false);
        } else {
            view_load(add_to_pic_commands[i].view_number);
            view_set(&object_view, add_to_pic_commands[i].view_number);
            select_loop(&object_view, add_to_pic_commands[i].loop_index);
            object_view.cel_offset = add_to_pic_commands[i].cel_index;
            object_view.x_pos = add_to_pic_commands[i].x_pos;
            object_view.y_pos = add_to_pic_commands[i].y_pos;
            object_view.priority_override = true;
            object_view.priority = add_to_pic_commands[i].priority;
            object_view.priority_set = true;
            object_view.baseline_priority = add_to_pic_commands[i].baseline_priority;
            sprite_draw_to_pic();
            view_unload(add_to_pic_commands[i].view_number);
        }
    }
    gfx_hold_flip(false);
    VICIV.bordercol = COLOR_BLACK;
}

void gamesave_load_from_disk(char *filename) {
    gamesave_cache = attic_memory + atticmem_allocoffset;
    strcat(filename, ".AGI,S,R");
    dialog_print_ascii(0,1, false, (uint8_t __far *)filename);
    select_kernel_mem();
    POKE(0xD030, 0x64);
    VICIV.bordercol = COLOR_SOYLENTGREEN;
    uint8_t buffer[256];

    simplecmdchan((uint8_t *)"I0:\r", 9);
    simpleerrchan(buffer, 9);
    simpleopen(filename, strlen(filename), 9);
    size_t bytes_read;
    do {
        bytes_read = simpleread(buffer);
        if (bytes_read > 0) {
            for (size_t idx = 0; idx < bytes_read; idx++) {
                gamesave_cache[idx] = buffer[idx];
            }
            gamesave_cache += bytes_read;
        }
    } while (bytes_read > 0);
    simpleclose();

    simpleerrchan(buffer, 9);

    VICIV.bordercol = COLOR_BLACK;
    POKE(0xD030, 0x44);
    select_execution_mem();

    dialog_print_ascii(0,2, false, (uint8_t __far *)buffer);

    if (buffer[0] < 0x32) {
        gamesave_cache = attic_memory + atticmem_allocoffset;
        gamesave_load_from_attic();
    }
}

void gamesave_dialog_handler(char *filename) {
    uint8_t len = strlen(filename);

    for (uint8_t i = 0; i < len; i++) {
        if ((filename[i] >= 97) && (filename[i] <= 122)) {
            filename[i] = filename[i] - 32;
        }
    }

    if (saving) {
        gamesave_save_to_disk(filename);
    } else {
        gamesave_load_from_disk(filename);
    }
    status_line_score = 255;
}

void gamesave_begin(bool save) {
    saving = save;
    dialog_show((uint8_t __far *)"Enter the name\nof the save file.\n(Uses device 9.)\n\n", true);
}