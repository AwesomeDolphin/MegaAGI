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

int main () {
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  VICIV.chrxscl = 120;
  simpleprint("\x93\x0b\x0e");
  simpleprint("AGI DEMO!\r");
  POKE(1,133);

  memmanage_init();

  init_system();
  
  VICIV.sdbdrwd_msb = VICIV.sdbdrwd_msb & ~(VIC4_HOTREG_MASK);
  run_loop();

  return 0;
}
