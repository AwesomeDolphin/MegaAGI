#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "gfx.h"
#include "main.h"
#include "logic.h"
#include "memmanage.h"
#include "simplefile.h"
#include "volume.h"
uint32_t __attribute__((zpage)) my_quad;

static uint16_t init_obj_data_offset;
static uint32_t init_midmem_offset;
static uint16_t init_midmem_size;
static uint32_t init_himem_offset;
static uint16_t init_himem_size;

static void init_internal(void);
static void init_load_words(void);
static void init_load_objects(void);

void loadseq_near(char *name, uint16_t load_address) {
    uint8_t *prg_mem = (uint8_t *)load_address;
    simpleopen(name, strlen(name), 8);
    size_t bytes_read;
    do {
        bytes_read = simpleread(prg_mem);
        if (bytes_read > 0) {
            prg_mem += bytes_read;
        }
    } while (bytes_read > 0);
    simpleclose();
}

void init_system(void) {
    simpleprint("INIT SYSTEM...\r");
    loadseq_near("INITS,S,R", 0x7500);
    init_internal();

    POKE(0xD030, 0x44);

    memmanage_memcpy_huge_far((uint8_t __far *)0x7500, attic_memory + init_midmem_offset, init_midmem_size);
    memmanage_memcpy_huge_far((uint8_t __far *)0xC000, attic_memory + init_himem_offset, init_himem_size);

    object_data_offset = init_obj_data_offset;
}

#pragma clang section bss="midmembss" data="initsdata" rodata="initsrodata" text="initstext"

uint32_t load_seq_attic(char *name, uint16_t *data_size) {
  uint8_t __huge *data_cache = attic_memory + atticmem_allocoffset;
  uint8_t buffer[256];

  simpleopen(name, strlen(name), 8);
  size_t bytes_read;
  *data_size = 0;
  do {
    bytes_read = simpleread(buffer);
    if (bytes_read > 0) {
      for (size_t idx = 0; idx < bytes_read; idx++) {
        data_cache[idx] = buffer[idx];
      }
      *data_size += bytes_read;
      data_cache += bytes_read;
    }
  } while (bytes_read > 0);
  simpleclose();
  return atticmem_alloc(*data_size);
}

uint16_t loadseq_chipmem2(char *name, uint16_t *data_size) {
  uint16_t data_offset = chipmem2_alloc(8000);
  uint8_t __far *data_target = chipmem2_base + data_offset;
  uint8_t buffer[256];

  simpleopen(name, strlen(name), 8);
  size_t bytes_read;
  do {
    bytes_read = simpleread(buffer);

    if (bytes_read > 0) {
      for (size_t idx = 0; idx < bytes_read; idx++) {
        data_target[idx] = buffer[idx];
      }
      *data_size += bytes_read;
      data_target += bytes_read;
    }
  } while (bytes_read > 0);
  simpleclose();
  chipmem2_free(data_offset);
  return chipmem2_alloc(*data_size);
}

void init_load_words(void) {
  uint16_t words_size = 0;
  uint16_t words_offset = loadseq_chipmem2("WORDS.TOK,S,R", &words_size);
}

void init_load_objects(void) {
  uint16_t objects_size = 0;
  init_obj_data_offset = loadseq_chipmem2("OBJECT,S,R", &objects_size);

  uint8_t __far *object_ptr = chipmem2_base + init_obj_data_offset;
  char avis_durgan[12] = "Avis Durgan";
  uint8_t avis = 0;
  for (uint16_t index = 0; index < objects_size; index++) {
    *object_ptr = *object_ptr ^ avis_durgan[avis];
    avis = (avis + 1) % 11;
  }
}

void init_internal(void) {
    uint8_t cmdchan[256];
    simpleprint("CMDCHAN 8: ");
    simplecmdchan(cmdchan, 8);
    simpleprint((char*)cmdchan);

    simpleprint("CMDCHAN 9: ");
    simplecmdchan(cmdchan, 9);  
    simpleprint((char*)cmdchan);

    simpleprint("LOADING MIDMEM...\r");
    init_midmem_offset = load_seq_attic("MIDMEM,S,R", &init_midmem_size);
    simpleprint("LOADING HIMEM...\r");
    init_himem_offset = load_seq_attic("HIMEM,S,R", &init_himem_size);
    simpleprint("LOADING VOLUME FILES...\r");
    load_volume_files();
    simpleprint("LOADING DIRECTORY FILES...\r");
    load_directory_files();
    simpleprint("LOADING WORDS.TOK...\r");
    init_load_words();
    simpleprint("LOADING OBJECTS...\r");
    init_load_objects();

    gfx_switchto();
    gfx_blackscreen();
}