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
#include <calypsi/intrinsics6502.h>
#include "gfx.h"
#include "main.h"
#include "logic.h"
#include "memmanage.h"
#include "parser.h"
#include "disk.h"
#include "volume.h"
uint32_t __attribute__((zpage)) my_quad;

static uint32_t init_objmem_offset;
static uint32_t init_objmem_size;
static uint32_t init_wdsmem_offset;
static uint32_t init_wdsmem_size;

#pragma clang section bss = "banked_bss" data = "initsdata" rodata = "initsrodata" text = "initstext"

static uint8_t __far * const initscrmem_base = (uint8_t __far *)0x12000;
static uint8_t __far * const initcolmem_base = (uint8_t __far *)0xff80000;
static uint16_t cursorpos;

void init_agi_files(void);

uint8_t mainpalettedata[] = {
    0x00, 0x00, 0x00,
    0x0F, 0x0F, 0x0F,
    0x0F, 0x00, 0x00,
    0x00, 0x0F, 0x0F,
    0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00,
    0x00, 0x00, 0x0F,
    0x0F, 0x0F, 0x00,
    0x0F, 0x06, 0x00,
    0x0A, 0x04, 0x00,
    0x0F, 0x07, 0x07,
    0x05, 0x05, 0x05,
    0x08, 0x08, 0x08,
    0x09, 0x0F, 0x09,
    0x09, 0x09, 0x0F,
    0x0B, 0x0B, 0x0B,
};

uint8_t mousepalettedata[] = {
    0x00, 0x00, 0x00,
    0x0F, 0x0F, 0x0F,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
};

static const unsigned char ascii_to_c64_screen[128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

const uint8_t copybankable[] =   {0x80,               // Source 0x80, attic ram
                            0x80,
                            0x81,               // Destination 0x00, chip ram
                            0x00,
                            0x00,               // End of token list
                            0x04,               // Copy command
                            0x00, 0xa0,         // count $A000 bytes
                            0x00, 0xA0, 0x00,   // source start $800A000
                            0x00, 0x00, 0x03,   // destination start $030000
                            0x00,               // command high byte
                            0x00,               // modulo
                            
                            0x80,               // Source 0x80, attic ram
                            0x80,
                            0x81,               // Destination 0x00, chip ram
                            0x00,
                            0x00,               // End of token list
                            0x04,               // Copy command
                            0x00, 0x40,         // count $4000 bytes
                            0x00, 0xC0, 0x01,   // source start $801C000
                            0x00, 0xA0, 0x03,   // destination start $03A000
                            0x00,               // command high byte
                            0x00,               // modulo
                            };

void init_print(char *printstring) {
  char *printchar = printstring;
  while (*printchar != '\0') {
    if (*printchar == '\n') {
      cursorpos += 80 - (cursorpos % 80);
      printchar++;
    } else {
      *(initscrmem_base + cursorpos) = ascii_to_c64_screen[(uint8_t)*printchar];
      *(initcolmem_base + cursorpos) = 0x01;
      cursorpos++;
      printchar++;
    }
  }
}


void init_system(void)
{
  __disable_interrupts();

  VICIV.key = 0x47;
  VICIV.key = 0x53;

  VICIV.palsel = 0;
  uint8_t palette_index = 0;
  for (int i = 0; i < 48; i += 3)
  {
    PALETTE.red[palette_index] = mainpalettedata[i];
    PALETTE.green[palette_index] = mainpalettedata[i + 1];
    PALETTE.blue[palette_index] = mainpalettedata[i + 2];
    palette_index++;
  }

  palette_index = 0;
  VICIV.palsel = (VICIV.palsel & 0x33) | 0x44;
  for (int i = 0; i < 48; i += 3)
  {
    PALETTE.red[palette_index] = mousepalettedata[i];
    PALETTE.green[palette_index] = mousepalettedata[i + 1];
    PALETTE.blue[palette_index] = mousepalettedata[i + 2];
    palette_index++;
  }
  VICIV.palsel = (VICIV.palsel & 0x3F);

  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.chrxscl = 120;
  VICIV.ctrlb = VICIV.ctrlb | VIC3_H640_MASK;
  VICIV.ctrlc = VICIV.ctrlc & (~VIC4_CHR16_MASK);
  VICIV.chrcount = 80;
  VICIV.linestep = 80;
  VICIV.sdbdrwd_msb = VICIV.sdbdrwd_msb & ~(VIC4_HOTREG_MASK);

  VICIV.scrnptr = 0x00012000;
  VICIV.colptr = 0x0000;
  VICIV.charptr = 0x29800;

  for (uint16_t charpos = 0; charpos < 2000; charpos++) {
    *(initscrmem_base + charpos) = 0x20;
  }

  cursorpos = 0;

  init_print("MEGA65 AGI -- Sierra AGI parser for the MEGA65! KQ1 version.\n");

  init_print("MEGA65-AGI Copyright (C) 2025 Keith Henrickson\n"
              "This program comes with ABSOLUTELY NO WARRANTY\n"
              "This is free software, you are welcome to redistribute it\n"
              "under certain conditions. See GPL Version 3.29 in COPYING\n");
  init_print(GIT_MSG);
  init_print("\n\n");
  init_print("Loading interpreter modules...\n");

  uint8_t result;
  uint32_t gamecode_size;
  result = disk_load_attic("GAMECODE", &gamecode_size, 8);
  
  DMA.dmahigh = (uint8_t)(((uint16_t)copybankable) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)copybankable) & 0xff);

  init_agi_files();
}

void init_load_words(void)
{
  init_wdsmem_offset = atticmem_allocoffset;
  disk_load_attic("WORDS.TOK", &init_wdsmem_size, 8);
  token_data_offset = init_wdsmem_offset;
}

void init_load_objects(void)
{
  object_data_offset = atticmem_allocoffset;
  disk_load_attic("OBJECT", &init_objmem_size, 8);

  uint8_t __huge *object_ptr = attic_memory + init_objmem_offset;
  char avis_durgan[12] = "Avis Durgan";
  uint8_t avis = 0;
  for (uint16_t index = 0; index < init_objmem_size; index++)
  {
    *object_ptr = *object_ptr ^ avis_durgan[avis];
    avis = (avis + 1) % 11;
    object_ptr += 1;
  }
}

void init_agi_files(void)
{
  init_print("While we get things started:\n");
  init_print("Change speed with 'slow', 'normal', 'fast' commands!\n");
  init_print("In game help available on the 'F1' or 'HELP' keys.\n");
  init_print("Player movement is via a joystick on control port 2.\n");
  init_print("There is no need to calibrate your joystick, CTRL-J does nothing.\n");
  init_print("Insert a save game disk into Unit 9.\n");
  init_print("Green border means game is thinking. Patience is a virtue.\n\n");
  init_print("I don't have a patreon, my work is a gift to the retro community.\n");
  init_print("'This is for everyone.' -- Tim Berners-Lee\n\n");
  load_volume_files();
  load_directory_files();

  init_load_words();
  init_load_objects();

  init_print("Press any key to play game!\n");
  while (ASCIIKEY == 0)
    ;
  ASCIIKEY = 0;

  gfx_switchto();
}