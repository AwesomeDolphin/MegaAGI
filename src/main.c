/***************************************************************************
    MEGA65-AGI -- Sierra AGI interpreter for the MEGA65
    Copyright (C) 2025  Keith Henrickson

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
***************************************************************************/

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
  simpleprint("mega65 agi -- sIERRA agi PARSER FOR THE mega65!\r");
  simpleprint("tHIS WILL ONLY WORK WITH kq1... eVERYTHING ELSE... ymmv!\r");
  POKE(1,133);

  memmanage_init();

  init_system();
  
  VICIV.sdbdrwd_msb = VICIV.sdbdrwd_msb & ~(VIC4_HOTREG_MASK);
  run_loop();

  return 0;
}
