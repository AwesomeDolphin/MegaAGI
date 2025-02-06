#ifndef GFX_H
#define GFX_H

void gfx_plotput(uint8_t screen_num, uint8_t x, uint8_t y, uint8_t color);
uint8_t gfx_getprio(uint8_t screen_num, uint8_t x, uint8_t y);
uint8_t gfx_get(uint8_t screen_num, uint8_t x, uint8_t y);
void gfx_drawfastline(uint8_t screen_num, int x1, int y1, int x2, int y2, unsigned char colour);
void gfx_drawslowline(uint8_t screen_num, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, unsigned char colour);
void gfx_print_splitchar(uint8_t character);
void gfx_print_splitascii(char *formatstring, ...);
void gfx_setupmem(void);
void gfx_blackscreen(void);
void gfx_cleargfx(uint8_t screen_num);
void gfx_copygfx(uint8_t screen_num);
void gfx_showgfx(uint8_t screen_num);
void gfx_switchto(void);
void gfx_switchfrom(void);

#endif