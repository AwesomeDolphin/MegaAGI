#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "gfx.h"
#include "main.h"
#include "memmanage.h"
#include "simplefile.h"
#include "volume.h"

typedef struct voldir_entry {
    uint8_t volume_number;
    uint32_t offset;
} voldir_entry_t;

static uint8_t __huge *volume_files[16] = {0};
#pragma clang section bss="extradata"
__far static voldir_entry_t logic_directory[256];
__far static voldir_entry_t pic_directory[256];
__far static voldir_entry_t sound_directory[256];
__far static voldir_entry_t view_directory[256];
#pragma clang section bss=""

#pragma clang section bss="nographicsbss" data="nographicsdata" rodata="nographicsrodata" text="nographicstext"


static uint8_t volumes;
static uint8_t logic_files;
static uint8_t pic_files;
static uint8_t sound_files;
static uint8_t view_files;

uint8_t load_directory_file(char *dir_file_name, voldir_entry_t __far *entries) {
    uint8_t entry_num = 0;
    uint8_t entrybuf[768];
    simpleopen(dir_file_name, strlen(dir_file_name));
    uint16_t entries_read = simpleread(&entrybuf[0]);
    entries_read += simpleread(&entrybuf[255]);
    entries_read += simpleread(&entrybuf[510]);
    entries_read += simpleread(&entrybuf[765]);
    uint16_t entry_index = 0;
    for (entry_num = 0; entry_num < entries_read / 3; entry_num++) {
        entries[entry_num].volume_number = entrybuf[entry_index + 0] >> 4;
        entries[entry_num].offset = ((uint32_t)(entrybuf[entry_index + 0] & 0x0f) << 16) | (((entrybuf[entry_index + 1]) << 8) & 0xffff) | entrybuf[entry_index + 2];
        entry_index += 3;
    }
    simpleclose();
    return entry_num;
}

void load_directory_files(void) {
    logic_files = load_directory_file("LOGDIR,S,R", logic_directory);
    pic_files = load_directory_file("PICDIR,S,R", pic_directory);
    sound_files = load_directory_file("SNDDIR,S,R", sound_directory);
    view_files = load_directory_file("VIEWDIR,S,R", view_directory);
}

uint8_t __huge *locate_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length) {
  voldir_entry_t __far *volobj_entry = NULL;
  switch (kind) {
    case voLogic:
      if (volobj_num < logic_files) {
        volobj_entry = &logic_directory[volobj_num];
      }
      break;
    case voPic:
      if (volobj_num < pic_files) {
        volobj_entry = &pic_directory[volobj_num];
      }
      break;
    case voSound:
      if (volobj_num < sound_files) {
        volobj_entry = &sound_directory[volobj_num];
      }
      break;
    case voView:
      if (volobj_num < view_files) {
        volobj_entry = &view_directory[volobj_num];
      }
      break;
  }
  if (volobj_entry != NULL) {
    uint8_t __huge *object_cache_memory = volume_files[volobj_entry->volume_number] + volobj_entry->offset;
    if ((object_cache_memory[0] == 0x12) && (object_cache_memory[1] == 0x34)) {
      *object_length = (object_cache_memory[4] << 8) + object_cache_memory[3];
      return (object_cache_memory + 5);
    } else {
      return NULL;
    }
  } else {
      return NULL;
  }
}

uint8_t copyattogamcmd[] = {0x80, 0x00,         // Source extended address
                            0x00,               // End of token list
                            0x00,               // Copy command
                            0x00, 0x00,         // count $0000 bytes
                            0x00, 0x00, 0x00,   // source start $000000
                            0x00, 0x00, 0x04,   // destination start $040000
                            0x00,               // command high byte
                            0x00, 0x00,         // modulo
                           };

uint16_t load_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length) {
    uint8_t __huge *volobj_file;
    uint16_t length;
    volobj_file = locate_volume_object(kind, volobj_num, &length);
    if (volobj_file == NULL) {
        return 0;
    }

    *object_length = length;
    copyattogamcmd[1] = ((uint32_t)volobj_file & 0xff00000) >> 20;
    copyattogamcmd[8] = ((uint32_t)volobj_file & 0x00f0000) >> 16;
    copyattogamcmd[7] = ((uint32_t)volobj_file & 0x000ff00) >> 8;
    copyattogamcmd[6] = (uint32_t)volobj_file & 0x00000ff;
    copyattogamcmd[5] = (length & 0xff00) >> 8;
    copyattogamcmd[4] = length & 0xff;
    uint16_t target_offset = chipmem_alloc(length);
    copyattogamcmd[10] = (target_offset & 0xff00) >> 8;
    copyattogamcmd[9] = (target_offset & 0xff);
    DMA.dmahigh = (uint8_t)(((uint16_t)copyattogamcmd) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)copyattogamcmd) & 0xff);
    return target_offset;
}

void load_volume_files(void) {
  char vol_name[32];
  strcpy(vol_name, "VOL.0,S,R");
  uint8_t __huge *volume_cache = attic_memory + atticmem_allocoffset;
  uint8_t buffer[256];
  int vol_number = 0;
  for (vol_number = 0; vol_number < 15; vol_number++) {
    simpleopen(vol_name, strlen(vol_name));
    vol_name[4]++; 
    volume_files[vol_number] = volume_cache;
    size_t bytes_read;
    uint32_t volume_size = 0;
    do {
      bytes_read = simpleread(buffer);
      if (bytes_read > 0) {
        for (size_t idx = 0; idx < bytes_read; idx++) {
          volume_cache[idx] = buffer[idx];
        }
        volume_size += bytes_read;
        volume_cache += bytes_read;
      }
    } while (bytes_read > 0);
    if (volume_size == 0) {
        volume_files[vol_number] = 0;
        simpleclose();
        break;
    } else {
      atticmem_alloc(volume_size);
    }
    simpleclose();
  }
  volumes = vol_number - 1;
}

