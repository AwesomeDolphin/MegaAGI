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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"
#include "dialog.h"
#include "irq.h"
#include "pic.h"
#include "sound.h"
#include "view.h"
#include "volume.h"
#include "parser.h"
#include "simplefile.h"
#include "sound.h"
#include "sprite.h"
#include "logic.h"
#include "memmanage.h"
#include "main.h"

volatile uint8_t frame_counter;
volatile bool run_engine;

#pragma clang section bss="midmembss" data="midmemdata" rodata="midmemrodata" text="midmemtext"

volatile uint8_t run_cycles;
volatile bool engine_running;
bool status_line_enabled;
uint8_t status_line_score;

static const uint8_t joystick_direction[16] = {
    0, 0, 0, 0, 0, 4, 2, 3,
    0, 6, 8, 7, 0, 5, 1, 0
};

void engine_update_status_line(void) {
    if (!game_text) {
        if (status_line_enabled) {
            if (logic_vars[3] != status_line_score) {
                status_line_score = logic_vars[3];
                dialog_print_ascii(0, 0, true, (uint8_t __far *)"Score: %d of %d%p40", logic_vars[3], logic_vars[7]);
            }
        }
    }
}

void engine_show_object(uint8_t view_num) {
    view_load(view_num);
    view_set(&object_view, view_num);
    select_loop(&object_view, 0);
    object_view.x_pos = 80 - (object_view.width / 2);
    object_view.y_pos = 155;
    object_view.priority_override = true;
    object_view.priority = 0x0f;
    show_object_view = true;
    uint8_t __far *desc_data = chipmem_base + object_view.desc_offset;
    dialog_show(false, desc_data);
}

/*
    0000 = inv
    0001 = inv
    0010 = inv
    0011 = inv
    0100 = inv
    0101 = right/down
    0110 = right/up
    0111 = right
    1000 = inv
    1001 = left/down
    1010 = left/up
    1011 = left
    1100 = inv
    1101 = down
    1110 = up
    1111 = inv
*/

void handle_movement_joystick(void) {
    if (!player_control) {
        return;
    }

    uint8_t port2joy = PEEK(0xDC00) & 0x0f;
    uint8_t new_object_dir = joystick_direction[port2joy];
    sprites[0].object_dir = new_object_dir;
}

void engine_statusline(bool enable) {
    status_line_enabled = enable;
    if (!enable) {
        dialog_print_ascii(0, 0, false, (uint8_t __far *)"%p40");
    }
}

void engine_clear_keyboard(void) {
    dialog_clear_keyboard();
}

void engine_allowinput(bool allowed) {
    input_ok = allowed;
    if (input_ok) {
        engine_clear_keyboard();
    } else {
        dialog_print_ascii(0, 22, false, (uint8_t __far *)"%p40");
    }
}

void run_loop(void) {
    frame_counter = 0;
    run_cycles = 0;
    run_engine = false;
    engine_running = false;
    sound_flag_needs_set = false;
    status_line_enabled = false;
    status_line_score = 255;
    hook_irq();
    view_init();
    sprite_init();
    logic_init();
    parser_init();
    dialog_init();
    input_ok = false;
    player_control = true;
    logic_set_flag(9);
    logic_set_flag(11);
    logic_vars[20] = 5;
    logic_vars[22] = 3;
    logic_vars[23] = 0x0f;
    logic_vars[24] = 37;
    logic_vars[25] = 3;
    while (1) {
        while(!run_engine);
        engine_running = true;
        run_engine = false;

        run_cycles++;
        if (!dialog_proc()) {
            if (run_cycles >= logic_vars[10]) {
                if (sound_flag_needs_set) {
                    sound_flag_needs_set = false;
                    logic_set_flag(sound_flag_end);
                }
                engine_update_status_line();
                sprite_erase_animated();
                handle_movement_joystick();
                logic_run();
                logic_reset_flag(11);
                sprite_draw_animated();
                run_cycles = 0;
                if (logic_flag_isset(2) || logic_flag_isset(4) || (logic_vars[9] > 0)) {
                    // Parser did something
                    logic_vars[9] = 0;
                    if (input_ok) {
                        dialog_clear_keyboard();
                    }
                }
                logic_reset_flag(2);
                logic_reset_flag(4);
                logic_reset_all_controllers();
            }
        }
        engine_running = false;
    }
}

#pragma clang section bss="" data="" rodata="" text=""

__attribute__((interrupt))
void engine_interrupt_handler(void) {
    volatile uint8_t __far *far_irr = (uint8_t __far *)0xffd3019;
    if (!((*far_irr) & 0x01)) {
        // not a raster interrupt, ignore
        return;
      }

    sound_interrupt_handler();
    
    frame_counter++;
    if (frame_counter >= 3) {
        gfx_flippage();
        run_engine = true;
        frame_counter = 0;
    }
    *far_irr = *far_irr; // ack interrupt
}
