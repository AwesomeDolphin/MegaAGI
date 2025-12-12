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
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "disk.h"
#include "gfx.h"
#include "main.h"
#include "memmanage.h"
#include "disk.h"
#include "volume.h"

typedef struct voldir_entry
{
    uint8_t volume_number;
    uint32_t offset;
} voldir_entry_t;

static uint8_t __huge *volume_files[16] = {0};
#pragma clang section bss = "extradata"
__far static voldir_entry_t logic_directory[256];
__far static voldir_entry_t pic_directory[256];
__far static voldir_entry_t sound_directory[256];
__far static voldir_entry_t view_directory[256];
#pragma clang section bss = ""

static uint8_t volumes;
static uint8_t logic_files;
static uint8_t pic_files;
static uint8_t sound_files;
static uint8_t view_files;

#pragma clang section bss = "banked_bss" data = "enginedata" rodata = "enginerodata" text = "enginetext"

uint8_t __huge *locate_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length)
{
    voldir_entry_t __far *volobj_entry = NULL;
    switch (kind)
    {
    case voLogic:
        if (volobj_num < logic_files)
        {
            volobj_entry = &logic_directory[volobj_num];
        }
        break;
    case voPic:
        if (volobj_num < pic_files)
        {
            volobj_entry = &pic_directory[volobj_num];
        }
        break;
    case voSound:
        if (volobj_num < sound_files)
        {
            volobj_entry = &sound_directory[volobj_num];
        }
        break;
    case voView:
        if (volobj_num < view_files)
        {
            volobj_entry = &view_directory[volobj_num];
        }
        break;
    }

    if (volobj_entry != NULL)
    {
        uint8_t __huge *object_cache_memory = volume_files[volobj_entry->volume_number] + volobj_entry->offset;
        if ((object_cache_memory[0] == 0x12) && (object_cache_memory[1] == 0x34))
        {
            *object_length = (object_cache_memory[4] << 8) + object_cache_memory[3];
            return (object_cache_memory + 5);
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

uint8_t copyattogamcmd[] = {
    0x80,
    0x00, // Source extended address
    0x81,
    0x00,
    0x00, // End of token list
    0x00, // Copy command
    0x00,
    0x00, // count $0000 bytes
    0x00,
    0x00,
    0x00, // source start $000000
    0x00,
    0x00,
    0x04, // destination start $040000
    0x00,
    0x00, // modulo
};

uint16_t load_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length)
{
    uint8_t __huge *volobj_file;
    uint16_t length;

    volobj_file = locate_volume_object(kind, volobj_num, &length);
    if (volobj_file == NULL)
    {
        return 0;
    }

    *object_length = length;
    copyattogamcmd[1] = ((uint32_t)volobj_file & 0xff00000) >> 20;
    copyattogamcmd[10] = ((uint32_t)volobj_file & 0x00f0000) >> 16;
    copyattogamcmd[9] = ((uint32_t)volobj_file & 0x000ff00) >> 8;
    copyattogamcmd[8] = (uint32_t)volobj_file & 0x00000ff;
    copyattogamcmd[7] = (length & 0xff00) >> 8;
    copyattogamcmd[6] = length & 0xff;
    uint16_t target_offset = chipmem_alloc(length);
    copyattogamcmd[12] = (target_offset & 0xff00) >> 8;
    copyattogamcmd[11] = (target_offset & 0xff);
    DMA.dmabank = 0x03;
    DMA.dmahigh = ((uint8_t)(((uint16_t)copyattogamcmd) >> 8)) + 0x60;
    DMA.etrig = (uint8_t)(((uint16_t)copyattogamcmd) & 0xff);
    return target_offset;
}

#pragma clang section bss = "banked_bss" data = "initsdata" rodata = "initsrodata" text = "initstext"
uint8_t load_directory_file(char *dir_file_name, voldir_entry_t __far *entries)
{
    uint8_t __huge *entrybuf = attic_memory + atticmem_allocoffset;
    uint32_t directory_size = 0;
    disk_load_attic(dir_file_name, &directory_size, 8);

    uint16_t entry_index = 0;
    uint16_t entry_num;
    for (entry_num = 0; entry_num < directory_size / 3; entry_num++)
    {
        entries[entry_num].volume_number = entrybuf[entry_index + 0] >> 4;
        entries[entry_num].offset = ((uint32_t)(entrybuf[entry_index + 0] & 0x0f) << 16) | (((entrybuf[entry_index + 1]) << 8) & 0xffff) | entrybuf[entry_index + 2];
        entry_index += 3;
    }
    return entry_num;
}

void load_directory_files(void)
{
    logic_files = load_directory_file("LOGDIR", logic_directory);
    pic_files = load_directory_file("PICDIR", pic_directory);
    sound_files = load_directory_file("SNDDIR", sound_directory);
    view_files = load_directory_file("VIEWDIR", view_directory);
}

void load_volume_files(void)
{
    char vol_name[32];
    strcpy(vol_name, "VOL.0");
    uint8_t __huge *volume_cache = attic_memory + atticmem_allocoffset;

    uint32_t volume_size = 0;
    uint8_t vol_number;
    for (vol_number = 0; vol_number < 15; vol_number++)
    {
        volume_files[vol_number] = volume_cache;
        disk_load_attic(vol_name, &volume_size, 8);
        vol_name[4]++;
        if (volume_size == 0)
        {
            volume_files[vol_number] = 0;
            break;
        }
        volume_cache = attic_memory + atticmem_allocoffset;
    }
    volumes = vol_number - 1;
}
