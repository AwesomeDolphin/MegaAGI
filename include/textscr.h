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

#ifndef TEXTSCR_H
#define TEXTSCR_H

#include "main.h"

extern __far uint8_t formatted_string_buffer[1024];
extern __far uint8_t print_string_buffer[1024];

void textscr_set_color(uint8_t foreground, uint8_t background);
uint16_t textscr_format_string_valist(uint8_t __far *formatstring, va_list ap);
void textscr_print_asciichar(uint8_t character);
void textscr_print_scncode(uint8_t scncode);
void textscr_set_printpos(uint8_t x, uint8_t y);
void textscr_clear_line(uint8_t y);
uint16_t textscr_print_ascii(uint8_t x, uint8_t y, uint8_t *formatstring, ...);
void textscr_set_textmode(bool enable_text);
void textscr_clear_keyboard(void);
void textscr_init(void);

#endif
