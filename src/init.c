#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "main.h"
#include "simplefile.h"

uint32_t __attribute__((zpage)) my_quad;

void init_load_raw(void) {
    char prg_name[32];
    strcpy(prg_name, "NOGRAPHICS,S,R");
    uint8_t *prg_mem = (uint8_t *)0x7000;
  
    simpleopen(prg_name, strlen(prg_name));
    size_t bytes_read;
    do {
      bytes_read = simpleread(prg_mem);
      if (bytes_read > 0) {
        prg_mem += bytes_read;
      }
    } while (bytes_read > 0);
    simpleclose();
}