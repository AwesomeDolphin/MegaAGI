#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"
#include "irq.h"
#include "main.h"

uint8_t vic4cache[14];
volatile uint8_t __far *drawing_xpointer[4][160];

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
                            0x00, 0x80,         // count $8000 bytes
                            0x00, 0x00, 0x00,   // fill value $00
                            0x00, 0x00, 0x05,   // destination start $050000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                            0x00,
                            0x03,               // Fill command
                            0x00, 0x00,         // Count $8000 bytes
                            0x00, 0x00, 0x00,   // Fill value $00
                            0x00, 0x80, 0x05,   // Destination start $058000
                            0x00,               // Command high byte
                            0x00, 0x00          // modulo
                           };

uint8_t clrscreen01cmd[] = {0x00,               // End of token list
                            0x07,               // Fill command
                            0x00, 0x40,         // count $4000 bytes
                            0xff, 0x00, 0x00,   // fill value $ff
                            0x00, 0x00, 0x05,   // destination start $050000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                            0x00,
                            0x03,               // Fill command
                            0x00, 0x40,         // Count $4000 bytes
                            0x44, 0x00, 0x00,   // Fill value $44
                            0x00, 0x40, 0x05,   // Destination start $054000
                            0x00,               // Command high byte
                            0x00, 0x00          // modulo
                           };

uint8_t clrscreen23cmd[] = {0x00,               // End of token list
                            0x07,               // Fill command
                            0x00, 0x40,         // count $4000 bytes
                            0xff, 0x00, 0x00,   // fill value $ff
                            0x00, 0x80, 0x05,   // destination start $058000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                            0x00,
                            0x03,               // Fill command
                            0x00, 0x40,         // Count $4000 bytes
                            0x44, 0x00, 0x00,   // Fill value $44
                            0x00, 0xC0, 0x05,   // Destination start $05C000
                            0x00,               // Command high byte
                            0x00, 0x00          // modulo
                           };

uint8_t copysr01d23cmd[] = {0x00,               // End of token list
                            0x00,               // Fill command
                            0x00, 0x80,         // count $8000 bytes
                            0x00, 0x00, 0x05,   // source start $050000
                            0x00, 0x80, 0x05,   // destination start $058000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

uint8_t copysr23d01cmd[] = {0x00,               // End of token list
                            0x00,               // Fill command
                            0x00, 0x80,         // count $8000 bytes
                            0x00, 0x80, 0x05,   // source start $058000
                            0x00, 0x00, 0x05,   // destination start $050000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

uint8_t setcolorcmd[] = {0x81, 0xff, 0x85, 0x02, 0x00, 0x03, 0xD0, 0x07, 0x08, 0x00, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00};
uint8_t setcolorcmd2[] = {0x81, 0xff, 0x85, 0x02, 0x00, 0x03, 0xD0, 0x07, 0x1f, 0x00, 0x00, 0x01, 0x10, 0x08, 0x00, 0x00, 0x00};
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

void savevic(void) {
  vic4cache[0] = VICIV.ctrl2;
  vic4cache[1] = VICIV.ctrla;
  vic4cache[2] = VICIV.ctrlb;
  vic4cache[3] = VICIV.ctrlc;
  vic4cache[4] = VICIV.linestep >> 8;
  vic4cache[5] = VICIV.linestep & 0xff;
  vic4cache[6] = VICIV.chrcount;
  vic4cache[7] = VICIV.palsel;
  vic4cache[8] = VICIV.scrnptr_mb;
  vic4cache[9] = VICIV.scrnptr_bnk;
  vic4cache[10] = VICIV.scrnptr_msb;
  vic4cache[11] = VICIV.scrnptr_lsb;
  vic4cache[12] = VICIV.colptr >> 8;
  vic4cache[13] = VICIV.colptr & 0xff;
}

void loadvic(void) {
  VICIV.ctrl2 = vic4cache[0];
  VICIV.ctrla = vic4cache[1];
  VICIV.ctrlb = vic4cache[2];
  VICIV.ctrlc = vic4cache[3];
  VICIV.linestep = (vic4cache[4] << 8) | vic4cache[5];
  VICIV.chrcount = vic4cache[6];
  VICIV.palsel = vic4cache[7];
  VICIV.scrnptr_mb = vic4cache[8];
  VICIV.scrnptr_bnk = vic4cache[9];
  VICIV.scrnptr_msb = vic4cache[10];
  VICIV.scrnptr_lsb = vic4cache[11];
  VICIV.colptr = (vic4cache[12] << 8) | vic4cache[13];
}

void gfx_print_splitchar(uint8_t character) {
  static uint16_t parser_printpos = 0;
  uint16_t __far *screen_memory0 = (uint16_t __far *)(0x012000 + (0 * 0x400));
  if (character == CHAR_CLEARHOME) {
    for (uint16_t i = 420; i < 580; i++) {
      screen_memory0[i] = 0x0020;
    }
    parser_printpos = 0;
  } else if (character == 0x14) {
    parser_printpos--;
    screen_memory0[parser_printpos + 420] = 0x0020;
  } else if (character == '\r') {
    // Move to beginning of next line
    parser_printpos = ((parser_printpos / 40) + 1) * 40;
    // If past end, scroll the text area
    if (parser_printpos >= 160) {
      for (uint16_t i = 420; i < 540; i++) {
        screen_memory0[i] = screen_memory0[i + 40];
      }
      for (uint16_t i = 540; i < 580; i++) {
        screen_memory0[i] = 0x0020;
      }
      parser_printpos -= 40;
    }
  } else {
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
    screen_memory0[parser_printpos + 420] = character;
    parser_printpos++;
    // If past end, scroll the text area
    if (parser_printpos >= 160) {
      for (uint16_t i = 420; i < 540; i++) {
        screen_memory0[i] = screen_memory0[i + 40];
      }
      for (uint16_t i = 540; i < 580; i++) {
        screen_memory0[i] = 0x0020;
      }
      parser_printpos = 120;
    }
  }
}

uint8_t my_ultoa_invert(unsigned long val, char *str, int base)
{
  uint8_t len = 0;
  do
    {
      int v;

      v   = val % base;
      val = val / base;

      if (v <= 9)
        {
          v += '0';
        }
      else
        {
          v += 'A' - 10;
        }

      *str++ = v;
      len++;
    }
  while (val);

  return len;
}

void gfx_print_splitascii(char *formatstring, ...) {
  va_list ap;
  va_start(ap, formatstring);
  uint8_t *ascii_string = (uint8_t *)formatstring;
  while (*ascii_string != 0) {
    uint8_t petsciichar = *ascii_string;
    if (petsciichar == '%') {
      ascii_string++;
      switch (*ascii_string) {
        case 'd':
        case 'x': {
          char buffer[17];
          uint32_t param = va_arg(ap, int);
          uint8_t len = my_ultoa_invert(param, buffer, (*ascii_string == 'x') ? 16 : 10);
          for (; len > 0; len--) {
            gfx_print_splitchar(buffer[len-1]);
          }
          break;
        }
        case 'D':
        case 'X': {
          char buffer[17];
          uint32_t param = va_arg(ap, unsigned long);
          uint8_t len = my_ultoa_invert(param, buffer, (*ascii_string == 'X') ? 16 : 10);
          for (; len > 0; len--) {
            gfx_print_splitchar(buffer[len-1]);
          }
          break;
        }
      }
      ascii_string++;
      continue;
    }
    gfx_print_splitchar(petsciichar);
    ascii_string++;
  }
  va_end(ap);
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

void gfx_plotput(uint8_t screen_num, uint8_t x, uint8_t y, uint8_t color) {
  uint8_t highcolor[16] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0};
  uint8_t highpix = x & 1;
  uint16_t row = y * 8;
  volatile uint8_t __far *target_pixel;
  target_pixel = drawing_xpointer[screen_num][x] + row;
  uint8_t curpix = *(target_pixel);
  if (highpix) {
       curpix = (curpix & 0x0f) | highcolor[color];
  } else {
       curpix = (curpix & 0xf0) | (color & 0x0f);
  }
  *target_pixel = curpix;
}

uint8_t gfx_getprio(uint8_t screen_num, uint8_t x, uint8_t y) {
  uint8_t highpix = x & 1;
  uint16_t row = y * 8;
  volatile uint8_t __far *prio_start = drawing_xpointer[screen_num][x] + row;
  if (highpix) {
    while (*prio_start < 0x40) {
      prio_start += 8;
    }
    return (*prio_start >> 4);
  } else {
    while ((*prio_start  & 0x0f) < 0x04) {
      prio_start += 8;
    }
    return (*prio_start & 0x0f);
  }
}

uint8_t gfx_get(uint8_t screen_num, uint8_t x, uint8_t y) {
  uint8_t highpix = x & 1;
  uint16_t row = y * 8;
  uint8_t pixelval = *(drawing_xpointer[screen_num][x] + row); 
  if (highpix) {
    return (pixelval >> 4);
  } else {
    return (pixelval & 0x0f);
  }
}

void gfx_drawslowline(uint8_t screen_num, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) {
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
         gfx_plotput(screen_num, roundx, roundy, colour);
         y.full_value += dependent.full_value;
      }
      gfx_plotput(screen_num, x2, y2, colour);
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
         gfx_plotput(screen_num, roundx, roundy, colour);
         x.full_value += dependent.full_value;
      }
      gfx_plotput(screen_num, x2,y2, colour);
   }
}

void gfx_drawfastline(uint8_t screen_num, int x1, int y1, int x2, int y2, unsigned char colour)
{
  gfx_drawslowline(screen_num, x1, y1, x2, y2, colour);
}

void gfx_setupmem(void) {
  DMA.dmahigh = (uint8_t)(((uint16_t)setcolorcmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)setcolorcmd) & 0xff);
  DMA.dmahigh = (uint8_t)(((uint16_t)setcolorcmd2) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)setcolorcmd2) & 0xff);

  uint16_t __far *color_memory = (uint16_t __far *)(0xff81000);
  uint16_t __far *screen_memory[4] = {(uint16_t __far *)0x012000, (uint16_t __far *)0x012400, (uint16_t __far *)0x012800, (uint16_t __far *)0x012C00};
  for (int y = 0; y < 21; y++) {
    for (int x = 0; x < 10; x++) {
      color_memory[(y * 20) + x + 10] = 0x0100;
      for (int scrn = 0; scrn < 4; scrn++) {
        screen_memory[scrn][(y * 20) + x] = 0x1400 + (0x100 * scrn) + (x * 25) + y;        
        screen_memory[scrn][(y * 20) + x + 10] = 0x0020;        
      }
    }
  }

  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 40; x++) {
      color_memory[(y * 40) + x + 420] = 0x0100;
      screen_memory[0][(y * 40) + x + 420] = 0x0020;        
      screen_memory[2][(y * 40) + x + 420] = 0x0020;        
    }
  }

  uint32_t column_address = 0x50000;
  for (uint8_t screen_num = 0; screen_num < 4; screen_num++) {
    for (uint8_t x = 0; x < 160; x++) {
      drawing_xpointer[screen_num][x] = (uint8_t __far *)column_address + (x >> 4) * 1600 + ((x >> 1) & 0x07);
    }
    column_address += 0x4000;
  }

  uint8_t palette_index = 16;
  for (int i = 0; i < 48; i += 3) {
    PALETTE.red[palette_index] = palettedata[i];
    PALETTE.green[palette_index] = palettedata[i+1];
    PALETTE.blue[palette_index] = palettedata[i+2];
    palette_index++;
  }

  DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen01cmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)clrscreen01cmd) & 0xff);
  DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen23cmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)clrscreen23cmd) & 0xff);

}

void gfx_blackscreen(void) {
  DMA.dmahigh = (uint8_t)(((uint16_t)blackscreencmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)blackscreencmd) & 0xff);
}

void gfx_cleargfx(uint8_t screen_num) {
  if (screen_num & 0x02) {
    DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen23cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)clrscreen23cmd) & 0xff);
  } else {
    DMA.dmahigh = (uint8_t)(((uint16_t)clrscreen01cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)clrscreen01cmd) & 0xff);
  }
}

void gfx_copygfx(uint8_t screen_num) {
  if (screen_num & 0x02) {
    DMA.dmahigh = (uint8_t)(((uint16_t)copysr23d01cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)copysr23d01cmd) & 0xff);
  } else {
    DMA.dmahigh = (uint8_t)(((uint16_t)copysr01d23cmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)copysr01d23cmd) & 0xff);
  }
}

void gfx_showgfx(uint8_t screen_num) {
  VICIV.scrnptr = 0x00012000 + (screen_num * 0x400);
}

void gfx_switchto(void) {
    savevic();
    
    gfx_setupmem();

    VICIV.ctrl1 = 0x0b;
    VICIV.ctrl2 = VICIV.ctrl2 | 0x10;
    VICIV.ctrla = VICIV.ctrla | VIC3_PAL_MASK;
    VICIV.ctrlb = VICIV.ctrlb & ~(VIC3_H640_MASK | VIC3_V400_MASK);
    VICIV.ctrlc = (VICIV.ctrlc & ~(VIC4_FCLRLO_MASK)) | (VIC4_FCLRHI_MASK | VIC4_CHR16_MASK);

    VICIV.scrnptr = 0x00012000;
    VICIV.colptr = 0x1000;
}

void gfx_switchfrom(void) {
    unhook_irq();
    loadvic();
}