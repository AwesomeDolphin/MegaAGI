#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <math.h>

#include "gfx.h"
#include "volume.h"
#include "pic.h"

typedef struct fill_info {
    int16_t x1;
    int16_t x2;
    int16_t y;
    int16_t dy;
} fill_info_t;

#define ASCIIKEY (*(volatile uint8_t *)0xd610)
static    uint8_t pic_color;
static    uint8_t priority_color;
static    uint8_t pic_on;
static    uint8_t priority_on;
static uint8_t last_relative_x;
static uint8_t last_relative_y;
static uint16_t fill_pointer;
static uint8_t drawing_screen;

#pragma clang section bss="extradata"
__far static fill_info_t fills[128];
#pragma clang section bss=""

void pset(uint8_t x, uint8_t y) {
    if (pic_on) {
        gfx_plotput(drawing_screen, x, y, pic_color);
    }
    if (priority_on) {
        gfx_plotput(drawing_screen+1, x, y, priority_color);
    }
}

void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if (pic_on) {
        gfx_drawslowline(drawing_screen, x1, y1, x2, y2, pic_color);
    }
    if (priority_on) {
        gfx_drawslowline(drawing_screen+1, x1, y1, x2, y2, priority_color);
    }
}

void draw_relative_line(uint8_t coord) {
    int8_t rel_x = coord >> 4;
    int8_t rel_y = coord & 0x0f;
    if (rel_x & 0x08) {
        rel_x = -1 * (rel_x & 0x07);
    }
    if (rel_y & 0x08) {
        rel_y = -1 * (rel_y & 0x07);
    }
    draw_line(last_relative_x, last_relative_y, last_relative_x + rel_x, last_relative_y + rel_y);
    last_relative_x = last_relative_x + rel_x;
    last_relative_y = last_relative_y + rel_y;
}

#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y
#define PEEK(X) (*(volatile uint8_t*)(X))

uint8_t can_fill(uint8_t x, uint8_t y) {
    if ((pic_on == 0) && (priority_on == 0)) {
        return 0;
    }
    if ((x >= 160) || (y >= 168)) {
        return 0;
    }
    if (pic_color == 15) {
        return 0;
    }
    if (pic_on == 0) {
        return (gfx_get(drawing_screen + 1, x, y) == 4);
    }
    return (gfx_get(drawing_screen, x, y) == 15);
}

uint8_t draw_fill(uint8_t in_x, uint8_t in_y) {
    if (!can_fill(in_x, in_y)) {
        return 0;
    }
    fill_pointer = 2;
    fills[1] = (fill_info_t){.x1 = in_x, .x2 = in_x, .y = in_y, .dy = 1};
    fills[2] = (fill_info_t){.x1 = in_x, .x2 = in_x, .y = in_y - 1, .dy = -1};
    uint8_t max_depth = 0;
    while (fill_pointer > 0) {
        if (fill_pointer > max_depth) {
            max_depth = fill_pointer;
        }
        fill_info_t cur_fill = fills[fill_pointer];
        fill_pointer--;

        int16_t loc_x = cur_fill.x1;
        if (can_fill(loc_x, cur_fill.y)) {
            int16_t start_x = loc_x - 1;
            while (can_fill(loc_x - 1, cur_fill.y)) {
                loc_x = loc_x - 1;
            }
            if (loc_x <= start_x) {
                draw_line(start_x, cur_fill.y, loc_x, cur_fill.y);
            }
            if (loc_x < cur_fill.x1) {
                fill_pointer++;
                fills[fill_pointer] = (fill_info_t){.x1 = loc_x, .x2 = cur_fill.x1 - 1, .y = cur_fill.y - cur_fill.dy, .dy = -cur_fill.dy};
            }
        }

        while(cur_fill.x1 <= cur_fill.x2) {
            int16_t start_x = cur_fill.x1;
            while (can_fill(cur_fill.x1, cur_fill.y)) {
                cur_fill.x1 = cur_fill.x1 + 1;
            }
            if (cur_fill.x1 > start_x) {
                draw_line(start_x, cur_fill.y, cur_fill.x1 - 1, cur_fill.y);
            }
            if (cur_fill.x1 > loc_x) {
                fill_pointer++;
                fills[fill_pointer] = (fill_info_t){.x1 = loc_x, .x2 = cur_fill.x1 - 1, .y = cur_fill.y + cur_fill.dy, .dy = cur_fill.dy};
            }
            if ((cur_fill.x1 - 1) > cur_fill.x2) {
                fill_pointer++;
                fills[fill_pointer] = (fill_info_t){.x1 = cur_fill.x2 + 1, .x2 = cur_fill.x1 - 1, .y = cur_fill.y - cur_fill.dy, .dy = -cur_fill.dy};
            }
            cur_fill.x1 = cur_fill.x1 + 1;
            while ((cur_fill.x1 < cur_fill.x2) && (!can_fill(cur_fill.x1, cur_fill.y))) {
                cur_fill.x1 = cur_fill.x1 + 1;
            }
            loc_x = cur_fill.x1;
        }
    }
    return max_depth;
}

uint8_t draw_pic(uint8_t screen_num, uint8_t pic_num, uint8_t draw_mode) {
    drawing_screen = screen_num;
    gfx_cleargfx(drawing_screen);
    uint8_t max_depth = 1;
    uint8_t __huge *pic_file;
    uint16_t length;
    pic_file = locate_volume_object(voPic, pic_num, &length);
    if (pic_file == NULL) {
        return 0;
    }

    pic_on = 0;
    priority_on = 0;
    uint16_t index = 0;
    do {
        uint8_t command = pic_file[index];
        index++;
        switch(command) {
            case 0xF0:
                pic_color = pic_file[index];
                index++;
                pic_on = 1;
                break;
            case 0xF1:
                pic_on = 0;
                break;
            case 0xF2:
                priority_color = pic_file[index];
                index++;
                priority_on = 1;
                break;
            case 0xF3:
                priority_on = 0;
                break;
            case 0xF4:
                last_relative_x = pic_file[index];
                last_relative_y = pic_file[index+1];
                index += 2;
                while ((pic_file[index] & 0xf0) != 0xf0) {
                    draw_line(last_relative_x, last_relative_y, last_relative_x, pic_file[index]);
                    last_relative_y = pic_file[index];
                    index += 1;
                    if ((pic_file[index] & 0xf0) == 0xf0) {
                        break;
                    }
                    draw_line(last_relative_x, last_relative_y, pic_file[index], last_relative_y);
                    last_relative_x = pic_file[index];
                    index += 1;
                }
                break;
            case 0xF5:
                last_relative_x = pic_file[index];
                last_relative_y = pic_file[index+1];
                index += 2;
                while ((pic_file[index] & 0xf0) != 0xf0) {
                    draw_line(last_relative_x, last_relative_y, pic_file[index], last_relative_y);
                    last_relative_x = pic_file[index];
                    index += 1;
                    if ((pic_file[index] & 0xf0) == 0xf0) {
                        break;
                    }
                    draw_line(last_relative_x, last_relative_y, last_relative_x, pic_file[index]);
                    last_relative_y = pic_file[index];
                    index += 1;
                }
                break;
            case 0xF6:
                pset(pic_file[index], pic_file[index + 1]);
                while (1) {
                    if ((pic_file[index + 2] >= 0xf0) || (pic_file[index + 3] >= 0xf0)) break;
                    draw_line(pic_file[index], pic_file[index+1], pic_file[index+2], pic_file[index+3]);
                    index += 2;
                } 
                index += 2;
                break;
            case 0xF7:
                last_relative_x = pic_file[index];
                last_relative_y = pic_file[index+1];
                pset(pic_file[index], pic_file[index+1]);
                index += 2;
                while ((pic_file[index] & 0xf0) != 0xf0) {
                    draw_relative_line(pic_file[index]);
                    index += 1;
                };
                break;
            case 0xF8:
                while ((pic_file[index] & 0xf0) != 0xf0) {
                    uint8_t depth = draw_fill(pic_file[index], pic_file[index+1]);
                    if (depth > max_depth) {
                        max_depth = depth;
                    }
                    index += 2;
                }
                break;
            case 0xFA:
            case 0xFF:
                return max_depth;
            default:
                gfx_switchfrom();
                while (ASCIIKEY==0);
                ASCIIKEY = 0;
                gfx_switchto();
                return 0;
        }
    } while (index < length);
    return max_depth;
}