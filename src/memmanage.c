#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <math.h>

#include "main.h"
#include "logic.h"
#include "memmanage.h"
#include "view.h"

uint8_t __far * const chipmem_base = (uint8_t __far *)0x40000;
uint8_t __far * const chipmem2_base = (uint8_t __far *)0xff8e000;
uint8_t __huge * const attic_memory = (uint8_t __huge *)0x8000000;

void memmanage_memcpy_far_huge(uint8_t __huge *dest_mem, uint8_t __far *src_mem, uint32_t length) {
    for (uint32_t pos = 0; pos < length; pos++) {
        *dest_mem = *src_mem;
        dest_mem++;
        src_mem++;
    }
}

void memmanage_memcpy_huge_far(uint8_t __far *dest_mem, uint8_t __huge *src_mem, uint32_t length) {
    for (uint32_t pos = 0; pos < length; pos++) {
        *dest_mem = *src_mem;
        dest_mem++;
        src_mem++;
    }
}

void memmanage_init(void) {
    chipmem_allocoffset = 1;
    chipmem_lockoffset = 1;
    chipmem2_allocoffset = 1;
    atticmem_allocoffset = 1;
}

uint16_t chipmem_alloc(uint16_t size) {
    uint16_t old_offset = chipmem_allocoffset;
    chipmem_allocoffset = chipmem_allocoffset + size;
    return old_offset;
}

void chipmem_free(uint16_t offset) {
    chipmem_allocoffset = offset;
    logic_purge(chipmem_allocoffset);
    view_purge(chipmem_allocoffset);
}

void chipmem_lock(void) {
    chipmem_lockoffset = chipmem_allocoffset;
}

void chipmem_free_unlocked(void) {
    chipmem_allocoffset = chipmem_lockoffset;
    logic_purge(chipmem_allocoffset);
    view_purge(chipmem_allocoffset);
}

uint16_t chipmem2_alloc(uint16_t size) {
    uint16_t old_offset = chipmem2_allocoffset;
    chipmem2_allocoffset = chipmem2_allocoffset + size;
    return old_offset;
}

void chipmem2_free(uint16_t offset) {
    chipmem2_allocoffset = offset;
}

uint32_t atticmem_alloc(uint32_t size) {
    uint32_t old_offset = atticmem_allocoffset;
    atticmem_allocoffset = atticmem_allocoffset + size;
    return old_offset;
}

void atticmem_free(uint32_t offset) {
    atticmem_allocoffset = offset;
}

