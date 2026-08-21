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

#ifndef SPRITE_H
#define SPRITE_H

#include "view.h"

static const uint8_t sprFlag1Drawable = 0x01;
static const uint8_t sprFlag1Updatable = 0x02;
static const uint8_t sprFlag1ObserveObjectCol = 0x04;
static const uint8_t sprFlag1ObserveHorizon = 0x08;
static const uint8_t sprFlag1ObserveBlocks = 0x10;
static const uint8_t sprFlag1Frozen = 0x20;
static const uint8_t sprFlag1Cycling = 0x40;
static const uint8_t sprFlag1CycleReverse = 0x80;

static const uint8_t sprFlag2LoopOverride = 0x01;
static const uint8_t sprFlag2OnWater = 0x02;
static const uint8_t sprFlag2OnLand = 0x04;

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

    uint8_t object_dir;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t step_size;
    uint8_t step_time;
    uint8_t step_count;

    prg_move_mode_t prg_movetype;
    int16_t prg_x_destination;
    int16_t prg_y_destination;
    int16_t prg_speed;
    int16_t prg_distance;
    uint8_t prg_complete_flag;
    uint8_t prg_dir;
    uint8_t prg_timer;

    uint8_t end_of_loop;

    uint8_t cycle_time;
    uint8_t cycle_count;

    bool ego;
} agisprite_t;

extern uint8_t priority_bands[11];
extern uint8_t priorities[169];
extern view_info_t object_view;

void sprite_draw_to_pic(bool force_draw);
void sprite_erase_from_pic(bool force_draw);
void autoselect_loop(agisprite_t *sprite);
uint8_t sprite_move(uint8_t spr_num, agisprite_t *sprite, uint8_t speed);
void sprite_erase(uint8_t sprite_num);
void sprite_draw(uint8_t sprite_num);
uint8_t sprite_get_view(uint8_t sprite_num);
void sprite_set_view(uint8_t sprite_num, uint8_t view_number);
void sprite_set_cel(agisprite_t *sprite, uint8_t cel_number);
void sprite_stop_all(void);
void sprite_unanimate_all(void);
void sprite_mark_drawable(uint8_t sprite_num);
void sprite_setedge(uint8_t sprite_num, uint8_t edgenum);
void sprite_clearall(void);
void sprite_undraw(void);
void sprite_updateanddraw(void);
void sprite_show_object(uint8_t view_num);
void sprite_init(void);

#endif