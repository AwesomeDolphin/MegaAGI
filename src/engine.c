#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"
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

#pragma clang section bss="nographicsbss" data="nographicsdata" rodata="nographicsrodata" text="nographicstext"

volatile uint8_t view_x = 40;
volatile uint8_t view_y = 40;
volatile uint8_t run_cycles;
static uint8_t picture = 1;
static uint8_t keypress_up;
static uint8_t keypress_down;
static uint8_t keypress_left;
static uint8_t keypress_right;
static uint8_t last_cursorreg;
static uint8_t last_columnzero;
static uint8_t keypress_f5;
volatile uint8_t frame_dirty;
volatile bool engine_running;
static char command_buffer[38];
static uint8_t cmd_buf_ptr = 0;
static bool input_ok;
uint8_t horizon_line;
bool player_control;
uint8_t dialog_first;
uint8_t dialog_last;
uint16_t dialog_time;
uint8_t box_width = 0;
uint8_t box_height = 0;
uint8_t line_length = 0;
uint8_t word_length = 0;
uint8_t msg_char;
uint8_t *msg_ptr;
uint8_t *last_word;

static const uint8_t joystick_direction[16] = {
    0, 0, 0, 0, 0, 4, 2, 3,
    0, 6, 8, 7, 0, 5, 1, 0
};

void engine_display_dialog(uint8_t *message_string) {
    msg_ptr = message_string;
    last_word = msg_ptr;
    word_length = 0;
    line_length = 0;
    box_width = 0;
    box_height = 1;
    
    do {
        msg_char = *msg_ptr;
        if (msg_char > 32) {
            word_length++;
        } else {
            bool wordfits = (line_length + word_length + 1) < 35;
            if (wordfits) {
                if (msg_char == 32) {
                    line_length += word_length + 1;
                    word_length = 0;
                    last_word = msg_ptr;
                } else {
                    line_length += word_length;
                    if (line_length > box_width) {
                        box_width = line_length;
                    }
                }
            } else {
                if (line_length > box_width) {
                    box_width = line_length - 1;
                }
                box_height++;
                line_length = word_length + 1;
                word_length = 0;
                *last_word = '\n';
            }
        }
        msg_ptr++;
    } while (msg_char != 0);

    uint8_t x_start = 20 - (box_width / 2) - 1;
    uint8_t y_start = 15 - (box_height / 2) - 1;
    dialog_first = y_start;

    msg_ptr = message_string;
    line_length = 0;

    gfx_begin_print(x_start, y_start);
    gfx_print_scncode(0xEC);
    while (line_length < box_width) {
        gfx_print_scncode(0XE2);
        line_length++;
    }
    gfx_print_scncode(0xFB);
    gfx_end_print();

    line_length = 0;
    y_start++;
    gfx_begin_print(x_start, y_start);
    gfx_print_scncode(0x61);
    do {
        msg_char = *msg_ptr;
        if (msg_char >= 32) {
            gfx_print_asciichar(msg_char, true);
            line_length++;
        } else {
            while (line_length < box_width) {
                gfx_print_asciichar(' ', true);
                line_length++;
            }
            gfx_print_scncode(0xE1);
            gfx_end_print();
            y_start++;
            gfx_begin_print(x_start, y_start);
            gfx_print_scncode(0x61);
            line_length = 0;
        }
        msg_ptr++;
    } while (*msg_ptr != 0);
    while (line_length < box_width) {
        gfx_print_asciichar(' ', true);
        line_length++;
    }
    gfx_print_scncode(0xE1);
    gfx_end_print();

    y_start++;
    dialog_last = y_start;
    line_length = 0;
    gfx_begin_print(x_start, y_start);
    gfx_print_scncode(0xFC);
    while (line_length < box_width) {
        gfx_print_scncode(0X62);
        line_length++;
    }
    gfx_print_scncode(0xFE);
    gfx_end_print();

    dialog_time = logic_vars[21];
    if (dialog_time > 0) {
        dialog_time = (dialog_time * 15) / 10;
    } else {
        dialog_time = 0xffff;
    }
}

void engine_dialog_close(void) {
    for (uint8_t line = dialog_first; line <= dialog_last; line++)
    {
        gfx_clear_line(line);
    }
    dialog_first = 0;
    dialog_last = 0;
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

/*
void handle_movement_keys(void) {
    __disable_interrupts();
    POKE(0xd614, 0);
    uint8_t cursor_reg = PEEK(0xd60f);
    uint8_t column_zero = PEEK(0xd613);
    if (cursor_reg != last_cursorreg) {
        column_zero = 0xff;
    }
    last_cursorreg = cursor_reg;
    if (column_zero != last_columnzero) {
        last_columnzero = column_zero;
        column_zero = 0xff;
    }
    __enable_interrupts();
    uint8_t up_key = cursor_reg & 0x02;
    uint8_t left_key = cursor_reg & 0x01;
    uint8_t right_key = (!(column_zero & 0x04)) && (!left_key);
    uint8_t down_key = (!(column_zero & 0x80)) && (!up_key);
    keypress_f5 = (!(column_zero & 0x40));
    if (down_key) {
        if (!keypress_down) {
            keypress_down = 1;
            object_dir = 5;
            set_dy(1);
            autoselect_loop();
        } 
    } else {
        keypress_down = 0;
    }
    if (right_key) {
        if (!keypress_right) {
            keypress_right = 1;
            object_dir = 3;
            set_dx(1);
            autoselect_loop();
        }
    } else {
        keypress_right = 0;
    }
    if (up_key) {
        if (!keypress_up) {
            keypress_up = 1;
            object_dir = 1;
            set_dy(-1);
            autoselect_loop();
        }
    } else {
        keypress_up = 0;
    }
    if (left_key) {
        if (!keypress_left) {
            keypress_left = 1;
            object_dir = 7;
            set_dx(-1);
            autoselect_loop();
        }
    } else {
        keypress_left = 0;
    }
}*/

void engine_unblock(void) {

}

void engine_statusline(bool enable) {
    
}

void engine_clear_keyboard(void) {
    command_buffer[0] = 0;
    cmd_buf_ptr=0;
    ASCIIKEY = 0;
    gfx_print_ascii(0, 22, ">                                       ");
}

void engine_allowinput(bool allowed) {
    input_ok = allowed;
    if (input_ok) {
        engine_clear_keyboard();
    } else {
        gfx_print_ascii(0, 22, "                                        ");
    }
}

void engine_handleinput(void) {
    if (dialog_first != dialog_last)
    {
        if (ASCIIKEY != 0) {
            ASCIIKEY = 0;
            engine_dialog_close();
        }
        return;
    }
    if (!input_ok) {
        return;
    }
    uint8_t ascii_key = ASCIIKEY;
    if (ascii_key != 0) {
        ASCIIKEY = 0;
        if (ascii_key == 0x14) {
            if (cmd_buf_ptr > 0) {
                cmd_buf_ptr--;
                gfx_set_printpos(cmd_buf_ptr + 3, 22);
                gfx_print_asciichar(' ', false);
            }
        } else if (ascii_key == 0x0d) {
            command_buffer[cmd_buf_ptr] = 0;
            parser_decode_string(command_buffer);
            engine_clear_keyboard();
        } else {
            if (cmd_buf_ptr < 37) {
                command_buffer[cmd_buf_ptr] = ascii_key;
                gfx_set_printpos(cmd_buf_ptr + 3, 22);
                gfx_print_asciichar(ascii_key, false);
                cmd_buf_ptr++;
            }
        }
    }


}

void run_loop(void) {
    view_init();
    sprite_init();
    gfx_switchto();
    gfx_blackscreen();
    logic_init();
    parser_init();
    frame_counter = 0;
    run_cycles = 0;
    run_engine = false;
    engine_running = false;
    hook_irq();
    input_ok = false;
    player_control = true;
    dialog_first = 0;
    dialog_last = 0;
    while (1) {
        while(!run_engine);
        engine_running = true;
        run_engine = false;

        run_cycles++;
        if (dialog_first != dialog_last) {
            if (dialog_time != 0xffff) {
                dialog_time--;
                if (dialog_time == 0) {
                    engine_dialog_close();
                }
            }
        }
        if (run_cycles >= logic_vars[10]) {
            logic_reset_flag(2);
            logic_reset_flag(4);
            engine_handleinput();
            sprite_erase_animated();
            handle_movement_joystick();
            logic_run(0);
            sprite_draw_animated();
            run_cycles = 0;
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
        if (gfx_flippage()) {
            run_engine = true;
            frame_counter = 0;
        }
    }
    *far_irr = *far_irr; // ack interrupt
}
