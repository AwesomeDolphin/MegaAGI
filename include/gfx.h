#ifndef GFX_H
#define GFX_H

#include <stdbool.h>

extern volatile uint8_t __far *drawing_xpointer[2][160];
extern uint8_t *fastdrawing_xpointer[160];
extern volatile uint8_t drawing_screen;
extern __far uint8_t priority_screen[16384];

void gfx_plotput(uint8_t x, uint8_t y, uint8_t color);
uint8_t gfx_getprio(uint8_t x, uint8_t y);
uint8_t gfx_get(uint8_t x, uint8_t y);
void gfx_drawslowline(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, unsigned char colour);
void gfx_print_asciichar(uint8_t character, bool reverse);
void gfx_print_scncode(uint8_t scncode);
void gfx_begin_print(uint8_t x, uint8_t y);
void gfx_end_print(void);
void gfx_print_ascii(uint8_t x, uint8_t y, uint8_t *formatstring, ...);
void gfx_set_printpos(uint8_t x, uint8_t y);
void gfx_clear_line(uint8_t y);
void gfx_setupmem(void);
void gfx_blackscreen(void);
void gfx_cleargfx(bool preserve_text);
bool gfx_flippage(void);
bool gfx_hold_flip(bool hold);
void gfx_switchto(void);
void gfx_set_textmode(bool enable_text);

#endif