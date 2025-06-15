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

volatile uint8_t run_cycles;
volatile bool engine_running;
static char command_buffer[38];
static uint8_t cmd_buf_ptr = 0;
uint8_t dialog_first;
uint8_t dialog_last;
uint16_t dialog_time;
uint8_t box_width = 0;
uint8_t box_height = 0;
uint8_t line_length = 0;
uint8_t word_length = 0;
uint8_t msg_char;
char *msg_ptr;
char *last_word;
char format_string_buffer[1024];

static const uint8_t joystick_direction[16] = {
    0, 0, 0, 0, 0, 4, 2, 3,
    0, 6, 8, 7, 0, 5, 1, 0
};

void engine_display_dialog(uint8_t __far *message_string) {
    logic_strcpy_far_near(format_string_buffer, message_string);

    msg_ptr = format_string_buffer;
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
    uint8_t y_start = 10 - (box_height / 2) - 1;
    dialog_first = y_start;

    msg_ptr = format_string_buffer;
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
    engine_display_dialog(desc_data);
}

void engine_dialog_close(void) {
    for (uint8_t line = dialog_first; line <= dialog_last; line++)
    {
        gfx_clear_line(line);
    }
    dialog_first = 0;
    dialog_last = 0;
    show_object_view = false;
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
    if (!input_ok) {
        return;
    }
    uint8_t ascii_key = ASCIIKEY;
    if (ascii_key != 0) {
        ASCIIKEY = 0;
        if (ascii_key == 0xf7) {
            gamesave_save_to_attic();
        }
        if (ascii_key == 0xf5) {
            gamesave_load_from_attic();
        }
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
        } else if (ascii_key < 127) {
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
    logic_set_flag(9);
    logic_set_flag(11);
    logic_vars[22] = 3;
    logic_vars[23] = 0x0f;
    logic_vars[24] = 37;
    logic_vars[25] = 3;
    while (1) {
        while(!run_engine);
        engine_running = true;
        run_engine = false;

        run_cycles++;
        if (dialog_first != dialog_last) {
            if (ASCIIKEY != 0) {
                ASCIIKEY = 0;
                engine_dialog_close();
            } else {
                if (dialog_time != 0xffff) {
                    dialog_time--;
                    if (dialog_time == 0) {
                        engine_dialog_close();
                    }
                }
            }
        } else {
            if (run_cycles >= logic_vars[10]) {
                logic_reset_flag(2);
                logic_reset_flag(4);
                engine_handleinput();
                sprite_erase_animated();
                handle_movement_joystick();
                logic_run();
                logic_reset_flag(11);
                sprite_draw_animated();
                run_cycles = 0;
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
