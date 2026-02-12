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

typedef struct menu_bar_entry {
    uint8_t text[25];
    uint8_t drop_start_x;
    uint8_t drop_height;
    uint8_t max_length;
    uint8_t menu_entries_ptr;
} menu_bar_entry_t;

typedef struct menu_entry {
    uint8_t text[25];
    uint8_t enabled;
    uint8_t controller;
    uint8_t end;
} menu_entry_t;

#pragma clang section bss="extradata"
uint8_t __far menu_bar_current;
uint8_t __far menu_opt_start;
uint8_t __far menu_opt_current;
uint8_t __far menu_bar_used;
uint8_t __far menu_opts_used;
menu_bar_entry_t __far main_menus[10];
menu_entry_t __far menu_entries[40];

#pragma clang section bss="banked_bss" data="gui_data" rodata="gui_rodata" text="gui_text"

typedef enum input_mode {
    imParser,
    imDialogField,
    imDialogMenu,
} input_mode_t;

typedef enum dialog_type {
    dtSave,
    dtRestore,
} dialog_type_t;

typedef struct keymap {
    uint8_t ascii;
    uint8_t alt_pressed;
    uint8_t controller;
} keymap_t;

typedef struct keycode_conv {
    uint8_t key2;
    uint8_t ascii;
    bool alt_pressed;
} keycode_conv_t;

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
static char prev_command_buffer[38];
static uint8_t cmd_buf_ptr = 0;
bool cursor_flag;
uint8_t cursor_delay;
uint8_t x_start;
uint8_t y_start;
input_mode_t dialog_input_mode;
dialog_type_t active_dialog;
static keymap_t keymaps[30];
uint8_t used_keymaps;

const keycode_conv_t pckeycodes[] = {
    // ALT A-Z
    {0x1e, 0xe5, true},
    {0x30, 0xfa, true},
    {0x2e, 0xe7, true},
    {0x30, 0xf0, true},
    {0x12, 0xe6, true},
    {0x21, 0x00, true},
    {0x22, 0xe8, true},
    {0x23, 0xfd, true},
    {0x17, 0xed, true},
    {0x24, 0xe9, true},
    {0x25, 0xe1, true},
    {0x26, 0xf3, true},
    {0x32, 0xb5, true},
    {0x31, 0xf1, true},
    {0x18, 0xf8, true},
    {0x19, 0xb6, true},
    {0x10, 0xa9, true},
    {0x13, 0xae, true},
    {0x1f, 0xa7, true},
    {0x14, 0xfe, true},
    {0x16, 0xfc, true},
    {0x2f, 0xd3, true},
    {0x11, 0xae, true},
    {0x2d, 0xd7, true},
    {0x15, 0xff, true},
    {0x2c, 0xf7, true},

    // ALT 1-0
    {0x78, 0xa1, true},
    {0x79, 0xaa, true},
    {0x7a, 0xa4, true},
    {0x7b, 0xa2, true},
    {0x7c, 0xb0, true},
    {0x7d, 0xa5, true},
    {0x7e, 0xb4, true},
    {0x7f, 0xe2, true},
    {0x80, 0xda, true},
    {0x81, 0xdb, true},

    // F1-F12
    {0x3b, 0xf1, false},
    {0x3c, 0xf2, false},
    {0x3d, 0xf3, false},
    {0x3e, 0xf4, false},
    {0x3f, 0xf5, false},
    {0x40, 0xf6, false},
    {0x41, 0xf7, false},
    {0x42, 0xf8, false},
    {0x43, 0xf9, false},
    {0x44, 0xfa, false},
    {0x85, 0xfb, false},
    {0x86, 0xfc, false},
};

static bool dialog_show_internal(bool accept_input, bool ok_cancel, bool draw_only);
static bool dialog_handlemenuinput(void);

void dialog_draw_menudrop_item_internal(uint8_t item_num) {
    textscr_set_printpos(2, item_num + 2);
    bool highlight = (item_num != menu_opt_current);
    uint8_t menu_item_num = (menu_opt_start + item_num);
    uint8_t ptr = 0;
    while (menu_entries[menu_item_num].text[ptr] != 0) {
        textscr_print_asciichar(menu_entries[menu_item_num].text[ptr], highlight);
        ptr++;
    }
    while (ptr < main_menus[menu_bar_current].max_length) {
        textscr_print_asciichar(' ', highlight);
        ptr++;
    }
}

void dialog_draw_menudrop_internal(void) {
    box_width = main_menus[menu_bar_current].max_length;
    box_height = main_menus[menu_bar_current].drop_height;
    x_start = main_menus[menu_bar_current].drop_start_x;
    y_start = 1;

    line_length = 0;
    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0xF0);
    while (line_length < box_width) {
        textscr_print_scncode(0XDD);
        line_length++;
    }
    textscr_print_scncode(0xEE);
    textscr_end_print();

    for (uint8_t cnt = 0; cnt < box_height; cnt++) {
        line_length = 0;
        y_start++;
        textscr_begin_print(x_start, y_start);
        textscr_print_scncode(0xDC);
        while (line_length < box_width) {
            textscr_print_asciichar(' ', true);
            line_length++;
        }
        textscr_print_scncode(0xDC);
        textscr_end_print();
    }

    y_start++;
    line_length = 0;
    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0xED);
    while (line_length < box_width) {
        textscr_print_scncode(0XDD);
        line_length++;
    }
    textscr_print_scncode(0xFD);
    textscr_end_print();
    
    y_start++;
    dialog_first = 1;
    dialog_last = y_start;
    menu_opt_current = 0;
    menu_opt_start = main_menus[menu_bar_current].menu_entries_ptr;
    for (uint8_t cnt = 0; cnt < main_menus[menu_bar_current].drop_height; cnt++) {
        dialog_draw_menudrop_item_internal(cnt);
    }
}

void dialog_draw_menubar_internal(void) {
    textscr_set_printpos(1, 0);
    for (uint8_t menu = 0; menu < menu_bar_used; menu++) {
        bool highlight = (menu != menu_bar_current);
        textscr_print_asciichar(' ', true);
        uint8_t ptr = 0;
        while (main_menus[menu].text[ptr] != 0) {
            textscr_print_asciichar(main_menus[menu].text[ptr], highlight);
            ptr++;
        }
    }
    dialog_draw_menudrop_internal();
}

bool dialog_nokeys_internal(void) {
    for (uint8_t ctr = 0; ctr < 9; ctr++) {
        POKE(0xd614, ctr);
        if (PEEK(0xD613) != 0xff) {
            return false;
        }
    }
    return true;
}

void dialog_handle_setkey_internal(uint8_t ascii, uint8_t keycode, uint8_t controller) {
    if (keycode == 0) {
        keymaps[used_keymaps].ascii = ascii;
        keymaps[used_keymaps].alt_pressed = false;
        keymaps[used_keymaps].controller = controller;
        used_keymaps++;
    } else {
        if (ascii == 0) {
            uint16_t keycode_num = sizeof(pckeycodes) / sizeof(keycode_conv_t);
            for (uint16_t cnt = 0; cnt < keycode_num; cnt++) {
                if (keycode == pckeycodes[cnt].key2) {
                    keymaps[used_keymaps].ascii = pckeycodes[cnt].ascii;
                    keymaps[used_keymaps].alt_pressed = pckeycodes[cnt].alt_pressed;
                    keymaps[used_keymaps].controller = controller;
                    used_keymaps++;
                }
            }
        }
    }
}
static bool dialog_handlemappedkey_internal(uint8_t ascii_key, bool alt_flag) {
    for (uint8_t cnt = 0; cnt < used_keymaps; cnt++) {
        if ((keymaps[cnt].ascii == ascii_key) && (keymaps[cnt].alt_pressed == alt_flag)) {
            logic_set_controller(keymaps[cnt].controller);
            return true;
        }
    }
    return false;
}

static bool dialog_handlemenuinput_internal(uint8_t ascii_key) {
    switch(ascii_key) {
        case 0x0d: {
            uint8_t menu_item_num = (menu_opt_start + menu_opt_current);
            logic_set_controller(menu_entries[menu_item_num].controller);
            return true;
        }
        case 0x11: {
            if (menu_opt_current < (main_menus[menu_bar_current].drop_height - 1)) {
                uint8_t old_sel = menu_opt_current;
                bool found = false;
                do {
                    menu_opt_current++;
                    uint8_t menu_item_num = (menu_opt_start + menu_opt_current);
                    if (menu_entries[menu_item_num].enabled) {
                        found = true;
                        break;
                    }
                } while (menu_opt_current < main_menus[menu_bar_current].drop_height);
                if (!found) {
                    menu_opt_current = old_sel;
                } else {
                    dialog_draw_menudrop_item_internal(old_sel);
                    dialog_draw_menudrop_item_internal(menu_opt_current);
                }
            }
            break;
        }
        case 0x1b:
            return true;
        case 0x1d: {
            if (menu_bar_current < (menu_bar_used - 1)) {
                dialog_close();
                menu_bar_current++;
                dialog_draw_menubar_internal();
            }
            break;
        }
        case 0x91: {
            if (menu_opt_current > 0) {
                uint8_t old_sel = menu_opt_current;
                bool found = false;
                do {
                    menu_opt_current--;
                    uint8_t menu_item_num = (menu_opt_start + menu_opt_current);
                    if (menu_entries[menu_item_num].enabled) {
                        found = true;
                        break;
                    }
                } while (menu_opt_current < 128);
                if (!found) {
                    menu_opt_current = old_sel;
                } else {
                    dialog_draw_menudrop_item_internal(old_sel);
                    dialog_draw_menudrop_item_internal(menu_opt_current);
                }
            }
            break;
        }
        case 0x9d: {
            if (menu_bar_current > 0) {
                dialog_close();
                menu_bar_current--;
                dialog_draw_menubar_internal();
            }
            break;
        }
    }
    return false;
}

static bool dialog_handleinput_internal(uint8_t ascii_key, bool alt_flag, bool *cancelled) {
    switch(ascii_key) {
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
            if (cancelled != NULL) {
                *cancelled = false;
            }
            return true;
        case 0x1b:
            if (cancelled != NULL) {
                *cancelled = true;
            }
            return true;
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

void dialog_recall_internal(void) {
    dialog_clear_keyboard();
    cmd_buf_ptr = 0;
    while (prev_command_buffer[cmd_buf_ptr] != 0) {
        command_buffer[cmd_buf_ptr] = prev_command_buffer[cmd_buf_ptr];
        textscr_set_printpos(cmd_buf_ptr + input_start_column, input_line);
        textscr_print_asciichar(prev_command_buffer[cmd_buf_ptr], false);
        cmd_buf_ptr++;
    }
}

static bool dialog_show_internal(bool accept_input, bool ok_cancel, bool draw_only) {
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
    textscr_print_scncode(0xF0);
    while (line_length < box_width) {
        textscr_print_scncode(0xDD);
        line_length++;
    }
    textscr_print_scncode(0xEE);
    textscr_end_print();

    line_length = 0;
    y_start++;
    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0xDC);
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
            textscr_print_scncode(0xDC);
            textscr_end_print();
            y_start++;
            textscr_begin_print(x_start, y_start);
            textscr_print_scncode(0xDC);
            line_length = 0;
        }
        msg_ptr++;
    } while (*msg_ptr != 0);
    while (line_length < box_width) {
        textscr_print_asciichar(' ', true);
        line_length++;
    }
    textscr_print_scncode(0xDC);
    textscr_end_print();

    if (accept_input) {
        line_length = 0;
        textscr_set_printpos(2, y_start);
        while (line_length < box_width) {
            textscr_print_asciichar(' ', false);
            line_length++;
        }
        command_buffer[0] = 0;
        cmd_buf_ptr=0;
        ASCIIKEY = 0;
        input_line = y_start;
        input_start_column = 2;
        input_max_length = 12;
        dialog_input_mode = imDialogField;
        textscr_print_ascii(0, 22, false, (uint8_t *)"%p40");
    }

    y_start++;
    dialog_last = y_start;
    line_length = 0;
    textscr_begin_print(x_start, y_start);
    textscr_print_scncode(0xED);
    while (line_length < box_width) {
        textscr_print_scncode(0XDD);
        line_length++;
    }
    textscr_print_scncode(0xFD);
    textscr_end_print();

    if (accept_input || draw_only) {
        return true;
    }
    
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
            uint8_t asciival = ASCIIKEY;
            if (asciival != 0) {
                ASCIIKEY = 0;
                while (!dialog_nokeys_internal()) {
                    ASCIIKEY = 0;
                }
                if (asciival == 0x1b) {
                    dialog_close();
                    dialog_input_mode = imParser;
                    return false;
                } else if (asciival == 0x0d) {
                    dialog_close();
                    dialog_input_mode = imParser;
                    return true;
                }
            }
            if (!ok_cancel && (joypress || mousepress || (dialog_time == 1))) {
                dialog_close();
                dialog_input_mode = imParser;
                return true;
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
    return true;
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
            dialog_show(false, false, print_string_buffer, errcode);
        } else {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Game saved.");
            dialog_show(false, false, print_string_buffer);
        }
    } else {
        select_gamesave_mem();
        errcode = gamesave_load_from_disk(filename);
        if (errcode == 255) {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Game save is not for this game.");
            dialog_show(false, false, print_string_buffer);
        } else if (errcode == 254) {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Game save is not for this version of MegaAGI.");
            dialog_show(false, false, print_string_buffer);
        } else if (errcode != 0) {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Disk error %d restoring game.");
            dialog_show(false, false, print_string_buffer, errcode);
        } else {
            memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Game restored.");
            dialog_show(false, false, print_string_buffer);
        }
    }
    status_line_score = 255;
}

bool dialog_proc(void) {
    bool retval = false;
    bool cancelled = false;
    switch (dialog_input_mode) {
        case imParser:
            if (dialog_handleinput(false, true, NULL)) {
                command_buffer[cmd_buf_ptr] = 0;
                cmd_buf_ptr = 0;
                while (command_buffer[cmd_buf_ptr] != 0) {
                    prev_command_buffer[cmd_buf_ptr] = command_buffer[cmd_buf_ptr];
                    cmd_buf_ptr++;
                }
                prev_command_buffer[cmd_buf_ptr] = 0;
                parser_decode_string(command_buffer);
            }
        break;
        case imDialogField:
            select_gui_mem();
            if (dialog_handleinput(false, false, &cancelled)) {
                command_buffer[cmd_buf_ptr] = 0;
                dialog_close();
                dialog_input_mode = imParser;
                if (!cancelled) {
                    dialog_gamesave_handler(command_buffer);
                }
                dialog_clear_keyboard();
            } else {
                retval = true;
            }
        break;
        case imDialogMenu:
            select_gui_mem();
            if (dialog_handlemenuinput()) {
                dialog_close();
                dialog_input_mode = imParser;
                dialog_clear_keyboard();
                status_line_score = 255;
                while (!dialog_nokeys_internal()) {
                    ASCIIKEY = 0;
                }
            } else {
                retval = true;
            }
        break;
    }
    return retval;
}

#pragma clang section bss="banked_bss" data="ls_spritedata" rodata="ls_spriterodata" text="ls_spritetext"
void dialog_draw_menubar(void) {
    textscr_print_ascii(0, 0, true, (uint8_t *)"%p40");
    menu_bar_current = 0;
    dialog_input_mode = imDialogMenu;
    select_gui_mem();
    dialog_draw_menubar_internal();
    while (!dialog_nokeys_internal()) {
        ASCIIKEY = 0;
    }
}

void dialog_enable_menu_item(uint8_t controller_number) {
    for (uint8_t counter = 0; counter < menu_opts_used; counter++) {
        if (menu_entries[counter].controller == controller_number) {
            menu_entries[counter].enabled = 1;
        }
    }
}

void dialog_disable_menu_item(uint8_t controller_number) {
    for (uint8_t counter = 0; counter < menu_opts_used; counter++) {
        if (menu_entries[counter].controller == controller_number) {
            menu_entries[counter].enabled = 0;
        }
    }
}

void dialog_set_menu(uint8_t message_number) {
    uint8_t __far *src_string = logic_locate_message(255, message_number);
    memmanage_strcpy_far_far(main_menus[menu_bar_used].text, src_string);
    uint8_t len = 0;
    while (src_string[len] != 0) {
        len++;
    }
    main_menus[menu_bar_used].max_length = 0;
    main_menus[menu_bar_used].menu_entries_ptr = menu_opts_used;
    main_menus[menu_bar_used].drop_start_x = menu_opt_start;
    main_menus[menu_bar_used].drop_height = 0;
    menu_bar_current = menu_bar_used;
    menu_opt_current = menu_opts_used;
    menu_opt_start += (len + 1);
    menu_bar_used++;
}

void dialog_set_menu_item(uint8_t message_number, uint8_t controller) {
    uint8_t __far *src_string = logic_locate_message(255, message_number);
    memmanage_strcpy_far_far(menu_entries[menu_opts_used].text, src_string);
    menu_entries[menu_opts_used].enabled = 1;
    menu_entries[menu_opts_used].controller = controller;
    menu_entries[menu_opt_current].end = 0;
    menu_entries[menu_opts_used].end = 1;
    main_menus[menu_bar_current].drop_height++;
    uint8_t len = 0;
    while (src_string[len] != 0) {
        len++;
    }
    if (len > main_menus[menu_bar_current].max_length) {
        main_menus[menu_bar_current].max_length = len;
        if ((main_menus[menu_bar_current].drop_start_x + main_menus[menu_bar_current].max_length + 2) > 39) {
            main_menus[menu_bar_current].drop_start_x = 40 - main_menus[menu_bar_current].max_length - 2;
        }
    }
    menu_opt_current = menu_opts_used;
    menu_opts_used++;
}

bool dialog_handlemenuinput(void) {
    uint8_t ascii_key = ASCIIKEY;
    if (ascii_key != 0) {
        ASCIIKEY = 0;
        select_gui_mem();
        return dialog_handlemenuinput_internal(ascii_key);
    }
    return false;
}

bool dialog_handleinput(bool force_accept, bool mapkeys, bool *cancelled) {
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
    uint8_t modkey = MODKEY;
    if (ascii_key != 0) {
        ASCIIKEY = 0;
        select_gui_mem();
        if (mapkeys) {
            if (dialog_handlemappedkey_internal(ascii_key, modkey & 0x10)) {
                // The mapped key was handled, so swallow it
                return false;
            }
        }
        return dialog_handleinput_internal(ascii_key, modkey & 0x10, cancelled);
    }
    return false;
}

void dialog_handle_setkey(uint8_t ascii, uint8_t keycode, uint8_t controller) {
    select_gui_mem();
    dialog_handle_setkey_internal(ascii, keycode, controller);
}

void dialog_recall(void) {
    select_gui_mem();
    dialog_recall_internal();
}

void dialog_gamesave_begin(bool save) {
    if (save) {
        active_dialog = dtSave;
        memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Enter the name of\nthe new save file.\n(Uses device 9.)\n");
        dialog_show(true, false, print_string_buffer);
    } else {
        active_dialog = dtRestore;
        memmanage_strcpy_near_far(print_string_buffer, (uint8_t *)"Enter the name of\nthe saved game to load.\n(Uses device 9.)\n");
        dialog_show(true, false, print_string_buffer);
    }
}

bool dialog_show(bool accept_input, bool ok_cancel, uint8_t __far *message_string, ...) {
    va_list ap;
    va_start(ap, message_string);
    textscr_format_string_valist(message_string, ap);
    va_end(ap);

    select_gui_mem();
    return dialog_show_internal(accept_input, ok_cancel, false);
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

    while(!dialog_handleinput(true, false, NULL)) {
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
    menu_bar_used = 0;
    menu_opts_used = 0;
    menu_opt_start = 0;
    main_menus[0].menu_entries_ptr = 0;
    menu_entries[0].enabled = 0;
    prev_command_buffer[0] = 0;
    used_keymaps = 0;
    dialog_first = 0;
    dialog_last = 0;
    input_line = 22;
    input_start_column = 3;
    input_max_length = 37;
    cursor_flag = false;
    cursor_delay = 0;
    dialog_input_mode = imParser;
    game_text = false;
}