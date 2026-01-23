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

#ifndef DIALOG_H
#define DIALOG_H

#include "main.h"

void dialog_gamesave_handler(char *filename);
void dialog_gamesave_begin(bool save);
void dialog_clear_keyboard(void);
void dialog_get_string(uint8_t destination_str, uint8_t prompt, uint8_t row, uint8_t column, uint8_t max);
void dialog_show(bool accept_input, uint8_t __far *message_string, ...);
bool dialog_handleinput(bool force_accept);
void dialog_close(void);
bool dialog_proc(void);
void dialog_init(void);

#endif
