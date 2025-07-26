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

#include "gfx.h"
#include "main.h"
#include "logic.h"
#include "memmanage.h"
#include "simplefile.h"
#include "volume.h"
uint32_t __attribute__((zpage)) my_quad;

static uint16_t init_obj_data_offset;
static uint32_t init_midmem_offset;
static uint16_t init_midmem_size;
static uint32_t init_himem_offset;
static uint16_t init_himem_size;
static uint32_t init_ultmem_offset;
static uint16_t init_ultmem_size;

static void init_internal(void);
static void init_load_words(void);
static void init_load_objects(void);

void loadseq_near(char *name, uint16_t load_address) {
    uint8_t *prg_mem = (uint8_t *)load_address;

    simpleopen(name, strlen(name), 8);
    size_t bytes_read;
    do {
        bytes_read = simpleread(prg_mem);
        if (bytes_read > 0) {
            prg_mem += bytes_read;
        }
    } while (bytes_read > 0);
    simpleclose();

    uint8_t buffer[64];
    simpleerrchan(buffer, 8);
    uint8_t errnum = simpleerrcode(buffer);
    if (errnum != 0) {
        simpleprint("eRROR LOADING FILE: ");
        simpleprint((char *)&buffer[0]);
        while(1); // Halt on error
    }
}

void init_system(void) {
    simpleprint("lOADING BOOTSTRAP ROUTINE...\r\r");
    loadseq_near("INITS,S,R", 0x7500);
    init_internal();

    POKE(0xD030, 0x44);

    memmanage_memcpy_huge_far((uint8_t __far *)0x7500, attic_memory + init_midmem_offset, init_midmem_size);
    memmanage_memcpy_huge_far((uint8_t __far *)0xC000, attic_memory + init_himem_offset, init_himem_size);
    memmanage_memcpy_huge_far((uint8_t __far *)0xE000, attic_memory + init_ultmem_offset, init_ultmem_size);

    object_data_offset = init_obj_data_offset;
}

#pragma clang section bss="midmembss" data="initsdata" rodata="initsrodata" text="initstext"

uint32_t load_seq_attic(char *name, uint16_t *data_size) {
  uint8_t __huge *data_cache = attic_memory + atticmem_allocoffset;
  uint8_t buffer[256];

  simpleopen(name, strlen(name), 8);
  size_t bytes_read;
  *data_size = 0;
  do {
    bytes_read = simpleread(buffer);
    if (bytes_read > 0) {
      for (size_t idx = 0; idx < bytes_read; idx++) {
        data_cache[idx] = buffer[idx];
      }
      *data_size += bytes_read;
      data_cache += bytes_read;
    }
  } while (bytes_read > 0);
  simpleclose();
  return atticmem_alloc(*data_size);
}

uint16_t loadseq_chipmem2(char *name, uint16_t *data_size) {
  uint16_t data_offset = chipmem2_alloc(8000);
  uint8_t __far *data_target = chipmem2_base + data_offset;
  uint8_t buffer[256];

  simpleopen(name, strlen(name), 8);
  size_t bytes_read;
  do {
    bytes_read = simpleread(buffer);

    if (bytes_read > 0) {
      for (size_t idx = 0; idx < bytes_read; idx++) {
        data_target[idx] = buffer[idx];
      }
      *data_size += bytes_read;
      data_target += bytes_read;
    }
  } while (bytes_read > 0);
  simpleclose();
  chipmem2_free(data_offset);
  return chipmem2_alloc(*data_size);
}

void init_load_words(void) {
  uint16_t words_size = 0;
  loadseq_chipmem2("WORDS.TOK,S,R", &words_size);
}

void init_load_objects(void) {
  uint16_t objects_size = 0;
  init_obj_data_offset = loadseq_chipmem2("OBJECT,S,R", &objects_size);

  uint8_t __far *object_ptr = chipmem2_base + init_obj_data_offset;
  char avis_durgan[12] = "Avis Durgan";
  uint8_t avis = 0;
  for (uint16_t index = 0; index < objects_size; index++) {
    *object_ptr = *object_ptr ^ avis_durgan[avis];
    avis = (avis + 1) % 11;
    object_ptr += 1;
  }
}

void init_internal(void) {
    simpleprint("mega65-agi cOPYRIGHT (c) 2025 kEITH hENRICKSON\r"
    "tHIS PROGRAM COMES WITH absolutely no warranty\r"
    "tHIS IS FREE SOFTWARE, AND YOU ARE WELCOME TO REDISTRIBUTE IT\r"
    "UNDER CERTAIN CONDITIONS. sEE gpl vERSION 3.29 IN copying\r\r");
    simpleprint("lOADING INTERPRETER MODULES...\r");
    init_midmem_offset = load_seq_attic("MIDMEM,S,R", &init_midmem_size);
    init_himem_offset = load_seq_attic("HIMEM,S,R", &init_himem_size);
    init_ultmem_offset = load_seq_attic("ULTMEM,S,R", &init_ultmem_size);

    uint8_t buffer[64];
    simplecmdchan((uint8_t *)"R0:VOL.0=VOL.0\r", 8);
    simpleerrchan(buffer, 8);
    uint8_t errnum = simpleerrcode(buffer);
    if (errnum == 62) {
      simpleprint("uNABLE TO LOCATE vol.0 AS A seq FILE!\r");
      simpleprint("yOU MUST PROVIDE YOUR OWN agi DATA FILES\r");
      simpleprint("agi GAMES ARE LEGALLY SOLD AT GOG.COM\r");
      while(1);
    }
    simpleprint("lOADING GAME FILES...\r\r");
    simpleprint("wHILE WE GET THINGS STARTED:\r");
    simpleprint("cHANGE SPEED WITH 'SLOW', 'NORMAL', and 'FAST' COMMANDS!\r");
    simpleprint("iN GAME HELP AVAILABLE ON THE 'f1' or 'help' KEYS.\r");
    simpleprint("pLAYER MOVEMENT IS VIA A JOYSTICK ON CONTROLLER PORT 2.\r");
    simpleprint("tHERE IS NO NEED TO CALIBRATE YOUR JOYSTICK, ctrl-j DOES NOTHING.\r");
    simpleprint("iNSERT A SAVE GAME DISK INTO UNIT 9.\r");
    simpleprint("green border means game is thinking! pATIENCE IS A VIRTUE.\r\r");
    simpleprint("i DON'T HAVE A PATREON, MY WORK IS A GIFT TO THE RETRO COMMUNITY.\r");
    simpleprint("'tHIS IS FOR EVERYONE.' -- tIM bERNERS-lEE\r\r");
    load_volume_files();
    load_directory_files();
    init_load_words();
    init_load_objects();

    simpleprint("pRESS ANY KEY TO PLAY GAME!\r");
    while(ASCIIKEY==0);
    ASCIIKEY = 0;

    gfx_switchto();
    gfx_blackscreen();
}