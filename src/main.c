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
#include "mapper.h"
#include "memmanage.h"
#include "engine.h"
#include "irq.h"

uint8_t copynonbankable[] =  {0x80,               // Source 0x80, attic ram
                              0x80,
                              0x81,               // Destination 0x00, chip ram
                              0x00,
                              0x00,               // End of token list
                              0x00,               // Copy command
                              0x00, 0x60,         // count $6000 bytes
                              0x00, 0x40, 0x00,   // source start $8004000
                              0x00, 0x20, 0x00,   // destination start $002000
                              0x00,               // command high byte
                              0x00,               // modulo
                            };

int main () {
  POKE(0xD020, 0x00);
  
  memmanage_init();

  init_system();

  DMA.dmahigh = (uint8_t)(((uint16_t)copynonbankable) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)copynonbankable) & 0xff);

  select_engine_diskdriver_mem();

  run_loop();

  return 0;
}
