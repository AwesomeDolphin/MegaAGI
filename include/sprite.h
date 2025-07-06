#ifndef SPRITE_H
#define SPRITE_H

#include "view.h"

typedef enum {
    pmmNone,
    pmmWander,
    pmmMoveTo,
    pmmFollow,
    pmmFollowWander,
    pmmFollowInit,
} prg_move_mode_t;

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
    bool on_land;
    bool frozen;

    prg_move_mode_t prg_movetype;
    int16_t prg_x_destination;
    int16_t prg_y_destination;
    int16_t prg_speed;
    int16_t prg_distance;
    uint8_t prg_complete_flag;
    uint8_t prg_dir;
    uint8_t prg_timer;

    uint8_t end_of_loop;

    bool cycling;
    uint8_t cycle_time;
    uint8_t cycle_count;
    bool reverse;

    bool ego;
} agisprite_t;

extern uint8_t priorities[169];
extern view_info_t object_view;
extern bool show_object_view;

void autoselect_loop(agisprite_t *sprite);
uint8_t sprite_move(uint8_t spr_num, agisprite_t *sprite, uint8_t speed);
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
void sprite_draw_to_pic(void);
void sprite_clearall(void);
void sprite_init(void);

#endif