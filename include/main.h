#ifndef MAIN_H
#define MAIN_H

#include "stdbool.h"
#include "gamesave.h"

struct __F018 {
  uint8_t dmalowtrig;
  uint8_t dmahigh;
  uint8_t dmabank;
  uint8_t enable018;
  uint8_t addrmb;
  uint8_t etrig;
};

#define CHAR_CLEARHOME '\223'
#define ASCIIKEY (*(volatile uint8_t *)0xd610)
#define PETSCIIKEY (*(volatile uint8_t *)0xd619)
#define VICREGS ((volatile uint8_t *)0xd000)
#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y
#define PEEK(X) (*(volatile uint8_t*)(X))

/// DMA controller
#define DMA (*(volatile struct __F018 *)0xd700)

#endif