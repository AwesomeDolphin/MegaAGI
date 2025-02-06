#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <math.h>

#include "gfx.h"
#include "simplefile.h"
#include "memmanage.h"
#include "volume.h"
#include "view.h"

#pragma clang section bss="extradata"
__far static uint16_t views[256];
#pragma clang section bss=""

extern volatile uint8_t drawing_screen;

uint8_t priorities[169];

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

void draw_cel_forward(view_info_t *info) {
    uint8_t objprio = priorities[info->y_pos + info->height];
    uint8_t right_side = info->x_pos + info->width;

    uint8_t __far *cel_data = chipmem_base + info->cel_offset;
    info->backbuffer_offset = chipmem_alloc(info->height * info->width);
    uint8_t __far *view_backbuf = chipmem_base + info->backbuffer_offset;
    uint8_t view_trans = cel_data[2] & 0x0f;
    int16_t cur_x = info->x_pos;
    int16_t cur_y = info->y_pos;
    uint16_t cel_offset = 3;
    uint16_t pixel_offset = 0;
    uint8_t draw_row = 0;

    do {
        uint8_t pixcol = cel_data[cel_offset] >> 4;
        uint8_t pixcnt = cel_data[cel_offset] & 0x0f;
        if ((pixcol == 0) && (pixcnt == 0)) {
            for (; cur_x < right_side; cur_x++) {
                view_backbuf[pixel_offset] = gfx_get(drawing_screen, cur_x, cur_y);
                pixel_offset++;
            }
            cur_x = info->x_pos;
            cur_y++;
            draw_row++;
        } else {
            for (uint8_t count = 0; count < pixcnt; count++) {
                view_backbuf[pixel_offset] = gfx_get(drawing_screen, cur_x, cur_y);
                pixel_offset++;
                if (pixcol != view_trans) {
                    uint8_t pix_prio = gfx_getprio(drawing_screen + 1, cur_x, cur_y);
                    if (objprio >= pix_prio) {
                        gfx_plotput(drawing_screen, cur_x, cur_y, pixcol);
                   }
                }
                cur_x++;
            }
        }
        cel_offset++;
    } while (draw_row < info->height);
}

void draw_cel(view_info_t *info, uint8_t cel) {
    uint8_t __far *loop_data = chipmem_base + info->loop_offset;
    uint8_t __far *cell_ptr = loop_data + (cel * 2) + 1;
    info->cel_offset = *cell_ptr | ((*(cell_ptr + 1)) << 8);

    uint8_t __far *cel_data = loop_data + info->cel_offset;
    info->width  = cel_data[0];
    info->height = cel_data[1];

    uint8_t mirrored = cel_data[2] & 0x80;
    uint8_t source_loop = (cel_data[2] & 0x70) >> 4;
    draw_cel_forward(info);
}

void erase_view(view_info_t *info) {
    uint16_t pixel_offset = 0;
    uint8_t right_side = info->x_pos + info->width;
    uint8_t bottom_side = info->y_pos + info->height;
    uint8_t __far *view_backbuf = chipmem_base + info->backbuffer_offset;

    for (int16_t cur_y = info->y_pos; cur_y < bottom_side; cur_y++) {
        for (int16_t cur_x = right_side; cur_x >= info->x_pos; cur_x--) {
            gfx_plotput(drawing_screen, cur_x, cur_y, view_backbuf[pixel_offset]);
            pixel_offset++;
        }
    }

    chipmem_free(info->backbuffer_offset);
}

bool select_loop(view_info_t *info, uint8_t loop_num) {
    uint8_t __far *view_data = chipmem_base + info->view_offset;
    if (loop_num < info->number_of_loops) {
        uint8_t __far *loop_ptr = view_data + (loop_num * 2) + 5;
        uint16_t loop_offset = *loop_ptr | ((*(loop_ptr + 1)) << 8);
        uint8_t __far *loop_data = chipmem_base + loop_offset;
        info->loop_offset = loop_offset;
        info->number_of_cels = loop_data[0];
        return true;
    }
    return false;
}

void view_set(view_info_t *info, uint8_t view_num) {
    info->view_offset = views[view_num];
    uint8_t __far *view_data = chipmem_base + info->view_offset;
    info->number_of_loops = view_data[2];
}

bool view_load(uint8_t view_num) {
    if (views[view_num] == 0) {
        uint16_t length;
        uint16_t view_offset = load_volume_object(voView, view_num, &length);
        if (view_offset == 0) {
            return false;
        }
        views[view_num] = view_offset;
    }
    return true;
}

void view_unload(uint8_t view_num) {
    if (views[view_num] != 0) {
        chipmem_free(views[view_num]);
        views[view_num] = 0;
    }
}

void view_init(void) {
    setup_priorities();
    for (int i = 0; i < 256; i++) {
        views[i] = 0;
    }
}