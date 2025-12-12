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

#ifndef GFX_H
#define GFX_H

#include <stdbool.h>

extern volatile uint8_t __far *drawing_xpointer[2][160];
extern uint8_t *fastdrawing_xpointer[160];
extern volatile uint8_t drawing_screen;
extern __far uint16_t screen_memory_0[1525];
extern __far uint16_t screen_memory_1[1525];
extern __far uint16_t color_memory[1024];
extern __far uint8_t priority_screen[16384];
extern __far uint8_t priority_screen[16384];
extern bool game_text;

void gfx_plotput(uint8_t x, uint8_t y, uint8_t color);
uint8_t gfx_getprio(uint8_t x, uint8_t y);
uint8_t gfx_get(uint8_t x, uint8_t y);
void gfx_drawslowline(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, unsigned char colour);
void gfx_setupmem(void);
void gfx_cleargfx(bool preserve_text);
bool gfx_flippage(void);
bool gfx_hold_flip(bool hold);
void gfx_switchto(void);

#endif