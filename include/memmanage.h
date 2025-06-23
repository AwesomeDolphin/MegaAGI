#ifndef MEMMANAGE_H
#define MEMMANAGE_H

extern uint8_t __far * const chipmem_base;
extern uint8_t __far * const chipmem2_base;
extern uint8_t __huge * const attic_memory;

void memmanage_memcpy_far_huge(uint8_t __huge *dest_mem, uint8_t __far *src_mem, uint32_t length);
void memmanage_memcpy_huge_far(uint8_t __far *dest_mem, uint8_t __huge *src_mem, uint32_t length);

void memmanage_init(void);
uint16_t chipmem_alloc(uint16_t size);
void chipmem_free(uint16_t offset);
void chipmem_lock(void);
void chipmem_free_unlocked(void);
uint16_t chipmem2_alloc(uint16_t size);
void chipmem2_free(uint16_t offset);
uint32_t atticmem_alloc(uint32_t size);
void atticmem_free(uint32_t offset);

#endif
