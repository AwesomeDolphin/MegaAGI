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
#include "pic.h"
#include "sprite.h"
#include "view.h"
#include "volume.h"

uint8_t __far *logic_vars = NULL;
uint8_t __far *logic_flags = NULL;

typedef struct logic_info {
    uint16_t offset;
    uint8_t __far *text_offset;
} logic_info_t;

#pragma clang section bss="extradata"
__far static logic_info_t logic_infos[256];
static uint8_t __far *logic_stack[16];
#pragma clang section bss=""

static uint8_t __far *program_counter;
static uint8_t __far *logic_strings;
static char game_id[8];
static uint8_t logic_stack_ptr;

void logic_set_flag(uint8_t flag) {
    uint8_t flag_reg = (flag >> 3);
    logic_flags[flag_reg] = logic_flags[flag_reg] | (1 << (flag & 0x07));
}

void logic_reset_flag(uint8_t flag) {
    uint8_t flag_reg = (flag >> 3);
    logic_flags[flag_reg] &= ~(logic_flags[flag_reg] | (1 << (flag & 0x07)));
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
        case 0x0C:
            // controller test
            result = false;
            program_counter += 2;
            break;
        case 0x0E:
            // said test
            result = false;
            program_counter += 2 + (program_counter[1] * 2);
            break;
        default:
            gfx_print_splitascii("FAULT: UNKTEST=%x\r", *program_counter);
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
    program_counter = chipmem_base + (logic_infos[logic_num].offset + 2);
    while(1) {
        gfx_print_splitascii("A:%X\r", (uint32_t)program_counter);
        gfx_print_splitascii("B:%x\r", *program_counter);
        switch (*program_counter) {
            case 0x01: {
                logic_vars[program_counter[1]]++;
                program_counter += 2;
                break;
            }
            case 0x02: {
                logic_vars[program_counter[1]]--;
                program_counter += 2;
                break;
            }
            case 0x03: {
                logic_vars[program_counter[1]] = program_counter[2];
                program_counter += 3;
                break;
            }
            case 0x04: {
                logic_vars[program_counter[1]] = logic_vars[program_counter[2]];
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
                sprite_stop_all();
                sprite_unanimate_all();
                chipmem_free_unlocked();
                engine_player_control(true);
                engine_unblock();
                engine_horizon(36);
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
            case 0x17: {
                // call.v
                logic_stack_ptr--;
                logic_stack[logic_stack_ptr] = program_counter + 2;
                program_counter = chipmem_base + (logic_infos[logic_vars[program_counter[1]]].offset + 2);
                break;
            }
            case 0x18: {
                program_counter += 2;
                break;
            }
            case 0x19: {
                // draw.pic
                draw_pic(0, logic_vars[program_counter[1]], 0);
                gfx_print_splitascii("DRAWN!");
                while(1);
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
                sprite_animate(program_counter[1]);
                program_counter += 2;
                break;
            }
            case 0x27: {
                // get.posn
                logic_vars[program_counter[2]] = sprites[program_counter[1]].view_info.x_pos;
                logic_vars[program_counter[3]] = sprites[program_counter[1]].view_info.y_pos;
                program_counter += 4;
                break;
            }
            case 0x2A: {
                sprite_set_view(program_counter[1], logic_vars[program_counter[2]]);
                program_counter += 3;
                break;
            }
            case 0x3d: {
                sprites[program_counter[1]].observe_horizon = false;
                program_counter += 2;
                break;
            }
            case 0x3e: {
                sprites[program_counter[1]].observe_horizon = true;
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
            case 0x6C: {
                // set.cursor.char
                program_counter += 2;
                break;
            }
            case 0x6F: {
                // configure.screen
                program_counter += 4;
                break;
            }
            case 0x72: {
                // set.string
                uint8_t __far *dest_string = logic_strings + (40 * program_counter[1]);
                uint8_t __far *src_string = logic_infos[logic_num].text_offset + logic_locate_message(logic_num, program_counter[2]);
                *dest_string = *src_string;
                while (*dest_string != 0) {
                    dest_string++;
                    src_string++;
                    *dest_string = *src_string;
                }
                program_counter += 3;
                break;
            }
            case 0x8e: {
                program_counter += 2;
                break;
            }
            case 0x8f: {
                uint16_t message_offset = logic_locate_message(logic_num, program_counter[1]);
                for (uint16_t counter = 0; counter < 7; counter++) {
                    game_id[counter] = logic_infos[logic_num].text_offset[message_offset + counter];
                    if (game_id[counter] == 0) {
                        break;
                    }
                }
                game_id[7] = 0;
                gfx_print_splitascii("Game ID: ");
                gfx_print_splitascii(game_id);
                gfx_print_splitascii("\r");
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
            gfx_print_splitascii("FAULT: UNKINSTR=%x\r", *program_counter);
            while(1);
        }
    }
}

void logic_load(uint8_t logic_num) {

    uint16_t length;
    logic_infos[logic_num].offset = load_volume_object(voLogic, logic_num, &length);
    if (logic_infos[logic_num].offset == 0) {
        gfx_print_splitascii("FAULT: Failed to load logic %d.\r", logic_num);
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