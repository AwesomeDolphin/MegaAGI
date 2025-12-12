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

#ifndef MEMMANAGE_H
#define MEMMANAGE_H

extern uint8_t __far * const chipmem_base;
extern uint8_t __huge * const attic_memory;

void memmanage_strcpy_far_far(uint8_t __far *dest_string, uint8_t __far *src_string);
void memmanage_strcpy_far_near(uint8_t *dest_string, uint8_t __far *src_string);
void memmanage_strcpy_near_far(uint8_t __far *dest_string, uint8_t *src_string);
void memmanage_memcpy_far_huge(uint8_t __huge *dest_mem, uint8_t __far *src_mem, uint32_t length);
void memmanage_memcpy_huge_far(uint8_t __far *dest_mem, uint8_t __huge *src_mem, uint32_t length);

void memmanage_init(void);
uint16_t chipmem_alloc(uint16_t size);
void chipmem_free(uint16_t offset);
void chipmem_lock(void);
void chipmem_free_unlocked(void);
uint32_t atticmem_alloc(uint32_t size);
void atticmem_free(uint32_t offset);

#endif
