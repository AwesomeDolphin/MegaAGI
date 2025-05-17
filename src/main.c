#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"
#include "init.h"
#include "pic.h"
#include "sound.h"
#include "view.h"
#include "volume.h"
#include "memmanage.h"
#include "simplefile.h"
#include "engine.h"
#include "irq.h"

#define ASCIIKEY (*(volatile uint8_t *)0xd610)
#define PETSCIIKEY (*(volatile uint8_t *)0xd619)
#define VICREGS ((volatile uint8_t *)0xd000)
#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y

int main () {
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  VICIV.chrxscl = 120;
  simplewrite(0x93);
  simplewrite(0x0b);
  simplewrite(0x0e);
  simpleprint("AGI DEMO!\r");
  POKE(1,133);

  simpleprint("LOADING HIMEM...\r");
  init_load_raw();

  memmanage_init();
  
  simpleprint("LOADING VOLUME FILES...\r");
  load_volume_files();
  simpleprint("LOADING DIRECTORY FILES...\r");
  load_directory_files();
  simpleprint("LOADING WORDS.TOK...\r");
  load_words_file();
  
  VICIV.sdbdrwd_msb = VICIV.sdbdrwd_msb & ~(VIC4_HOTREG_MASK);
  run_loop();

  return 0;
}
