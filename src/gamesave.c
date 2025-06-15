#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gamesave.h"
#include "gfx.h"
#include "main.h"
#include "memmanage.h"
#include "pic.h"

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

#pragma clang section bss="extradata"
__far agisprite_t sprites[256];
__far uint8_t animated_sprites[256];
__far uint16_t views[256];
__far logic_info_t logic_infos[256];
__far uint8_t object_locations[256];
#pragma clang section bss=""

uint8_t __huge *gamesave_cache;

void gamesave_memcpy_far_huge(uint8_t __huge *dest_mem, uint8_t __far *src_mem, uint32_t length) {
    for (uint32_t pos = 0; pos < length; pos++) {
        *dest_mem = *src_mem;
        dest_mem++;
        src_mem++;
    }
}

void gamesave_memcpy_huge_far(uint8_t __far *dest_mem, uint8_t __huge *src_mem, uint32_t length) {
    for (uint32_t pos = 0; pos < length; pos++) {
        *dest_mem = *src_mem;
        dest_mem++;
        src_mem++;
    }
}

void gamesave_save_to_attic(void) {
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

    uint32_t offset = 29;
    gamesave_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)sprites, sizeof(sprites));
    offset += sizeof(sprites);
    gamesave_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)animated_sprites, sizeof(animated_sprites));
    offset += sizeof(animated_sprites);
    gamesave_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)views, sizeof(views));
    offset += sizeof(views);
    gamesave_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)logic_infos, sizeof(logic_infos));
    offset += sizeof(logic_infos);
    gamesave_memcpy_far_huge(&gamesave_cache[offset], (uint8_t __far *)object_locations, sizeof(object_locations));
    offset += sizeof(object_locations);

    gamesave_memcpy_far_huge(&gamesave_cache[offset], chipmem_base, 0x10000);
    VICIV.bordercol = COLOR_BLACK;
    gfx_print_ascii(0, 0, "%X", (uint32_t)gamesave_cache);
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

    uint32_t offset = 29;
    gamesave_memcpy_huge_far((uint8_t __far *)sprites, &gamesave_cache[offset], sizeof(sprites));
    offset += sizeof(sprites);
    gamesave_memcpy_huge_far((uint8_t __far *)animated_sprites, &gamesave_cache[offset], sizeof(animated_sprites));
    offset += sizeof(animated_sprites);
    gamesave_memcpy_huge_far((uint8_t __far *)views, &gamesave_cache[offset], sizeof(views));
    offset += sizeof(views);
    gamesave_memcpy_huge_far((uint8_t __far *)logic_infos, &gamesave_cache[offset], sizeof(logic_infos));
    offset += sizeof(logic_infos);
    gamesave_memcpy_huge_far((uint8_t __far *)object_locations, &gamesave_cache[offset], sizeof(object_locations));
    offset += sizeof(object_locations);

    gamesave_memcpy_huge_far(chipmem_base, &gamesave_cache[offset], 0x10000);
    logic_set_flag(12);

    gfx_hold_flip(true);
    VICIV.bordercol = COLOR_GREEN;
    pic_load(logic_vars[0]);
    draw_pic();
    pic_discard(logic_vars[0]);
    gfx_hold_flip(false);
    VICIV.bordercol = COLOR_BLACK;
}
