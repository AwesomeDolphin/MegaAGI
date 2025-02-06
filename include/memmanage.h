#ifndef MEMMANAGE_H
#define MEMMANAGE_H

extern uint8_t __far * const chipmem_base;

void memmanage_init(void);
uint16_t chipmem_alloc(uint16_t size);
void chipmem_free(uint16_t offset);
void chipmem_lock(void);
void chipmem_free_unlocked(void);

#endif
