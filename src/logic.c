#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "engine.h"
#include "gfx.h"
#include "logic.h"
#include "main.h"
#include "memmanage.h"
#include "parser.h"
#include "pic.h"
#include "sound.h"
#include "sprite.h"
#include "view.h"
#include "volume.h"

typedef struct logic_info {
    uint16_t offset;
    uint8_t __far *text_offset;
} logic_info_t;

#pragma clang section bss="extradata"
__far static logic_info_t logic_infos[256];
#pragma clang section bss=""

#pragma clang section bss="nographicsbss" data="nographicsdata" rodata="nographicsrodata" text="nographicstext"

uint8_t __far *logic_vars = NULL;
uint8_t __far *logic_flags = NULL;

typedef struct logic_stack_entry {
    uint8_t __far *return_address;
    uint8_t return_logic_num;
} logic_stack_entry_t;

static logic_stack_entry_t logic_stack[16];
static uint8_t __far *program_counter;
static uint8_t __far *logic_strings;
static char game_id[8];
static uint8_t logic_stack_ptr;
static uint8_t local_string[256];

void logic_strcpy_far_far(uint8_t __far *dest_string, uint8_t __far *src_string) {
    *dest_string = *src_string;
    while (*dest_string != 0) {
        dest_string++;
        src_string++;
        *dest_string = *src_string;
    }
}

void logic_strcpy_far_near(uint8_t *dest_string, uint8_t __far *src_string) {
    *dest_string = *src_string;
    while (*dest_string != 0) {
        dest_string++;
        src_string++;
        *dest_string = *src_string;
    }
}

void logic_set_flag(uint8_t flag) {
    uint8_t flag_reg = (flag >> 3);
    uint8_t flag_reg_val = (1 << (flag & 0x07));
    logic_flags[flag_reg] |= flag_reg_val;
}

void logic_reset_flag(uint8_t flag) {
    uint8_t flag_reg = (flag >> 3);
    uint8_t flag_reg_val = ~(1 << (flag & 0x07));
    logic_flags[flag_reg] &= flag_reg_val;
}

uint16_t logic_locate_message(uint8_t logic_num, uint8_t message_num) {
    uint16_t message_index = (message_num * 2) + 1;
    uint16_t message_offset = logic_infos[logic_num].text_offset[message_index + 1] << 8;
    message_offset |= logic_infos[logic_num].text_offset[message_index] + 1;
    return message_offset;
}

bool logic_test_commands(void) {
    bool result;
    switch (*program_counter) {
        case 0x01:
            // equaln test
            result = (logic_vars[program_counter[1]] == program_counter[2]);
            program_counter += 3;
            break;
        case 0x02:
            // equalv test
            result = (logic_vars[program_counter[1]] == logic_vars[program_counter[2]]);
            program_counter += 3;
            break;
        case 0x03:
            // lessn test
            result = (logic_vars[program_counter[1]] < program_counter[2]);
            program_counter += 3;
            break;
        case 0x04:
            // lessv test
            result = (logic_vars[program_counter[1]] < logic_vars[program_counter[2]]);
            program_counter += 3;
            break;
         case 0x05:
            // greatern test
            result = logic_vars[program_counter[1]] > program_counter[2];
            program_counter += 3;
            break;
        case 0x07:
            // isset test
            result = ((logic_flags[program_counter[1] >> 3]) >> (program_counter[1] & 0x07)) & 0x01;
            program_counter += 2;
            break;
        case 0x0B: {
            // posn test
            view_info_t vi = sprites[program_counter[1]].view_info;
            if ((program_counter[2] <= vi.x_pos) && (vi.x_pos <= program_counter[4]) && (program_counter[3] <= vi.y_pos) && (vi.y_pos <= program_counter[5])) {
                result = true;
            } else {
                result = false;
            }
            program_counter += 6;
            break;
        }
        case 0x0C:
            // controller test
            // TODO: implement controller test
            result = false;
            program_counter += 2;
            break;
        case 0x0D:
            // havekey test
            result = (ASCIIKEY != 0);
            program_counter += 1;
            break;
        case 0x0E:
        {
            // said test
            bool player_entry = ((logic_flags[2 >> 3]) >> (2 & 0x07)) & 0x01;
            bool accepted_entry = ((logic_flags[4 >> 3]) >> (4 & 0x07)) & 0x01;
            uint8_t numwords = program_counter[1];
            if (!player_entry || accepted_entry) {
                result = false;
            } else {
                result = true;
                uint16_t wordnum;
                for (uint8_t i = 0; i < numwords; i++) {
                    uint8_t argpos = (i * 2) + 2;
                    wordnum = program_counter[argpos];
                    wordnum |= program_counter[argpos + 1] << 8;
                    if (wordnum == 1) {
                        // Matches any, keep going
                    } else if (wordnum == 9999) {
                        // matches anything
                        break;
                    } else if (i >= parser_word_index) { 
                        // expected more than provided
                        result = false;
                        break;
                    } else if (wordnum != parser_word_numbers[i]) {
                        result = false;
                        break;
                    }
                }
                if (result) {
                    if (wordnum != 9999) {
                        if (numwords != parser_word_index) {
                            result = false;
                        }
                    }
                }
                if (result) {
                    logic_set_flag(4);
                }
            }
            program_counter += 2 + (numwords * 2);
            break;
        }
        default:
            gfx_print_ascii(0, 0, "FAULT: UNKTEST=%x:%X", *program_counter,(uint32_t)program_counter);
            while(1);
    }
    return result;
}

bool logic_test(void) {
    bool truth = true;
    bool invert_test = false;
    bool or_mode = false;
    bool or_result;
    while(1) {
        switch (*program_counter) {
            case 0xfc: {
                if (or_mode) {
                    or_mode = false;
                    truth &= or_result;
                } else {
                    or_mode = true;
                    or_result = false;
                }
                program_counter += 1;
                break;
            }
            case 0xfd:
                invert_test = true;
                program_counter += 1;
                continue;
            case 0xff:
                // end of tests
                program_counter += 1;
                return truth;
            default: {
                bool result = logic_test_commands();
                if (invert_test) {
                    result = !result;
                }
                if (or_mode) {
                    or_result |= result;
                } else {
                    truth &= result;
                }
            }
        }
        invert_test = false;
    }
}

void logic_run(uint8_t logic_num) {
    static bool debug=false;
    program_counter = chipmem_base + (logic_infos[logic_num].offset + 2);
    while(1) {
        if (logic_num == 53) {
            //debug = true;
        }
        if (debug) {
            gfx_print_ascii(0,0,"A:%x %X", logic_num, (uint32_t)program_counter);
            gfx_print_ascii(0,1,"B:%x", *program_counter);
            while(ASCIIKEY==0) {
    
            }
            ASCIIKEY=0;
        }
        switch (*program_counter) {
            case 0x00: {
                // return
                if (logic_num == 0) {
                    logic_reset_flag(5);
                    return;
                }
                program_counter = logic_stack[logic_stack_ptr].return_address;
                logic_num = logic_stack[logic_stack_ptr].return_logic_num;
                logic_stack_ptr++;
                break;
            }
            case 0x01: {
                if (logic_vars[program_counter[1]] < 255) {
                    logic_vars[program_counter[1]]++;
                }
                program_counter += 2;
                break;
            }
            case 0x02: {
                if (logic_vars[program_counter[1]] > 0) {
                    logic_vars[program_counter[1]]--;
                }
                program_counter += 2;
                break;
            }
            case 0x03: {
                // assignn
                logic_vars[program_counter[1]] = program_counter[2];
                program_counter += 3;
                break;
            }
            case 0x04: {
                // assignv
                logic_vars[program_counter[1]] = logic_vars[program_counter[2]];
                program_counter += 3;
                break;
            }
            case 0x05: {
                // addn
                logic_vars[program_counter[1]] += program_counter[2];
                program_counter += 3;
                break;
            }
            case 0x06: {
                // addv
                logic_vars[program_counter[1]] += logic_vars[program_counter[2]];
                program_counter += 3;
                break;
            }
            case 0x07: {
                // subn
                logic_vars[program_counter[1]] -= program_counter[2];
                program_counter += 3;
                break;
            }
            case 0x08: {
                // subv
                logic_vars[program_counter[1]] -= logic_vars[program_counter[2]];
                program_counter += 3;
                break;
            }
            case 0x0B: {
                logic_vars[logic_vars[program_counter[1]]] = program_counter[2];
                program_counter += 3;
                break;
            }
            case 0x0C: {
                logic_set_flag(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x0D: {
                logic_reset_flag(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x0f: {
                logic_set_flag(logic_vars[program_counter[1]]);
                program_counter += 2;
                break;
            }
            case 0x10: {
                logic_reset_flag(logic_vars[program_counter[1]]);
                program_counter += 2;
                break;
            }
            case 0x12: {
                // new.screen
                sprite_stop_all();
                sprite_unanimate_all();
                chipmem_free_unlocked();
                player_control = true;
                sprites[0].frozen = false;
                engine_unblock();
                horizon_line = 36;
                logic_vars[1] = logic_vars[0];
                logic_vars[0] = program_counter[1];
                logic_vars[4] = 0;
                logic_vars[5] = 0;
                logic_vars[16] = sprite_get_view(0);
                logic_load(logic_vars[0]);
                sprite_setedge(0, logic_vars[2]);
                logic_vars[2] = 0;
                logic_set_flag(5);
                engine_clear_keyboard();
                program_counter += 2;
                return;
            }
            case 0x14: {
                // load.logics
                logic_load(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x16: {
                // call
                logic_stack_ptr--;
                logic_stack[logic_stack_ptr].return_address = program_counter + 2;
                logic_stack[logic_stack_ptr].return_logic_num = logic_num;
                logic_num = program_counter[1];
                program_counter = chipmem_base + (logic_infos[logic_num].offset + 2);
                break;
            }
            case 0x17: {
                // call.v
                logic_stack_ptr--;
                logic_stack[logic_stack_ptr].return_address = program_counter + 2;
                logic_stack[logic_stack_ptr].return_logic_num = logic_num;
                logic_num = logic_vars[program_counter[1]];
                program_counter = chipmem_base + (logic_infos[logic_num].offset + 2);
                break;
            }
            case 0x18: {
                // load.pic
                pic_load(logic_vars[program_counter[1]]);
                program_counter += 2;
                break;
            }
            case 0x19: {
                // draw.pic
                gfx_hold_flip(true);
                draw_pic();
                program_counter += 2;
                break;
            }
            case 0x1A: {
                // show.pic
                gfx_hold_flip(false);
                program_counter += 1;
                break;
            }
            case 0x1B: {
                // discard.pic
                pic_discard(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x1E: {
                // load.view
                view_load(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x1F: {
                // load.view.v
                view_load(logic_vars[program_counter[1]]);
                program_counter += 2;
                break;
            }
            case 0x21: {
                // animate_obj
                animated_sprites[animated_sprite_count] = program_counter[1];
                sprites[program_counter[1]].view_info.priority_override = false;
                sprites[program_counter[1]].observe_horizon = true;
                sprites[program_counter[1]].observe_blocks = true;
                sprites[program_counter[1]].updatable = true;
                sprites[program_counter[1]].step_size = 1;
                if (program_counter[1] == 0) {
                    sprites[program_counter[1]].ego = true;
                } else {
                    sprites[program_counter[1]].ego = false;
                }
                sprites[program_counter[1]].prg_movetype = pmmNone;
                animated_sprite_count++;
                program_counter += 2;
                break;
            }
            case 0x23: {
                // draw
                sprite_mark_drawable(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x24: {
                // erase
                sprites[program_counter[1]].drawable = false;
                program_counter += 2;
                break;
            }
            case 0x25: {
                // position
                sprites[program_counter[1]].view_info.x_pos = program_counter[2];
                sprites[program_counter[1]].view_info.y_pos = program_counter[3];
                if (!sprites[program_counter[1]].view_info.priority_override) {
                    sprites[program_counter[1]].view_info.priority = priorities[sprites[program_counter[1]].view_info.y_pos];
                }
                program_counter += 4;
                break;
            }
            case 0x26: { 
                // position.v
                sprites[program_counter[1]].view_info.x_pos = logic_vars[program_counter[2]];
                sprites[program_counter[1]].view_info.y_pos = logic_vars[program_counter[3]];
                if (!sprites[program_counter[1]].view_info.priority_override) {
                    sprites[program_counter[1]].view_info.priority = priorities[sprites[program_counter[1]].view_info.y_pos];
                }
                program_counter += 4;
                break;
            }
            case 0x27: {
                // get.posn
                logic_vars[program_counter[2]] = sprites[program_counter[1]].view_info.x_pos;
                logic_vars[program_counter[3]] = sprites[program_counter[1]].view_info.y_pos;
                program_counter += 4;
                break;
            }
            case 0x28: { 
                // reposition
                int16_t x_offset = logic_vars[program_counter[2]];
                int16_t y_offset = logic_vars[program_counter[3]];
                if (x_offset & 0x80) {
                    x_offset |= 0xff00;
                }
                if (y_offset & 0x80) {
                    y_offset |= 0xff00;
                }
                sprites[program_counter[1]].view_info.x_pos += x_offset;
                sprites[program_counter[1]].view_info.y_pos += y_offset;
                if (!sprites[program_counter[1]].view_info.priority_override) {
                    sprites[program_counter[1]].view_info.priority = priorities[sprites[program_counter[1]].view_info.y_pos];
                }
                program_counter += 4;
                break;
            }
            case 0x29: {
                // set.view
                sprite_set_view(program_counter[1], program_counter[2]);
                program_counter += 3;
                break;
            }
            case 0x2A: {
                // set.view.v
                sprite_set_view(program_counter[1], logic_vars[program_counter[2]]);
                program_counter += 3;
                break;
            }
            case 0x2B: {
                // set.loop
                agisprite_t sprite = sprites[program_counter[1]];
                sprite.loop_index = program_counter[2];
                sprite.loop_offset = select_loop(&sprite.view_info, program_counter[2]);
                program_counter += 3;
                break;
            }
            case 0x2C: {
                // set.loop.v
                agisprite_t sprite = sprites[program_counter[1]];
                sprite.loop_index = logic_vars[program_counter[2]];
                sprite.loop_offset = select_loop(&sprite.view_info, logic_vars[program_counter[2]]);
                program_counter += 3;
                break;
            }
            case 0x2F: {
                // set.cel
                sprites[program_counter[1]].cel_index = program_counter[2];
                program_counter += 3;
                break;
            }
            case 0x30: {
                // set.cel.v
                sprites[program_counter[1]].cel_index = logic_vars[program_counter[2]];
                program_counter += 3;
                break;
            }
            case 0x36: {
                // set.priority
                sprites[program_counter[1]].view_info.priority = program_counter[2];
                sprites[program_counter[1]].view_info.priority_override = true;
                program_counter += 3;
                break;
            }
            case 0x37: {
                // set.priority.v
                sprites[program_counter[1]].view_info.priority = logic_vars[program_counter[2]];
                sprites[program_counter[1]].view_info.priority_override = true;
                program_counter += 3;
                break;
            }
            case 0x3A: {
                sprites[program_counter[1]].updatable = false;
                program_counter += 2;
                break;
            }
            case 0x3B: {
                sprites[program_counter[1]].updatable = true;
                program_counter += 2;
                break;
            }
            case 0x3D: {
                sprites[program_counter[1]].observe_horizon = false;
                program_counter += 2;
                break;
            }
            case 0x3E: {
                sprites[program_counter[1]].observe_horizon = true;
                program_counter += 2;
                break;
            }
            case 0x3F: {
                horizon_line = program_counter[1];
                program_counter += 2;
                break;
            }
            case 0x40: {
                // object.on.water
                sprites[program_counter[1]].on_water = true;
                program_counter += 2;
                break;
            }
            case 0x43: {
                sprites[program_counter[1]].observe_object_collisions = false;
                program_counter += 2;
                break;
            }
            case 0x44: {
                sprites[program_counter[1]].observe_object_collisions = true;
                program_counter += 2;
                break;
            }
            case 0x45: {
                if (sprites[program_counter[1]].drawable && sprites[program_counter[2]].drawable) {
                    logic_vars[program_counter[3]]  =
                    (sprites[program_counter[1]].view_info.x_pos - sprites[program_counter[2]].view_info.x_pos) +
                    (sprites[program_counter[1]].view_info.y_pos - sprites[program_counter[2]].view_info.y_pos);
                } else {
                    logic_vars[program_counter[3]]  = 0xff;
                }
                program_counter += 4;
                break;
            }
            case 0x46: {
                // stop.cycling
                sprites[program_counter[1]].cycling = false;
                program_counter += 2;
                break;
            }
            case 0x47: {
                // start.cycling
                sprites[program_counter[1]].cycling = true;
                program_counter += 2;
                break;
            }
            case 0x48: {
                // normal.cycle
                sprites[program_counter[1]].reverse = false;
                program_counter += 2;
                break;
            }
            case 0x49: { 
                // end.of.loop
                sprites[program_counter[1]].cycling = false;
                sprites[program_counter[1]].end_of_loop = program_counter[2];
                sprites[program_counter[1]].reverse = false;
                sprites[program_counter[1]].cel_index = 0;
                program_counter += 3;
                break;
            }
            case 0x4A: {
                // reverse.cycle
                sprites[program_counter[1]].reverse = false;
                program_counter += 2;
                break;
            }
            case 0x4B: { 
                // end.of.loop
                sprites[program_counter[1]].cycling = false;
                sprites[program_counter[1]].end_of_loop = program_counter[2];
                sprites[program_counter[1]].reverse = true;
                sprites[program_counter[1]].cel_index = sprites[program_counter[1]].view_info.number_of_cels - 1;
                program_counter += 3;
                break;
            }
            case 0x4C: {
                // cycle.time
                sprites[program_counter[1]].cycle_time = logic_vars[program_counter[2]];
                sprites[program_counter[1]].cycle_count = logic_vars[program_counter[2]];
                program_counter += 3;
                break;
            }
            case 0x4D: {
                // stop.motion
                if (program_counter[1] == 0) {
                    player_control = false;
                }
                sprites[program_counter[1]].object_dir = 0;
                sprites[program_counter[1]].frozen = true;
                program_counter += 2;
                break;
            }
            case 0x4E: {
                // start.motion
                if (program_counter[1] == 0) {
                    player_control = true;
                }
                sprites[program_counter[1]].object_dir = 0;
                sprites[program_counter[1]].frozen = false;
                program_counter += 2;
                break;
            }
            case 0x51: {
                // move.obj
                sprites[program_counter[1]].prg_movetype = pmmMoveTo;
                sprites[program_counter[1]].prg_x_destination = program_counter[2];
                sprites[program_counter[1]].prg_y_destination = program_counter[3];
                if (program_counter[4] > 0) {
                    sprites[program_counter[1]].prg_speed = program_counter[4];
                } else {
                    sprites[program_counter[1]].prg_speed = sprites[program_counter[1]].step_size;
                }
                sprites[program_counter[1]].prg_speed_squared = sprites[program_counter[1]].prg_speed * sprites[program_counter[1]].prg_speed;
                sprites[program_counter[1]].prg_complete_flag = program_counter[5];
                sprites[program_counter[1]].updatable = true;
                program_counter += 6;
                break;
            }
            case 0x52: {
                // move.obj.v
                sprites[program_counter[1]].prg_movetype = pmmMoveTo;
                sprites[program_counter[1]].prg_x_destination = logic_vars[program_counter[2]];
                sprites[program_counter[1]].prg_y_destination = logic_vars[program_counter[3]];
                if (program_counter[4] > 0) {
                    sprites[program_counter[1]].prg_speed = program_counter[4];
                } else {
                    sprites[program_counter[1]].prg_speed = sprites[program_counter[1]].step_size;
                }
                sprites[program_counter[1]].prg_speed_squared = sprites[program_counter[1]].prg_speed * sprites[program_counter[1]].prg_speed;
                sprites[program_counter[1]].prg_complete_flag = program_counter[5];
                sprites[program_counter[1]].updatable = true;
                program_counter += 6;
                break;
            }
            case 0x53: {
                // follow.ego
                sprites[program_counter[1]].prg_movetype = pmmFollow;
                if (program_counter[4] > 0) {
                    sprites[program_counter[1]].prg_speed = program_counter[2];
                } else {
                    sprites[program_counter[1]].prg_speed = sprites[program_counter[1]].step_size;
                }
                sprites[program_counter[1]].prg_speed_squared = sprites[program_counter[1]].prg_speed * sprites[program_counter[1]].prg_speed;
                sprites[program_counter[1]].prg_complete_flag = program_counter[3];
                sprites[program_counter[1]].updatable = true;
                program_counter += 4;
                break;
            }
            case 0x54: {
                sprites[program_counter[1]].prg_movetype = pmmWander;
                sprites[program_counter[1]].prg_dir = 0;
                if (program_counter[1] == 0) {
                    player_control = false;
                }
                program_counter += 2;
                break;
            }
            case 0x58: {
                sprites[program_counter[1]].observe_blocks = false;
                program_counter += 2;
                break;
            }
            case 0x59: {
                sprites[program_counter[1]].observe_blocks = true;
                program_counter += 2;
                break;
            }
            case 0x62: {
                // load.sound
                program_counter += 2;
                break;
            }
            case 0x63: {
                // play.sound
                sound_play(program_counter[1], program_counter[2]);
                program_counter += 3;
                break;
            }
            case 0x64: {
                sound_stop();
                program_counter += 1;
                break;
            }
            case 0x65: {
                // print
                uint8_t __far *src_string = logic_infos[logic_num].text_offset + logic_locate_message(logic_num, program_counter[1]);
                logic_strcpy_far_near(local_string, src_string);
                engine_display_dialog(local_string);
                program_counter += 2;
                break;
            }
            case 0x66: {
                // print.v
                uint8_t __far *src_string = logic_infos[logic_num].text_offset + logic_locate_message(logic_num, logic_vars[program_counter[1]]);
                logic_strcpy_far_near(local_string, src_string);
                engine_display_dialog(local_string);
                program_counter += 2;
                break;
            }
            case 0x67: {
                // display
                uint8_t __far *src_string = logic_infos[logic_num].text_offset + logic_locate_message(logic_num, program_counter[3]);
                logic_strcpy_far_near(local_string, src_string);
                gfx_print_ascii(program_counter[2], program_counter[1], (char*)local_string);
                program_counter += 4;
                break;
            }
            case 0x68: {
                // display.v
                uint8_t __far *src_string = logic_infos[logic_num].text_offset + logic_locate_message(logic_num, logic_vars[program_counter[3]]);
                logic_strcpy_far_near(local_string, src_string);
                gfx_print_ascii(logic_vars[program_counter[2]], logic_vars[program_counter[1]], (char *)local_string);
                program_counter += 4;
                break;
            }
            case 0x69: {
                // clear.line
                for (uint8_t line = program_counter[1]; line <= program_counter[2]; line++) {
                    gfx_clear_line(line);
                }
                program_counter += 4;
                break;
            }
            case 0x6A: {
                // text.screen
                engine_allowinput(false);
                gfx_set_textmode(true);
                program_counter += 1;
                break;
            }
            case 0x6B: {
                // graphics 
                gfx_set_textmode(false);
                engine_allowinput(true);
                program_counter += 1;
                break;
            }
            case 0x6C: {
                // set.cursor.char
                program_counter += 2;
                break;
            }
            case 0x6D: {
                // set.text.attribute
                program_counter += 3;
                break;
            }
            case 0x6F: {
                // configure.screen
                program_counter += 4;
                break;
            }
            case 0x70: {
                // status.line.on
                program_counter += 1;
                break;
            }
            case 0x71: {
                // status.line.off
                program_counter += 1;
                break;
            }
            case 0x72: {
                // set.string
                uint8_t __far *dest_string = logic_strings + (40 * program_counter[1]);
                uint8_t __far *src_string = logic_infos[logic_num].text_offset + logic_locate_message(logic_num, program_counter[2]);
                logic_strcpy_far_far(dest_string, src_string);
                program_counter += 3;
                break;
            }
            case 0x77: {
                // prevent.input
                engine_allowinput(false);
                program_counter += 1;
                break;
            }
            case 0x78: {
                // accept.input
                engine_allowinput(true);
                program_counter += 1;
                break;
            }
            case 0x79: {
                // set.key
                program_counter += 4;
                break;
            }
            case 0x80: {
                // restart.game
                sprite_stop_all();
                sprite_unanimate_all();
                chipmem_free_unlocked();
                engine_allowinput(false);
                player_control = true;
                engine_dialog_close();
                engine_unblock();
                horizon_line = 36;
                for (int counter = 0; counter < 256; counter++) {
                    logic_vars[counter] = 0;
                }
                for (int counter = 0; counter < 32; counter++) {
                    logic_flags[counter] = 0;
                }
                logic_set_flag(5);
                logic_set_flag(6);
                sprite_clearall();
                engine_clear_keyboard();
                return;
            }
            case 0x83: {
                // program.control
                player_control = false;
                program_counter += 1;
                break;
            }
            case 0x84: {
                // player.control
                player_control = true;
                program_counter += 1;
                break;
            }
            case 0x8E: {
                // script.size
                program_counter += 2;
                break;
            }
            case 0x8F: {
                uint16_t message_offset = logic_locate_message(logic_num, program_counter[1]);
                for (uint16_t counter = 0; counter < 7; counter++) {
                    game_id[counter] = logic_infos[logic_num].text_offset[message_offset + counter];
                    if (game_id[counter] == 0) {
                        break;
                    }
                }
                game_id[7] = 0;
                program_counter += 2;
                break;
            }
            case 0x9C:
                // Set Menu
                program_counter += 2;
                break;
            case 0x9D:
                // Set Menu Item
                program_counter += 3;
                break;
            case 0x9E:
                // Submit Menu
                program_counter += 1;
                break;
            case 0x9F: {
                // enable.item
                program_counter += 2;
                break;
            }
            case 0xA0: {
                // disable.item
                program_counter += 2;
                break;
            }
            case 0xfe: {
                int16_t branch_bytes = program_counter[2];
                branch_bytes = (branch_bytes << 8) | program_counter[1]; 
                program_counter += (branch_bytes + 3);
                break;
            }
            case 0xff: {
                // if handler
                program_counter++;
                if (!logic_test()) {
                    int16_t skipbytes = program_counter[1];
                    skipbytes = (skipbytes << 8) | program_counter[0]; 
                    program_counter += skipbytes;
                }
                program_counter += 2;
                break;
            }
            default:
            gfx_print_ascii(0, 0, "FAULT: UNKINSTR=%x:%X", *program_counter,(uint32_t)program_counter);
            while(1);
        }
    }
}

void logic_load(uint8_t logic_num) {

    uint16_t length;
    if (logic_infos[logic_num].offset != 0) {
        return;
    }

    logic_infos[logic_num].offset = load_volume_object(voLogic, logic_num, &length);
    if (logic_infos[logic_num].offset == 0) {
        gfx_print_ascii(0, 0, "FAULT: Failed to load logic %d.", logic_num);
        return;
    }

    uint8_t __far *logic_ptr = chipmem_base + logic_infos[logic_num].offset;
    uint16_t text_offset = (logic_ptr[1] << 8) | logic_ptr[0];
    logic_infos[logic_num].text_offset = chipmem_base + text_offset + logic_infos[logic_num].offset + 2;

    uint8_t num_strings = logic_infos[logic_num].text_offset[0];
    uint16_t start_offset = (num_strings * 2) + 3;
    uint16_t end_offset = logic_infos[logic_num].text_offset[2] << 8;
    end_offset |= logic_infos[logic_num].text_offset[1];
    char avis_durgan[12] = "Avis Durgan";
    uint8_t avis = 0;
    for (uint16_t index = start_offset; index <= end_offset; index++) {
        logic_infos[logic_num].text_offset[index] ^= avis_durgan[avis];
        avis = (avis + 1) % 11;
    }  
}

void logic_purge(uint16_t freed_offset) {
    for (int i = 0; i < 256; i++) {
        if (logic_infos[i].offset >= freed_offset) {
            logic_infos[i].offset = 0;
        }
    }
}

void logic_init(void) {
    for (int counter = 0; counter < 0xffff; counter++) {
        chipmem_base[counter] = 0x77;
    }

    uint16_t vars_offset = chipmem_alloc(256);
    logic_vars = chipmem_base + vars_offset;
    for (int counter = 0; counter < 256; counter++) {
        logic_vars[counter] = 0;
    }
    uint16_t flags_offset = chipmem_alloc(32);
    logic_flags = chipmem_base + flags_offset;
    for (int counter = 0; counter < 32; counter++) {
        logic_flags[counter] = 0;
    }

    logic_strings = chipmem_base + chipmem_alloc(24 * 40);
    for (int counter = 0; counter < (24 * 40); counter++) {
        logic_strings[counter] = 0x99;
    }

    logic_stack_ptr = 16;
    logic_load(0);
    chipmem_lock();
}