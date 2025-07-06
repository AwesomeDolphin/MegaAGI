#ifndef PIC_H
#define PIC_H

extern uint16_t pic_offset;

void draw_pic(bool clear_screen);
void pic_load(uint8_t pic_num);
void pic_discard(uint8_t pic_num);


#endif