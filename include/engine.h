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

#ifndef ENGINE_H
#define ENGINE_H

#include "main.h"

extern uint8_t status_line_score;
extern bool quit_flag;
extern bool mouse_down;

void engine_update_status_line(bool force);
void run_loop(void);
void engine_interrupt_handler(void);
void engine_clear_keyboard(void);
void engine_allowinput(bool allowed);
void engine_statusline(bool enable);
void engine_bridge_pic_load(uint8_t pic_num);
void engine_bridge_draw_pic(uint8_t pic_num, bool clear);
void engine_bridge_add_to_pic(uint8_t add_command_num);

#endif