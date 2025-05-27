#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "engine.h"
#include "irq.h"
#include "logic.h"
#include "main.h"
#include "memmanage.h"
#include "sprite.h"
#include "view.h"
#include "gfx.h"

static void sprite_update_sprite(uint8_t sprite_num);
static void sprite_move_at_speed(agisprite_t *sprite);

#pragma clang section bss="extradata"
__far agisprite_t sprites[256];
__far uint8_t animated_sprites[256];
#pragma clang section bss=""
uint8_t animated_sprite_count;
uint16_t free_point;
static uint8_t wander_pointers[9][3] = {
    {1,3,7},
    {4,5,6},
    {5,6,7},
    {6,7,8},
    {1,7,8},
    {2,1,8},
    {1,2,3},
    {2,3,4},
    {3,4,5},
};

void sprite_sort(void) {
    uint8_t i=1;
    while (i < animated_sprite_count) {
        uint8_t x = animated_sprites[i];
        uint8_t j = i;
        while (j > 0) {
            uint8_t y = animated_sprites[j - 1];
            uint8_t px = sprites[x].view_info.priority;
            uint8_t py = sprites[y].view_info.priority;
            if (py < px) {
                break;
            }
            if (py == px) {
                uint8_t hx = sprites[x].view_info.y_pos;
                uint8_t hy = sprites[y].view_info.y_pos;
                if (hy <= hx) {
                    break;
                }
            }
            animated_sprites[j] = animated_sprites[j - 1];
            j--;
        }
        animated_sprites[j] = x;
        i++;
    }
}

void sprite_draw_animated(void) {
    for (int i = 0; i < animated_sprite_count; i++) {
        if (sprites[animated_sprites[i]].updatable) {
            sprite_update_sprite(animated_sprites[i]);
        }
    }

    sprite_sort();

    free_point = chipmem_alloc(1);

    gfx_hold_flip(true);
    if (drawing_screen == 0) { 
        select_graphics0_mem();
    } else {
        select_graphics1_mem();
    }
    
    for (int i = 0; i < animated_sprite_count; i++) {
        agisprite_t sprite = sprites[animated_sprites[i]];
        if (sprite.drawable) {
            draw_cel(&sprite.view_info, sprite.cel_index);
            sprites[animated_sprites[i]] = sprite;
        }
    }

    select_execution_mem();
    gfx_hold_flip(false);
}

void sprite_erase_animated(void) {
    gfx_hold_flip(true);
    if (drawing_screen == 0) { 
        select_graphics0_mem();
    } else {
        select_graphics1_mem();
    }
    
    for (int i = animated_sprite_count; i > 0; i--) {
        agisprite_t sprite = sprites[animated_sprites[i-1]];
        if (sprite.drawable) {
            erase_view(&sprite.view_info);
        }
    } 

    select_execution_mem();
    gfx_hold_flip(false);

    if (free_point > 0) {
        chipmem_free(free_point);
    }
}


#pragma clang section bss="nographicsbss" data="nographicsdata" rodata="nographicsrodata" text="nographicstext"

uint8_t priorities[169];
uint8_t block_active, block_x1, block_y1, block_x2, block_y2;

void setup_priorities(void) {
    uint8_t priority_level = 4;
    for (uint8_t counter = 0; counter < 169; counter++) {
        switch(counter) {
            case 168:
            case 156:
            case 144:
            case 132:
            case 120:
            case 108:
            case 96:
            case 84:
            case 72:
            case 60:
            case 48:
                priority_level++;
                break;
        }
        priorities[counter] = priority_level;
    }
}

void sprite_move_at_speed(agisprite_t *sprite) {

    int16_t x1 = sprite->view_info.x_pos;
    int16_t y1 = sprite->view_info.y_pos;
    int16_t x2 = sprite->prg_x_destination;
    int16_t y2 = sprite->prg_y_destination;
    int16_t z = sprite->prg_speed_squared;

    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    
    int16_t distance_squared = dx*dx + dy*dy;
    
    // Check if z exceeds the distance
    if (z >= distance_squared) {
        // We would reach or pass the target, so just go to target
        sprite->view_info.x_pos = x2;
        sprite->view_info.y_pos = y2;
        logic_set_flag(sprite->prg_complete_flag);
        sprite->prg_speed = 0;
        sprite->object_dir = 0;
        sprite->prg_movetype = pmmNone;
        return;
    }

    if (abs(dx) > abs(dy) * 2) {
        sprite->object_dir = (dx > 0) ? 3 : 7; // Right or Left
    } else if (abs(dy) > abs(dx) * 2) {
        sprite->object_dir = (dy > 0) ? 5 : 1; // Down or Up
    } else {
        if (dx > 0 && dy > 0) sprite->object_dir = 4; // Down-Right
        if (dx > 0 && dy < 0) sprite->object_dir = 2; // Up-Right
        if (dx < 0 && dy > 0) sprite->object_dir = 6; // Down-Left
        if (dx < 0 && dy < 0) sprite->object_dir = 8; // Up-Left
    }
}

void autoselect_loop(agisprite_t *sprite) {
    if (sprite->view_info.number_of_loops == 1) {
        return;
    }
    uint8_t new_loop = 0xff;

    if (sprite->view_info.number_of_loops < 4) {
        uint8_t loop_indexes[9] = {255, 255, 0, 0, 0, 255, 1, 1, 1};
        new_loop = loop_indexes[sprite->object_dir];
    } else {
        uint8_t loop_indexes[9] = {255, 3, 0, 0, 0, 2, 1, 1, 1};
        new_loop = loop_indexes[sprite->object_dir];
    }
    if (new_loop != 255) {
        if (new_loop != sprite->loop_index) {
            sprite->loop_index = new_loop;
            sprite->loop_offset = select_loop(&sprite->view_info, new_loop);
        }
    }
}

uint8_t sprite_move(agisprite_t *sprite, uint8_t speed) {
    int16_t view_dx;
    int16_t view_dy;
    if (sprite->frozen) {
        return false;
    }
    switch (sprite->object_dir) {
        case 0:
        view_dx = 0;
        view_dy = 0;
        break;
        case 1:
        view_dx = 0;
        view_dy = -speed;
        break;
        case 2:
        view_dx = speed;
        view_dy = -speed;
        break;
        case 3:
        view_dx = speed;
        view_dy = 0;
        break;
        case 4:
        view_dx = speed;
        view_dy = speed;
        break;
        case 5:
        view_dx = 0;
        view_dy = speed;
        break;
        case 6:
        view_dx = -speed;
        view_dy = speed;
        break;
        case 7:
        view_dx = -speed;
        view_dy = 0;
        break;
        case 8:
        view_dx = -speed;
        view_dy = -speed;
        break;
    }

    uint8_t ego_border = 0;
    int16_t new_xpos = sprite->view_info.x_pos + view_dx;
    int16_t new_ypos = sprite->view_info.y_pos + view_dy;
    if (new_xpos <= -1) {
        if (sprite->ego) {
            ego_border = 4;
        }
        sprite->object_dir = 0;
    } else if (new_xpos > (160 - sprite->view_info.width)) {
        if (sprite->ego) {
            ego_border = 2;
        }
        sprite->object_dir = 0;
    } 
    
    if ((new_ypos - sprite->view_info.height) <= -1) {
        if (sprite->ego) {
            ego_border = 1;
        }
        sprite->object_dir = 0;
    } else if (sprite->observe_horizon && (new_ypos <= horizon_line)) {
        ego_border = 1;
        sprite->object_dir = 0;
    } else if (new_ypos > 167) {
        if (sprite->ego) {
            ego_border = 3;
        }
        sprite->object_dir = 0;
    }

    uint8_t left_prio = gfx_getprio(new_xpos, new_ypos);
    uint8_t right_prio = gfx_getprio(new_xpos + sprite->view_info.width - 1, new_ypos);
    if ((left_prio == 0) || (right_prio == 0)) {
        sprite->object_dir = 0;
    }

    if (((left_prio == 1) || (right_prio == 1)) && sprite->observe_blocks){
        sprite->object_dir = 0;
    }

    if (block_active) {
        if (sprite->observe_blocks) {
            bool sprite_entered = ((new_xpos < block_x1) && (new_xpos + sprite->view_info.width > block_x1) && (new_ypos >= block_y1) && (new_ypos <= block_y2));
            if (sprite_entered) {
                sprite->object_dir = 0;
            }
        }
    }

    if ((left_prio == 2) || (right_prio == 2)) {
        if (sprite->ego) {
            logic_set_flag(3);
        } 
    } else {
        if (sprite->ego) {
            logic_reset_flag(3);
        } 
    }

    if ((left_prio == 3) && (right_prio == 3)) {
        if (sprite->ego) {
            logic_set_flag(0);
        } 
        if (sprite->on_land) {
            sprite->object_dir = 0;
        }
    } else {
        if (sprite->ego) {
            logic_reset_flag(0);
        } 
        if (sprite->on_water) {
            sprite->object_dir = 0;
        }
    }

    if (sprite->ego) {
        logic_vars[2] = ego_border;
        logic_vars[6] = sprite->object_dir;
    } 
    
    if (sprite->object_dir != 0) {
        sprite->view_info.x_pos = sprite->view_info.x_pos + view_dx;
        sprite->view_info.y_pos = sprite->view_info.y_pos + view_dy;
        return true;
    }

    sprite->object_dir = 0;
    return false;
}

void sprite_stop_all(void) {

}

void sprite_unanimate_all(void) {
    for (uint16_t sprite_num = 0; sprite_num < 256; sprite_num++) {
        sprites[sprite_num].drawable = false;
        animated_sprites[sprite_num] = 0;
    }
    animated_sprite_count = 0;
}

void sprite_mark_drawable(uint8_t sprite_num) {
    sprites[sprite_num].drawable = true;
    sprites[sprite_num].cycling = true;
}

void sprite_setedge(uint8_t sprite_num, uint8_t edgenum) {
    switch(edgenum) {
        case 1:
            sprites[sprite_num].view_info.y_pos = 167;
            break;
        case 2:
            sprites[sprite_num].view_info.x_pos = 0;
            break;
        case 3:
            sprites[sprite_num].view_info.y_pos = horizon_line + sprites[sprite_num].view_info.height;
            break;
        case 4:
            sprites[sprite_num].view_info.x_pos = 160 - sprites[sprite_num].view_info.width;
            break;
    }
}

void sprite_set_view(uint8_t sprite_num, uint8_t view_number) {
    agisprite_t sprite = sprites[sprite_num];

    sprite.view_number = view_number;
    sprite.object_dir = 0;
    sprite.cel_index = 0;
    sprite.loop_index = 0;
    sprite.cycle_time = 1;
    sprite.cycle_count = 1;
    sprite.reverse = false;
    view_set(&sprite.view_info, view_number);
    select_loop(&sprite.view_info, 0);
    sprites[sprite_num] = sprite;
}

uint8_t sprite_get_view(uint8_t sprite_num) {
    return sprites[sprite_num].view_number;
}

void sprite_update_sprite(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num]; 
    uint8_t speed = 1;
    switch (sprite.prg_movetype) {
        case pmmNone:
            speed = sprite.step_size;
        break;
        case pmmFollowWander:
            sprite.prg_timer++;
            if (sprite.prg_timer >= 5) {
                sprite.prg_movetype = pmmFollow;
                speed = sprite.prg_speed;
                sprite.prg_x_destination = sprites[0].view_info.x_pos;
                sprite.prg_y_destination = sprites[0].view_info.y_pos;
                sprite_move_at_speed(&sprite);
            }
        case pmmWander:
            speed = sprite.step_size;
            if (sprite.object_dir == 0)
            {
                uint8_t rand_dir = (rand() % 3);
                uint8_t new_dir = wander_pointers[sprite.prg_dir][rand_dir];
                sprite.object_dir = new_dir;
                sprite.prg_dir = new_dir;
            }
        break;
        case pmmMoveTo:
            speed = sprite.prg_speed;
            sprite_move_at_speed(&sprite);
        break;
        case pmmFollow:
            if (sprite.object_dir == 0) {
                sprite.prg_movetype = pmmFollowWander;
                sprite.prg_timer = 0;
            } else {
                speed = sprite.prg_speed;
                sprite.prg_x_destination = sprites[0].view_info.x_pos;
                sprite.prg_y_destination = sprites[0].view_info.y_pos;
                sprite_move_at_speed(&sprite);
            }
        break;
        default:
        break;
    }

    sprite_move(&sprite, speed);
    if (!sprite.view_info.priority_override) {
        sprite.view_info.priority = priorities[sprite.view_info.y_pos];
    }

    autoselect_loop(&sprite);

    sprite.cycle_count--;
    if (sprite.cycle_count == 0) {
        sprite.cycle_count  = sprite.cycle_time;
        if (sprite.cycling) {
            if (sprite.reverse) {
                if (sprite.cel_index == 0) {
                    sprite.cel_index = sprite.view_info.number_of_cels;
                }
                sprite.cel_index--;
            } else {
                sprite.cel_index++;
                if (sprite.cel_index == sprite.view_info.number_of_cels) {
                    sprite.cel_index = 0;
                }
            }
        }
        if (sprite.end_of_loop > 0) {
            if (sprite.reverse) {
                if (sprite.cel_index == 0) {
                    logic_set_flag(sprite.end_of_loop);
                    sprite.end_of_loop = 0;
                } else {
                    sprite.cel_index--;
                }
            } else {
                sprite.cel_index++;
                if (sprite.cel_index == (sprite.view_info.number_of_cels - 1)) {
                    logic_set_flag(sprite.end_of_loop);
                    sprite.end_of_loop = 0;
                }
            }
        }
    }
    sprites[sprite_num] = sprite;
}

void sprite_clearall(void) {
    uint8_t __far *sprdatptr = (uint8_t __far *)sprites;
    for (int i = 0; i < sizeof(sprites); i++) {
        sprdatptr[i] = 0;
    }
}

void sprite_init(void) {
    setup_priorities();
    animated_sprite_count = 0;
    free_point = 0;
    sprite_clearall();
}