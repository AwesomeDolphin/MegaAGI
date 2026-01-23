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

#include "dialog.h"
#include "engine.h"
#include "gfx.h"
#include "main.h"
#include "mapper.h"
#include "memmanage.h" 
#include "parser.h"
#include "pic.h"
#include "ports.h"
#include "textscr.h"

#pragma clang section bss="banked_bss" data="gui_data" rodata="gui_rodata" text="gui_text"

typedef enum input_mode {
    imParser,
    imDialogField,
} input_mode_t;

typedef enum dialog_type {
    dtSave,
    dtRestore,
} dialog_type_t;

uint8_t dialog_first;
uint8_t dialog_last;
uint16_t dialog_time;
uint8_t box_width = 0;
uint8_t box_height = 0;
uint8_t line_length = 0;
uint8_t word_length = 0;
uint8_t msg_char;
uint8_t __far *msg_ptr;
uint8_t __far *last_word;

uint8_t input_line;
uint8_t input_start_column;
uint8_t input_max_length;

static char command_buffer[38];
static uint8_t cmd_buf_ptr = 0;
bool cursor_flag;
uint8_t cursor_delay;
uint8_t x_start;
uint8_t y_start;
input_mode_t dialog_input_mode;
dialog_type_t active_dialog;
bool show_priority;

static bool dialog_handleinput_internal(uint8_t ascii_key) {
    switch(ascii_key) {
        case 0x09:
            logic_set_controller(4);
            break;
        case 0x14:
            if (cmd_buf_ptr > 0) {
                textscr_set_printpos(cmd_buf_ptr + input_start_column, input_line);
                textscr_print_asciichar(' ', false);
                cmd_buf_ptr--;
                textscr_set_printpos(cmd_buf_ptr + input_start_column, input_line);
                textscr_print_asciichar(' ', false);
            }
            break;
        case 0x0d:
            return true;
        case 0xf5:
            logic_set_controller(1);
            break;
        case 0xf7:
            logic_set_controller(2);
            break;
        case 0xf9:
            logic_set_controller(3);
            break;
        case 0x1b:
            logic_set_controller(14);
            break;
        case 0x1f:
            logic_set_controller(18);
            break;
        case 0xf1:
            logic_set_controller(18);
            break;
        case 0x3d:
            logic_set_controller(5);
            break;
        case 0x30:
            logic_set_controller(7);
            break;
        case 0x2d:
            logic_set_controller(8);
            break;
        case 0xfd:
            show_priority = true;
            break;
        default:
            if ((ascii_key >= 32) && (ascii_key < 127)) {
                if (cmd_buf_ptr < input_max_length) {
                    command_buffer[cmd_buf_ptr] = ascii_key;
                    textscr_set_printpos(cmd_buf_ptr + input_start_column, input_line);
                    textscr_print_asciichar(ascii_key, false);
                    cmd_buf_ptr++;
                }
            }
            break;
    }
    return false;
}

void dialog_show_internal(bool accept_input) {
    msg_ptr = formatted_string_buffer;
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
            bool wordfits = (line_length + word_length + 1) < 31;
            if (wordfits) {
                if (msg_char == 32) {
                    line_length += word_length + 1;
                    word_length = 0;
                    last_word = msg_ptr;
                } else if (msg_char == 10) {
                    line_length += word_length;
                    if (line_length > box_width) {
                        box_width = line_length;
                    }
                    box_height++;
                    line_length = 0;
                    word_length = 0;
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
                *last_word = '\n';
                if (msg_char == 10) {
                    line_length = 0;
                    word_length = 0;
                    box_height++;
                } else {
                    line_length = word_length + 1;
                    word_length = 0;
                }
            }
        }
        msg_ptr++;
    } while (msg_char != 0);

    x_start = 20 - (box_width / 2) - 1;
    y_start = 10 - (box_height / 2) - 1;
    dialog_first = y_start;

    msg_ptr = formatted_string_buffer;
    line_length = 0;

    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0xEC);
    while (line_length < box_width) {
        textscr_print_scncode(0XE2);
        line_length++;
    }
    textscr_print_scncode(0xFB);
    textscr_end_print();

    line_length = 0;
    y_start++;
    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0x61);
    do {
        msg_char = *msg_ptr;
        if (msg_char >= 32) {
            textscr_print_asciichar(msg_char, true);
            line_length++;
        } else {
            while (line_length < box_width) {
                textscr_print_asciichar(' ', true);
                line_length++;
            }
            textscr_print_scncode(0xE1);
            textscr_end_print();
            y_start++;
            textscr_begin_print(x_start, y_start);
            textscr_print_scncode(0x61);
            line_length = 0;
        }
        msg_ptr++;
    } while (*msg_ptr != 0);
    while (line_length < box_width) {
        textscr_print_asciichar(' ', true);
        line_length++;
    }
    textscr_print_scncode(0xE1);
    textscr_end_print();

    if (accept_input) {
        line_length = 0;
        textscr_set_printpos(3, y_start - 1);
        while (line_length < (box_width - 2)) {
            textscr_print_asciichar(' ', false);
            line_length++;
        }
        command_buffer[0] = 0;
        cmd_buf_ptr=0;
        ASCIIKEY = 0;
        input_line = y_start - 1;
        input_start_column = 3;
        input_max_length = 12;
        dialog_input_mode = imDialogField;
        textscr_print_ascii(0, 22, false, (uint8_t *)"%p40");
    }

    y_start++;
    dialog_last = y_start;
    line_length = 0;
    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0xFC);
    while (line_length < box_width) {
        textscr_print_scncode(0X62);
        line_length++;
    }
    textscr_print_scncode(0xFE);
    textscr_end_print();

    if (!logic_flag_isset(15)) {
        dialog_time = logic_vars[21];
        if (dialog_time > 0) {
            dialog_time = (dialog_time * 15) / 10;
        }

        joyports_poll();
        uint8_t prev_joybutton = joystick_fire;
        uint8_t prev_mousebutton = mouse_leftclick;

        while (1) {
            joyports_poll();
            uint8_t joypress = !prev_joybutton && joystick_fire;
            uint8_t mousepress = !prev_mousebutton && mouse_leftclick;

            if ((ASCIIKEY != 0) || joypress || mousepress || (dialog_time == 1)) {
                ASCIIKEY = 0;
                dialog_close();
                dialog_input_mode = imParser;
                break;
            } else {
                prev_joybutton = joystick_fire;
                prev_mousebutton = mouse_leftclick;
            }

            if (run_engine) {
                run_engine = false;
                if (dialog_time > 0) {
                    dialog_time--;
                }
            }
        }
    }
}

#pragma clang section bss="banked_bss" data="eh_data" rodata="eh_rodata" text="eh_text"

void dialog_gamesave_handler(char *filename) {
    uint8_t len = strlen(filename);

    for (uint8_t i = 0; i < len; i++) {
        if ((filename[i] >= 97) && (filename[i] <= 122)) {
            filename[i] = filename[i] - 32;
        }
    }

    uint8_t errcode;
    if (active_dialog == dtSave) {
        select_gamesave_mem();
        errcode = gamesave_save_to_disk(filename);
        if (errcode != 0) {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Disk error %d saving game.");
            dialog_show(false, print_string_buffer, errcode);
        }
    } else {
        select_gamesave_mem();
        errcode = gamesave_load_from_disk(filename);
        if (errcode == 255) {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Game save is not for this game.");
            dialog_show(false, print_string_buffer, errcode);
        } else if (errcode != 0) {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Disk error %d restoring game.");
            dialog_show(false, print_string_buffer, errcode);
        }
    }
    status_line_score = 255;
}

bool dialog_proc(void) {
    bool retval = false;
    switch (dialog_input_mode) {
        case imParser:
            if (dialog_handleinput(false)) {
                command_buffer[cmd_buf_ptr] = 0;
                parser_decode_string(command_buffer);
            }
        break;
        case imDialogField:
            select_gui_mem();
            if (dialog_handleinput(false)) {
                command_buffer[cmd_buf_ptr] = 0;
                dialog_close();
                dialog_input_mode = imParser;
                dialog_gamesave_handler(command_buffer);
                dialog_clear_keyboard();
            } else {
                retval = true;
            }
        break;
    }
    if (show_priority) {
        select_picdraw_mem();
        pic_show_priority();
    }
    return retval;
}

#pragma clang section bss="banked_bss" data="ls_spritedata" rodata="ls_spriterodata" text="ls_spritetext"

bool dialog_handleinput(bool force_accept) {
    if (!input_ok && !force_accept) {
        return false;
    }
    cursor_delay++;
    if (cursor_delay > 5) {
        if (cursor_flag) {
            textscr_set_printpos(cmd_buf_ptr + input_start_column, input_line);
            textscr_print_asciichar(' ', false);
        } else {
            textscr_set_printpos(cmd_buf_ptr + input_start_column, input_line);
            textscr_print_asciichar(' ', true);
        }
        cursor_flag = !cursor_flag;
        cursor_delay = 0;
    }

    uint8_t ascii_key = ASCIIKEY;
    if (ascii_key != 0) {
        ASCIIKEY = 0;
        select_gui_mem();
        return dialog_handleinput_internal(ascii_key);
    }
    return false;
}

void dialog_gamesave_begin(bool save) {
    if (save) {
        active_dialog = dtSave;
        memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Enter the name of\nthe new save file.\n(Uses device 9.)\n\n");
        dialog_show(true, print_string_buffer);
    } else {
        active_dialog = dtRestore;
        memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Enter the name of\nthe saved game to load.\n(Uses device 9.)\n\n");
        dialog_show(true, print_string_buffer);
    }
}

void dialog_show(bool accept_input, uint8_t __far *message_string, ...) {
    va_list ap;
    va_start(ap, message_string);
    textscr_format_string_valist(message_string, ap);
    va_end(ap);

    select_gui_mem();
    dialog_show_internal(accept_input);
}

void dialog_close(void) {
    for (uint8_t line = dialog_first; line <= dialog_last; line++)
    {
        textscr_clear_line(line);
    }
    dialog_first = 0;
    dialog_last = 0;
    show_object_view = false;
}

void dialog_get_string(uint8_t destination_str, uint8_t prompt, uint8_t row, uint8_t column, uint8_t max) {
    command_buffer[0] = 0;
    cmd_buf_ptr=0;
    ASCIIKEY = 0;
    input_line = row;
    input_max_length = max;

    input_start_column = column + textscr_print_ascii(column, row, false, (uint8_t *)"%M", prompt);

    while(!dialog_handleinput(true)) {
        while(!run_engine);
        run_engine = false;
    }
    uint8_t __far *dest_string = global_strings + (40 * destination_str);
    memmanage_strcpy_near_far(dest_string, (uint8_t *)command_buffer);
}

void dialog_clear_keyboard(void) {
    if (input_ok && (dialog_input_mode != imDialogField)) {
        command_buffer[0] = 0;
        cmd_buf_ptr=0;
        ASCIIKEY = 0;
        input_line = 22;
        input_start_column = 3;
        input_max_length = 37;

        textscr_print_ascii(0, 22, false, (uint8_t *)">%p40");
    }
}

void dialog_init(void) {
    dialog_first = 0;
    dialog_last = 0;
    input_line = 22;
    input_start_column = 3;
    input_max_length = 37;
    cursor_flag = false;
    cursor_delay = 0;
    dialog_input_mode = imParser;
    game_text = false;
    show_priority = false;
}