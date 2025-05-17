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
    bool drawable;
    bool updatable;
    bool observe_object_collisions;
    bool observe_horizon;
    bool observe_blocks;
    uint8_t step_size;
    bool on_water;
    bool wander;
    uint8_t wander_dir;
    bool frozen;
    int16_t x_destination;
    int16_t y_destination;
    int16_t speed;
    uint8_t move_complete;

    uint8_t end_of_loop;

    bool cycling;
    uint8_t cycle_time;
    uint8_t cycle_count;
    bool reverse;
} agisprite_t;

extern __far agisprite_t sprites[256];
extern __far uint8_t animated_sprites[256];
extern uint8_t animated_sprite_count;

void sprite_update_prio(uint8_t sprite_num);
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
void sprite_mark_drawable(uint8_t sprite_num);
void sprite_setedge(uint8_t sprite_num, uint8_t edgenum);
void sprite_draw_animated(void);
void sprite_erase_animated(void);
void sprite_clearall(void);
void sprite_init(void);

#endif