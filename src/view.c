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
#include "main.h"
#include "memmanage.h"
#include "volume.h"
#include "view.h"

#pragma clang section bss="extradata"
__far static uint16_t views[256];
#pragma clang section bss=""

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

void draw_cel_uncompressed(view_info_t *info) {
    uint8_t colorval[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    uint8_t __far *cel_data = chipmem_base + info->cel_offset;
    info->backbuffer_offset = chipmem_alloc(info->height * info->width);
    uint8_t __far *view_backbuf = chipmem_base + info->backbuffer_offset;
    uint8_t view_trans = colorval[cel_data[2] & 0x0f];
    int16_t view_left = info->x_pos;
    int16_t view_top = info->y_pos - info->height + 1;
    uint8_t view_right = info->x_pos + info->width;
    uint8_t view_bottom = info->y_pos;
    uint16_t pixel_offset = 0;

    uint8_t objprio;
    if (info->priority_override) {
        objprio = colorval[info->priority];
    } else {
        objprio = colorval[priorities[view_bottom]];
    }

    cel_data += 3;
    for (int16_t draw_col = view_left; draw_col < view_right; draw_col++) {
        volatile uint8_t __far *draw_pointer = drawing_xpointer[drawing_screen][draw_col] + (view_top * 8);
        uint8_t xcolumn = draw_col >> 1;
        uint8_t highpix = draw_col & 1;
        volatile uint8_t __far *priority_pointer = &priority_screen[(view_top * 80) + xcolumn];
        uint8_t search_length = 0;
        uint8_t prioval;
        if (highpix) {
            for (int16_t draw_row = view_top; draw_row <= view_bottom; draw_row++) {
                prioval = *priority_pointer & 0xf0;
                priority_pointer += 80;
                search_length--;
                view_backbuf[pixel_offset] = *draw_pointer;
                if (*cel_data != view_trans) {
                    if ((prioval) <= (objprio & 0xf0)) {
                        *draw_pointer = *cel_data;
                    }
                }
                draw_pointer += 8;
                pixel_offset++;
                cel_data++;
            }
        } else {
            for (int16_t draw_row = view_top; draw_row <= view_bottom; draw_row++) {
                prioval = *priority_pointer & 0x0f;
                priority_pointer += 80;
                search_length--;
                view_backbuf[pixel_offset] = *draw_pointer;
                if (*cel_data != view_trans) {
                    if ((prioval) <= (objprio & 0x0f)) {
                        *draw_pointer = *cel_data;
                    }
                }
                draw_pointer += 8;
                pixel_offset++;
                cel_data++;
            }
        }
    }
}

void draw_cel(view_info_t *info, uint8_t cel) {
    uint8_t __far *loop_data = chipmem_base + info->loop_offset;
    uint8_t __far *cell_ptr = loop_data + (cel * 2) + 1;
    info->cel_offset = (*cell_ptr | ((*(cell_ptr + 1)) << 8));

    uint8_t __far *cel_data = chipmem_base + info->cel_offset;
    info->width  = cel_data[0];
    info->height = cel_data[1];

    draw_cel_uncompressed(info);
}

void erase_view(view_info_t *info) {
    uint16_t pixel_offset = 0;
    int16_t view_left = info->x_pos;
    int16_t view_top = info->y_pos - info->height + 1;
    uint8_t view_right = info->x_pos + info->width;
    uint8_t view_bottom = info->y_pos;
    uint8_t __far *view_backbuf = chipmem_base + info->backbuffer_offset;

    for (int16_t draw_col = view_left; draw_col < view_right; draw_col++) {
        volatile uint8_t __far *draw_pointer = drawing_xpointer[drawing_screen][draw_col] + (view_top * 8);
        for (int16_t draw_row = view_top; draw_row <= view_bottom; draw_row++) {
            *draw_pointer = view_backbuf[pixel_offset];
            draw_pointer += 8;
            pixel_offset++;
        }
    }

    chipmem_free(info->backbuffer_offset);
}

bool select_loop(view_info_t *info, uint8_t loop_num) {
    uint8_t __far *view_data = chipmem_base + info->view_offset;
    if (loop_num < info->number_of_loops) {
        uint8_t __far *loop_ptr = view_data + (loop_num * 2) + 1;
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
    info->number_of_loops = view_data[0];
}

void unpack_view(uint8_t view_num, uint8_t __huge *view_location) {
    bool debug = false;
    uint8_t colorval[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};


    uint8_t loop_count = view_location[2];
    uint16_t view_offset = chipmem_alloc(1 + (loop_count * 2));

    uint8_t __far *view_dest_location = chipmem_base + view_offset;
    views[view_num] = view_offset;
    view_dest_location[0] = loop_count;
    for (uint8_t loop_index = 0; loop_index < loop_count; loop_index++) {
        uint8_t loop_offset_loc = (loop_index * 2) + 5;
        uint16_t loop_source_offset = ((view_location[loop_offset_loc + 1]) << 8);
        loop_source_offset |= view_location[loop_offset_loc];
        uint8_t __huge *loop_source_location = view_location + loop_source_offset;
        uint8_t cels_in_loop  = loop_source_location[0];
        uint16_t loop_overhead = 1 + (cels_in_loop * 2);
        uint16_t loop_dest_offset = chipmem_alloc(loop_overhead);
        view_dest_location[1 + (loop_index * 2)] = loop_dest_offset & 0xff;
        view_dest_location[2 + (loop_index * 2)] = (loop_dest_offset & 0xff00) >> 8;

        uint8_t __far *loop_dest_location = chipmem_base + loop_dest_offset;
        loop_dest_location[0] = cels_in_loop;
    
        for (uint8_t cel_index = 0; cel_index < cels_in_loop; cel_index++) {
            uint8_t cel_offset_loc = (cel_index * 2) + 1;
            uint16_t cel_source_offset = ((loop_source_location[cel_offset_loc + 1]) << 8);
            cel_source_offset |= loop_source_location[cel_offset_loc];
            uint8_t __huge *cel_source_location = loop_source_location + cel_source_offset;
            uint8_t cel_width = cel_source_location[0];
            uint8_t cel_height = cel_source_location[1];
            uint8_t cel_transparency = cel_source_location[2] & 0x0f;
            uint8_t cel_mirroring = cel_source_location[2] & 0x80;
            uint8_t cel_notmirrored_loop = (cel_source_location[2] & 0x70) >> 4;
            bool cel_mirrored = cel_mirroring ? (loop_index != cel_notmirrored_loop) : false;
            uint16_t cel_size = 3 + (cel_width * cel_height);
            uint16_t cel_dest_offset = chipmem_alloc(cel_size);
            loop_dest_location[1 + (cel_index * 2)] = cel_dest_offset & 0xff;
            loop_dest_location[2 + (cel_index * 2)] = (cel_dest_offset & 0xff00) >> 8;

            uint8_t __far *cel_dest_location = chipmem_base + cel_dest_offset;
            cel_dest_location[0] = cel_width;
            cel_dest_location[1] = cel_height;
            cel_dest_location[2] = cel_source_location[2];
        
            uint16_t cur_x = 0;
            uint16_t cur_y = 0;
            uint16_t cel_offset = 3;
            cel_dest_location += 3;
            do {
                uint8_t pixcol = cel_source_location[cel_offset] >> 4;
                uint8_t pixcnt = cel_source_location[cel_offset] & 0x0f;
                if ((pixcol == 0) && (pixcnt == 0)) {
                    for (; cur_x < cel_width; cur_x++) {
                        if (cel_mirrored) {
                            uint16_t mirrored_col = cel_width - cur_x - 1;
                            cel_dest_location[(mirrored_col * cel_height) + cur_y] = colorval[cel_transparency];
                        } else {
                            cel_dest_location[(cur_x * cel_height) + cur_y] = colorval[cel_transparency];
                        }
                    }
                    cur_x = 0;
                    cur_y++;
                } else {
                    for (uint8_t count = 0; count < pixcnt; count++) {
                        if (cel_mirrored) {
                            uint16_t mirrored_col = cel_width - cur_x - 1;
                            cel_dest_location[(mirrored_col * cel_height) + cur_y] = colorval[pixcol];
                        } else {
                            cel_dest_location[(cur_x * cel_height) + cur_y] = colorval[pixcol];
                        }
                        cur_x++;
                    }
                }
                cel_offset++;
            } while (cur_y < cel_height);
        }
    }

}

bool view_load(uint8_t view_num) {
    if (views[view_num] == 0) {
        uint16_t length;
        uint8_t __huge *view_location = locate_volume_object(voView, view_num, &length);
        if (view_location == 0) {
            return false;
        }
        unpack_view(view_num, view_location);
    }
    return true;
}

void view_unload(uint8_t view_num) {
    if (views[view_num] != 0) {
        chipmem_free(views[view_num]);
        views[view_num] = 0;
    }
}

void view_purge(uint16_t freed_offset) {
    for (int i = 0; i < 256; i++) {
        if (views[i] >= freed_offset) {
            views[i] = 0;
        }
    }
}

void view_init(void) {
    setup_priorities();
    for (int i = 0; i < 256; i++) {
        views[i] = 0;
    }
}