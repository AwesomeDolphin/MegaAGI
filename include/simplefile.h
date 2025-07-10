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

void simpleopen(char *filename, uint8_t namelength, uint8_t device_number);
uint8_t simpleread(uint8_t *destination);
void simplewrite(uint8_t *source, uint8_t length);
void simpleprint(char *string);
void simpleclose(void);
void simplecmdchan(uint8_t *drive_command, uint8_t device_number);
void simpleerrchan(uint8_t *statusmessage, uint8_t device_number);
