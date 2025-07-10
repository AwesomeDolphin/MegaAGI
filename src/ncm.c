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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"
#include "irq.h"
#include "main.h"
#include "dialog.h"

volatile uint8_t __far *drawing_xpointer[2][160];
uint8_t *fastdrawing_xpointer[160];
static uint16_t printpos = 0;
static uint16_t endpos;
volatile uint8_t drawing_screen = 1;
volatile uint8_t viewing_screen = 0;
static volatile bool stop_flip;
static uint8_t highcolor[16] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0};
bool game_text;

#pragma clang section bss="screenmem0"
__far static uint16_t screen_memory_0[1525];
#pragma clang section bss="screenmem1"
__far static uint16_t screen_memory_1[1525];
#pragma clang section bss="colorram"
__far static uint16_t color_memory[1024];
#pragma clang section bss="prioritydata"
__far uint8_t priority_screen[16384];
#pragma clang section bss=""

#define Q15_16_INT_TO_Q(QVAR, WHOLE) \
  QVAR.part.whole = WHOLE;\
  QVAR.part.fractional = 0;

typedef union q15_16 {
  int32_t full_value;
  struct part_tag {
    uint16_t fractional;
    int16_t whole;
  } part;
} q15_16t;

uint8_t blackscreencmd[] = {0x00,               // End of token list
                            0x07,               // Fill command
                            0x00, 0x7d,         // count $7d00 bytes
                            0x00, 0x00, 0x00,   // fill value $00
                            0x00, 0x00, 0x05,   // destination start $050000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                            0x00,
                            0x03,               // Fill command
                            0x00, 0x7d,         // Count $7d00 bytes
                            0x00, 0x00, 0x00,   // Fill value $00
                            0x00, 0x80, 0x05,   // Destination start $058000
                            0x00,               // Command high byte
                            0x00, 0x00          // modulo
                           };

uint8_t clrscreen0cmd[] =  {0x00,               // End of token list
                            0x07,               // Fill command
                            0x00, 0x7d,         // count $7d00 bytes
                            0xff, 0x00, 0x00,   // fill value $ff
                            0x00, 0x00, 0x05,   // destination start $050000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                            0x00,               // End of token list
                            0x03,               // Fill command
                            0xff, 0x3f,         // count $3fff bytes
                            0x44, 0x00, 0x00,   // fill value $44
                            0x00, 0x40, 0x01,   // destination start $014000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

uint8_t clrscreen1cmd[] =  {0x00,               // End of token list
                            0x07,               // Fill command
                            0x00, 0x7d,         // count $7d00 bytes
                            0xff, 0x00, 0x00,   // fill value $ff
                            0x00, 0x80, 0x05,   // destination start $058000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                            0x00,               // End of token list
                            0x03,               // Fill command
                            0xff, 0x3f,         // count $3fff bytes
                            0x44, 0x00, 0x00,   // fill value $44
                            0x00, 0x40, 0x01,   // destination start $014000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

uint8_t copys0d1cmd[] =    {0x00,               // End of token list
                            0x00,               // Copy command
                            0x00, 0x7d,         // count $7d00 bytes
                            0x00, 0x00, 0x05,   // source start $050000
                            0x00, 0x80, 0x05,   // destination start $058000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

uint8_t copys1d0cmd[] =    {0x00,               // End of token list
                            0x00,               // Copy command
                            0x00, 0x7d,         // count $7d00 bytes
                            0x00, 0x80, 0x05,   // source start $058000
                            0x00, 0x00, 0x05,   // destination start $050000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

void gfx_print_asciichar(uint8_t character, bool reverse) {
  if (character < 32) {
    character = character + 0x80;
  } else if (character < 64) {
    character = character + 0x00;
  } else if (character < 96) {
    character = character + 0x00;
  } else if (character < 128) {
    character = character + 0xA0;
  } else if (character < 160) {
    character = character + 0x40;
  } else if (character < 192) {
    character = character + 0xC0;
  } else {
    character = character + 0x80;
  }
  if (reverse) {
    character = character ^ 0x80;
  }
  screen_memory_0[printpos] = character;
  screen_memory_1[printpos] = character;
  color_memory[printpos] = 0x0100;
  printpos++;
}

void gfx_print_scncode(uint8_t scncode) {
  screen_memory_0[printpos] = scncode;
  screen_memory_1[printpos] = scncode;
  color_memory[printpos] = 0x0100;
  printpos++;
}

void gfx_set_printpos(uint8_t x, uint8_t y) {
  printpos = (y * 61) + 20 + x; 
}

void gfx_begin_print(uint8_t x, uint8_t y) {
  if (game_text) {
    printpos = (y * 61) + 21 + x;
  } else {
    printpos = (y * 61) + 20;
    endpos = printpos+40;
    screen_memory_0[printpos] = x * 8;
    screen_memory_1[printpos] = x * 8;
    printpos++;
  }
}

void gfx_end_print(void) {
  if (!game_text) {
    if (printpos < endpos) {
      screen_memory_0[printpos] = 0x140;
      screen_memory_1[printpos] = 0x140;
      color_memory[printpos] = 0x0010;       
    } else if (printpos == endpos) {
      screen_memory_0[printpos] = 0x0020;
      screen_memory_1[printpos] = 0x0020;
      color_memory[printpos] = 0x0100;       
    }
  }
}

void gfx_print_asciistr(uint8_t x, uint8_t y, bool reverse, uint8_t __far *output) {
  gfx_begin_print(x,y);

  uint8_t __far *ascii_string = output;
  while (*ascii_string != 0) {
    uint8_t asciichar = *ascii_string;
    gfx_print_asciichar(asciichar, reverse);
    ascii_string++;
  }
  gfx_end_print();
}

void gfx_clear_line(uint8_t y) {
  printpos = (y * 61) + 20;
  screen_memory_0[printpos] = 0x140;
  screen_memory_1[printpos] = 0x140;
  color_memory[printpos] = 0x0010;       
}

void gfx_set_textmode(bool enable_text) {
  if (enable_text && !game_text) {
    for (uint8_t i = 0; i < 25; i++) {
      dialog_print_ascii(0, i, false, (uint8_t __far *)"%p40");
    }
    game_text = true;
  } else {
    for (uint8_t i = 0; i < 25; i++) {
      gfx_clear_line(i);
    }
    game_text = false;
  }
}

void gfx_waitf1(void) {
  uint8_t foo;
  do {
      foo = ASCIIKEY;
      ASCIIKEY = 0;
  } while (foo != 0xF1);
}

void gfx_waitnokey(void) {
  while (ASCIIKEY != 0) {
      ASCIIKEY = 0;
  }
}

int agi_q15round(q15_16t aNumber, int16_t dirn)
{
  int16_t floornum = aNumber.part.whole;
  int16_t ceilnum = aNumber.part.whole+1;

   if (dirn < 0)
      return ((aNumber.part.fractional <= 0x8042) ?
        floornum : ceilnum);
   return ((aNumber.part.fractional < 0x7fbe) ?
        floornum : ceilnum);
}

void gfx_plotput(uint8_t x, uint8_t y, uint8_t color) {
  if (y > 167) {POKE(0xD020,7); while(1);}
  if (color & 0x80) {
    uint8_t highpix = x & 1;
    uint8_t xcolumn = x >> 1;
    uint8_t curpix = priority_screen[(y * 80) + xcolumn];
    if (highpix) {
         curpix = (curpix & 0x0f) | highcolor[color & 0x0f];
    } else {
         curpix = (curpix & 0xf0) | (color & 0x0f);
    }
    priority_screen[(y * 80) + xcolumn] = curpix;
  } else {
    uint8_t colorval[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint16_t row = y * 8;
    volatile uint8_t __far *target_pixel = drawing_xpointer[drawing_screen][x];
    target_pixel += row;
    *target_pixel = colorval[color];
    }
}

uint8_t gfx_get(uint8_t x, uint8_t y) {
  uint16_t row = y * 8;
  uint8_t pixelval = *(drawing_xpointer[drawing_screen][x] + row); 
  return (pixelval & 0x0f);
}

uint8_t gfx_getprio(uint8_t x, uint8_t y) {
  uint8_t highpix = x & 1;
  uint8_t xcolumn = x >> 1;
  uint8_t curpix = priority_screen[(y * 80) + xcolumn];
  if (highpix) {
    return (curpix >> 4);
  } else {
    return (curpix & 0x0f);
  }
}

void gfx_drawslowline(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) {
   int16_t height, width;
   q15_16t x, y, dependent;
   int8_t increment;

   height = ((int16_t)y2 - y1);
   width = ((int16_t)x2 - x1);
   uint8_t absheight = abs(height);
   uint8_t abswidth = abs(width);
   if (abs(width) > abs(height)) {
      Q15_16_INT_TO_Q(x, x1);
      Q15_16_INT_TO_Q(y, y1);
      if (width > 0) {
        increment = 1;
      } else {
        increment = -1;
      }
      Q15_16_INT_TO_Q(dependent, height);
      dependent.full_value = (width  == 0 ? 0:(dependent.full_value/abswidth));
      for (; x.part.whole != x2; x.part.whole += increment) {
        int roundx = agi_q15round(x, increment);
        int roundy = agi_q15round(y, dependent.part.whole);
         gfx_plotput(roundx, roundy, colour);
         y.full_value += dependent.full_value;
      }
      gfx_plotput(x2, y2, colour);
   }
   else {
      Q15_16_INT_TO_Q(x, x1);
      Q15_16_INT_TO_Q(y, y1);
      if (height > 0) {
        increment = 1;
      } else {
        increment = -1;
      }
      Q15_16_INT_TO_Q(dependent, width);
      dependent.full_value = (height == 0 ? 0:(dependent.full_value/absheight));
      for (; y.part.whole!=y2; y.part.whole += increment) {
        uint8_t roundx = agi_q15round(x, dependent.part.whole);
        uint8_t roundy = agi_q15round(y, increment);
         gfx_plotput(roundx, roundy, colour);
         x.full_value += dependent.full_value;
      }
      gfx_plotput(x2,y2, colour);
   }
}

void gfx_blackscreen(void) {
  DMA.dmahigh = (uint8_t)(((uint16_t)blackscreencmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)blackscreencmd) & 0xff);
}

void gfx_cleargfx(bool preserve_text) {
  if (drawing_screen == 0) {
    DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen0cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)clrscreen0cmd) & 0xff);
  } else {
    DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen1cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)clrscreen1cmd) & 0xff);
  }

  for (int y = 0; y < 20; y++) {
    int x=20;
    screen_memory_0[(y * 61) + x] = 0x0140;        
    screen_memory_1[(y * 61) + x] = 0x0140;
    color_memory[(y * 61) + x] = 0x0010;       
  }
if (!preserve_text) {
    for (int y = 21; y < 25; y++) {
      int x=20;
      screen_memory_0[(y * 61) + x] = 0x0140;        
      screen_memory_1[(y * 61) + x] = 0x0140;
      color_memory[(y * 61) + x] = 0x0010;       
    }
  }

  volatile uint8_t __far *target_pixel;
  for (uint8_t row = 168; row < 176; row++) {
    for (uint8_t col = 0; col < 8; col++) {
      target_pixel = drawing_xpointer[drawing_screen][col] + (row * 8);
      *target_pixel = 0x00;
    }
  }
}

void gfx_copygfx(uint8_t screen_num) {
  if (screen_num == 1) {
    DMA.dmahigh = (uint8_t)(((uint16_t)copys1d0cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)copys1d0cmd) & 0xff);
  } else {
    DMA.dmahigh = (uint8_t)(((uint16_t)copys0d1cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)copys0d1cmd) & 0xff);
  }
}

bool gfx_flippage(void) {
  if (!stop_flip) {
    drawing_screen ^= viewing_screen;
    viewing_screen ^= drawing_screen;
    drawing_screen ^= viewing_screen;
  
    //VICIV.bordercol = viewing_screen ? COLOR_RED : COLOR_GREEN;
  
    VICIV.scrnptr = 0x00012000 + (viewing_screen * 0xC00);
    gfx_copygfx(viewing_screen);
    return true;
  }
  return false;
}

bool gfx_hold_flip(bool hold) {
  __disable_interrupts();
  bool old_hold = stop_flip;
  stop_flip = hold;
  __enable_interrupts(); 
  return old_hold;
}

#pragma clang section bss="midmembss" data="initsdata" rodata="initsrodata" text="initstext"

uint8_t palettedata[] =  {0x00, 0x00, 0x00,
                          0x00, 0x00, 0x8A,
                          0x00, 0x8A, 0x00,
                          0x00, 0x8A, 0x8A,
                          0x8A, 0x00, 0x00,
                          0x8A, 0x00, 0x8A,
                          0x8A, 0x45, 0x00,
                          0x8A, 0x8A, 0x8A,
                          0x45, 0x45, 0x45,
                          0x45, 0x45, 0xCF,
                          0x45, 0xCF, 0x45,
                          0x45, 0xCF, 0xCF,
                          0xCF, 0x45, 0x45,
                          0xCF, 0x45, 0xCF,
                          0xCF, 0xCF, 0x45,
                          0xCF, 0xCF, 0xCF};


void gfx_setupmem(void) {
  for (int y = 0; y < 25; y++) {
    for (int x = 0; x < 20; x++) {
      if ((y > 0) && (y < 22)) {
        // Initializing the NCM graphics memory, twenty rows of 20 characters, 16 pixels per character
        // Initializing in column order, so that maybe someday DMA can be used to copy the data
        // to the screen.
        // Character 0x1400 points to 0x50000, which is the first bitmap screen
        // Character 0x1600 points to 0x58000, which is the second bitmap screen
        // We set up memory for a 320x200 screen, but only use 20 of the rows
        // But since memory is arranged that way, each vertical strip starts 25 characters apart
        screen_memory_0[(y * 61) + x] = 0x1400 + (0x200 * 0) + (x * 25) + (y - 1);        
        screen_memory_1[(y * 61) + x] = 0x1400 + (0x200 * 1) + (x * 25) + (y - 1);
        // Color memory is set as 0x1f = Select palette 1, where the PC palette is loaded, and color 0x0f is the foreground color
        // The foreground color is important, because this is the color that will be used when the NCM screen has 0xf as a color
        // The 0x08 selects NCM mode, with no other flags
        color_memory[(y * 61) + x] = 0x1f08;       
      } else {
        // Point all characters on lines that do not have game graphics to the same character, which is a blank character
        screen_memory_0[(y * 61) + x] = 0x1400 + 21;        
        screen_memory_1[(y * 61) + x] = 0x1400 + 21;
        // Color memory is set as 0x1f = Select palette 1, where the PC palette is loaded, and color 0x0f is the foreground color
        // The foreground color is important, because this is the color that will be used when the NCM screen has 0xf as a color
        // The 0x08 selects NCM mode, with no other flags
        color_memory[(y * 61) + x] = 0x1f08;       
      }
    }
    int x=20;
    // This is the GOTOX flag character, at character 20, so after the graphics pointers
    // We reposition the buffer writing to pixel 320, which is pointless, except that it keeps the text OFF the screen
    screen_memory_0[(y * 61) + x] = 0x0140;        
    screen_memory_1[(y * 61) + x] = 0x0140;
    color_memory[(y * 61) + x] = 0x0010;       
    for (int x = 21; x < 61; x++) {
      // Set the characters to < 255, because want to use the ROM character set for these
      // Color memory simply sets the foreground color to palette 0 (Commodore colors), color 1, (white).
      screen_memory_0[(y * 61) + x] = 0x0041;        
      screen_memory_1[(y * 61) + x] = 0x0041;
      color_memory[(y * 61) + x] = 0x0100;       
    }
  }

  uint32_t column0_address = 0x50000;
  uint32_t column1_address = 0x58000;
  uint32_t fastcolumn_address = 0x8000;
  for (uint8_t x = 0; x < 160; x++) {
    drawing_xpointer[0][x] = (uint8_t __far *)column0_address + (x >> 3) * 1600 + (x & 0x07);
    drawing_xpointer[1][x] = (uint8_t __far *)column1_address + (x >> 3) * 1600 + (x & 0x07);
    fastdrawing_xpointer[x] = (uint8_t *)fastcolumn_address + (x >> 3) * 1600 + (x & 0x07);
  }

  uint8_t palette_index = 16;
  for (int i = 0; i < 48; i += 3) {
    PALETTE.red[palette_index] = palettedata[i];
    PALETTE.green[palette_index] = palettedata[i+1];
    PALETTE.blue[palette_index] = palettedata[i+2];
    palette_index++;
  }

  DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen0cmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)clrscreen0cmd) & 0xff);
  DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen1cmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)clrscreen1cmd) & 0xff);

}

void gfx_switchto(void) {
    stop_flip = false;
    
    gfx_setupmem();

    game_text = 0;

    VICIV.ctrl1 = 0x1b;
    VICIV.ctrl2 = VICIV.ctrl2 | 0x10;
    VICIV.ctrla = VICIV.ctrla | VIC3_PAL_MASK;
    VICIV.ctrlb = VICIV.ctrlb & ~(VIC3_H640_MASK | VIC3_V400_MASK);
    VICIV.ctrlc = (VICIV.ctrlc & ~(VIC4_FCLRLO_MASK)) | (VIC4_FCLRHI_MASK | VIC4_CHR16_MASK);

    VICIV.scrnptr = 0x00012000;
    VICIV.colptr = 0x1000;
    VICIV.chrcount = 61;
    VICIV.linestep = 122;
}
