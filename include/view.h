#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>

typedef struct view_info {
    uint16_t backbuffer_offset;
    uint16_t view_offset;
    uint16_t loop_offset;
    uint16_t cel_offset;
    uint8_t number_of_cels;
    uint8_t number_of_loops;
    uint8_t width;
    uint8_t height;
    int16_t x_pos;
    int16_t y_pos;
    bool priority_override;
    uint8_t priority;
} view_info_t;

void draw_cel(view_info_t *info, uint8_t cel);
void erase_view(view_info_t *info);
bool select_loop(view_info_t *info, uint8_t loop_num);
uint8_t get_num_loops(uint8_t view_number);
void view_set(view_info_t *info, uint8_t view_num);
bool view_load(uint8_t view_num);
void view_unload(uint8_t view_num);
void view_purge(uint16_t freed_offset);
void view_init(void);

#endif