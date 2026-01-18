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

#ifndef PIC_H
#define PIC_H

typedef struct pic_descriptor {
    uint16_t offset;
    uint16_t length;
} pic_descriptor_t;

extern __far pic_descriptor_t pic_descriptors[256];

void draw_pic(uint8_t pic_num, bool clear_screen);
void pic_add_to_pic(uint8_t pic_command);
void pic_show_priority(void);
void pic_load(uint8_t pic_num);


#endif