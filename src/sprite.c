#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "memmanage.h"
#include "sprite.h"
#include "view.h"

#pragma clang section bss="extradata"
__far agisprite_t sprites[256];
#pragma clang section bss=""

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

    sprite.view_info.x_pos += view_dx;
    sprite.view_info.y_pos += view_dy;
    if (sprite.view_info.x_pos == -1) {
        sprite.view_info.x_pos = 0;
        view_dx = 0;
    } else if (sprite.view_info.x_pos > (159 - sprite.view_info.width)) {
        sprite.view_info.x_pos = 159 - sprite.view_info.width;
        view_dx = 0;
    }
    
    if (sprite.view_info.y_pos == -1) {
        sprite.view_info.y_pos = 0;
        view_dy = 0;
    } else if (sprite.view_info.y_pos > (167 - sprite.view_info.height)) {
        sprite.view_info.y_pos = 167 - sprite.view_info.height;
        view_dy = 0;
    }

    sprites[sprite_num] = sprite;
    return ((view_dx != 0) || (view_dy != 0));
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
    erase_view(&sprite.view_info);
}

void sprite_draw(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    sprite.cel_index++;
    if (sprite.cel_index == sprite.view_info.number_of_cels) {
        sprite.cel_index = 0;
    }
    draw_cel(&sprite.view_info, sprite.cel_index);
    sprites[sprite_num] = sprite;
}

void sprite_stop_all(void) {

}

void sprite_unanimate_all(void) {
    for (uint16_t sprite_num = 0; sprite_num < 256; sprite_num++) {
        sprites[sprite_num].animated = false;
    }
}

void sprite_animate(uint8_t sprite_num) {
    sprites[sprite_num].animated = true;
}

void sprite_setedge(uint8_t sprite_num, uint8_t edgenum) {
    switch(edgenum) {
        case 1:
            sprites[sprite_num].view_info.y_pos = 166;
            break;
        case 2:
            sprites[sprite_num].view_info.x_pos = 1;
            break;
        case 3:
            sprites[sprite_num].view_info.y_pos = 36;
            break;
        case 4:
            sprites[sprite_num].view_info.x_pos = 158;
            break;
    }
}

void sprite_set_view(uint8_t sprite_num, uint8_t view_number) {
    agisprite_t sprite = sprites[sprite_num];

    view_load(view_number);
    sprite.view_number = view_number;
    sprite.object_dir = 0;
    sprite.cel_index = 0;
    sprite.loop_index = 0;
    sprites[sprite_num] = sprite;
}

uint8_t sprite_get_view(uint8_t sprite_num) {
    return sprites[sprite_num].view_number;
}
