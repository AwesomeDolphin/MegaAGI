#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <math.h>

#include "logic.h"
#include "memmanage.h"
#include "view.h"

uint8_t __far * const chipmem_base = (uint8_t __far *)0x40000;
uint8_t __far * const chipmem2_base = (uint8_t __far *)0x1d700;
static uint16_t chipmem_allocoffset;
static uint16_t chipmem_lockoffset;
static uint16_t chipmem2_allocoffset;

void memmanage_init(void) {
    chipmem_allocoffset = 1;
    chipmem_lockoffset = 1;
    chipmem2_allocoffset = 1;
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

