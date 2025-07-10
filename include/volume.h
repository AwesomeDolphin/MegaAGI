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

#ifndef VOLUME_H
#define VOLUME_H

typedef enum volobj_kind {
    voLogic,
    voPic,
    voSound,
    voView,
} volobj_kind_t;

void load_directory_files(void);
void load_volume_files(void);
uint16_t load_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length);
uint8_t __huge *locate_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length);

#endif