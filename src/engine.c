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
#include "simplefile.h"
#include "sprite.h"
#include "logic.h"
#include "memmanage.h"
#include "main.h"

volatile uint8_t view_x = 40;
volatile uint8_t view_y = 40;
volatile uint8_t frame_counter;
static uint8_t picture = 1;
static uint8_t keypress_up;
static uint8_t keypress_down;
static uint8_t keypress_left;
static uint8_t keypress_right;
static uint8_t last_cursorreg;
static uint8_t last_columnzero;
static uint8_t keypress_f5;
volatile uint8_t drawing_screen = 2;
volatile uint8_t viewing_screen = 0;
volatile uint8_t frame_dirty;
static char command_buffer[38];
static uint8_t cmd_buf_ptr = 0;


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
    const uint8_t joystick_direction[16] = {
        0, 0, 0, 0, 0, 4, 2, 3,
        0, 6, 8, 7, 0, 5, 1, 0
    };
    uint8_t port2joy = PEEK(0xDC00) & 0x0f;
    uint8_t new_object_dir = joystick_direction[port2joy];
    sprite_set_direction(0, new_object_dir);
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

void engine_player_control(bool enable) {

}

void engine_unblock(void) {

}

void engine_horizon(uint8_t horizon_line) {

}

void parse_debug_command(char *command) {
    char *cmd = strtok(command, " ");
    if (0 == strcmp(cmd, "PIC")) {
        char *arg = strtok(NULL, " ");
        if (arg == NULL) {
            return;
        }
        sprite_erase(0);
        sprite_set_direction(0,0);
        uint8_t pic_num = atoi(arg);
        draw_pic(drawing_screen, pic_num, 0);
        sprite_draw(0);
        drawing_screen ^= viewing_screen;
        viewing_screen ^= drawing_screen;
        drawing_screen ^= viewing_screen;
        gfx_showgfx(viewing_screen);
        gfx_copygfx(viewing_screen);
    } else if (0 == strcmp(cmd, "SOUND")) {
        char *arg = strtok(NULL, " ");
        if (arg == NULL) {
            return;
        }
        uint8_t sound_num = atoi(arg);
        play_sound(sound_num);
    } else if (0 == strcmp(cmd, "VIEW")) {
        char *arg = strtok(NULL, " ");
        if (arg == NULL) {
            return;
        }
        sprite_erase(0);
        uint8_t view_number = atoi(arg);
        sprite_set_position(0, 40, 40);
        sprite_set_view(0, view_number);
        sprite_draw(0);
        frame_dirty = 1;
    }
}

void engine_clear_keyboard(void) {
    command_buffer[0] = 0;
    cmd_buf_ptr=0;
    gfx_print_splitascii("\r>");
}

void run_loop(void) {
    view_init();
    gfx_switchto();
    gfx_blackscreen();
    gfx_print_splitchar(CHAR_CLEARHOME);
    gfx_showgfx(0);
    hook_irq();
    gfx_print_splitascii("Welcome to AGI demo!\r");
    logic_init();
    while (1) {
        logic_run(0);
        gfx_print_splitascii("Logic cycle ended\r");
    }
    while (1) {
        handle_movement_joystick();
        if (frame_counter == 0) {
            if (ASCIIKEY != 0) {
                uint8_t petscii = PETSCIIKEY;
                ASCIIKEY = 0;
                if (petscii == 0x14) {
                    if (cmd_buf_ptr > 0) {
                        cmd_buf_ptr--;
                        gfx_print_splitchar(petscii);
                    }
                } else if (petscii == 0x0d) {
                    command_buffer[cmd_buf_ptr] = 0;
                    parse_debug_command(command_buffer);
                    cmd_buf_ptr=0;
                    gfx_print_splitchar(petscii);
                    gfx_print_splitchar('>');
                } else {
                    if (cmd_buf_ptr < 37) {
                        command_buffer[cmd_buf_ptr] = petscii;
                        gfx_print_splitchar(petscii);
                        cmd_buf_ptr++;
                    }
                }
            }

            if (sprite_move(0)) {
                sprite_draw(0);
                frame_dirty = 1;
            }
            while(frame_dirty);
        }
    }
}

void engine_interrupt_handler(void) {
    frame_counter++;
    if (frame_counter < 2) {
        return;
    }
    frame_counter = 0;
    if (frame_dirty) {
        drawing_screen ^= viewing_screen;
        viewing_screen ^= drawing_screen;
        drawing_screen ^= viewing_screen;
        gfx_showgfx(viewing_screen);
        gfx_copygfx(viewing_screen);
        frame_dirty = 0;
    }
}
