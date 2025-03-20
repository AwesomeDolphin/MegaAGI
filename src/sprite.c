#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "engine.h"
#include "logic.h"
#include "memmanage.h"
#include "sprite.h"
#include "view.h"
#include "gfx.h"

#pragma clang section bss="extradata"
__far agisprite_t sprites[256];
__far uint8_t animated_sprites[256];
#pragma clang section bss=""
uint8_t animated_sprite_count;

void autoselect_loop(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    if (sprite.view_info.number_of_loops == 1) {
        return;
    }
    if (sprite.view_info.number_of_loops < 4) {
        switch(sprite.object_dir) {
            case 2:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(&sprite.view_info, 0);
            break;
            case 3:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(&sprite.view_info, 0);
            break;
            case 4:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(&sprite.view_info, 0);
            break;
            case 6:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(&sprite.view_info, 1);
            break;
            case 7:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(&sprite.view_info, 1);
            break;
            case 8:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(&sprite.view_info, 1);
            break;
        }
    } else {
        switch(sprite.object_dir) {
            case 1:
            sprite.loop_index = 3;
            sprite.loop_offset = select_loop(&sprite.view_info, 3);
            break;
            case 2:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(&sprite.view_info, 0);
            break;
            case 3:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(&sprite.view_info, 0);
            break;
            case 4:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(&sprite.view_info, 0);
            break;
            case 5:
            sprite.loop_index = 2;
            sprite.loop_offset = select_loop(&sprite.view_info, 2);
            break;
            case 6:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(&sprite.view_info, 1);
            break;
            case 7:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(&sprite.view_info, 1);
            break;
            case 8:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(&sprite.view_info, 1);
            break;
        }
    }
    sprites[sprite_num] = sprite;
}

uint8_t sprite_move(uint8_t sprite_num) {
    int16_t view_dx;
    int16_t view_dy;
    agisprite_t sprite = sprites[sprite_num];
    if (sprite.frozen) {
        return false;
    }
    switch (sprite.object_dir) {
        case 0:
        view_dx = 0;
        view_dy = 0;
        break;
        case 1:
        view_dx = 0;
        view_dy = -1;
        break;
        case 2:
        view_dx = 1;
        view_dy = -1;
        break;
        case 3:
        view_dx = 1;
        view_dy = 0;
        break;
        case 4:
        view_dx = 1;
        view_dy = 1;
        break;
        case 5:
        view_dx = 0;
        view_dy = 1;
        break;
        case 6:
        view_dx = -1;
        view_dy = 1;
        break;
        case 7:
        view_dx = -1;
        view_dy = 0;
        break;
        case 8:
        view_dx = -1;
        view_dy = -1;
        break;
    }

    uint8_t ego_border = 0;
    int16_t new_xpos = sprite.view_info.x_pos + view_dx;
    int16_t new_ypos = sprite.view_info.y_pos + view_dy;
    if (new_xpos == 0) {
        ego_border = 4;
        sprite.object_dir = 0;
    } else if (new_xpos > (159 - sprite.view_info.width)) {
        ego_border = 2;
        sprite.object_dir = 0;
    } 
    
    if ((new_ypos - sprite.view_info.height) == -1) {
        ego_border = 1;
        sprite.object_dir = 0;
    } else if (sprite.observe_horizon && (new_ypos <= horizon_line)) {
        ego_border = 1;
        sprite.object_dir = 0;
    } else if (new_ypos > 167) {
        ego_border = 3;
        sprite.object_dir = 0;
    }

    uint8_t left_prio = gfx_getprio(new_xpos, new_ypos);
    uint8_t right_prio = gfx_getprio(new_xpos + sprite.view_info.width, new_ypos);
    if ((left_prio == 0) || (right_prio == 0)) {
        sprite.object_dir = 0;
    }

    if (((left_prio == 1) || (right_prio == 1)) && sprite.observe_blocks){
        sprite.object_dir = 0;
    }

    if ((left_prio == 2) || (right_prio == 2)) {
        if (sprite_num == 0) {
            logic_set_flag(3);
        } 
    } else {
        if (sprite_num == 0) {
            logic_reset_flag(3);
        } 
    }

    if ((left_prio == 3) && (right_prio == 3)) {
        if (sprite_num == 0) {
            logic_set_flag(0);
        } 
        if (!sprite.on_water) {
            sprite.object_dir = 0;
        }
    } else {
        if (sprite_num == 0) {
            logic_reset_flag(0);
        } 
        if (sprite.on_water) {
            sprite.object_dir = 0;
        }
    }

    if (sprite_num == 0) {
        logic_vars[2] = ego_border;
        logic_vars[6] = sprite.object_dir;
    } 
    
    if (sprite.object_dir != 0) {
        sprite.view_info.x_pos = sprite.view_info.x_pos + view_dx;
        sprite.view_info.y_pos = sprite.view_info.y_pos + view_dy;
        sprites[sprite_num] = sprite;
        return true;
    }

    sprites[sprite_num].object_dir = 0;
    return false;
}

void sprite_set_direction(uint8_t sprite_num, uint8_t direction) {
    sprites[sprite_num].object_dir = direction;
    autoselect_loop(sprite_num);
}

void sprite_set_position(uint8_t sprite_num, uint8_t pos_x, uint8_t pos_y) {
    sprites[sprite_num].view_info.x_pos = pos_x;
    sprites[sprite_num].view_info.y_pos = pos_y;
}

void sprite_erase(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    if (!sprite.drawable) {
        return;
    }
    erase_view(&sprite.view_info);
}

void sprite_draw(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    if (!sprite.drawable) {
        return;
    }
    draw_cel(&sprite.view_info, sprite.cel_index);
    sprites[sprite_num] = sprite;
}

void sprite_stop_all(void) {

}

void sprite_unanimate_all(void) {
    for (uint16_t sprite_num = 0; sprite_num < 256; sprite_num++) {
        sprites[sprite_num].drawable = false;
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
            sprites[sprite_num].view_info.y_pos = 166;
            break;
        case 2:
            sprites[sprite_num].view_info.x_pos = 2;
            break;
        case 3:
            sprites[sprite_num].view_info.y_pos = horizon_line + sprites[sprite_num].view_info.height;
            break;
        case 4:
            sprites[sprite_num].view_info.x_pos = 157 - sprites[sprite_num].view_info.width;
            break;
    }
}

void sprite_set_view(uint8_t sprite_num, uint8_t view_number) {
    agisprite_t sprite = sprites[sprite_num];

    sprite.view_number = view_number;
    sprite.object_dir = 0;
    sprite.cel_index = 0;
    sprite.loop_index = 0;
    view_set(&sprite.view_info, view_number);
    select_loop(&sprite.view_info, 0);
    sprites[sprite_num] = sprite;
}

uint8_t sprite_get_view(uint8_t sprite_num) {
    return sprites[sprite_num].view_number;
}

void sprite_update_sprite(uint8_t sprite_num) {
    sprite_move(sprite_num);            
    agisprite_t sprite = sprites[sprite_num];
    if (sprite.wander) {
        if (sprite.object_dir == 0)
        {
            sprite.object_dir = (rand() % 7) + 1;
        }
        
    }
    if (sprite.cycling) {
        sprite.cel_index++;
        if (sprite.cel_index == sprite.view_info.number_of_cels) {
            sprite.cel_index = 0;
        }
    }
    sprites[sprite_num] = sprite;
}

void sprite_draw_animated(void) {
    for (int i = 0; i < animated_sprite_count; i++) {
        agisprite_t sprite = sprites[animated_sprites[i]];
        if (sprite.updatable) {
            sprite_update_sprite(animated_sprites[i]);
        }
        sprite_draw(animated_sprites[i]);
    }
}

void sprite_erase_animated(void) {
    for (int i = animated_sprite_count; i > 0; i--) {
        if (sprites[animated_sprites[i-1]].updatable) {
            sprite_erase(animated_sprites[i-1]);
        } 
    }
}

void sprite_init(void) {
    animated_sprite_count = 0;
}