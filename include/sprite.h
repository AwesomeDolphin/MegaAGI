#ifndef SPRITE_H
#define SPRITE_H

#include "view.h"

typedef struct {
    view_info_t view_info;

    uint8_t view_number;

    uint16_t loop_offset;
    uint8_t loop_index;
    uint8_t cel_index;

    uint8_t object_dir;
    bool animated;
    bool observe_object_collisions;
    bool observe_horizon;
    bool observe_blocks;
    
    bool cycling;
} agisprite_t;

extern __far agisprite_t sprites[256];

void autoselect_loop(uint8_t sprite_num);
uint8_t sprite_move(uint8_t sprite_num);
void sprite_set_direction(uint8_t sprite_num, uint8_t direction);
void sprite_set_position(uint8_t sprite_num, uint8_t pos_x, uint8_t pos_y);
void sprite_erase(uint8_t sprite_num);
void sprite_draw(uint8_t sprite_num);
uint8_t sprite_get_view(uint8_t sprite_num);
void sprite_set_view(uint8_t sprite_num, uint8_t view_number);
void sprite_stop_all(void);
void sprite_unanimate_all(void);
void sprite_animate(uint8_t sprite_num);
void sprite_setedge(uint8_t sprite_num, uint8_t edgenum);

#endif