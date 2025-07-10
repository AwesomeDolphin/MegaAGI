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
#include "gfx.h"
#include "main.h"
#include "memmanage.h" 
#include "parser.h"

#pragma clang section bss="extradata"
__far uint8_t format_string_buffer[1024];

#pragma clang section bss="midmembss" data="ultmemdata" rodata="ultmemrodata" text="ultmemtext"

typedef enum input_mode {
    imParser,
    imPressKey,
    imDialogField,
} input_mode_t;

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

uint8_t my_ultoa_invert(unsigned long val, char __far *str, int base)
{
  uint8_t len = 0;
  do
    {
      int v;

      v   = val % base;
      val = val / base;

      if (v <= 9)
        {
          v += '0';
        }
      else
        {
          v += 'A' - 10;
        }

      *str++ = v;
      len++;
    }
  while (val);

  return len;
}

uint8_t my_atoi(uint8_t __far *str, uint8_t __far **endptr)
{
  uint8_t val = 0;
  uint8_t digit;
  while (*str != 0) {
    if (*str >= '0' && *str <= '9') {
      digit = *str - '0';
    } else {
      break;
    }
    val = (val * 10) + digit;
    str++;
  }
  if (endptr) {
    *endptr = str;
  }
  return val;
}

void dialog_format_string_valist(uint8_t __far *formatstring, va_list ap) {
  char buffer[17];
  uint16_t padlen = 0;
  uint8_t __far *ascii_string = (uint8_t __far *)formatstring;
  while (*ascii_string != 0) {
    uint8_t asciichar = *ascii_string;
    if (asciichar == '%') {
      ascii_string++;
      switch (*ascii_string) {
        case 'p': {
          uint8_t padcol = my_atoi(ascii_string + 1, &ascii_string);
          while (padlen < padcol) {
            format_string_buffer[padlen] = ' ';
            padlen++;
          }
          break;
        }
        case 'w': {
          uint8_t wordnum = my_atoi(ascii_string + 1, &ascii_string);
          const char *wordptr = parser_word_pointers[wordnum - 1];
          uint8_t wordchr = (uint8_t)*wordptr;
          while (wordchr != 0) {
            format_string_buffer[padlen] = wordchr;
            padlen++;
            wordptr++;
            wordchr = (uint8_t)*wordptr;
          };
          break;
        }
        case 'd':
        case 'x': {
          uint32_t param = va_arg(ap, unsigned int);
          uint8_t len = my_ultoa_invert(param, (char __far *)buffer, (*ascii_string == 'x') ? 16 : 10);
          for (; len > 0; len--) {
            format_string_buffer[padlen] = buffer[len-1];
            padlen++;
          }
          ascii_string++;
          break;
        }
        case 'D':
        case 'X': {
          uint32_t param = va_arg(ap, unsigned long);
          uint8_t len = my_ultoa_invert(param, (char __far *)buffer, (*ascii_string == 'X') ? 16 : 10);
          for (; len > 0; len--) {
            format_string_buffer[padlen] = buffer[len-1];
            padlen++;
          }
          ascii_string++;
          break;
        }
      }
      continue;
    } else if (*ascii_string == '\\') {
        ascii_string++;
        if (*ascii_string == 'n') {
            asciichar = '\n';
        } else {
            asciichar = *ascii_string;
        }
    }
    format_string_buffer[padlen] = asciichar;
    padlen++;
    ascii_string++;
  }
  format_string_buffer[padlen] = 0;
}

void dialog_format_string(uint8_t __far *formatstring, ...) {
    va_list ap;
    va_start(ap, formatstring);
    dialog_format_string_valist(formatstring, ap);
    va_end(ap);
}

void dialog_print_ascii(uint8_t x, uint8_t y, bool reverse, uint8_t __far *formatstring, ...) {
    va_list ap;
    va_start(ap, formatstring);
    dialog_format_string_valist(formatstring, ap);
    va_end(ap);

    gfx_print_asciistr(x, y, reverse, format_string_buffer);
}

void dialog_clear_keyboard(void) {
    command_buffer[0] = 0;
    cmd_buf_ptr=0;
    ASCIIKEY = 0;
    input_line = 22;
    input_start_column = 3;
    input_max_length = 37;

    dialog_print_ascii(0, 22, false, (uint8_t __far *)">%p40");
}

static bool dialog_handleinput(void) {
    if (!input_ok) {
        return false;
    }
    cursor_delay++;
    if (cursor_delay > 5) {
        if (cursor_flag) {
            gfx_set_printpos(cmd_buf_ptr + input_start_column, input_line);
            gfx_print_asciichar(' ', false);
        } else {
            gfx_set_printpos(cmd_buf_ptr + input_start_column, input_line);
            gfx_print_asciichar(' ', true);
        }
        cursor_flag = !cursor_flag;
        cursor_delay = 0;
    }

    uint8_t ascii_key = ASCIIKEY;
    if (ascii_key != 0) {
        ASCIIKEY = 0;
        switch(ascii_key) {
            case 0x09:
                logic_set_controller(4);
                break;
            case 0x14:
                if (cmd_buf_ptr > 0) {
                    gfx_set_printpos(cmd_buf_ptr + input_start_column, input_line);
                    gfx_print_asciichar(' ', false);
                    cmd_buf_ptr--;
                    gfx_set_printpos(cmd_buf_ptr + input_start_column, input_line);
                    gfx_print_asciichar(' ', false);
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
            default:
                if ((ascii_key >= 32) && (ascii_key < 127)) {
                    if (cmd_buf_ptr < input_max_length) {
                        command_buffer[cmd_buf_ptr] = ascii_key;
                        gfx_set_printpos(cmd_buf_ptr + input_start_column, input_line);
                        gfx_print_asciichar(ascii_key, false);
                        cmd_buf_ptr++;
                    }
                }
                break;
        }
    }
    return false;
}

void dialog_show(uint8_t __far *message_string, bool accept_input) {
    if (accept_input) {
        dialog_print_ascii(0, 22, false, (uint8_t __far *)"%p40");
    }
    dialog_format_string(message_string);

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

    if (accept_input) {
        line_length = 0;
        gfx_set_printpos(3, y_start - 1);
        while (line_length < (box_width - 2)) {
            gfx_print_asciichar(' ', false);
            line_length++;
        }
        input_line = y_start - 1;
        input_start_column = 3;
        input_max_length = 12;
        dialog_input_mode = imDialogField;
    } else {
        dialog_input_mode = imPressKey;
    }

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

void dialog_close(void) {
    for (uint8_t line = dialog_first; line <= dialog_last; line++)
    {
        gfx_clear_line(line);
    }
    dialog_first = 0;
    dialog_last = 0;
    show_object_view = false;
}

bool dialog_proc(void) {
    bool retval = false;
    switch (dialog_input_mode) {
        case imParser:
            if (dialog_handleinput()) {
                command_buffer[cmd_buf_ptr] = 0;
                parser_decode_string(command_buffer);
            }
        break;
        case imPressKey:
            if ((ASCIIKEY != 0) || (dialog_time == 1)) {
                ASCIIKEY = 0;
                dialog_close();
                dialog_input_mode = imParser;
            } else {
                retval = true;
            }
            if (dialog_time > 0) {
                dialog_time--;
            }
        break;
        case imDialogField:
            if (dialog_handleinput()) {
                command_buffer[cmd_buf_ptr] = 0;
                dialog_close();
                dialog_input_mode = imParser;
                gamesave_dialog_handler(command_buffer);
                dialog_clear_keyboard();
            } else {
                retval = true;
            }
        break;
    }
    return retval;
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
}