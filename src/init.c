#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "main.h"
#include "logic.h"
#include "memmanage.h"
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

void init_load_words(void) {
  char wds_name[32];
  strcpy(wds_name, "WORDS.TOK,S,R");
  uint16_t words_offset = chipmem2_alloc(8000);
  uint8_t __far *words_target = chipmem2_base + words_offset;
  uint8_t buffer[256];

  simpleopen(wds_name, strlen(wds_name));
  size_t bytes_read;
  uint32_t words_size = 0;
  do {
    bytes_read = simpleread(buffer);

    if (bytes_read > 0) {
      for (size_t idx = 0; idx < bytes_read; idx++) {
        words_target[idx] = buffer[idx];
      }
      words_size += bytes_read;
      words_target += bytes_read;
    }
  } while (bytes_read > 0);
  simpleclose();
  chipmem2_free(words_offset);
  chipmem2_alloc(words_size);
}

void init_load_objects(void) {
  char obj_name[32];
  strcpy(obj_name, "OBJECT,S,R");
  uint16_t objects_offset = chipmem2_alloc(8000);
  uint8_t __far *objects_target = chipmem2_base + objects_offset;
  uint8_t buffer[256];

  simpleopen(obj_name, strlen(obj_name));
  size_t bytes_read;
  uint32_t objects_size = 0;
  do {
    bytes_read = simpleread(buffer);

    if (bytes_read > 0) {
      for (size_t idx = 0; idx < bytes_read; idx++) {
        objects_target[idx] = buffer[idx];
      }
      objects_size += bytes_read;
      objects_target += bytes_read;
    }
  } while (bytes_read > 0);
  simpleclose();
  chipmem2_free(objects_offset);

  object_data_offset = chipmem2_alloc(objects_size);
  uint8_t __far *object_ptr = chipmem2_base + object_data_offset;
  char avis_durgan[12] = "Avis Durgan";
  uint8_t avis = 0;
  for (uint16_t index = 0; index < objects_size; index++) {
    *object_ptr = *object_ptr ^ avis_durgan[avis];
    avis = (avis + 1) % 11;
  }
}

